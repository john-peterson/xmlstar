/*  $Id: xml_trans.c,v 1.17 2002/11/26 05:32:19 mgrouch Exp $  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trans.h"

/*
 *  TODO:
 *        1. proper command line arguments handling
 *        2. review and clean up all code
 *        3. tests
 *        4. --novalid option analog (no dtd validation)
 *        5. --nonet option analog (no network for external entities)
 *        6. html and docbook input documents
 *        7. -s for string parameters instead of -p
 *        8. check embedded stylesheet support
 */

static const char trans_usage_str[] =
"XMLStarlet Toolkit: Transform XML document(s) using XSLT\n"
"Usage: xml tr [<options>] <xsl-file> {-p|-s <name>=<value>} [ <xml-file> ... ]\n"
"where\n"
"   <xsl-file>      - main XSLT stylesheet for transformation\n"
"   <xml-file>      - input XML document file name (stdin is used if missing)\n"
"   <name>=<value>  - name and value of the parameter passed to XSLT processor\n"
"   -p              - parameter is an XPATH expression (\"'string'\" to quote string)\n"
"   -s              - parameter is a string literal\n"
"<options> are:\n"
"   --show-ext      - show list of extensions\n"
"   --noval         - do not validate against DTDs or schemas\n"
"   --nonet         - refuse to fetch DTDs or entities over network\n"
#ifdef LIBXML_XINCLUDE_ENABLED
"   --xinclude      - do XInclude processing on document input\n"
#endif
"   --maxdepth val  - increase the maximum depth\n"
#ifdef LIBXML_HTML_ENABLED
"   --html          - input document(s) is(are) in HTML format\n"
#endif
#ifdef LIBXML_DOCB_ENABLED
"   --docbook       - input document(s) is(are) in SGML docbook format\n"
#endif
#ifdef LIBXML_CATALOG_ENABLED
"   --catalogs      - use SGML catalogs from $SGML_CATALOG_FILES\n"
"                     otherwise XML Catalogs starting from\n"
"                     file:///etc/xml/catalog are activated by default\n\n";
#endif

/**
 *  Display usage syntax
 */
void
trUsage(int argc, char **argv)
{
    extern const char more_info[];
    extern const char libxslt_more_info[];
    FILE* o = stderr;
    fprintf(o, trans_usage_str);
    fprintf(o, more_info);
    fprintf(o, libxslt_more_info);
    exit(1);
}

/**
 *  Parse global command line options
 */
void
trParseOptions(xsltOptionsPtr ops, int argc, char **argv)
{
    int i;
    
    if (argc <= 2) return;
    for (i=2; i<argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "--show-ext"))
            {
                ops->show_extensions = 1;
            }
            else if (!strcmp(argv[i], "--noval"))
            {
                ops->noval = 1;
            }
            else if (!strcmp(argv[i], "--nonet"))
            {
                ops->nonet = 1;
            }
            else if (!strcmp(argv[i], "--maxdepth"))
            {
                int value;
                i++;
                if (i >= argc) trUsage(0, NULL);
                if (sscanf(argv[i], "%d", &value) == 1)
                    if (value > 0) xsltMaxDepth = value;
            }
#ifdef LIBXML_XINCLUDE_ENABLED
            else if (!strcmp(argv[i], "--xinclude"))
            {
                ops->xinclude = 1;
            }
#endif
#ifdef LIBXML_HTML_ENABLED
            else if (!strcmp(argv[i], "--html"))
            {
                ops->html = 1;
            }
#endif
#ifdef LIBXML_DOCB_ENABLED
            else if (!strcmp(argv[i], "--docbook"))
            {
                ops->docbook = 1;
            }
#endif
            else if (!strcmp(argv[i], "--nonet"))
            {
                ops->nonet = 1;
            }
        }
        else
            break;
    }
}

/**
 *  Initialize LibXML
 */
void
trInitLibXml(xsltOptionsPtr ops)
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

    /*
     * Register the EXSLT extensions
     */
    exsltRegisterAll();

    /*
     * Register the test module
    */
    xsltRegisterTestModule();

    if (ops->show_extensions) xsltDebugDumpExtensions(stderr);

    /*
     * Register entity loader
     */
    defaultEntityLoader = xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader(xsltExternalEntityLoader);
    if (ops->nonet) defaultEntityLoader = xmlNoNetExternalEntityLoader;
    
    if (ops->noval == 0)
        xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    else
        xmlLoadExtDtdDefaultValue = 0;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (ops->xinclude)
        xsltSetXIncludeDefault(1);
#endif

#ifdef LIBXML_CATALOG_ENABLED
    if (ops->catalogs)
    {
        char *catalogs = getenv("SGML_CATALOG_FILES");
        if (catalogs == NULL)
            fprintf(stderr, "Variable $SGML_CATALOG_FILES not set\n");
        else
            xmlLoadCatalogs(catalogs);
    }    
#endif

    /*
     * Replace entities with their content.
     */
    xmlSubstituteEntitiesDefault(1);
}

/**
 *  Cleanup memory
 */
void
trCleanup()
{
    xsltCleanupGlobals();
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
}

/**
 *  This is the main function for 'tr' option
 */
int
trMain(int argc, char **argv)
{
    xsltOptions ops;

    int errorno = 0;
    int i;
    
    if (argc <= 2) trUsage(argc, argv);

    xsltInitOptions(&ops);
    trParseOptions(&ops, argc, argv);
    trInitLibXml(&ops);

    /* find xsl file name */
    for (i=2; i<argc; i++)
        if (argv[i][0] != '-') break;

    /* set parameters */
    /* TODO */
        
    /* run transformation */
    errorno = xsltRun(&ops, argv[i], argc-i-1, argv+i+1);

    trCleanup();
    
    return(errorno);                                                
}
