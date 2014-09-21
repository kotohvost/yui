/*
	$Id: collect.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef _COLLECTION_H
#define _COLLECTION_H

#define ccIndex long

class Collection
{
public:
    Collection( ccIndex aLimit, ccIndex aDelta );
    Collection();
    virtual ~Collection();

    void *at( ccIndex index )
     {
      if (index<0 || index>=count)
	error(1,index);
      return items[index];
     }

    virtual ccIndex indexOf( void *item );

    void atFree( ccIndex index );
    void atRemove( ccIndex index );
    void remove( void *item );
    void removeAll();
    void freeAll();

    void atInsert( ccIndex index, void *item );
    void atPut( ccIndex index, void *item );
    virtual ccIndex insert( void *item );
    virtual ccIndex minsert(void *item);
    void error( ccIndex code, ccIndex info );

    void setLimit( ccIndex aLimit );

    ccIndex getCount()
	  { return count; }
    void **getItems() {return items;}

protected:

    void **items;
    ccIndex count;
    ccIndex limit;
    ccIndex _delta;
    int shouldDelete;

    virtual void freeItem( void *item );

};

class SortedCollection: public Collection
{
public:
    SortedCollection( ccIndex aLimit, ccIndex aDelta );
    SortedCollection();
    virtual int search( void *key, ccIndex& index );
    virtual ccIndex indexOf( void *item );
    virtual ccIndex insert( void *item );
    virtual void *keyOf( void *item );
    int duplicates;

private:
    virtual int compare( void *key1, void *key2 ) ;

};

class SortedPtrCollection : public SortedCollection
{
protected:
   int compare( void *p1, void *p2 )
      { return int(p1 < p2 ? -1 : (p1 > p2 ? 1 : 0)); }
public:
   SortedPtrCollection();
   void setShouldDelete( int del ) { shouldDelete=del; }
};

#endif
