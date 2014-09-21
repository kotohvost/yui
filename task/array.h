/*
	$Id: array.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef _ARRAY_H
#define _ARRAY_H

#ifndef ccIndex
#define ccIndex long
#endif

class Array
{
public:
    Array( ccIndex aLimit, ccIndex aDelta );
    Array();
    virtual ~Array();

    virtual int sizeOf() = 0;

    void *at( ccIndex index )
     {
      if (index<0 || index>=count)
	error(1,index);
      return ((char *)items)+ sizeOf()*index;
     }

    virtual ccIndex indexOf( void *item );

    void atRemove( ccIndex index );
    void remove( void *item );
    void removeAll();
    void freeAll(){ removeAll(); }

    void atInsert( ccIndex index, void *item );
    void atPut( ccIndex index, void *item );
    virtual ccIndex insert( void *item );
    void error( ccIndex code, ccIndex info );

    void setLimit( ccIndex aLimit );

    ccIndex getCount()
	  { return count; }
    void *getItems() {return (void *)items;}

protected:

    char *items;
    ccIndex count;
    ccIndex limit;
    ccIndex delta;
};

class intArray : public Array
{
public:
    intArray( ccIndex aLimit, ccIndex aDelta );
    int sizeOf() { return sizeof(int); }
};

class longArray : public Array
{
public:
    longArray( ccIndex aLimit, ccIndex aDelta );
    int sizeOf() { return sizeof(long); }
};

class SortedArray: public Array
{
public:
    SortedArray( ccIndex aLimit, ccIndex aDelta);
    SortedArray();
    virtual int search( void *key, ccIndex& index );

    virtual ccIndex indexOf( void *item );
    virtual ccIndex insert( void *item );

    int duplicates;
    virtual void *keyOf( void *item );

protected:
    virtual int compare( void *key1, void *key2 )=0;
};

#endif
