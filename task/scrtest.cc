/*
	$Id: scrtest.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "term.h"
#include "screen.h"

int main()
{
   Screen::initScreen( BASE_TERMINFO );
//   Screen::initScreen( BASE_TERMCAP );
   Screen::clearScreen();
//   Screen::attrSet( Rect(0,0,25,80), BLACK | BACK(BLUE) );
   Screen::frame( Rect(12,0,15,6) );
//   Screen::sync();
//   sleep(2);

   for( int x=0, y=0; x<16; x++, y++ ) {
      Screen::put( y, x, (unsigned char*)"Test line (русский текст)", x | BG_BLACK );
//      Screen::move( 10, 5 );
//      Screen::sync(0,1);
//      sleep(1);

//      Screen::put( 2, 4, (unsigned char*)"Test line 3" );
//      Screen::sync(2,1);
//      sleep(1);
    }
   Screen::sync();
   getc( stdin );

#ifdef MALLOC_DEBUG
   malloc_shutdown();
#endif
}
