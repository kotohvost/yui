/*
	$Id: keytask.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "task.h"
#include "keytask.h"

#ifndef DJGPP
#include "term.h"
#else
#include <conio.h>
#endif

#include "hashcode.h"
#include "keycodes.h"

static unsigned long KbFunc1, KbFunc2;

InputKeyTask::InputKeyTask( Task *Reciever, unsigned long Kb_func1, unsigned long Kb_func2,
		unsigned long Kb_cyr, unsigned long BlankTime ) :
	Task("InputKeyTask", 4096),
	blankTime(BlankTime),
	reciever(Reciever)
{
   set_func1( Kb_func1 );
   set_func2( Kb_func2 );
   set_cyr( Kb_cyr );

   if ( !blankTime )
      blankTime=WaitForever;
   if ( reciever )
       addref( reciever );
}

InputKeyTask::~InputKeyTask()
{
   if ( reciever )
       delref( reciever );
}

#define LEFT_BUTTON_MASK   0x80
#define MIDDLE_BUTTON_MASK 0x40
#define RIGHT_BUTTON_MASK  0x20

long InputKeyTask::main()
{
   unsigned long key;
   int match=0;
   int funcFlag=0, fixFuncFlag=0, func2flag=0, cyrFlag=0, mouseFlag=0;
   unsigned char mousePars[3];

#ifndef DJGPP
   unsigned char ch;
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

	  switch( mouseFlag )
	   {
	     case 1:
	       mousePars[0]=ch;
	       mouseFlag=2;
	       continue;
	     case 2:
	       mousePars[1]=ch;
	       mouseFlag=3;
	       continue;
	     case 3:
	       mousePars[2]=ch;
	       mouseFlag=0;
	       if (mousePars[0])
	       {
		 MouseEvent ev;
		 ev.x = mousePars[1]-' ';
		 ev.y = mousePars[2]-' ';
		 ev.type = LeftClick;
		 if (mousePars[0] & RIGHT_BUTTON_MASK )
		    ev.type = RightClick;

		 sendMessage(reciever, new MouseMessage(ev));
	       }
	       continue;
	   }

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

	   if ( !mouseFlag && key==meta('S'))
	      {
		mouseFlag=1;
		continue;
	      }
#endif

	   if ( funcFlag && (func2flag || key!=kb_func2) )
	      { funcFlag=0; key |= 0x200; }
	   else if (key==kb_func1)
	      {
		if (func2flag)
		  func2flag=0;
		else
		 {
		   funcFlag=1;
		   continue;
		 }
	      }

	   if ( func2flag )
	      {
		func2flag=0;
		if (key!=kb_func2 && key!=kb_func1)
		  key |= 0x400;
	      }
	   else if (key==kb_func2)
	      { func2flag=1; continue; }

	   if ( key== (kb_func1|0x200) )
	      { func2flag=1; continue; }
#ifndef DJGPP
	   if (key==kb_cyr)
	      { cyrFlag = !cyrFlag; continue; }

	   if (key<256)
	     {
	       if (cyrFlag)
		 key=Term::cyrInputTable[key];
/*	       else
		 key=Term::inputTable[key];*/
	     }

#endif
	}
       else
	{
	  key = HASH_cmBlankScreen;
	}

       sendMessage(reciever, new KeyMessage(key));
    }
}

void InputKeyTask::set_reciever( Task *r )
{
	if ( reciever )
	    delref( reciever );

	reciever = r;

	if ( reciever )
	    addref( reciever );
}

void InputKeyTask::set_func1( unsigned long l )
{
	KbFunc1 = kb_func1 = l;
}

void InputKeyTask::set_func2( unsigned long l )
{
	KbFunc2 = kb_func2 = l;
}

void InputKeyTask::set_cyr( unsigned long l )
{
	kb_cyr = l;
}

const char *_descriptKey( int key )
{
  static char str[20]; str[0]=0;
  if( !ISKEY(key) )
    return "Command";

  int prefix=0;
  if ( ISFUNC1(key) )
    { sprintf( str, "^%c", (char)KbFunc1 + '@' ); prefix=1; }
  else if ( ISFUNC2(key) )
    { sprintf( str, "^%c", (char)KbFunc2 + '@' ); prefix=1; }
  else if ( ISFUNC12(key) )
    { sprintf( str, "^%c^%c", (char)KbFunc1 + '@', (char)KbFunc2 + '@' ); prefix=1; }

  key &= 0x1ff;

  if (key>32 && key<256)
   {
     int len=strlen(str);
     if (prefix)
       str[len++]=',';
     str[len++]=key;
     str[len]=0;
   }
  else if (key==' ')
     { if (prefix) strcat(str,","); strcat(str, "Space"); }
  else if (key==kbEnter)
     { if (prefix) strcat(str,","); strcat(str, "Enter"); }
  else if (key==kbTab)
     { if (prefix) strcat(str,","); strcat(str, "Tab"); }
  else if (key==kbEsc)
     { if (prefix) strcat(str,","); strcat(str, "Esc"); }
  else if (key==kbBS)
     { if (prefix) strcat(str,","); strcat(str, "BS"); }
  else if (key<32)
   {
     strcat (str, "^") ;
     int len=strlen(str);
     str[len]=key+64;
     str[len+1]=0;
   }
  else
   {
    if (prefix) strcat(str,",");
    switch(key)
     {
       case kbF1: strcat(str, "F1"); break;
       case kbF2: strcat(str, "F2"); break;
       case kbF3: strcat(str, "F3"); break;
       case kbF4: strcat(str, "F4"); break;
       case kbF5: strcat(str, "F5"); break;
       case kbF6: strcat(str, "F6"); break;
       case kbF7: strcat(str, "F7"); break;
       case kbF8: strcat(str, "F8"); break;
       case kbF9: strcat(str, "F9"); break;
       case kbF10:  strcat(str, "F10"); break;
       case kbF11:  strcat(str, "F11"); break;
       case kbF12:  strcat(str, "F12"); break;
       case kbUp:   strcat(str, "Up"); break;
       case kbDown: strcat(str, "Down"); break;
       case kbLeft: strcat(str, "Left"); break;
       case kbRight: strcat(str, "Right"); break;
       case kbHome: strcat(str, "Home"); break;
       case kbEnd:  strcat(str, "End"); break;
       case kbPgUp: strcat(str, "PgUp"); break;
       case kbPgDn: strcat(str, "PgDn"); break;
       case kbIns:  strcat(str, "Ins"); break;
       case kbDel:  strcat(str, "Del"); break;
       default:     strcat(str, "???"); break;
     }
   }
  return str;
}



