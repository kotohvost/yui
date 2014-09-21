/*
	$Id: visual.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "visual.h"

bound visual::B(-1,-1,-1);

int visual::compare( void *p1, void *p2 )
{
   bound *b1 = (bound*)p1,
	 *b2 = (bound*)p2;
   if ( b2->top < b1->top )
      return 1;
   if ( b2->top > b1->bott )
      return -1;
   return 0;
}

void visual::remove( int top, int bott, ccIndex index )
{
   ccIndex i, i1, i2;
   B.top=top; B.bott=bott;
   if ( !search( &B, i1 ) )
      return;
   B.top=bott;
   if ( !search( &B, i2 ) )
      return;
   it = (bound**)items;
   while( ++i2 < count && it[i2-1]->top == it[i2]->top );
   for( i=i2-1; i>=i1; i-- )
      if ( it[i]->index == index )
	{
	  atFree( i );
	  it = (bound**)items;
	  i2--;
	}
   if ( count<=0 )
      return;
   //--------------------- попытки всяческих склеиваний ----------------
   ccIndex k = i1, kk=i1;
   int j;
   for( int ii=0; ii<2 && k<count; ii++, k=kk=i2 )
    {
      top = it[k]->top;
      for( i=1; ++k < count && top==it[k]->top; i++ );
      if ( k+i <= count )
	{
	  int flag = 1;
	  k=kk;
	  for( j=i; j>0; j--, k++ )
	    if ( it[k]->index != it[k+i]->index )
	       flag=0;
	  if ( k+i < count && it[k]->top == it[k+i]->top )
	     flag=0;
	  if ( flag )
	    {
	       k=kk;
	       top = it[k]->top;
	       for( j=i; j>0; j-- )
		 {
		   it[k+i]->top = top;
		   atFree( k );
		   it = (bound**)items;
		   i2--;
		 }
	    }
	}
      k=kk;
      if ( k-i < 0 )
	 return;
      int flag=1;
      for( j=i; j>0; j--, k++ )
	if ( it[k]->index != it[k-i]->index )
	   flag=0;
      if ( flag )
	{
	   k=kk;
	   bott = it[k]->bott;
	   for( j=i; j>0; j-- )
	     {
	       it[k-j]->bott = bott;
	       atFree( k );
	       it = (bound**)items;
	       i2--;
	     }
	}
    }
}

int visual::separator( ccIndex index, int pos )
{
   bound *b = it[index];
   int top = b->top, bott = b->bott, i=0;
   while( top==b->top )
    {
      b->bott = pos; i++; index++;
      if ( index >= count )
	 break;
      b = it[index];
    }
   pos++;
   for( int j=1; j<=i; j++ )
     atInsert( index, new bound( pos, bott, it[index-j]->index ) );
   it=(bound**)items;
   return i;
}

void visual::add( int top, int bott, ccIndex index )
{
   if ( top > bott ) return;
   B.top = top; B.bott=bott; B.index=index;
   bound *bb, *b2;
   ccIndex i1=-1, i2=-1;
   int r1 = search( &B, i1 );
   B.top = bott;
   int r2 = search( &B, i2 );
   int t=top, b=bott;
   it = (bound**)items;
   //---------------- границы внутри одного диапазона --------------
   if ( i1==i2 )
     {
       if ( r1 )
	 {
	   b2=it[i1];
	   if ( top==b2->top )
	     {
m1:
	       if ( bott==b2->bott )
		 {
m2:
		   bb = new bound( t, b, index );
		   insert( bb );
		   return;
		 }
	       separator( i2, bott );
	       goto m2;
	     }
	   i2 += separator( i1, top-1 );
	   b2=it[i1+1];
	   goto m1;
	 }
       if ( !r2 )
	  goto m2;
       b2=it[i2];
       insert( new bound( top, b2->top-1, index ) );
       t=b2->top;
       i2++;
       goto m1;
     }
   //------------------- границы внутри разных диапазонов --------------
   if ( !r1 )
     {
       i1 = insert( new bound( t, it[i1]->top - 1, index ) );
       i2++;
     }
   else
     {
       int pr=0;
       if ( t != it[i1]->top )
	  pr = separator( i1, t-1 );
       i1 = insert( new bound( t, it[ i1+pr ]->bott, index ) );
       i2 += pr + 1;
     }
   it = (bound**)items;
   if ( r2 )
     {
       if ( b != it[i2]->bott )
	  separator( i2, b );
       i2 = insert( new bound( it[i2]->top, b, index ) );
     }
   else
       i2 = insert( new bound( it[i2-1]->bott+1, b, index ) );

   it = (bound**)items;
   t = it[i1]->top;
   while( ++i1 < count && t == it[i1]->top );
   t = it[i2]->top;
   while( i2>i1 )
     {
       b2 = it[--i2];
       if ( t == b2->top )
	   continue;
       if ( b2->bott+1 == t )
	   insert( new bound( t=b2->top, b2->bott, index ) );
       else
	   insert( new bound( t=b2->bott+1, t-1, index ) );
       i2++;
     }
   if ( it[i1-1]->bott+1 != it[i1]->top )
      insert( new bound( it[i1-1]->bott+1, it[i1]->top-1, index ) );
}

void visual::region( int top, int bott, int *map )
{
   if ( count <= 0 )
      return;
   ccIndex i1=-1, i2=-1;
   B.top=top; B.bott=bott;
   search( &B, i1 );
   if ( i1 >= count )
      return;
   B.top=bott;
   int r=search( &B, i2 );
   if ( !r )
      i2--;
   if ( i2<0 ) return;
   it = (bound**)items;
   while( ++i2 < count && it[i2-1]->top == it[i2]->top );
   for( ccIndex i=i1, ind; i<i2; i++ )
     {
       ind = it[i]->index;
       map[ ind >> 5 ] |= 1 << ( 31 - ind & 31 );
     }
}
