/*
	$Id: gdb.h,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#ifndef _GDB_H
#define _GDB_H

#include <reg_expr.h>
#include <program.h>
#include <yterm.h>
#include <array.h>

class Edit;

#define DEBUG_LINES	100

struct editRect
{
   Edit *e;
   Rect r;
   editRect( Edit *E, Rect &R ): e(E), r(R) {;}
};

class MemRects: public SortedArray
{
public:
   int sizeOf() { return sizeof(editRect); }
   editRect & operator [](ccIndex index)
		{ return *(editRect*)items[index]; }
   ccIndex insert( editRect &item )
		{ return SortedArray::insert( &item ); }
   void remove( editRect &item )
		{ SortedArray::remove( &item ); }
   void atInsert( ccIndex index, editRect &item )
		{ SortedArray::atInsert( index, &item ); }
   void atPut( ccIndex index, editRect &item )
		{ SortedArray::atPut( index, &item ); }
   int search( editRect &key, ccIndex &index )
		{ return SortedArray::search( &key, index ); }
   MemRects() : SortedArray( 5, 5 ) {;}

protected:
   int compare( void *p1, void *p2 );
};

class Debug : public YTerm
{
protected:
   static Regexpr expBpoint;
   static Regexpr expLine;
//   static Regexpr expDelete;
   static char Buf[256];
   unsigned started:1;
   Edit *E;
   void setBreakpoint( char *str );
   void unsetBreakpoint( int number );
   MemRects memRects;
   Collection startCommands;

public:
   Debug( Rect r );
   ~Debug();
   static int flagInit;
   int init( void *data=0 );
   long handleKey( long key, void *&ptr );
   void processStr( const char *str, int str_len );
};

extern Task *debug;

#endif
