/*  $Id: xml_format.c,v 1.25 2005/01/07 02:33:40 mgrouch Exp $  */

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002 Mikhail Grushinskiy.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>

#include "xmlstar.h"

#ifndef HAVE_STRDUP
#include "strdup.h"
#endif

/*
 *  TODO:  1. Attribute formatting options (as every attribute on a new line)
 *         2. exit values on errors
 */

typedef struct _foOptions {
    int indent;               /* indent output */
    int indent_tab;           /* indent output with tab */
    int indent_spaces;        /* num spaces for indentation */
    int omit_decl;            /* omit xml declaration */
    int recovery;             /* try to recover what is parsable */
    int dropdtd;              /* remove the DOCTYPE of the input docs */
    int options;              /* global parsing flags */ 
#ifdef LIBXML_HTML_ENABLED
    int html;                 /* inputs are in HTML format */
#endif
    int quiet;                 /* quiet mode */
} foOptions;

typedef foOptions *foOptionsPtr;

/*
 * usage string chunk : 509 char max on ISO C90
 */
static const char format_usage_str_1[] =
"XMLStarlet Toolkit: Format XML document\n"
"Usage: %s fo [<options>] <xml-file>\n"
"where <options> are\n"
"  -n or --noindent            - do not indent\n"
"  -t or --indent-tab          - indent output with tabulation\n"
"  -s or --indent-spaces <num> - indent output with <num> spaces\n"
"  -o or --omit-decl           - omit xml declaration <?xml version=\"1.0\"?>\n";

static const char format_usage_str_2[] =
"  -R or --recover             - try to recover what is parsable\n"
"  -D or --dropdtd             - remove the DOCTYPE of the input docs\n"
"  -C or --nocdata             - replace cdata section with text nodes\n"
"  -N or --nsclean             - remove redundant namespace declarations\n"
"  -e or --encode <encoding>   - output in the given encoding (utf-8, unicode...)\n"
#ifdef LIBXML_HTML_ENABLED
"  -H or --html                - input is HTML\n"
#endif
"  -Q or --quiet               - Suppress errors from libxml2\n"
"  -h or --help                - print help\n\n";

const char *encoding = NULL;

/**
 *  Print small help for command line options
 */
void
foUsage(int argc, char **argv, exit_status status)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, "%s", format_usage_str_1);
    fprintf(o, "%s", format_usage_str_2);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  Initialize global command line options
 */
void
foInitOptions(foOptionsPtr ops)
{
    ops->indent = 1;
    ops->indent_tab = 0;
    ops->indent_spaces = 2;
    ops->omit_decl = 0;
    ops->recovery = 0;
    ops->dropdtd = 0;
    ops->options = 0;
#ifdef LIBXML_HTML_ENABLED
    ops->html = 0;
#endif
    ops->quiet = 0;
}

/**
 *  Initialize LibXML
 */
void
foInitLibXml(foOptionsPtr ops)
{
    /*
     * Initialize library memory
     */
    xmlInitMemory();

    LIBXML_TEST_VERSION

    /*
     * Store line numbers in the document tree
     */
    xmlLineNumbersDefault(1);

    xmlSubstituteEntitiesDefault(1);
    xmlKeepBlanksDefault(0);
    xmlPedanticParserDefault(0);
    
    xmlGetWarningsDefaultValue = 1;
    xmlDoValidityCheckingDefaultValue = 0;
    xmlLoadExtDtdDefaultValue = 0;

    xmlTreeIndentString = NULL;
    if (ops->indent)
    {
        xmlIndentTreeOutput = 1;
        if (ops->indent_tab) 
        {
            xmlTreeIndentString = strdup("\t");
        }
        else if (ops->indent_spaces > 0)
        {
            int i;
            char *p;
            p = malloc(ops->indent_spaces + 1);
            xmlTreeIndentString = p;
            for (i=0; i<ops->indent_spaces; i++) p[i] = ' ';
            p[ops->indent_spaces] = 0;
        }
    }
    else
        xmlIndentTreeOutput = 0;
}

/**
 *  Parse global command line options
 */
