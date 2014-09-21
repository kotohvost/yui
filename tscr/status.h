/*
	$Id: status.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _STATUS_H_
#define _STATUS_H_

#include "window.h"

/*
struct statusColor
{
   unsigned char normal;
   unsigned char light;
};
*/
extern unsigned char monoStatus[2];
extern unsigned char colorStatus[2];
extern unsigned char laptopStatus[2];

class StatusLine : public Window
{
public:
   StatusLine();
   virtual int 	isType( long typ );
   virtual void setColors( int type );
   int init( void *data=0 );
   virtual int draw( int Redraw=0, int sy=0, int sx=0 ) { return 1; }
   int draw( char *str, int ignoreVisible=0 );
   void putStr( const char *str );
   virtual void setScrMap();
   char visible;
};

#endif
