/*  $Id: xml_depyx.c,v 1.2 2003/08/22 19:42:42 mgrouch Exp $  */

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


#define INSZ 4*1024

static const char depyx_usage_str[] =
"XMLStarlet Toolkit: Convert PYX into XML\n"
"Usage: xml p2x [<pyx-file>]\n"
"where\n"
"   <pyx-file> - input PYX document file name (stdin is used if missing)\n\n"
"The PYX format is a line-oriented representation of\n"
"XML documents that is derived from the SGML ESIS format.\n"
"(see ESIS - ISO 8879 Element Structure Information Set spec,\n"
"ISO/IEC JTC1/SC18/WG8 N931 (ESIS))\n"
"\n";

static void
depyxUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, depyx_usage_str);
    fprintf(o, more_info);
    exit(1);
}

/**
 *  Decode PYX string
 *
 */
void
pyxDecode(char *str)
{
   while (*str)
   {
      if ((*str == '\\') && (*(str+1) == 'n'))
      {
         printf("\n");
         str++;
      }
      else if ((*str == '\\') && (*(str+1) == 't'))
      {
         printf("\t");
         str++;
      }
      else if ((*str == '\\') && (*(str+1) == '\\'))
      {
         printf("\\");
         str++;
      }
      else
         printf("%c", *str);

      str++;
   }
}

/**
 *  Decode PYX file
 *
 */
int
pyxDePyx(char *file)
{
   static char line[INSZ];
   FILE *in = stdin;

   if (strcmp(file, "-"))
   {
       in = fopen(file, "r");
       if (in == NULL)
       {
          fprintf(stderr, "error: could not open: %s\n", file);
          exit(2);
       }
   }
   
   while (!feof(in))
   {
       if (fgets(line, INSZ - 1, in))
       {
           if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';


           if (line[0] == '(')
           {
               printf("<%s", line+1);
               if (!feof(in))
               {
                   if (fgets(line, INSZ - 1, in))
                   {
                       if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

                       while(line[0] == 'A')
                       {
                           char *value;

                           printf(" ");
                           value = line+1;
                           while(*value && (*value != ' '))
                           {
                               printf("%c", *value);
                               value++;
                           }
                           if (*value == ' ')
                           {
                               value++;
                               printf("=\"");
                               pyxDecode(value);
                               printf("\"");
                           }
                           if (!feof(in))
                           {
                               if (fgets(line, INSZ - 1, in))
                               {
                                   if(line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';

                               }
                           }
                       }
                       printf(">");
                   }
               }
           }
           if (line[0] == '-')
           {
               pyxDecode(line+1);
           }
           else if (line[0] == '?')
           {
               printf("<?%s %s?", line+1, line+1);
           }
           else if (line[0] == ')')
           {
               printf("</%s>", line+1);
           }
       }
   }

   return 0;
}

/**
 *  Main function for 'de-PYX'
 *
 */
int
depyxMain(int argc, char **argv)
{
   int ret;

   if ((argc >= 3) && (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help")))
   {
       depyxUsage(argc, argv);
   }
   else if (argc == 3)
   {
       ret = pyxDePyx(argv[2]);
   }
   else if (argc == 2)
   {  
       ret = pyxDePyx("-");
   }
   else
   {
       depyxUsage(argc, argv);     
   }
   
   printf("\n");

   return ret;
}
