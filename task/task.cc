/*
	$Id: task.cc,v 3.2.2.2 2008/08/09 17:51:48 shelton Exp $
*/
#include <string.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>

#if defined(SCO_SYSV) || defined(AIX_PPC)
#include <sys/select.h>
#else
#include <sys/time.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <signal.h>

#ifdef BSDI		/* void exit( int status ) */
#include <stdlib.h>
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "task.h"
#include "hashcode.h"

#if defined(LINUX_X86_LIBC6) || defined(LINUX_ALPHA_LIBC6) \
 || defined(SOLARIS_25) || defined(SOLARIS_26_X86)
  #define TASK_STACK_MIN	16384
#else
  #if defined(AIX_PPC)
     #define TASK_STACK_MIN	49152
  #else
     #define TASK_STACK_MIN	4096
  #endif
#endif

TaskList Task::readyTasks;
TaskList Task::waitTasks;
TaskList Task::zombieTasks;
TaskList Task::msgTasks;
TaskList Task::respTasks;
SemaList Task::semaList;
TaskCollection Task::allTasks;

u_long Task::seqNo=1;
Task *Task::currTask=0;
int Task::fullcount=0;
fd_set Task::readFiles;
fd_set Task::writeFiles;
fd_set Task::exceptFiles;
jmp_buf Task::schEnv;
static struct tms tms_buf;

#if defined( SCO_SYSV )
  #undef MAP_ANON
#else
  #include <sys/mman.h>
#endif

// ][ Task

void Task::addref(Task *tp)
{
  tp->refcount++;
}

void Task::delref(Task *tp)
{
  if (tp && --(tp->refcount)<1)
    delete tp;
}

TaskCollection::TaskCollection()
{
}

TaskCollection::~TaskCollection()
{
  removeAll();
}

void TaskCollection::freeItem( void *item )
{
  // они удаляются сами
}

int TaskCollection::compare( void *key1, void *key2 )
{
  u_long k1=*(u_long*)key1;
  u_long k2=*(u_long*)key2;
  return k1<k2 ? -1 : ( k1>k2 ? 1:0 );
}

void *TaskCollection::keyOf(void *item)
{
  return &(((Task*)item) -> no);
}


void *TaskList::getNext(void *item) { return (void*) ((Task*)item)->next; }
void *TaskList::getPrev(void *item) { return (void*) ((Task*)item)->prev; }
void TaskList::setNext(void *item, void *next) { ((Task*)item)->next = (Task*)next; }
void TaskList::setPrev(void *item, void *prev) { ((Task*)item)->prev = (Task*)prev; }
void TaskList::freeItem(void *item)
   { if ( --(((Task*)item)->refcount)<1 ) delete (Task*)item; }
Task *TaskList::item(){ return (Task*) getCurrent(); }

void TaskList::makeZombie()
{
  for(int i=first(); i; i=next() )
     item()->state=Task::Zombie;
}

static void FD_CLR_BY(fd_set *fds, fd_set *mask)
{
  unsigned int sz = sizeof(fd_set) / sizeof(unsigned int);
  unsigned int *fptr = (unsigned int*)fds;
  unsigned int *mptr = (unsigned int*)mask;
  for( unsigned int i=0; i < sz; i++, fptr++, mptr++ )
      *fptr &= ~(*mptr);
}

static void FD_AND_BY(fd_set *fds, fd_set *mask)
{
  unsigned int sz = sizeof(fd_set) / sizeof(unsigned int);
  unsigned int *fptr = (unsigned int*)fds;
  unsigned int *mptr = (unsigned int*)mask;
  for( unsigned int i=0; i < sz; i++, fptr++, mptr++ )
      *fptr &= *mptr;
}

static void FD_SET_BY(fd_set *fds, fd_set *mask)
{
  unsigned int sz = sizeof(fd_set) / sizeof(unsigned int);
  unsigned int *fptr = (unsigned int*)fds;
  unsigned int *mptr = (unsigned int*)mask;
  for( unsigned int i=0; i < sz; i++, fptr++, mptr++ )
      *fptr |= *mptr;
}

static int FD_ISSET_BY(fd_set *fds, fd_set *mask)
{
  unsigned int sz = sizeof(fd_set) / sizeof(unsigned int);
  unsigned int *fptr = (unsigned int*)fds;
  unsigned int *mptr = (unsigned int*)mask;
  for( unsigned int i=0; i < sz; i++, fptr++, mptr++ )
     if ( *fptr & *mptr )
	 return 1;
  return 0;
}

