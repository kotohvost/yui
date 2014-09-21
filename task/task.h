/*
	$Id: task.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef TASK_H
#define TASK_H

#include <setjmp.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "list.h"
#include "message.h"
#include "collect.h"

#define NoWait      0L
#define WaitForever LONG_MAX

class Task;

class TaskList: public TList
{
protected:
  virtual void *getNext(void *item);
  virtual void *getPrev(void *item);
  virtual void setNext(void *item, void *next);
  virtual void setPrev(void *item, void *prev);
  virtual void freeItem(void *item);
public:
  TaskList(){;}
  Task *item();
  void makeZombie();
};

class TaskCollection: public SortedCollection
{
protected:
  virtual void freeItem( void *item );
  virtual int compare( void *key1, void *key2 );
  virtual void *keyOf( void *item );
public:
  TaskCollection();
  ~TaskCollection();
};

// межзадачная синхронизация
class SemaBody;

class SemaList: public TList
{
protected:
  virtual void *getNext(void *item);
  virtual void *getPrev(void *item);
  virtual void setNext(void *item, void *next);
  virtual void setPrev(void *item, void *prev);
  virtual void freeItem(void *item);
public:
  SemaList();
  ~SemaList();
  SemaBody *item();
  void makeZombie();
};


class SemaBody
{
  friend class SemaList;
  friend class Sema;
  friend class Task;

  SemaBody *next;
  SemaBody *prev;
  int count;		// счетчик ссылок
  int value;		//
  TaskList tasks;  	// список блокированных на семафоре задач по приоритетам
  void addSema(Sema *sem, Task *tp);
  void removeTask(Task *tp);

  SemaBody();
  ~SemaBody();
};



class Sema
{
friend class SemaBody;
friend class Task;

  int priority;
  SemaBody *sp;
  static int taskcount;

public:
  static int taskCount(){ return taskcount; }
  Sema(int prio, int closed=0);  // приоритет на захват ресурса: 0-минимальный
  void setPrio(int newprio){ priority=newprio; }
  int  getPrio(){ return priority; }
  Sema( Sema &sema, int prio);
  Sema( Sema &sema );
  ~Sema();

  int close();	// дождаться и захватить семафор
  int open(); 	// разблокировать семафор
  int waitClose(); // дождаться захвата семафора кем-либо
};

/*
class SemaPipe
{
  Sema send;
  Sema recv;
public:
  SemaPipe(int pri=0): send(pri),recv(pri,1) {;}
  SemaPipe(SemaPipe&s): send(s.send), recv(s.recv) {;}
  SemaPipe(SemaPipe&s, int pri=0): send(s.send,pri), recv(s.recv,pri) {;}
  int send_beg() { return send.close(); }
  int send_end() { return recv.open();  }
  int recv_beg() { return recv.close(); }
  int recv_end() { return send.open();  }
};
*/

class Task
{
    friend class TaskHolder;
    friend class TaskCollection;
    friend class Message;
    friend class TaskMessage;
    friend class TaskList;
    friend class PrioList;
    friend class SemaBody;
    friend class SemaList;
    friend class Sema;
    friend class FileBuf;
    friend void  callTaskMain(Task *t);
#ifdef DJGPP
protected:
#endif
    const char  *tskname;       //  name of task

    // communication lists
    MessageList recvlist;	//  принятые сообщения (входная очередь)
    MessageList proclist;	//  обрабатываемые сообщения

    int refcount;


    //  flags for task state
    unsigned  	 fInited:1      //  задача инициализирована
		,fTimed:1       //  активизирована по истечению таймаута
		,fStarted:1     //  помещена в список готовых
		,isMapped:1     //  стек выделен в виртуальной памяти
		,rfile:1
		,wfile:1
		,efile:1
		;
    enum {
	   Ready	//  задача активна
	  ,Zombie	//  задача умерла
	  ,Wait		//  ждет готовность файла или таймаут
	  ,Msg		//  ждет сообщения
	  ,Resp		//  ждет ответа
	  ,Sem		//  ждет на семафоре
    } state:8;


    static jmp_buf  schEnv;     //  environment in scheduler
    jmp_buf     tskEnv;         //  environment for context switching
    int         stklen;         //  length of task's stack
    unsigned char *stack;       //  this task's stack space
    union{
    clock_t     wakeUp;         //  when to wake up if asleep
    int priority;		//  действует в очереди к семафору
    };

    SemaBody *sem;
    Task *next;
    Task *prev;

    void  init();    //  set up stack and begin execution

    //  return to place of last yield()
    void  resume(int code = 1);

    static TaskCollection allTasks; // все задачи
    static TaskList readyTasks;
    static TaskList waitTasks;
    static TaskList msgTasks;    // ожидают сообщения
    static TaskList respTasks;   // ожидают ответа на сообщение
    static SemaList semaList;    // список семафоров
    static TaskList zombieTasks;
    static Task * currTask;

    static int fullcount; // full number of tasks
    static u_long seqNo;
    u_long no;     // уникальный номер задачи

    static fd_set readFiles;
    static fd_set writeFiles;
    static fd_set exceptFiles;

    fd_set rfileset;
    fd_set wfileset;
    fd_set efileset;

    void addTo(TaskList &list);
    void addToWait();  // with appropiate wakeup time place
    void addToReady();
    void addToZombie();
    void addToMsg();
    void addToResp();
    void removeFrom(TaskList &list);
    void RemoveFrom(TaskList &list); // не проверяет принадлежность к списку


    static Task *nextTask(); 	// set currTask

