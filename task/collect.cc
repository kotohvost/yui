/*
	$Id: collect.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "collect.h"

const long maxCollectionSize = (long)((0x7ffffff - 16)/sizeof( void * ));


Collection::Collection( ccIndex aLimit, ccIndex aDelta ) :
    items( 0 ),
    count( 0 ),
    limit( 0 ),
    _delta( aDelta ),
    shouldDelete( 1 )
{
    //fprintf(stderr, "Collection constructor %d %d \n", aLimit, aDelta); fflush(stderr);
    setLimit( aLimit );
}

Collection::Collection() :
    items( 0 ),
    count( 0 ),
    limit( 0 ),
    _delta( 2 ),
    shouldDelete( 1 )
{
    setLimit( 10 );
}

Collection::~Collection()
{
    if( shouldDelete )
	  freeAll();
    //setLimit(0);
    if (items)
//     {
//      memset(items, sizeof(void *)*limit, 0);
      delete items;
//     }
}

/*
void *Collection::at( ccIndex index )
{
    if (index<0 || index>=count)
	error(1,index);
    return items[index];
}
*/
void Collection::atRemove( ccIndex index )
{
    if( index<0 || index >= count )
      error(2,index);

    count--;
    memmove( &items[index], &items[index+1], (count-index)*sizeof(void *) );
}

void Collection::atFree( ccIndex index )
{
    if( index<0 || index >= count )
      error(2,index);
    void *item = at( index );
    atRemove( index );
    Collection *cp=this;
    cp->freeItem( item );
}

void Collection::atInsert(ccIndex index, void *item)
{
    if( index < 0 )
	error(3,index);
    if( count == limit )
       {
	  ccIndex le=count/4;
	  if (le<_delta)
	    le=_delta;
	  setLimit(count + le);
       }
    if (limit > count)
      {
	memmove( &items[index+1], &items[index], (count-index)*sizeof(void *) );
	count++;
	items[index] = item;
      }
}

void Collection::atPut( ccIndex index, void *item )
{
    if( index >= count || index<0 )
	error(4,index);

    items[index] = item;
}

void Collection::remove( void *item )
{
    ccIndex no=indexOf(item);
    if (no>=0)
      atRemove( no );
}

void Collection::removeAll()
{
    count = 0;
}

void Collection::error( ccIndex code, ccIndex index )
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

/*
void Collection::free( void *item )
{
    remove( item );
    Collection *cp=this;
    cp->freeItem( item );
}
*/

void Collection::freeAll()
{
    for( ccIndex i =  0; i < count; i++ )
      {
	Collection *cp=this;
	void *item = items[i];  //at(i);
	if ( item )
	   cp->freeItem( item );
      }
    count = 0;
}

void Collection::freeItem( void *item )
{
    delete item;
}


ccIndex Collection::indexOf(void *item)
{
    for( ccIndex i = 0; i < count; i++ )
	if( item == items[i] )
	    return i;

    return -1;
}

ccIndex Collection::insert( void *item )
{
    ccIndex ret=count;
    atInsert( count, item );
    return ret;
}

ccIndex Collection::minsert(void *item)
{
  for( ccIndex i=0; i<getCount(); i++ )
  {
    if( !at(i) )
    {
      //Šª±¥ Šª©¼ º¢ª©¹Š  Šª©¹ ½ †Šº¹¸Š ‚‰º¥ªŠ»
      atPut(i,item);
      return i;
    }
  }
  return insert(item);
}

/*
void Collection::pack()
{
    void **curDst = items;
    void **curSrc = items;
    void **last = items + count;
    while( curSrc < last )
	{
	if( *curSrc != 0 )
	    *curDst++ = *curSrc;
	*curSrc++;
	}
}
*/

void Collection::setLimit(ccIndex aLimit)
{
    if( aLimit < count )
	aLimit =  count;
    if( aLimit > maxCollectionSize)
	aLimit = maxCollectionSize;
    if (count > aLimit)
	count = aLimit;
    if( aLimit != limit )
	{
	void **aItems;
	//if (aLimit == 0 )
	//    aItems = 0;
	//else
	    {
	    aItems = new void *[aLimit];
	    if( aItems && count !=  0 )
		memcpy( aItems, items, count*sizeof(void *) );
	    }
	if (aItems)
	  {
	    if ( items )
		delete items;
	    items =  aItems;
	    limit =  aLimit;
	  }
	}
}
