/*
	$Id: sortcol.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "collect.h"

SortedCollection::SortedCollection( ccIndex aLimit, ccIndex aDelta ) :
	Collection( aLimit, aDelta ), duplicates(0)
{
	_delta = aDelta;
	setLimit( aLimit );
}

SortedCollection::SortedCollection() : duplicates(0)
{
}

int SortedCollection::compare(void *key1, void *key2)
{
  return strcmp((char *)key1, (char *)key2);
}

ccIndex SortedCollection::indexOf(void *item)
{
    ccIndex  i;

    if( search( keyOf(item), i ) == 0 )
	return -1;
    else
	{
	if( duplicates )
	    {
	    while( i < count && item != items[i] )
		i++;
	    }
	if( i < count )
	    return i;
	else
	    return -1;
	}
}

ccIndex SortedCollection::insert( void *item )
{
    ccIndex  i;
    if( search( keyOf(item), i ) == 0 || duplicates )   // order dependency!
	atInsert( i, item );                            // must do Search
							// before calling
							// AtInsert
    return i;
}

void *SortedCollection::keyOf( void *item )
{
    return item;
}

int SortedCollection::search( void *key, ccIndex& index )
{
    ccIndex l = 0;
    ccIndex h = count - 1;
    int res = 0;
    while( l <= h )
      {
	ccIndex i = (l + h) >> 1;
	ccIndex c = compare( keyOf( items[i] ), key );
	if ( c < 0 )
	    l = i + 1;
	else
	  {
	    h = i - 1;
	    if ( c == 0 )
	      {
		res = 1;
		if ( !duplicates )
		    l = i;
	      }
	  }
      }
    index = l;
    return res;
}

SortedPtrCollection::SortedPtrCollection() : SortedCollection( 10, 10 )
{
	shouldDelete=0;
}

