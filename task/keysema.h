/*
	$Id: keysema.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef KEYSEMA_H
#define KEYSEMA_H

#include "keycodes.h"

extern long keyEvent;

extern Sema sendInputKey;
extern Sema recvInputKey;

class kbInput: public Task
{
     u_long blankTime;
     void (*blankFunc)();
     Sema send;
     Sema recv;
     u_long kb_func1;
     u_long kb_func2;
     u_long kb_cyr;
     virtual int canBeKilled() { return 0;}
  public:
     kbInput( u_long Kb_func1=0xa/*^J*/, u_long Kb_func2=0xb/*^K*/,
	      u_long Kb_cyr=0xe/*^N*/,
	      u_long BlankTime = 0, void (*BlankFunc)() = 0 );
     ~kbInput();
     long main();
};



class KeyInput
{
public:
    Sema send;
    Sema recv;
    KeyInput(int Prio);
    int  getPrio();
    void setPrio(int Prio);
    long get();
    void put(long key);
};


#endif