long TaskMessage::type()
{
  return HASH_Task;
}

TaskHolder::TaskHolder( Task *Tp ): tp(Tp)
{
  tp->start();
  Task::addref(tp);
}

TaskHolder::~TaskHolder()
{
  tp->suicide();
  Task::delref(tp);
}

void Task::destroy()
{
}

void Task::addTo(TaskList &list)
{
  list.prepend(this);
  refcount++;
}

void Task::addToReady()
{
  readyTasks.prepend(this);
  refcount++;
  state=Ready;
}

void Task::addToZombie()
{
  zombieTasks.prepend(this);
  refcount++;
  state = Zombie;
}



void Task::addToWait()
{
  if (rfile)
    { FD_SET_BY( &readFiles, &rfileset ); }
  if (wfile)
    { FD_SET_BY( &writeFiles, &wfileset ); }
  if (efile)
    { FD_SET_BY( &exceptFiles, &efileset ); }

  refcount++;
  state = Wait;

  for(int n=waitTasks.first(); n; n=waitTasks.next() )
   if (wakeUp <= waitTasks.item()->wakeUp)
    {
     waitTasks.insertBefore(this);
     return;
    }

  waitTasks.append(this);
}

void Task::addToMsg()
{
  refcount++;
  state = Msg;
  msgTasks.append(this);
}

void Task::addToResp()
{
  refcount++;
  state = Resp;
  respTasks.append(this);
}


void Task::removeFrom(TaskList &list)
{
  if ( list.remove(this) )
    refcount--;
}

void Task::RemoveFrom(TaskList &list)
{
  list.removeIt(this);
  refcount--;
}


Task::~Task()
{
  allTasks.remove(this);
  #ifdef MAP_ANON
  if (isMapped)
    munmap((char*)stack,stklen);
  else
  #else
    #ifdef USE_SHMEM
    if (isMapped)
       shmdt(stack);
    else
    #endif
  #endif
    delete stack;
}


Task::Task(const char *Name, int Stklen):
      tskname(Name), refcount(0), isMapped(0), rfile(0), wfile(0), efile(0),
      next(0)
{
    fInited = 0;
    fTimed = 0;
    fStarted = 0;
    parent = 0;

    if (++seqNo==0)
       seqNo=1;
    no=seqNo;

    if (Stklen < TASK_STACK_MIN)
	Stklen = TASK_STACK_MIN;

    #if defined (MAP_ANON) || defined (USE_SHMEM)
    int psize = getpagesize();
    if ( Stklen >= psize )
     {
       int n = Stklen/psize;
       int rest = Stklen % psize;
       Stklen = n*psize + (rest?psize:0);
       isMapped=1;
     }
    #endif

    stklen = Stklen;
    stack = 0;
    state = Zombie;

    if ( !fullcount )
     {
       // static constructor
       FD_ZERO(&readFiles);
       FD_ZERO(&writeFiles);
       FD_ZERO(&exceptFiles);
       readyTasks.freeAll();
       waitTasks.freeAll();
       zombieTasks.freeAll();
       msgTasks.freeAll();
       allTasks.removeAll();
       currTask=0;
     }

    fullcount++;

    allTasks.insert(this);
}


