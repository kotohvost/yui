/*
	$Id: testrpc.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>

extern "C" {
#include <rpc/rpc.h>
callrpc();
pmap_unset();
}

#include <stdarg.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "task.h"
#include "keytask.h"
#include "screen.h"

static u_long progno=1900003, versno=2, procno=1;

static eprint( char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   Screen::printVect( 22, 0, HIWHITE|BACK(YELLOW), fmt, ap );
   Screen::sync();
   Task::sleep(2000);

   va_end(ap);
}


class RpcServer: public Task
{
  static void dispatch( struct svc_req *rqstp, SVCXPRT *transp);

public:
  RpcServer();
  ~RpcServer(){destroy();}
  main();
  destroy();
};

class RpcClient: public Task
{
public:
  RpcClient();
  main();
};

RpcClient::RpcClient(): Task("rpc client", 8192)
{
}

RpcClient::main()
{
  for(int i=0; ;i++)
   {
      static unsigned long insym=0, outsym=0;
      int stat;

      stat = callrpc("itk",  progno, versno, procno,
		      xdr_u_long, &insym, xdr_u_long, &outsym);
      Screen::print(0,0,HIWHITE|BACK(RED),"%s: %d iter (progno=%d, versno=%2d, procno=%2d)" ,
		    name(), i, progno, versno, procno );
      Screen::print(1,0,HIWHITE|BACK(RED),"call state: %s; in=%8x  out=%8x" ,
		    clnt_sperrno(stat), insym, outsym );
      Screen::sync();
      sleep(2100);
      insym+=100;
   }
}


RpcServer::RpcServer(): Task("rpc server", 8192)
{
	       SVCXPRT *transp;

	       transp = svcudp_create(RPC_ANYSOCK);
	       if (transp == NULL){
		    eprint("can't create an RPC server\n");
		    shutDown();
	       }
	       pmap_unset(progno, versno);
	       if (!svc_register(transp, progno, versno,
			   dispatch, IPPROTO_UDP)) {
		    eprint("can't register %d %d service\n", progno, procno);
		    shutDown();
	       }

}

RpcServer::destroy()
{
	//eprint("server destroy");
	svc_unregister(progno, versno);
	//pmap_unset(progno, versno);
}

RpcServer::main()
{
	svc_run();  /* Never returns */
}


void RpcServer::dispatch( struct svc_req *rqstp, SVCXPRT *transp)
{
	       u_long sym;
	       static int iter;
	       switch (rqstp->rq_proc) {
	       case 0:
		    if (!svc_sendreply(transp, xdr_void, 0))
			 eprint( "can't reply to RPC call");
		    return;
	       case 1:

		    if (!svc_getargs(transp, xdr_u_long, &sym))
		      {
			 eprint("can't decode arguments\n");
			 svcerr_decode(transp);
			 break;
		      }
		    if (!svc_sendreply(transp, xdr_u_long, &sym))
			 eprint( "can't reply to RPC call");
		    Screen::print( 3,0, HIWHITE|BACK(GREEN), "rpc sever echo request %d processed (%x)", iter++, sym);
		    Screen::sync();
		    return;
	       default:
		    svcerr_noproc(transp);
		    return;
	       }
}


class MKiller: public Task
{
  Task *reciever;
public:
  MKiller(Task *Reciever):Task("MKiller", 1024),reciever(Reciever)
	     { addref(reciever); }
  ~MKiller() { delref(reciever); }
  main();
};

MKiller::main()
{
   for( ; ; )
    {
       Message *msg=getMessage();

       if ( msg->type()==HASH_Key )
	 {
	   KeyMessage *kmsg=(KeyMessage*)msg;
	   if ( kmsg->data==FUNC1(kbCtrlQ) )
	     {
		 eprint( "%s detect: 0x%08x",name(), kmsg->data);
		 shutDown();
	     }
	 }
       forward(reciever, msg);
    }
}



class MessSender: public Task
{
  u_long recv;
public:
  MessSender(char *name,u_long Recv):Task(name),recv(Recv) {;}
  main();
};

class MessReciever: public Task
{
  MessPath handleMessage( Message *msg );
public:
  MessReciever(char *name):Task(name){;}
  main();
};

MessSender::main()
{
   for(int i=0 ; i<100 ; i++)
    {
       Message *msg=getMessage();

       if ( msg->type()==HASH_Key )
	 {
	   KeyMessage *kmsg=(KeyMessage*)msg;
	   long k=kmsg->data;
	   Screen::print(15,5, HIYELLOW|BACK(GREEN), "%s take 0x%04x key",name(), k );
	   Screen::sync();
	   //IntMessage msg(k);  //ˆ±¹¸¥·¢„ŽŠŠ ª¹¹ˆŽŠ¾¥Š
	   IntMessage *msg=new IntMessage(k); //¾Šˆ±¹¸¥·¢„ŽŠŠ ª¹¹ˆŽŠ¾¥Š
	   sendMessage(recv, msg);
	   //sendAll( msg);
	}
       respond(msg);
    }

  shutDown();
}

Task::MessPath MessReciever::handleMessage( Message *msg )
{
  Screen::print(16,5,HIYELLOW|BACK(BLUE), "%s handleMessage: type=0x%x",name(), msg->type() );
  Screen::sync();
  return QUEUE;
}

MessReciever::main()
{
   for(int i=0 ; i<100 ; i++)
    {
       Message *msg=getMessage();
       if(!msg)
	  break;

       if (msg->type()==HASH_Int)
	{
	    IntMessage *imsg=(IntMessage*)msg;
	    Screen::print(17,5,HIYELLOW|BACK(BLUE), "%s recieve IntMessage: type=0x%x data=0x%04x",name(), imsg->type(), imsg->data );
	    Screen::sync();
	 }

       respond(msg);
       sleep(1000);
    }

  shutDown();
}

main()
{
   Screen::initScreen();

   TaskHolder	reciever(new MessReciever("Reciever"));
   TaskHolder	sender(new MessSender("Sender",reciever.task()->getNo()));

   TaskHolder   killer(new MKiller( sender.task() ) );
   TaskHolder   key(new InputKeyTask( killer.task()) );
   TaskHolder   rpcserv(new RpcServer);
   TaskHolder   rpcclnt(new RpcClient);

   //Regexpr r("\\w+");

   Screen::box( Screen::center(10, 40), " asdf ‰¯…½‰ ", HIYELLOW | BACK(GREEN) );
   Screen::put( 5,15, "faasdfasdfasdf", 29);
   Screen::put( 6,40, "faasdfasdfasdf", 19);
   Screen::move( 10, 0);
   Screen::sync();

   Task::run();
}

