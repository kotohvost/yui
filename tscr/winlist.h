/*
	$Id: winlist.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _WINDOWS_LIST
#define _WINDOWS_LIST

#include "listbox.h"

class winList : public listBox
{
protected:
   int *no;
   void setSelectedWin();
public:
   winList( int *n );
   ~winList();
   virtual void resizeScreen( Rect &old );
   virtual int isType(long typ);
   long handleKey( int long, void *&ptr );
   int init( void *data=0 );
};

#endif  /* _WINDOWS_LIST */