    static void testEvent()	{ waitEvent(0); }
    static void waitEvent(int block=1);

    virtual int canBeKilled();
    Task *parent;
    long chieldRes;

protected:
    Task(const char *tname, int stk = 1024);
    static int  yield0(); //  give up control & clear currTask
    static int _sendMessage(Task *receiver, Message &msg, int Block, int Free);

public:

    static void addref(Task *tp);
    static void delref(Task *tp);

    void start();
    static int run();
    static int shutDown();
    void suicide();
    virtual ~Task();
    virtual int identify(const char *data) 	{ return 1; }
    u_long getNo() {return no;}
    static Task* getCurrent(){ return currTask; }

    const char *name()          { return tskname; }

    static int  yield(); //  give up control

    int WakeUp(); // принудительный обрыв ожидания

    // запускает chield и ждет его завершения
    long spawn(Task *chield);

    //  ждут готовность файла или истечения таймаута
    //  !!! ОЖИДАНИЕ СВЯЗАНО С ОБРАБОТКОЙ СООБЩЕНИЙ:
    //    - прибытие сообщения задаче обрывает ожидание (cm handleMessage)

    static int  sleep(unsigned long msec);  // 1-истечение таймаута, 0-прерывание
    static void bsleep(unsigned long msec); // безусловное ожидание (не реагирует на прибытие сообщений)
    static int  waitRead(int fd, unsigned long msec=WaitForever);
    static int  waitWrite(int fd, unsigned long msec=WaitForever);
    // the same as system "select" call
    static int  Select(int nfds,
		fd_set *readfds,
		fd_set *writefds,
		fd_set *exceptfds,
		struct timeval *timeout);

    // the main routine for each task
    virtual long main() = 0;
    // выполняет функции деструктора
    virtual void destroy();


    // получить сообщение из очереди задачи
    static Message *peekMessage(); // если нет сообщений, возвращает 0
    static Message *getMessage();  // если нет сообщений, блокируется
    /*
      Асинхронный обработчик сообщений
       - выполняется в режиме прерывания задачи
	 ( блокированной или отдавшей управление через yield() )
       - выполняется в стеке вызывающей задачи
       - не должен вызывать блокирующие функции
       - общие сообщения (с установленным флагом fCommon) попадают
	 только сюда ( т.к. они никогда не должны блокироваться )
       - адресованные задаче сообщения попадают сначала сюда и
	 затем (если не обработались) - во входную очередь задачи
       - здесь должны обрабатываться события типа
	 "идентификация", "перерисовка", "информация о задаче",
	 "установление связи", и т.п., связанные с получением
	 состояния задачи и не связанные с изменением состояния
	 задачи
       - должен возвращать:
	 PROCESSED  если сообщение обработано
	 QUEUE	если сообщение должно попасть во входную очередь
	 UNBLOCK	если сообщение должно попасть во входную очередь
			и разблокировать задачу
	 BACK	если сообщение НЕ должно попасть во входную очередь
			(должно возвращаться для неизвестных сообщений)
       - общие сообщения (посланные через sendAll ) должны вызывать только
	 BACK или PROCESSED, в противном случае сообщение обрабатывается
	 как BACK
    */
    enum MessPath {
	   PROCESSED,
	   QUEUE,
	   UNBLOCK,
	   BACK
	 };
    virtual MessPath handleMessage( Message *msg );

    /* сообщение без ожидания ответа (уничтожается автоматически после обработки) */
    static int sendMessage(u_long receiver, Message *msg);

    /* сообщение с ожиданием ответа (автоматически не уничтожается) */
    static int sendMessage(u_long receiver, Message &msg);

    /* общее сообщение (автоматически не уничтожается) */
    static int sendAll(Message &msg);

    static int sendMessage(Task *receiver, Message *msg);
    static int sendMessage(Task *receiver, Message &msg);


    /*
       посылает ответ на сообщение ( resp уничтожается автоматически )
       - если msg не требует ответа, то уничтожается
       !!! обработка любого сообщения должна завершаться либо этим
       вызовом, либо forward, иначе сообщение навсегда останется в
       списке обрабатываемых
    */
    static int respond(Message *msg, Message *resp=0);

    /* пересылает сообщение другой задаче */
    static int forward(Task  *receiver, Message *msg);
    static int forward(u_long receiver, Message *msg);

    static int kill(u_long task);

    static Task *findTask(u_long taskid);
    /* для добавления сообщений из функций - обработчиков системных сигналов */
    void messToQueue( Message *msg );
};


// должна вызываться только из стека Task
extern "C" int  task_select(int nfds,
		fd_set *readfds,
		fd_set *writefds,
		fd_set *exceptfds,
		struct timeval *timeout);


class TaskHolder
{
   Task *tp;
public:
   void start(){ if (tp) tp->start(); }
   TaskHolder( Task *Tp );
   ~TaskHolder();

   Task *task(){ return tp; }
};


class TaskMessage: public Message
{
   Task *data;
 public:
   TaskMessage(Task *Data) { Task::addref(data=Data); 	}
   ~TaskMessage() 	{ if (data) Task::delref(data); }
   Task *item()		{ return data; }
   Task *getItem()   	{ Task *ret=data; data=0; return ret; }
   virtual long type();
};

class Timer : public Task
{
protected:
   unsigned closed:1;
   unsigned long msec;
public:
   Timer( Task *recv, long com, int sec=INT_MAX );
   long main();
   void setTime( int sec ) { msec = sec * 1000; }
   void open() { closed=0; }
   void close() { closed=1; }
   Task *reciever;
   long command;
};

#endif
