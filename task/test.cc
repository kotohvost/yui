/*
	$Id: test.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>

#include "task.h"
#include "iobuf.h"
#include "keytask.h"
#include "keysema.h"
#include "screen.h"
#include "reg_expr.h"
#include "term.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define LIB_STACK_SIZE	1024
#define STACK_NORM_SIZE(t_size) ( (t_size)+ LIB_STACK_SIZE )

static void eprint( char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   Screen::sync();
   Task::sleep(2000);
   va_end(ap);
}

/*
class T1: public Task
{
public:
  T1(const char *Name):Task(Name){;}
  long main();
};

long T1::main()
{
  for(int i=0;i<7;i++)
   {
      printf("task %s: %d iteration\n" ,name(), i);
      sleep(4000);
   }
}

class T2: public Task
{
public:
  T2(const char *Name):Task(Name){;}
  long main();
};

long T2::main()
{
  for(int i=0; i<10 ;i++)
   {
      int r=waitRead(0, 5000);
      printf("%s: file %s\n", name(),  r ? "READY" : "NOT READY");
      //printf("task %s: %d iteration\n" ,name(), i);
      if (r)
	 sleep(500);
   }
}
*/

class Sender: public Task
{
protected:
  int prio;
public:
  Sender(const char *Name, int Prio=0):Task(Name), prio(Prio){;}
  long main();
};

class Receiver: public Task
{
public:
  Receiver(const char *Name):Task(Name){;}
  long main();
};

Sema s_send(0);
Sema s_recv(0, 1);

char msg[256];

long Sender::main()
{
   Sema send(s_send, prio);
   Sema recv(s_recv);
   for(int i=0; i<3; i++)
    {
	 send.close();
	    printf( "%s send %d\n", name(), i);
	    sprintf(msg, "%s message %d\n", name(), i);
	 recv.open();
    }
   return 0;
}

long Receiver::main()
{
   Sema send(s_send);
   Sema recv(s_recv);
   for(int i=0 ; ; i++)
    {
       //printf( "%s wait %d\n",name(),i);
       recv.close();
	 printf( "%s received: %s\n",name(), msg);
       send.open();
    }
   return 0;
}

class Killer: public Task
{
protected:
  int prio;
  Sema send;
  Sema recv;
public:
  Killer(const char *Name, int Prio);
  long main();
};

Killer::Killer(const char *Name, int Prio):
      Task(Name,1024), prio(Prio),
      send(sendInputKey),
      recv(recvInputKey, Prio)
{
}

long Killer::main()
{
   for( ; ; )
    {
       recv.close();
       if ( keyEvent==FUNC1(kbCtrlQ) )
	 {
	   printf( "%s detect: 0x%08lx\n",name(), keyEvent);
	   shutDown();
	 }
       recv.open();
       recv.waitClose();
    }
}

class KeyReader: public Task
{
protected:
  int prio;
  KeyInput key;
public:
  KeyReader(const char *Name, int Prio);
  long main();
};

KeyReader::KeyReader(const char *Name, int Prio):
      Task(Name,1024), prio(Prio),
      key(Prio)
{
}

long KeyReader::main()
{
   for(int i=0 ; i<3 ; i++)
    {
       printf( "%s wait %d key\n",name(),i);
       printf( "%s received: 0x%08lx\n",name(), key.get());
       Screen::sync();
    }
  shutDown();
  return 0;
}


class FileTester: public Task
{
protected:
  FileBuf *in, *out;
public:
  FileTester(const char *Name, const char *infile, const char *outfile);
  void destroy();
  long main();
};

FileTester::FileTester(const char *Name, const char *infile, const char *outfile):
	Task(Name,1024)
{
  in  = new FileBuf(infile, "r");
  out = new FileBuf(outfile, "w");
  if (in->state()!=Buf::Good)
     Screen::print( FG_HI_WHITE | BG_RED, "file '%s'open failure\n", infile);
  if (out->state()!=Buf::Good)
     Screen::print( FG_HI_WHITE | BG_RED,"file '%s'open failure\n", outfile);

}

void FileTester::destroy()
{
  delete in;
  delete out;
}

long FileTester::main()
{
  char *s = new char[4096];
  int l, i, sum;
  for( i=0, sum=0 ;
       (l=in->readstr(s, 3000)) >=0   ;
       i++, sum+=l )
   {
     Point p=Screen::cursor();
     Screen::ClearLine(4, FG_WHITE | BG_BLACK);
     Screen::print(4,1, FG_HI_WHITE | BG_BLUE, "string %d: %s", i, s);
     if ( out->writestr(s)<0 )
       break;
     Screen::move(p);

     if (! (i%100) )
       { Screen::sync(); sleep(100); }
   }

  //out->close();
  Screen::print(3, 30, FG_HI_YELLOW | BG_YELLOW, "file copyed: %d bytes", sum);
  Screen::sync();

  delete s;
  return 0;
}


