/*
	$Id: sema.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "task.h"


void *SemaList::getNext(void *item)
  { return (void*) ((SemaBody*)item)->next; }

void *SemaList::getPrev(void *item)
  { return (void*) ((SemaBody*)item)->prev; }

void SemaList::setNext(void *item, void *next)
  { ((SemaBody*)item)->next = (SemaBody*)next; }

void SemaList::setPrev(void *item, void *prev)
  { ((SemaBody*)item)->prev = (SemaBody*)prev; }

void SemaList::freeItem(void *item)
  { SemaBody *sp=(SemaBody*)item; if (-- (sp->count) <1) delete sp; }

SemaList::SemaList()
  {;}

SemaList::~SemaList()
  { freeAll(); }

SemaBody *SemaList::item()
  { return (SemaBody*) getCurrent(); }

void SemaList::makeZombie()
{
  for(int i=first(); i; i=next() )
   {
     SemaBody *sb=item();
     for(int j=sb->tasks.first(); j; j=sb->tasks.next() )
       sb->tasks.item()->state=Task::Zombie;
   }
}


SemaBody::SemaBody(): next(0), prev(0), count(0), value(0)
{
}

SemaBody::~SemaBody()
{
}

void SemaBody::addSema(Sema *sem, Task *tp)
{
  tp->state = Task::Sem;
  tp->priority = sem->priority;
  tp->sem = this;

  for(int n=tasks.first(); n; n=tasks.next() )
   if ( sem->priority > tasks.item()->priority )
    {
     tasks.insertBefore(tp);
     return;
    }
  tasks.append(tp);
}

void SemaBody::removeTask(Task *tp)
{
  if (tasks.remove(tp))
    tp->refcount--;
  if (value)
   {
    //taskcount--;
    value--;
   }
}

int Sema::taskcount=0;

Sema::Sema(int prio, int closed): priority(prio)
{
  sp=new SemaBody;
  sp->count++;
  Task::semaList.append(sp);
  if (closed)
    sp->value=closed;

}

Sema::Sema( Sema &sema, int prio): priority(prio)
{
  sp=sema.sp;
  sp->count++;
}

Sema::Sema( Sema &sema): priority(sema.priority)
{
  sp=sema.sp;
  sp->count++;
}

Sema::~Sema()
{
  if (!sp)
    return;

  Task::semaList.freeIt(sp);
}

int Sema::close()
{
  if (!sp)
    return 0;
  if( sp->value > 0 )
   {
     Task *tp=Task::currTask;
     Task::readyTasks.removeIt(tp);
     sp->addSema(this, tp);
     taskcount++;
     sp->value++;
     Task::yield0();
   }
  else // ªŠ ‰¯¹· ª½¹ˆ¹€Š¾
   {
     sp->value++;

     // ·‰‚ˆ±¹¸¥·¹½‰©¼ ‚‰€‰¥, ¤€¢Ž¥Š ‚‰«½‰©‰ ªŠ ‰¯¹·‰
     for(int i=sp->tasks.first(); i; i=sp->tasks.next() )
      {
	Task *tp=sp->tasks.item();
	sp->tasks.remove();
	taskcount--;
	tp->refcount--;
	tp->addToReady();
      }

     Task::yield();
   }
  return 1;
}

int Sema::waitClose()
{
  if (!sp)
     return 0;

  Task *tp;
  if ( ! sp->value )
   {
     Task *tp=Task::currTask;
     Task::readyTasks.removeIt(tp);
     sp->addSema(this, tp);
     taskcount++;
     Task::yield0();

     return 1;
   }
  return 1;
}


int Sema::open()
{
  if (!sp)
     return 0;

  Task *tp;
  if ( sp->value>1 )
   {
     if (sp->tasks.first())
      {
	tp=sp->tasks.item();
	sp->tasks.remove();
	taskcount--;
	tp->refcount--;
	tp->addToReady();
      }
     sp->value--;
     return 1;
   }
  sp->value=0;
  return 1;
}

