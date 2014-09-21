%{
unsigned long hashstr(const char* x)
{
  unsigned long h = 0;
  unsigned long g;

  while (*x)
  {
    h = (h << 4) + *x++;

    if ((g = h & 0xf0000000) != 0)
      h = (h ^ (g >> 24)) ^ g;

    /*
    if ((g = h & 0x0f000000) != 0)
      h = (h ^ (g >> 20)) ^ g;
    */
  }
  h &= 0x0fffffff;
  h |= 0x40000000;
  return h;
}
%}

ID      [A-Za-z][A-Za-z0-9_]*

%%

[^H\n]+		/*ECHO;*/
"HASH_"{ID}     printf( "#define\t%-32s\t0x%08lX\n", yytext, hashstr(yytext+5) );
.|\n		/*ECHO;*/

%%

#ifdef DMALLOC
#include <dmalloc.h>
#endif

int Argc;
char **Argv;

int main( int argc, char **argv )
{
    Argv=++argv+1;
    Argc=--argc-1;
    if ( argc > 0 )
     {
	    yyin = fopen( argv[0], "r" );
	    if (!yyin)
	      { fprintf(stderr, "cannot open file %s\n", argv[0]); exit(2); }
     }
    else
	    yyin = stdin;

    yylex();
    return 0;
}

int hashwrap()
{
  if (Argc>0)
   {
     fclose(yyin);
     yyin = fopen( Argv[0], "r" );
     if (!yyin)
       { fprintf(stderr, "cannot open file %s\n", Argv[0]); exit(2); }
     Argc--;
     Argv++;
     return 0;
   }
  return 1;
}