int
foParseOptions(foOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while(i < argc)
    {
        if (!strcmp(argv[i], "--noindent") || !strcmp(argv[i], "-n"))
        {
            ops->indent = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--encode") || !strcmp(argv[i], "-e"))
        {
            i++;
            encoding = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--indent-tab") || !strcmp(argv[i], "-t"))
        {
            ops->indent_tab = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--omit-decl") || !strcmp(argv[i], "-o"))
        {
            ops->omit_decl = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--dropdtd") || !strcmp(argv[i], "-D"))
        {
            ops->dropdtd = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--recover") || !strcmp(argv[i], "-R"))
        {
            ops->recovery = 1;
	    ops->options |= XML_PARSE_RECOVER;
            i++;
        }
        else if (!strcmp(argv[i], "--nocdata") || !strcmp(argv[i], "-C"))
        {
            ops->options |= XML_PARSE_NOCDATA;
	    i++;
        }
        else if (!strcmp(argv[i], "--nsclean") || !strcmp(argv[i], "-N"))
        {
            ops->options |= XML_PARSE_NSCLEAN;
	    i++;
        }
        else if (!strcmp(argv[i], "--indent-spaces") || !strcmp(argv[i], "-s"))
        {
            int value;
            i++;
            if (i >= argc) foUsage(argc, argv, EXIT_BAD_ARGS);
            if (sscanf(argv[i], "%d", &value) == 1)
            {
                if (value > 0) ops->indent_spaces = value;
            }
            else
            {
                foUsage(argc, argv, EXIT_BAD_ARGS);
            }
            ops->indent_tab = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-Q"))
        {
            ops->quiet = 1;
            i++;
        }
#ifdef LIBXML_HTML_ENABLED
        else if (!strcmp(argv[i], "--html") || !strcmp(argv[i], "-H"))
        {
            ops->html = 1;
            i++;
        }
#endif
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
        {
            foUsage(argc, argv, EXIT_BAD_ARGS);
        }
        else if (!strcmp(argv[i], "-"))
        {
            i++;
            break;
        }
        else if (argv[i][0] == '-')
        {
            foUsage(argc, argv, EXIT_BAD_ARGS);
        }
        else
        {
            i++;
            break;
        }
    }

    return i-1;
}

void my_error_func(void* ctx, const char * msg, ...) {
  /* do nothing */
}

void my_structured_error_func(void * userData, xmlErrorPtr error) {
  /* do nothing */
}

/**
 *  'process' xml document(s)
 */
int
foProcess(foOptionsPtr ops, int start, int argc, char **argv)
{
    int ret = 0;
    xmlDocPtr doc = NULL;
    char *fileName = "-";

    if ((start > 1) && (start < argc) && (argv[start][0] != '-') &&
        strcmp(argv[start-1], "--indent-spaces") &&
        strcmp(argv[start-1], "-s"))
    {
        fileName = argv[start];   
    }
/*
    if (ops->recovery)
    {
        doc = xmlRecoverFile(fileName);
    }
    else    
*/
    if (ops->quiet) {
      xmlSetGenericErrorFunc(NULL, my_error_func);
      xmlSetStructuredErrorFunc(NULL, my_structured_error_func);
    }

#ifdef LIBXML_HTML_ENABLED
    if (ops->html)
    {
        doc = htmlReadFile(fileName, NULL, ops->options);
    }
    else
#endif
        doc = xmlReadFile(fileName, NULL, ops->options);

    if (doc == NULL)
    {
        /*fprintf(stderr, "%s:: error: XML parse error\n", fileName);*/
        return 2;
    }

    /*
     * Remove DOCTYPE nodes
     */
    if (ops->dropdtd) {
        xmlDtdPtr dtd;

        dtd = xmlGetIntSubset(doc);
        if (dtd != NULL) {
            xmlUnlinkNode((xmlNodePtr)dtd);
            xmlFreeDtd(dtd);
        }
    }
 
    if (!ops->omit_decl)
    {
        if (encoding != NULL)
        {
            xmlSaveFormatFileEnc("-", doc, encoding, 1);
        }
        else
        {
            xmlSaveFormatFile("-", doc, 1);
        }
    }
    else
    {
        int format = 1;
        int ret = 0;
        xmlOutputBufferPtr buf = NULL;
        xmlCharEncodingHandlerPtr handler = NULL;
        buf = xmlOutputBufferCreateFile(stdout, handler);

        if (doc->children != NULL)
        {
            xmlNodePtr child = doc->children;
            while (child != NULL)
            {
                xmlNodeDumpOutput(buf, doc, child, 0, format, encoding);
                xmlOutputBufferWriteString(buf, "\n");
                child = child->next;
            }
        }
        ret = xmlOutputBufferClose(buf);
    }
    
    xmlFreeDoc(doc);
    return ret;
}

/**
 *  Cleanup memory
 */
void
foCleanup()
{
    if (xmlTreeIndentString) free((char *)xmlTreeIndentString);
    xmlTreeIndentString = NULL;
    
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
}

/**
 *  This is the main function for 'format' option
 */
int
foMain(int argc, char **argv)
{
    int ret = 0;
    int start;
    static foOptions ops;

    if (argc <=1) foUsage(argc, argv, EXIT_BAD_ARGS);
    foInitOptions(&ops);
    start = foParseOptions(&ops, argc, argv);
    if (argc-start > 1) foUsage(argc, argv, EXIT_BAD_ARGS);
    foInitLibXml(&ops);
    ret = foProcess(&ops, start, argc, argv);
    foCleanup();
    
    return ret;
}
