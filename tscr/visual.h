/*
	$Id: visual.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _VISUAL_H
#define _VISUAL_H

#include "collect.h"
#include "point.h"

struct bound
{
   int top;
   int bott;
   ccIndex index;
   bound( int t, int b, ccIndex i ) : top(t), bott(b), index(i) {;}
};

class visual : public SortedCollection
{
protected:
   void freeItem( void *item ) { delete (bound*)item; }
   int compare( void *p1, void *p2 );
   int separator( ccIndex index, int pos );
   bound **it;
   static bound B;

public:
   visual() : SortedCollection(50,50) { duplicates=1; }
   virtual ~visual() { freeAll(); }
   void add( int top, int bott, ccIndex index );
   void remove( int top, int bott, ccIndex index );
   void region( int top, int bott, int *map );
};

#endif
