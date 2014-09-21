/*
	$Id: bitmap.h,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#ifndef _BITMAP_H
#define _BITMAP_H

// Отсутствуют проверки на выход за границы массива!

#define MASK	0x80000000
#define BITS	31
#define COUNT	5

class bitmap
{
protected:
   unsigned *map;
   long len;

public:
   bitmap( long bits );
   ~bitmap() { if( map ) delete map; }

   void resize( long bits );

   inline int get( long no )
	{ return map[no>>COUNT] & (MASK >> (no & BITS)); }
   inline void set( long no )
	{ map[no>>COUNT] |= MASK >> (no & BITS); }

   inline void clr( long no )
	{ map[no>>COUNT] &= ~(MASK >> (no & BITS)); }

   int and( long no, int val );
   int or ( long no, int val );

   void inverse();

   inline void clear()
	{ memset( map, 0, len*sizeof(int) ); }
   inline void setall()
	{ memset( map, 0xff, len*sizeof(int) ); }
};

#endif
