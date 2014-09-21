/*
	$Id: program.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _PROGRAM_H
#define _PROGRAM_H

#include <task.h>
#include <keytask.h>
#include <point.h>

#include "keymap.h"

#define HELP_CONTEXT	1
#define HELP_INDEX	2
#define HELP_PREV	3
#define HELP_ON_HELP	4

class Window;
class winList;
class KeyBox;

/*-----------------------------------------------------*/

struct ExecWindows : public Collection
{
   ExecWindows() : Collection(5,5) { shouldDelete=0; }
   ~ExecWindows() { freeAll(); }
};

/*-----------------------------------------------------*/

#define INSERTED	0
#define EXEC		1

class winExecutor : public Task
{
protected:
   winExecutor *we;
public:
   winExecutor( Window *w, unsigned isIns, int stk = 0 );
   ~winExecutor();
   long main();
   Window *win;
   long resultKey;
   short index;                // number in Program collection
   ExecWindows wins;
   unsigned selfClose:1;
   unsigned type:2;
   int firstHandle();
   virtual MessPath handleMessage( Message *msg );
   virtual Window *message( long command, void *ptr, long winPid );
};

/*-----------------------------------------------------*/

class BackMessage : public KeyMessage
{
public:
   BackMessage( long Data, void *Ptr=0 ) : KeyMessage( Data, Ptr ) {;}
   virtual long type();
};

/*-----------------------------------------------------*/

class IdentMessage: public KeyMessage
{
public:
  IdentMessage( long com=-1, void *ptr=0 ): KeyMessage( com, ptr ) {;}
  virtual long type();
};

/*-----------------------------------------------------*/

class TimerMessage: public KeyMessage
{
public:
  TimerMessage( long com=-1, long pid=-1 ): KeyMessage( com, 0 ),
		winPid(pid) {;}
  virtual long type();
  long winPid;
};

class Timer2 : public Timer
{
public:
   Timer2( Task *recv, long com, long pid, int sec=INT_MAX ) :
	Timer( recv, com, sec ), winPid(pid) {;}
   virtual long main();
   long winPid;
};

/*-----------------------------------------------------*/

extern long programKeyMap[];
extern Keymap ProgramKeyMap;
extern CommandDescr programCommandDescr[];
extern char getStringBuf[1024];
extern char getPasswordBuf[1024];
extern int backgroundSymbol;
//extern int backgroundAttr;
extern int inputLineFillChar;

class MenuManager;
class StatusLine;
class Menu;

class Program: public Collection, public Task
{
friend class Menu;
friend class Window;
friend class winExecutor;

protected:
   int current;
   int newCurrent;
   void setActive( int newcur );
   void newPosition();
   void newSize();
   void cascadeWindows();
   void tileWindows();
   int drawContinue;
   int flagDrawStatus;
   Lang status;
   virtual void makeMenu() {;}
   static Collection winStack;
   static void insertExecWindow( Window *w );
   KeyHolder keyHolder;
   virtual int saveStatus() { return 1; }
   virtual int restStatus() { return (started=1); }
   Window *getWin( int index );
   void nextWin();
   void prevWin();

public:
   Program();
   ~Program();

   void resizeScreen();

   long main();
   virtual long handleKey( long key, void *&ptr ) { return key; }
   Window *topWindowPtr();
   void redraw( int sync=1 );
   void drawShadow();
   void drawWindow( Window *w, int shadow=1 );
   void drawExecWindows( Collection *wins );
   virtual void drawBackground();
   virtual void drawStatus( int fromRedraw=0 );

   int select(int winNo);	// select window by winNo
   int getIndexByNo( int winNo );
   int getIndexByPid( long winPid );
   int select(Window *w);
   int nselect(int no);		// select window by No
   void setRect( Window *who, Rect newRect );
   ccIndex insert( Window *item );
   ccIndex Insert( Window *item );
   int closeWin( int pos, int Redraw=1 );
   int closeWin( winExecutor *we, int Redraw=1 );
   virtual void processClose( winExecutor *we ) {;}
   int moveWin( int src, int dst );
   int closeExecutor( winExecutor *we );
   int closeAll();
   int getCurrent() { return current; }

