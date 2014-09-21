/*
	$Id: window.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef WINDOW_H
#define WINDOW_H

#include "getobj.h"

const int maxLineLength=1000;

extern Keymap WindowKeyMap;
extern int inputLineFillChar;

#define FLAG_LINE	( _flags & 0x001 )
#define FLAG_COL	( _flags & 0x002 )
#define FLAG_BLOCKLINE	( _flags & 0x004 )
#define FLAG_BLOCKCOL	( _flags & 0x008 )
#define BLOCK_EXIST	( _flags & 0x00f )

#define SET_LINE	_flags |= 0x001
#define SET_COL		_flags |= 0x002
#define SET_BLOCKLINE	_flags |= 0x004
#define SET_BLOCKCOL	_flags |= 0x008

#define CLEAR_LINE	_flags &= ~0x001
#define CLEAR_COL	_flags &= ~0x002
#define CLEAR_BLOCKLINE	_flags &= ~0x004
#define CLEAR_BLOCKCOL	_flags &= ~0x008
#define CLEAR_BLOCK	_flags &= ~0x00f

#define BLOCK_LINE	0
#define BLOCK_COL	1

#define UNKNOWN		-1
#define MONO		0
#define COLOR		1
#define LAPTOP		2

#define COLOR_FRAME	(this == appl->topWindow ? clr[0] : clr[1])
#define COLOR_BLOCK	clr[3]
#define COLOR_FIND	clr[4]

struct blockInfo
{
   long top;
   long bott;
   short left;
   short right;
};

class scrInfo
{
public:
  int lines, cols, maplen;
  char **map;
  char **orig;
  char **bord;
  scrInfo();
  ~scrInfo();
  void free_map();
  void alloc_map();
};

class Program;

class Window : public getobj, public Collection
{
friend class Program;
friend class winList;
friend class winExecutor;

protected:
   static long WinPid;
   Rect unzoomed;       // unzoomed window rect
   Point size;          // size of window
   Point minSize;       // minimum size of window
   Point maxSize;       // minimum size of window
   //-------------------------- for srolling ----------------------------
   PointL oldDelta;
   int topOffset, bottOffset;
   long topDraw, bottDraw;
   //--------------------------------------------------------------------
   int changed, current;
   short _flags;
   unsigned drawFrameOnly:1;
   Lang *title;
   Lang *ident;
   Lang *status;
   char *helpContext;
   int	scroll( int attr, int flagRedraw );
   void zoom( int flag=0 );
   int	isVisual();
   scrInfo scr;
   virtual void beforeExec();
   virtual void afterExec( long key );

   virtual void markBlock( int type );
   virtual void unmarkBlock();
   virtual void correctBlock();
   virtual blockInfo *validBlock();
   blockInfo block;

public:
   Window( Rect r=Rect(0,0,0,0), Lang *tit=0, Lang *id=0, Lang *st=0, int Box=1 );
   ~Window();
   virtual void reallocScrMap();
   virtual void resizeScreen( Rect &old );
   virtual int 	isType(long typ);

   virtual const char *winName()   // full name of window
	   { return title->get(); }
   virtual const char *winIdent()
	   { return ident->get(); }

   virtual int init( void *data=0 );
   virtual int nextKey() { return 0; }
   virtual int draw( int Redraw=0, int sy=0, int sx=0 );
   virtual void setOrigScrMap();
   virtual void setScrMap();
   virtual int close() { return 1; }
   virtual void drawStatus();

   virtual char *currentWord() { return ""; }
   virtual void makeMenu( Menu *m=0 ) {;}
   static void MoveCursor();

   long test( Collection *Text, char *Title=0, int cm1=0, char *s1=0,
		     int cm2=0, char *s2=0, int cm3=0, char *s3=0,
		     int cm4=0, char *s4=0, int cm5=0, char *s5=0 );
   long test( char *str, char *Title=0, int cm1=0, char *s1=0,
		     int cm2=0, char *s2=0, int cm3=0, char *s3=0,
		     int cm4=0, char *s4=0, int cm5=0, char *s5=0 );
   char *getString( char *prompt=0, int scrlen=10, Collection *hist=0, char *initStr=0, char *outStr=0, char *title=0, char *help=0, int fillChar=inputLineFillChar, char *status=0, InputType itype=INPUT );
   char *getPassword( char *prompt=0, int scrlen=32, char *title=0, char *help=0, int fillChar=inputLineFillChar, char *status=0 );

//   virtual void *getState() { return 0; }
//   virtual void setState( void *st ) {;}
   virtual const char *getHelpContext() { return helpContext; }
   virtual const char *setHelpContext( const char *newContext );

   void setStatus(const char *format, ...);
   void setTitle(const char *format, ...);
   void setIdent(const char *format, ...);

   long execUp( Window *w );
   long exec( Window *w );
   long exec( Collection *father, winExecutor *executor=0 );
   void redrawAppl();
   void	nonDraw();		// обнуление битовой карты рисования
   virtual int canDraw( int internalCall=1 ) { return 1; } // для запрета рисования из производных классов

   unsigned firstHandle:1;
   int winNo;			// window number
   int stk_len;
   long retCommand;
   unsigned selectable:1;
   unsigned redrawAfterExec:1;
   unsigned box:1;
   unsigned flagDelete:1;
   unsigned programRedraw:1;
   unsigned timerMessage:1;
   long winPid;
   virtual void	setRect( Rect &nRect );
   virtual void	setRect( Point &p );

   char *target_name;		// для использования во фреймах
};

extern char *wordDelim;
extern Collection clipBoard;
extern Program *appl;

extern int borderVisible;
Rect scrRect();

#endif
