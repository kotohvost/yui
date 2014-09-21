/*
	$Id: test2.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>

#include "task.h"
#include "screen.h"
#include "msgtypes.h"
#include "keytask.h"
#include "keysema.h"
#include "iobuf.h"


static eprint( char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   Screen::printVect( 22, 0, HIWHITE|BACK(YELLOW), fmt, ap );
   Screen::sync();
   Task::sleep(2000);

   va_end(ap);
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
   for( i=0 ; i<100 ; i++)
    {
       Message *msg=getMessage();

       if ( msg->type()==HASH_Key )
         {
           KeyMessage *kmsg=(KeyMessage*)msg;
           long k=kmsg->data;
           Screen::print(15,5, HIYELLOW|BACK(GREEN), "%s take '%s' key (%d)",name(), descriptKey(k), i );
           Screen::sync();
           //IntMessage msg(k);  //ˆ±¹¸¥·¢„ŽŠŠ ª¹¹ˆŽŠ¾¥Š
           IntMessage *msg=new IntMessage(k); //¾Šˆ±¹¸¥·¢„ŽŠŠ ª¹¹ˆŽŠ¾¥Š
           sendMessage(recv, msg);
        }
       respond(msg);
    }
  eprint("MessSender exit (%d)", i);
  shutDown();
}

Task::MessPath MessReciever::handleMessage( Message *msg )
{
  Screen::print(16,5,HIYELLOW|BACK(BLUE), "%s handleMessage: type=0x%x",name(), msg->type() );
  Screen::sync();
  return QUEUE;
}

long MessReciever::main()
{
   int i;
   for(i=0 ; i<100 ; i++)
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
  eprint("MessReciever exit (%d)", i );
  shutDown();
}

int main()
{
   Screen::initScreen();

   TaskHolder	reciever(new MessReciever("Reciever"));
   TaskHolder	sender( new MessSender("Sender",reciever.task()->getNo()) );
   TaskHolder   key( new InputKeyTask( sender.task()) );

   Screen::box( Screen::center(10, 40), (u_char*)" asdf ‰¯…½‰ ", HIYELLOW | BACK(GREEN) );
   Screen::put( 5,15, (u_char*)"faasdfasdfasdf", 29);
   Screen::put( 6,40, (u_char*)"faasdfasdfasdf", 19);
   Screen::move( 10, 0);
   Screen::sync();

   Task::run();

   return 0;
}
