/*
	$Id: keysema.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>
#include <stdlib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "task.h"
#include "keysema.h"

#ifndef DJGPP
#include "term.h"
#else
#include <conio.h>
#endif

#include "hashcode.h"

long keyEvent;

Sema sendInputKey(0);
Sema recvInputKey(0, 1);

kbInput::kbInput( u_long Kb_func1, u_long Kb_func2, u_long Kb_cyr,
		  u_long BlankTime, void (*BlankFunc)() ):
	Task("kbInput", 256),
	blankTime(BlankTime),
	blankFunc(BlankFunc),
	send( sendInputKey ),
	recv( recvInputKey ),
	kb_func1(Kb_func1),
	kb_func2(Kb_func2),
	kb_cyr(Kb_cyr)
{
  if ( !blankTime )
    blankTime=WaitForever;
}

kbInput::~kbInput()
{
}

long kbInput::main()
{
   u_long key;
   int match=0;
   int funcFlag=0, fixFuncFlag=0, func2flag=0, cyrFlag=0;

#ifndef DJGPP
   u_char ch;
   KeyMap *kp, *lp;
   int lokey=256, hikey=0;

   for (kp=Term::keytab; kp->str; kp++)
    {
     ch = kp->str[0];
     if ( ch<lokey )
       lokey=ch;
     if ( ch>hikey )
       hikey=ch;
    }
   kp=0;
#else
   int zeroFlag=0;
#endif

   for(;;)
    {
       if ( waitRead(0, blankTime) )
	{
#ifdef DJGPP
	  key = getch();
	  if ( !key )
	    {
	      zeroFlag = 1;
	      key = getch();
	    }
	  if ( zeroFlag )
	    {
	      zeroFlag = 0;
	      key |= 0400;
	    }
#else
	  read(0, &ch, 1);
	  if (!kp)
	    {
	      if (ch<lokey || ch >hikey)
		key=ch;
	      else
	      {
		for ( kp=Term::keytab; kp->str; ++kp)
		  if ( ch == kp->str[0])
		     break;
		if ( !kp->str)
		  { kp=0; key=ch; }
		else if (! kp->str [1])
		  { key = kp->val; kp=0; }
		else
		  {
		    match=1;
		    continue;
		  }
	      }
	    }
	  else
	    {
	      if (kp->str[match] == ch)
	       {
		match++;
		if (kp->str[match]==0 )
		  { key = kp->val; kp=0; }
		else
		  continue;
	       }
	      else
	       {
		 lp = kp;
		 do
		  {
		   ++kp;
		   if (! kp->str)
		     { kp=0; break; }
		  }
		 while (kp->str [match] != ch);

		 if (!kp)
		   continue;

		 if (lp->str [match-1] != kp->str [match-1])
		    { kp=0; continue; }

		 match++;
		 if (kp->str[match]==0 )
		   { key = kp->val; kp=0; }
		 else
		   continue;
	       }
	    }
#endif
	}
       else
	{
	  if (blankFunc)
	    blankFunc();
	  continue;
	}

       if ( funcFlag && (func2flag || key!=kb_func2) )
	  { funcFlag=0; key |= 0x200; }
       else if (key==kb_func1)
	  { funcFlag=1; continue; }

       if ( func2flag )
	  { func2flag=0; key |= 0x400; }
       else if (key==kb_func2)
	  { func2flag=1; continue; }

       if ( key== (kb_func1|0x200) )
	  { fixFuncFlag=!fixFuncFlag; continue; }

       if ( fixFuncFlag )
	 switch(key)
	  {
	   case 27:
	   case 13:
	     break;
	   default:
	    if ( (key & ~0x200) < 32 )
	      break;
	    else if ( key & 0x200)
	      key &= ~0x200;
	    else
	      key |= 0x200;
	  }

#ifndef DJGPP
       if (key==kb_cyr)
	 { cyrFlag = !cyrFlag; continue; }

       if (key<256)
	 {
	   if (cyrFlag)
	     key=Term::cyrInputTable[key];
/*	   else
	     key=Term::inputTable[key];*/
	 }
#endif

       send.close();
	 keyEvent = key;
       recv.open();
    }
}


KeyInput::KeyInput(int Prio):
      send(sendInputKey),
      recv(recvInputKey, Prio)
{
}

long KeyInput::get()
{
    long ret=0;
    for( ;!ret; )
    {
       recv.close();
	   ret = keyEvent;
       send.open();
    }
   return ret;
}

void KeyInput::put(long key)
{
   send.close();
	 keyEvent = key;
   recv.open();
}


int KeyInput::getPrio()
{
  return recv.getPrio();
}

void KeyInput::setPrio(int Prio)
{
  recv.setPrio(Prio);
}

