/*
	$Id: sema.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef SEMA_H
#define SEMA_H

#include "list.h"
#include "task.h"

class Sema
{
friend class Task;
  Sema *next;
  Sema *prev;
  TaskList tasks;

public:
  Sema(){;}
};

class SemaList: public TList
{
  virtual void *getNext(void *item) { return (void*) ((Sema*)item)->next; }
  virtual void *getPrev(void *item) { return (void*) ((Sema*)item)->prev; }
  virtual void setNext(void *item, void *next) { ((Sema*)item)->next = (Sema*)next; }
  virtual void setPrev(void *item, void *prev) { ((Sema*)item)->prev = (Sema*)prev; }
  virtual void freeItem(void *item) { delete (Sema*)item; }
public:
  SemaList() { shouldDelete=1; }
  ~SemaList(){ freeAll(); }
  Sema *item()   { return (Sema*) getCurrent(); }
};

#endif
