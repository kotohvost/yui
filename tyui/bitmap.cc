/*
	$Id: bitmap.cc,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "bitmap.h"

bitmap::bitmap( long bits ) : map(0), len(0)
{
   resize( bits );
}

void bitmap::resize( long bits )
{
   long bytes = bits / sizeof(int) + 1;
   if ( len == bytes )
      return;
   if ( map )
       delete map;
   map=new unsigned [bytes];
   len = bytes;
   clear();
}

int bitmap::and( long no, int val )
{
   if ( !val )
      { clr( no ); return 0; }
   return get( no );
}

int bitmap::or ( long no, int val )
{
   if ( val )
      { set( no ); return 1; }
   return get( no );
}

void bitmap::inverse()
{
   for( long i=0; i<len; i++ )
      map[i] = ~map[i];
}

#ifdef EXAMPLE

#include <stdio.h>

int main()
{
   puts( "Bitmap of 32 bits:" );

   bitmap b( 64 );

   b.set(32);
   b.set(34);
   b.set(36);

   for( int i=0; i<8; i++ )
      printf( "%d", (b.get(32+i) ? 1:0) );
   puts( "" );
}

#endif

