/*  $Id: xml.c,v 1.18 2002/11/26 05:32:18 mgrouch Exp $  */

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern int edMain(int argc, char **argv);
extern int selMain(int argc, char **argv);
extern int trMain(int argc, char **argv);
extern int valMain(int argc, char **argv);

static const char usage_str[] =
"XMLStarlet Toolkit: Command line utilities for XML\n"
"Usage: xml [<options>] <command> [<cmd-options>]\n"
"where <command> is one of:\n"
"   ed   (or edit)      - Edit XML document(s)\n"
"   sel  (or select)    - Select data or query XML document(s)\n"
"   tr   (or transform) - Transform XML document(s)\n"
"   val  (or validate)  - Validate XML document(s)\n"
"<options> are:\n"
"   --version           - show version\n"
"   --help              - show help\n"
"Type: xml <command> --help <ENTER> for command help\n\n";

const char more_info[] =
"XMLStarlet is a command line toolkit to query/edit/check/transform\n"
"XML documents (for more information see http://xmlstar.sourceforge.net/)\n";

const char libxslt_more_info[] =
"\n"
"Current implementation uses libxslt from GNOME codebase as XSLT processor\n"
"(see http://xmlsoft.org/ for more details)\n";

/**
 *  Display usage syntax
 */
void
usage(int argc, char **argv)
{
    FILE* o = stderr;
    fprintf(o, usage_str);
    fprintf(o, more_info);
    exit(1);
}  

/**
 *  This is the main function
 */
int
main(int argc, char **argv)
{
    int ret = 0;

    if (argc <= 1) usage(argc, argv);

    if (!strcmp(argv[1], "ed") || !strcmp(argv[1], "edit"))
    {
        ret = edMain(argc, argv);
    }
    else if (!strcmp(argv[1], "sel") || !strcmp(argv[1], "select"))
    {
        ret = selMain(argc, argv);
    }
    else if (!strcmp(argv[1], "tr") || !strcmp(argv[1], "transform"))
    {
        ret = trMain(argc, argv);
    }
    else if (!strcmp(argv[1], "val") || !strcmp(argv[1], "validate"))
    {
        ret = valMain(argc, argv);
    }
    else if (!strcmp(argv[1], "--version"))
    {
        fprintf(stdout, "0.0.1\n");  /* TODO */
        ret = 0;
    }
    else
    {
        usage(argc, argv);
    }
    
    exit(ret);
}
