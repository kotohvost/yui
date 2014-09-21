/*
	$Id: array.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "array.h"

const long maxCollectionSize = (long)((0x7ffffff - 16)/sizeof( void * ));

Array::Array( ccIndex aLimit, ccIndex aDelta ) :
    items( 0 ),
    count( 0 ),
    limit( 0 ),
    delta( aDelta )
{
    //setLimit( aLimit );
}

Array::Array() :
    items( 0 ),
    count( 0 ),
    limit( 0 ),
    delta( 8 )
{
    //setLimit( 8 );
}

Array::~Array()
{
  if (items)
    delete items;
}

void Array::atRemove( ccIndex index )
{
    if( index<0 || index >= count )
      error(2,index);

    count--;
    memmove( items+index*sizeOf(), items+(index+1)*sizeOf(), (count-index)*sizeOf() );
}

void Array::atInsert(ccIndex index, void *item)
{
    if( index < 0 )
	error(3,index);
    if( count == limit )
       {
	  ccIndex le=count/4;
	  if (le<delta)
	    le=delta;
	  setLimit(count + le);
       }
    long l=index*sizeOf();
    if (limit > count)
      {
	memmove( items+l+sizeOf(), items+l, (count-index)*sizeOf() );
	count++;
	memcpy(items+l, item, sizeOf());
	//items[index] = item;
      }
}

void Array::atPut( ccIndex index, void *item )
{
    if( index >= count || index<0 )
	error(4,index);

    //items[index] = item;
    memcpy(items+sizeOf()*index, item, sizeOf());
}

void Array::remove( void *item )
{
    ccIndex no=indexOf(item);
    if (no>=0)
      atRemove( no );
}

void Array::removeAll()
{
    count = 0;
}

void Array::error( ccIndex code, ccIndex index )
{
    fprintf(stderr, "\nCollection error: %s error code %ld index %ld count %ld limit %ld\n"
	    , code<=4 ? "index out of range" : ""
	    , code, index, count, limit);
    #ifdef unix
    raise(SIGQUIT);
    #else
    raise(SIGABRT);
    #endif
}


ccIndex Array::indexOf(void *item)
{
    for( ccIndex i = 0; i < count; i++ )
	if( !memcmp(item,items+i*sizeOf(), sizeOf()) )
	    return i;
    return -1;
}

ccIndex Array::insert( void *item )
{
    ccIndex ret=count;
    atInsert( count, item );
    return ret;
}

void Array::setLimit(ccIndex aLimit)
{
    if( aLimit < count )
	aLimit =  count;
    if( aLimit > maxCollectionSize)
	aLimit = maxCollectionSize;
    if (count > aLimit)
	count = aLimit;
    if( aLimit != limit )
	{
	char *aItems = new char [aLimit*sizeOf()];
	if( aItems && count !=  0 )
	  memcpy( aItems, items, count*sizeOf() );
	if (aItems)
	  {
	    if ( items )
		delete items;
	    items =  aItems;
	    limit =  aLimit;
	  }
	}
}

intArray::intArray( ccIndex aLimit, ccIndex aDelta ) :
	Array( aLimit, aDelta )
{
}

longArray::longArray( ccIndex aLimit, ccIndex aDelta ) :
	Array( aLimit, aDelta )
{
}

SortedArray::SortedArray( ccIndex aLimit, ccIndex aDelta) :
	Array( aLimit, aDelta ), duplicates(0)
{
}

SortedArray::SortedArray()
{
  duplicates = 0;
}

ccIndex SortedArray::indexOf(void *item)
{
    ccIndex  i;

    if( search( keyOf(item), i ) == 0 )
	return -1;
    else
	{
	if( duplicates )
	    {
	    while( i < count && compare(item, items+i*sizeOf()) )
		i++;
	    }
	if( i < count )
	    return i;
	else
	    return -1;
	}
}

ccIndex SortedArray::insert( void *item )
{
    ccIndex  i;
    if( search( keyOf(item), i ) == 0 || duplicates )   // order dependency!
       atInsert( i, item );                             // must do Search
    //else                                                // before calling
    //   return -1;                                       // AtInsert
    return i;
}

void *SortedArray::keyOf( void *item )
{
    return item;
}

int SortedArray::search( void *key, ccIndex& index )
{
    ccIndex l = 0;
    ccIndex h = count - 1;
    int res = 0;
    while( l <= h )
	{
	ccIndex i = (l +  h) >> 1;
	ccIndex c = compare( keyOf( items+i*sizeOf() ), key );
	if( c < 0 )
	    l = i + 1;
	else
	    {
	    h = i - 1;
	    if( c == 0 )
		{
		res = 1;
		if( !duplicates )
		    l = i;
		}
	    }
	}
    index = l;
    return res;
}

