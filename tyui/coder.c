/*
	$Id: coder.c,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

char *codeStr="sprintf(fileIn,\"%i5\",num1,sqwo2)";

#define MaxLen	512

char number[128], buf[MaxLen+1], buf2[MaxLen];
FILE *in, *out;
int i, j, count, len;

int main( int argc, char *argv[] )
{
    if ( argc != 2 )
      {
	 puts("One argument, please: \"coder copyright_file\"");
	 return 1;
      }
    in = fopen( argv[1], "rt" );
    out=fopen( "copyrigh.h", "wt" );

    if ( !in || !out )
       return 2;

    i=0; count=0;
    for( ; fgets( buf, MaxLen, in ); i++ );

    sprintf( number,"%i\n", i );
    fputs( number, out );

    fseek( in, SEEK_SET, 0 );
    while( fgets( buf, MaxLen, in ) )
      {
	 buf[MaxLen]=0;
	 len = strlen( buf );
	 if ( buf[len-1] == '\n' )
	    buf[ --len ] = 0;
	 sprintf( buf2, ",%i", len );
	 for( j=0; buf[j]; j++ )
	   {
	      if ( !codeStr[count] )
		 count=0;
	      buf[j] += codeStr[count++];
	      sprintf( number, ",%i", (unsigned char)buf[j] );
	      strcat( buf2, number );
	   }
	 strcat( buf2, "\n" );
	 fputs( buf2, out );
      }
    fclose(in);
    fclose(out);
    return 0;
}