void Task::start()
{
  if (!fStarted)
    {
      #ifdef MAP_ANON
      if (isMapped)
        {
   	  stack = (unsigned char *)mmap(0, stklen, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
          if (MAP_FAILED == stack)
     	    {
       	      const char *msg = "Cannot mmap for stack in Task::start()\n";
       	      write(2, msg, strlen(msg));
       	      const char *msg2 = strerror(errno);
       	      write(2, msg2, strlen(msg2));
       	      ::sleep(5);
       	      exit(100);
           }
        }
       else
       #else
       #ifdef USE_SHMEM
         if (isMapped)
           {
             int id=shmget(IPC_PRIVATE, stklen, 0666 | IPC_CREAT);

       	     stack = shmat( id, 0, 0666 );
       	     shmctl(id, IPC_RMID, 0 );

       	     if ( id==-1 ||  -1==(int)stack || stack==0)
	       {
	         const char *msg="Cannot shmget for stack in Task::start()\n";
	         write(2, msg, strlen(msg));
	         const char *msg2=strerror(errno);
	         write(2, msg2, strlen(msg2));
	         ::sleep(5);
	         exit(100);
	       }
           }
          else
          #endif
          #endif
            {
	      stack = new unsigned char [stklen];
	    }

      addToReady();
      fStarted=1;
    }
}

void callTaskMain(Task *t)
{
    long r=t->main();      //  call the main function ...
    if (t->parent)
     {
       t->parent->chieldRes=r;
       t->parent->addToReady();
     }
    t->suicide();   //  if it returns, the task wants to die
    // Task::yield0();
}

#if defined(SOLARIS_25)

/*
 * Constants defining a stack frame.
 */
#define WINDOWSIZE	(16*4)		/* size of window save area */
#define ARGPUSHSIZE	(6*4)		/* size of arg dump area */
/*#define ARGPUSH		(WINDOWSIZE+4)*/ /* arg dump area offset */
#define MINFRAME	(WINDOWSIZE+ARGPUSHSIZE+4) /* min frame */

/*
 * Stack alignment macros.
 */
#define STACK_ALIGN	8
#define SA(X)		(((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))

#endif


void Task::init()
{
    fInited = 1;    //  we are initialized

    //  switch to private stack now

    static jmp_buf  stkswitch;

    if (!setjmp(stkswitch)) {

    #undef UNKNOWN_SYSTEM
    #define UNKNOWN_SYSTEM

    #ifdef __TURBOC__
	uchar far   *farstk = (uchar far *)stack;

	stkswitch[ 0 ].j_ss = FP_SEG(farstk);
	stkswitch[ 0 ].j_sp = FP_OFF(farstk) + stklen;
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef DJGPP
	stkswitch[ 0 ].__esp = (unsigned int)(stack + stklen - sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef LINUX_X86_LIBC5
	stkswitch[ 0 ].__sp = stack + stklen-sizeof(jmp_buf);
	#undef UNKNOWN_SYSTEM
    #endif

    #if defined(LINUX_X86_LIBC6)
	stkswitch[0].__jmpbuf[JB_SP] = (unsigned int)(stack + stklen - sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #if defined(LINUX_ALPHA_LIBC6)
	stkswitch[0].__jmpbuf[JB_SP] = (long int)(stack + stklen - sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef BSDI
	stkswitch[ 2 ] = (unsigned int)(stack + stklen-sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef FreeBSD
    	#ifdef amd64
	stkswitch[0]._jb[ 2 ] = (long int)(stack + stklen-sizeof(jmp_buf));
        #else
	stkswitch[0]._jb[ 2 ] = (unsigned int)(stack + stklen-sizeof(jmp_buf));
        #endif
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef NETBSD
	((unsigned*)&stkswitch)[ 2 ] = (unsigned int)(stack + stklen-sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef SUNOS
	stkswitch[ 0 ].__fp = stack + stklen-sizeof(jmp_buf);
	#undef UNKNOWN_SYSTEM
    #endif

    #if defined(SOLARIS_26_X86) || defined(SCO_SYSV)
	((unsigned*)&stkswitch)[ 4 ] = (unsigned int)(stack + stklen-sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #if defined(SOLARIS_25)
	unsigned char *sp = (unsigned char*)stack + stklen;
	sp -= WINDOWSIZE + SA(MINFRAME);
	sp = (unsigned char*)((unsigned long)(sp) & 0xfffffff8);
	((void**)&stkswitch)[ 1 ] = (void*)sp;
	#undef UNKNOWN_SYSTEM
    #endif

    #if defined(AIX_PPC)
	((unsigned*)&stkswitch)[ 3 ] = (unsigned int)(stack + stklen-sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #if defined(SINIX)
	((unsigned*)&stkswitch)[ _JB_SP ] = (unsigned int)(stack + stklen-sizeof(jmp_buf));
	#undef UNKNOWN_SYSTEM
    #endif

    #ifdef UNKNOWN_SYSTEM
      #error Unknown System!
    #endif
	longjmp(stkswitch, 1);
    }

    //  start task running ...
    callTaskMain(currTask); //  ... and never return
}

void Task::suicide()
{
  switch ( state )
   {
     case Zombie: return;
     case Ready:  RemoveFrom(readyTasks); break;
     case Wait:   RemoveFrom(waitTasks); break;
     case Msg:	  RemoveFrom(msgTasks); break;
     case Resp:   RemoveFrom(respTasks); break;
     case Sem:    sem->removeTask(this); break;
   }

  currTask=0;
  addToZombie();
  destroy();
  if (! setjmp(tskEnv) )
      longjmp( schEnv, 1 );
}


void Task::resume( int code )
{
  if ( state != Zombie )
    longjmp(tskEnv, code);
}


long Task::spawn(Task *chield)
{
  if (!chield || chield->fStarted)
    return -1;
  chield->parent=this;
  addref(chield);
  chield->start();
  RemoveFrom(readyTasks);
  yield0();
  delref(chield);
  return chieldRes;
}


Task *Task::nextTask()
{
   readyTasks.Next();
   return currTask=readyTasks.item() ;
}

int Task::run()
{
    signal(SIGPIPE, SIG_IGN);
#ifdef DJGPP
    signal(SIGINT, SIG_IGN);
#endif
    int code = setjmp(schEnv);

    zombieTasks.freeAll();

    while (code >= 0 && (!readyTasks.empty() || !waitTasks.empty() || Sema::taskCount() ) )
    {
	if ( nextTask() )
	    testEvent();
	else
	  {
	    waitEvent();
	    continue;
	  }
	if ( !currTask->fInited )
	    currTask->init();
	else
	    currTask->resume();
    }


    currTask = 0;
    return code < 0;
}

int Task::shutDown()
{
   currTask = 0;
   readyTasks.makeZombie();
   readyTasks.freeAll();
   waitTasks.makeZombie();
   waitTasks.freeAll();
   msgTasks.makeZombie();
   msgTasks.freeAll();
   respTasks.makeZombie();
   respTasks.freeAll();
   semaList.makeZombie();
   semaList.freeAll();
   Sema::taskcount=0;

   longjmp( schEnv, -1);
}

void Task::waitEvent(int block)
{
  Task *tp;

  if (waitTasks.empty())
    return;

  // check timeouts
  clock_t tim=times(&tms_buf);

  int n=0;
  for ( waitTasks.first() ; (tp=waitTasks.item())!=0 ; )
    if (tp->wakeUp <= tim)
      {
	waitTasks.remove();
	readyTasks.prepend(tp);
	tp->state = Ready;
	tp->fTimed=1;
	if ( tp->rfile )
	   { FD_CLR_BY( &readFiles, &(tp->rfileset) ); tp->rfile=0; }
	if ( tp->wfile  )
	   { FD_CLR_BY( &writeFiles, &(tp->wfileset) ); tp->wfile=0; }
	if (tp->efile)
	   { FD_CLR_BY( &exceptFiles, &(tp->efileset) ); tp->efile=0; }
	n++;
      }
    else
       break;

  if ( waitTasks.empty() )
    return;

  if ( n )
     block=0;

  // check files
  timeval tv;

  if (block)
   {
     clock_t dt=waitTasks.item()->wakeUp - tim+1;
     tv.tv_sec  = dt/CLK_TCK;
     tv.tv_usec = (dt%CLK_TCK) * (1000000/CLK_TCK);
   }
  else
   {
     tv.tv_sec=0;
     tv.tv_usec=0;
   }

  fd_set rfds=readFiles;
  fd_set wfds=writeFiles;
  fd_set efds=exceptFiles;
  int r;
  if ( (r=select(FD_SETSIZE, &rfds, &wfds, &efds, &tv)) > 0 )
   {
     for( n=waitTasks.first() ; n && (tp=waitTasks.item())!=0  ;  )
      {
       if (    tp->rfile && FD_ISSET_BY( &rfds, &(tp->rfileset) )
	    || tp->wfile && FD_ISSET_BY( &wfds, &(tp->wfileset) )  )
	     {
	       waitTasks.remove();
	       readyTasks.prepend(tp);
	       tp->state = Ready;
	       n=!waitTasks.empty();
	       FD_AND_BY( &(tp->rfileset), &rfds );
	       FD_AND_BY( &(tp->wfileset), &wfds );
	       FD_AND_BY( &(tp->efileset), &efds );
	       //if (tp->efile)
	       //  FD_CLR_BY( &exceptFiles, &(tp->efileset) );
	       tp->wfile = tp->rfile = tp->efile = 0;
	       continue;
	     }
       n=waitTasks.next();
      }
     FD_CLR_BY(&readFiles, &rfds);
     FD_CLR_BY(&writeFiles, &wfds);
     FD_CLR_BY(&exceptFiles, &efds);
   }

 if (block)
  {
  // check timeouts
  tim=times(&tms_buf);

  for ( waitTasks.first() ; (tp=waitTasks.item())!=0 ; )
     if (tp->wakeUp <= tim)
      {
	waitTasks.remove();
	readyTasks.prepend(tp);
	tp->state = Ready;
	tp->fTimed=1;
	if ( tp->rfile )
	   { FD_CLR_BY( &readFiles, &(tp->rfileset) ); tp->rfile=0; }
	if ( tp->wfile )
	   { FD_CLR_BY( &writeFiles, &(tp->wfileset) ); tp->wfile=0; }
	if (tp->efile)
	   { FD_CLR_BY( &exceptFiles, &(tp->efileset) ); tp->efile=0; }
      }
     else
       break;
 }


}

static clock_t calcWakeup(unsigned long msec)
{
    if ( msec == WaitForever )
	return times(&tms_buf) + 60*60*24*CLK_TCK;
#if 0
    clock_t n = msec / ( 1000/CLK_TCK );
#else
    double clk_msec = double(CLK_TCK) / 1000;
    clock_t n = clock_t(clk_msec * msec);
#endif
    if ( !n )
	n=1;
    return times(&tms_buf)+n;
}

static clock_t calcWakeup(timeval *tv)
{
    //if (!tv)
    //  return WaitForever;
    if (!tv)
      return  times(&tms_buf)+60*60*1/*24*/*CLK_TCK;
    clock_t n = tv->tv_sec * CLK_TCK +  tv->tv_usec / ( 1000000/CLK_TCK );
    if (!n)
     n=1;
    return times(&tms_buf)+n;
}


int Task::yield()
{
    if (!currTask)
      return 0;
    int n;
    if (!(n = setjmp(currTask->tskEnv)))
       longjmp( schEnv, 1 );
    return n;
}

int Task::yield0()
{
    if (!currTask)
      return 0;
    int n;
    if (!(n = setjmp(currTask->tskEnv)))
      {
	currTask=0;
	longjmp( schEnv, 1 );
      }
    return n;
}


static void calcTv(timeval &tv, u_long msec)
{
  if(msec==0)
    msec=1;
  tv.tv_sec  = msec/1000;
  tv.tv_usec = (msec%1000) * 1000;
}

int Task::sleep(unsigned long msec)
{
  if (!currTask)
   {
      timeval tv;
      calcTv(tv, msec);
      select(0, 0, 0, 0, &tv);
      return 1;
   }
  currTask->wakeUp=calcWakeup(msec);
  currTask->RemoveFrom( readyTasks );
  currTask->addToWait( );
  currTask->fTimed=0;
  yield();
  return currTask->fTimed==0;
}

void Task::bsleep(unsigned long msec)
{
  if (!currTask)
   {
      timeval tv;
      calcTv(tv, msec);
      select(0, 0, 0, 0, &tv);
      return;
   }
  currTask->wakeUp=calcWakeup(msec);
  currTask->RemoveFrom( readyTasks );
  currTask->addToWait( );
  currTask->fTimed=1;

  yield();

}


int Task::waitRead(int fd, unsigned long msec)
{
  timeval tv;
  if (!currTask)
   {
      fd_set set;
      FD_ZERO( & set);
      FD_SET( fd, & set);
      calcTv(tv, msec);
      return select(FD_SETSIZE, & set, 0, 0, &tv);
   }

  FD_ZERO( & currTask->rfileset);
  FD_SET( fd, &currTask->rfileset);
  tv.tv_sec=tv.tv_usec=0;
  int r=select(FD_SETSIZE, &currTask->rfileset, 0, 0, &tv);
  if ( r !=0 )
   { yield(); return r; }

  currTask->rfile=1;
  FD_SET( fd, &currTask->rfileset);

  currTask->wakeUp=calcWakeup(msec);
  currTask->RemoveFrom( readyTasks );
  currTask->addToWait( );
  currTask->fTimed = 0;

  yield();

  currTask->rfile=0;
  return currTask->fTimed==0;
}

int  Task::waitWrite(int fd, unsigned long msec)
{
  timeval tv;
  if (!currTask)
   {
      fd_set set;
      FD_ZERO( & set);
      FD_SET( fd, & set);
      calcTv(tv, msec);
      return select(FD_SETSIZE, 0, & set, 0, &tv);
   }
  FD_ZERO( & currTask->wfileset);
  FD_SET( fd, & currTask->wfileset);
  tv.tv_sec=tv.tv_usec=0;
  int r = select(FD_SETSIZE, 0, & currTask->wfileset, 0, &tv);
  if ( r != 0 )
   { yield(); return r; }

  currTask->wfile=1;
  FD_SET( fd, & currTask->wfileset);

  currTask->wakeUp=calcWakeup(msec);
  currTask->RemoveFrom( readyTasks );
  currTask->addToWait( );
  currTask->fTimed = 0;

  yield();

  currTask->wfile=0;
  return currTask->fTimed==0;
}

int Task::Select(int nfds,
		fd_set *readfds,
		fd_set *writefds,
		fd_set *exceptfds,
		struct timeval *timeout)
{
  if ( !currTask )
     return select(nfds, readfds, writefds, exceptfds, timeout);

  if ( readfds )
   {
    currTask->rfile=1;
    currTask->rfileset = *readfds;
   }
  else
    FD_ZERO( & currTask->rfileset);

  if ( writefds )
   {
    currTask->wfile=1;
    currTask->wfileset = *writefds;
   }
  else
    FD_ZERO( & currTask->wfileset);

  if ( exceptfds )
   {
    currTask->efile=1;
    currTask->efileset = *exceptfds;
   }
  else
    FD_ZERO( & currTask->efileset);


  timeval tv;
  tv.tv_sec=tv.tv_usec=0;
  int r=select( nfds, readfds, writefds, exceptfds, &tv );
  if (  r!=0 || timeout && !timeout->tv_sec && !timeout->tv_usec )
   {  yield(); currTask->rfile=currTask->wfile=currTask->efile=0; return r; }


  currTask->wakeUp=calcWakeup(timeout);
  currTask->RemoveFrom( readyTasks );
  currTask->addToWait( );
  currTask->fTimed = 0;

  yield();

  if (readfds)
    *readfds= currTask->rfileset;
  if (writefds)
    *writefds= currTask->wfileset;
  if (exceptfds)
    *exceptfds= currTask->efileset;

  currTask->wfile = currTask->rfile = currTask->efile = 0;
  return currTask->fTimed==0 ? 1 : 0;
}

extern "C" int  task_select(int nfds,
		fd_set *readfds,
		fd_set *writefds,
		fd_set *exceptfds,
		struct timeval *timeout)
{
  return Task::Select(nfds, readfds, writefds, exceptfds, timeout);
}



Task::MessPath Task::handleMessage( Message *msg )
{
  return QUEUE;
}

Message *Task::peekMessage( )
{
  yield();
  if ( currTask->recvlist.empty() )
    return 0;
  currTask->recvlist.first();
  Message *mp = currTask->recvlist.item();
  currTask->recvlist.remove();
  currTask->proclist.append(mp);
  mp->state = Message::Proc;
  return mp;

}

Message *Task::getMessage( )
{
  if ( currTask->recvlist.empty() )
   {
	 // блокировка
	 currTask->fTimed = 0;
	 currTask->RemoveFrom(readyTasks);
	 currTask->addToMsg();

	 yield0();

	 if ( currTask->recvlist.empty() )
	    return 0;
   }
  else
   {
     yield();
   }
  currTask->recvlist.first();
  Message *mp = currTask->recvlist.item();
  currTask->recvlist.remove();
  currTask->proclist.append(mp);
  mp->state = Message::Proc;
  return mp;
}

int Task::respond(Message *msg, Message *resp )
{
  if (resp)
   {
    resp->fFree=1;
    resp->state = Message::Resp;
   }

  msg->resp = resp;
  msg->state = Message::Resp;

  int ret=1;
  if ( msg->fBlock )
   {
     Task *sender=findTask(msg->sender);
     if ( sender && sender->state==Resp )
	  {
	   sender->RemoveFrom(respTasks);
	   sender->addToReady();
	  }
     else
	  ret=0;
   }
  currTask->proclist.freeIt(msg);

  return ret;
}

int Task::forward(Task  *reciever, Message *msg)
{
  currTask->proclist.removeIt(msg);
  return sendMessage(reciever, msg);
}

int Task::forward(u_long reciever, Message *msg)
{
  currTask->proclist.removeIt(msg);
  return sendMessage(reciever, msg);
}


int Task::sendAll(Message &msg)
{
  msg.fFree=0;
  msg.fBlock=0;
  msg.sender=currTask->no;

  Task *tp;

  for(int i=0; i< allTasks.getCount(); i++)
   {
     tp = (Task*)allTasks.at(i);
     if (tp!=currTask)
      switch( tp->handleMessage(&msg) )
       {
	case PROCESSED:
	  msg.receiver=tp->no;
	  return 1;
	case BACK:
	case QUEUE:
	case UNBLOCK:
	  break;
       }
   }
  return 0;
}



int Task::sendMessage(u_long receiver, Message *msg)
{
  Task *tp=findTask(receiver);
  if (tp)
      return _sendMessage( tp, *msg, 0, 1);
  return 0;
}

int Task::sendMessage(u_long receiver, Message &msg)
{
  Task *tp=findTask(receiver);
  if (tp)
      return _sendMessage( tp, msg, 1, 0);
  return 0;
}

int Task::sendMessage(Task *receiver, Message *msg)
{
  return _sendMessage( receiver, *msg, 0, 1);
}

int Task::sendMessage(Task *receiver, Message &msg)
{
  return _sendMessage( receiver, msg, 1, 0);
}



int Task::_sendMessage(Task *tp,  Message &Mssg, int Block, int Free)
{
  Message *msg=&Mssg;
  msg->fBlock = Block;
  msg->fFree = Free;

  msg->receiver=tp->no;
  msg->sender=currTask->no;

  int ret=0;

  if (tp->state==Zombie)
    goto ex;
  int s;
  switch( s=tp->handleMessage(msg) )
    {
      case PROCESSED:
	ret=1; goto ex;
      case BACK:
	goto ex;
      case QUEUE:
      case UNBLOCK:

	// добавляем сообщение во входную очередь задачи
	tp->recvlist.append(msg);

	if ( tp->state==Msg )
	 {
	     tp->RemoveFrom(msgTasks);
	     tp->addToReady();
	 }

	if ( s==UNBLOCK && tp->state==Wait )
	  tp->WakeUp();

	if (msg->fBlock && currTask!=tp)
	 {
	    // ждать ответа
	    currTask->RemoveFrom(readyTasks);
	    currTask->addToResp();
	    currTask->fTimed = 0;

	    yield0();

	    return  currTask->fTimed==0;
	 }
	else
	   return 1;
    }
  ret=1;

ex:
  if (msg->fFree)
    delete msg;

 return ret;
}


int Task::WakeUp()
{
  if ( state==Wait )
    { // разблокировать задачу
      RemoveFrom(waitTasks);
      fTimed=1;
      addToReady();
      if ( rfile )
	   { FD_CLR_BY( &readFiles, &rfileset ); rfile=0; }
      if ( wfile )
	   { FD_CLR_BY( &writeFiles, &wfileset ); wfile=0; }
      if ( efile )
	   { FD_CLR_BY( &exceptFiles, &efileset ); efile=0; }
      return 1;
    }
  return 0;
}

int Task::canBeKilled()
{
 return 1;
}

int Task::kill(u_long task)
{
  Task* tp = findTask(task);
  if ( tp && tp->canBeKilled() && tp->state!=Zombie )
    {
      tp->suicide();
      return 1;
    }
  return 0;
}


Task *Task::findTask(u_long taskid)
{
  long ind;
  if ( allTasks.search( &taskid, ind ) )
     return (Task*)allTasks.at( ind );
  return 0;
}

void Task::messToQueue( Message *msg )
{
  if ( !msg )
     return;
  recvlist.append( msg );
  switch( state )
   {
     case Msg:
	 RemoveFrom( msgTasks );
	 addToReady();
	 break;
     case Wait:
	 WakeUp();
	 break;
     default:
	 break;
   }
}

// Task ][ Timer

Timer::Timer( Task *recv, long com, int sec ) :
	Task( "Timer", 256 ), closed(0), reciever(recv), command(com)
{
   setTime( sec );
}

long Timer::main()
{
   KeyMessage *km;
   while( 1 )
    {
      bsleep( msec );
      if ( closed || !reciever || !(km=new KeyMessage( command )) )
	 continue;
      sendMessage( reciever, km );
    }
  return 0;
}

// Timer ]