   static long test( /*Task *task,*/ Collection *Text, char *Title=0, int cm1=0, char *s1=0,
		     int cm2=0, char *s2=0, int cm3=0, char *s3=0,
		     int cm4=0, char *s4=0, int cm5=0, char *s5=0 );

   static long test( /*Task *task,*/ char *str, char *Title=0, int cm1=0, char *s1=0,
		     int cm2=0, char *s2=0, int cm3=0, char *s3=0,
		     int cm4=0, char *s4=0, int cm5=0, char *s5=0 );
/*
   static long test( Collection *Text, char *Title=0, int cm1=0, char *s1=0,
		     int cm2=0, char *s2=0, int cm3=0, char *s3=0,
		     int cm4=0, char *s4=0, int cm5=0, char *s5=0 )
	{ return test( 0, Text, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 ); }

   static long test( char *str, char *Title=0, int cm1=0, char *s1=0,
		     int cm2=0, char *s2=0, int cm3=0, char *s3=0,
		     int cm4=0, char *s4=0, int cm5=0, char *s5=0 )
	{ return test( 0, str, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 ); }
*/
   static long exec( Task *task, Window *w );
/*
   static char *getString( long comamnd, Task *task, char *prompt=0,
			int scrlen=10,
			Collection *hist=0, char *initStr=0, char *outstr=0,
			char *title=0, char *help=0,
			int fillChar=inputLineFillChar,
			char *status=0, InputType itype=INPUT );

   static char *getString( Task *task, char *prompt=0, int scrlen=10,
			Collection *hist=0, char *initStr=0, char *outstr=0,
			char *title=0, char *help=0,
			int fillChar=inputLineFillChar,
			char *status=0, InputType itype=INPUT  )
	{ return getString( (long)0, task, prompt, scrlen, hist, initStr, outstr, title, help, fillChar, status, itype ); }
*/
   static char *getString( long command, char *prompt=0, int scrlen=10,
			Collection *hist=0, char *initStr=0, char *outstr=0,
			char *title=0, char *help=0,
			int fillChar=inputLineFillChar,
			char *status=0, InputType itype=INPUT  );
//	{ return getString( command, (Task*)0, prompt, scrlen, hist, initStr, outstr, title, help, fillChar, status, itype ); }

   static char *getString( char *prompt=0, int scrlen=10,
			Collection *hist=0, char *initStr=0, char *outstr=0,
			char *title=0, char *help=0,
			int fillChar=inputLineFillChar,
			char *status=0, InputType itype=INPUT  )
	{ return getString( (long)0, /*(Task*)0,*/ prompt, scrlen, hist, initStr, outstr, title, help, fillChar, status, itype ); }

/*
   static char *getPassword( Task *task, char *prompt=0, int scrlen=32, char *title=0, char *help=0,
			int fillChar=inputLineFillChar,
			char *status=0 );

   static char *getPassword( char *prompt=0, int scrlen=32, char *title=0, char *help=0,
			int fillChar=inputLineFillChar,
			char *status=0 )
	{ return getPassword( 0, prompt, scrlen, title, help, fillChar, status ); }
*/
   static winExecutor *insertToStack( Window *w );

   ccIndex findWindow( long command, void *ptr=0 );
   void setColors( int type );
   virtual void runHelp( long type ) {;}

   int started;
   MenuManager *menuMgr;
   StatusLine *statusLine;
   Menu *menu;
   Window *topWindow;
   winList *winLst;
   KeyBox *keyBox;
   unsigned char *clr;
   int quit_empty;
};

extern Program *appl;
extern int colorsType;
//extern int flagResizeScreen;

#endif
