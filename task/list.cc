/*
	$Id: list.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "list.h"

int  TList::first()
{
  if (Head)
   {
    Current=Head;
    return 1;
   }
  return 0;
}

int  TList::last()
{
  if (Head)
   {
     Current=getPrev(Head);
     return 1;
   }
  return 0;
}

int  TList::next()
{
  if (Head)
   {
     void *item= getNext(Current);
     if (item!=Head)
      {
	Current=item;
	return 1;
      }
   }
  return 0;
}

void TList::Next()
{
  if (Head)
     Current=getNext(Current);
}

int TList::prev()
{
  if (Head && Current!=Head )
   {
     Current=getPrev(Current);
     return 1;
   }
  return 0;
}

void TList::Prev()
{
  if (Head)
     Current=getPrev(Current);
}

int  TList::seek(void *item)
{
  if (!Head)
    return 0;

  for(void *p=Head; ; )
   {
     if (p==item)
      {
	Current=p;
	return 1;
      }
     p=getNext(p);
     if(p==Head)
       return 0;
   }
  return 0;
}


void  TList::insert(void *item)
{
  if (!Head)
   {
     Head=Current=item;
     setNext(item, item);
     setPrev(item, item);
   }
  else
   {
     void *p=getNext(Current);
     setNext(Current, item);
     setPrev(p, item);
     setPrev(item, Current);
     setNext(item, p);
     Current=item;
   }
}

void  TList::insertBefore(void *item)
{
  if (!Head)
   {
     Head=Current=item;
     setNext(item, item);
     setPrev(item, item);
   }
  else
   {
     void *p=getPrev(Current);
     setPrev(Current, item);
     setNext(p, item);
     setNext(item, Current);
     setPrev(item, p);

     if (Current==Head)
       Head=item;
     Current=item;
   }
}

void  TList::append(void *item)
{
  if (!Head)
   {
     Head=Current=item;
     setNext(item, item);
     setPrev(item, item);
   }
  else
   {
     void *tail=getPrev(Head);

     setNext(tail, item);
     setPrev(Head, item);
     setPrev(item, tail);
     setNext(item, Head);
   }
}

void  TList::prepend(void *item)
{
  if (!Head)
   {
     Head=Current=item;
     setNext(item, item);
     setPrev(item, item);
   }
  else
   {
     void *tail=getPrev(Head);
     setPrev(Head, item);
     setNext(tail, item);
     setNext(item, Head);
     setPrev(item, tail);

     Head=item;
   }
}


int  TList::remove(void *item)
{
   if (seek(item))
    {
     remove();
     return 1;
    }
   return 0;
}

int  TList::free_item(void *item)
{
   if (seek(item))
    {
     free_cur();
     return 1;
    }
   return 0;
}

void  TList::remove()
{
   if (!Head)
     return;
   void *p=getPrev(Current);
   void *n=getNext(Current);

   if (p==Current) // last member
     Head=Current=0;
   else
    {
      setNext(p,n);
      setPrev(n,p);
      if (Head==Current)
	Head=Current=n;
      else
	Current=n;
    }
}

void  TList::free_cur()
{
   if (!Head)
     return;
   void *p=getPrev(Current);
   void *n=getNext(Current);

   if (p==Current)
    {
      Head=Current=0;
      freeItem(p);
    }
   else
    {
      setNext(p,n);
      setPrev(n,p);
      void *c=Current;
      if (Head==Current)
	Head=Current=n;
      else
	Current=n;
      freeItem(c);
    }
}

void TList::put(void *item)
{
  if (!Head)
    return;
  setNext(item, getNext(Current));
  setPrev(item, getPrev(Current));
  if (Head==Current)
    Head=item;
  Current=item;
}

void TList::putFree(void *item)
{
  if (!Head)
    return;
  setNext(item, getNext(Current));
  setPrev(item, getPrev(Current));
  if (Head==Current)
    Head=item;
  freeItem(Current);
  Current=item;
}

void TList::removeAll()
{
  Current=Head=0;
}

void TList::freeAll()
{
  if (!Head)
    return;

  void *p, *n;

  for( p=Head ; ; )
    {
      n = getNext(p);
      freeItem(p);
      if (n==Head)
	break;
      p = n;
    }

  Current=Head=0;
}

void TList::removeIt(void *item)
{
  if (Current!=item)
   {
     void *curr=Current;
     Current=item;
     remove();
     Current=curr;
   }
  else
    remove();
}

void TList::freeIt(void *item)
{
  if (Current!=item)
   {
     void *curr=Current;
     Current=item;
     free_cur();
     Current=curr;
   }
  else
    free_cur();
}