class MKiller: public Task
{
protected:
  Task *reciever;
public:
  MKiller(Task *Reciever):Task("MKiller", STACK_NORM_SIZE(512)),reciever(Reciever)
	     { addref(reciever); }
  ~MKiller() { delref(reciever); }
  long main();
};

long MKiller::main()
{
   for(int i=0 ; ; i++)
    {
       Message *msg=getMessage();

       if ( msg->type()==HASH_Key )
	 {
	   KeyMessage *kmsg=(KeyMessage*)msg;
	   //eprint("%s recieve key %08x (%d)iter", name(), kmsg->data, i);
	   if ( kmsg->data==FUNC1(kbCtrlQ) || kmsg->data==FUNC2(kbCtrlQ) )
	     {
		 eprint( "\n%s detect: 0x%08x\n",name(), kmsg->data);
		 shutDown();
	     }
	 }
       forward(reciever, msg);
    }
  eprint("Mkiller exit");
}



class MessSender: public Task
{
protected:
  u_long recv;
public:
  MessSender(char *name,u_long Recv):Task(name, 8192/*512*/ ),recv(Recv) {;}
  long main();
};

class MessReciever: public Task
{
protected:
  MessPath handleMessage( Message *msg );
public:
  MessReciever(char *name):Task(name, 5000 /*512*/ ){;}
  long main();
};

long MessSender::main()
{
   int i;
   for( i=0 ; i<10 ; i++)
    {
       Message *msg=getMessage();
       //Message *msg=new KeyMessage(i);

       if ( msg->type()==HASH_Key )
	 {
	   KeyMessage *kmsg=(KeyMessage*)msg;
	   long k=kmsg->data;
	   Screen::print(15,5, FG_HI_YELLOW | BG_GREEN, "%s take '%s' key (%d)",name(), _descriptKey(k), i );
	   Screen::sync();
	   //IntMessage msg(k);  //ˆ±¹¸¥·¢„ŽŠŠ ª¹¹ˆŽŠ¾¥Š
	   IntMessage *msg=new IntMessage(k); //¾Šˆ±¹¸¥·¢„ŽŠŠ ª¹¹ˆŽŠ¾¥Š
	   sendMessage(recv, msg);
	}
       respond(msg);
    }

  eprint("MessSender exit (%d)", i);
  shutDown();
  return 0;
}

Task::MessPath MessReciever::handleMessage( Message *msg )
{
  Screen::print(16,5,FG_HI_YELLOW | BG_BLUE, "%s handleMessage: type=0x%x",name(), msg->type() );
  Screen::sync();
  return QUEUE;
}

long MessReciever::main()
{
   int i;
   for(i=0 ; i<10 ; i++)
    {
       Message *msg=getMessage();
       if(!msg)
	  break;


       if (msg->type()==HASH_Int)
	{
	    IntMessage *imsg=(IntMessage*)msg;
	    Screen::print(17,5,FG_HI_YELLOW | BG_BLUE, "%s recieve IntMessage: type=0x%x data=0x%04x",name(), imsg->type(), imsg->data );
	    Screen::sync();
	 }

       respond(msg);
       sleep(1000);
    }
  eprint("MessReciever exit (%d)", i );

  shutDown();
  return 0;
}

int main()
{
#if defined(FreeBSD)
   Screen::initScreen( BASE_TERMCAP );
#else
   Screen::initScreen( BASE_TERMINFO );
#endif

   /*
   TaskHolder  	t1(new T1("t1"))
		//,t2(new T1("t2"))
		//,t3(new T1("t3"))
		,t4(new T1("t4"));

   TaskHolder  	t22(new T2("__t22"))
		,t33(new T2("__t33"));

   TaskHolder  	tsend1(new Sender("Sender 1",1)),
		tsend2(new Sender("Sender 2",1)),
		trecv(new Receiver("Receiver"));
   */

   TaskHolder	reciever(new MessReciever("Reciever"));
   TaskHolder	sender(new MessSender("Sender",reciever.task()->getNo()));

   TaskHolder   killer(new MKiller( sender.task() ) );
   TaskHolder   key(new InputKeyTask( killer.task()) );
   Regexpr r("\\w+");

   Screen::put( 4, 5, (u_char*)"default attribute text" );
   Screen::put( 5,15, (u_char*)"black on white text", FG_BLACK | BG_WHITE );
   Screen::put( 6,40, (u_char*)"white on black text", FG_WHITE | BG_BLACK );
   Screen::box( Screen::center(10, 40), (u_char*)" asdf ÆÙ×Á ", FG_HI_YELLOW | BG_GREEN );
   Screen::move( 10, 0);
   Screen::sync();

   Task::run();
#ifdef MALLOC_DEBUG
   malloc_shutdown();
#endif
  Term::ResetTty();
  Term::destroy();
}

