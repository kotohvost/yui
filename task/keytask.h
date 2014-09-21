/*
	$Id: keytask.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef KEYTASK_H
#define KEYTASK_H

#include "keycodes.h"
#include "task.h"

class InputKeyTask: public Task
{
protected:
     u_long blankTime;
     Task *reciever;
     unsigned long kb_func1;
     unsigned long kb_func2;
     unsigned long kb_cyr;
     virtual int canBeKilled() { return 0;}
public:
     InputKeyTask( Task *reciever,
		unsigned long Kb_func1=0xa /*^J*/,
		unsigned long Kb_func2=0xb /*^K*/,
		unsigned long Kb_cyr=0x20e /*^J^N*/,
		unsigned long BlankTime = 0 );
     ~InputKeyTask();
     long main();
     void set_reciever( Task * );
     void set_func1( unsigned long l );
     void set_func2( unsigned long l );
     void set_cyr( unsigned long l );
     Task *get_reciever() { return reciever; }
     unsigned long get_func1() { return kb_func1; }
     unsigned long get_func2() { return kb_func2; }
     unsigned long get_cyr() { return kb_cyr; }
};



#endif
