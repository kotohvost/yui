/*
	$Id: listbox.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _LISTBOX_H
#define _LISTBOX_H

#include <array.h>
#include "window.h"

extern long listBoxKeyMap[];
extern Keymap ListBoxKeyMap;
extern long inputLineKeyMap[];
extern Keymap InputLineKeyMap;
extern int inputLineFillChar;

class listItem
{
public:
  listItem( const char *item, const char *info, int state );
  ~listItem();
  char *item, *info;
  int state;
};

class listBox : public Window
{
protected:
   Rect maskRect;
   int lines, scrItems, columns, width, infoLines;
   int cmpItem( const char *str1, const char *str2, int ch, int pos );
   int newMask( int type );
   int boxMask( char *tit=0 );
   int *result;
   short normColor;
   short selColor;
   short curColor;
   short curSelColor;
   int countIns;
   void decrementState( int s );
   virtual void freeItem( void *item );

public:
   listBox( Rect r=Rect(0,0,0,0), int Width=1, int *res=0,
	    Collection *Items=0, Collection *State=0, Collection *Info=0,
	    int cur=0, int Delta=0 );
   ~listBox();
   virtual int 	isType(long typ);
   virtual int init( void *data=0 );
   virtual void setColors( int type );
   int draw( int Redraw=0, int sy=0, int sx=0 );
   virtual u_short *scrLine( int line, int &len );
   long handleKey( long key, void *&ptr );
   Point getCursor( int *hide=0 );
   void moveCursor();
   ccIndex add( const char *str, const char *Info=0, int State=0 );
   int type() { return TYPE_LIST; }
   int accept( int flagMessage=1 ) { if (result) *result=current; return 1; }

   void newItems( Collection *Items=0, Collection *State=0,
		  Collection *Info=0, int cur=-1, int Delta=-1 );

   void moveHome();
   void moveEnd();
   ccIndex getCurrent() { return current; }
   char *getItem( ccIndex index );
   int setCurrent( ccIndex newcur );
   int separator, manualPos;
   char *mask;
   void *	getData()		{ return &current; }
   int getWidth() { return size.x - (box ? 2 : 0); }
   int setWidth( int w );
};

class inputLine : public getobj
{
friend class Dialog;
protected:
   char *str, *origStr;
   Point origP;
   int max_len, origScrLen, lines, delta, pos;
   int insMode, fillChar;
   int runHistory();
   virtual void addChar( int ch );
   virtual void deleteChar();
   virtual void backspace();
   virtual void copyToClip();
   virtual void copyFromClip();
   char *result;
   unsigned firstkey:1;
   unsigned password:1;
   virtual void clearStr();

public:
   inputLine( Rect r, const char *s=0, char *res=0, int l=0,
	      const char *reg=0, Collection *hist=0,
	      int fchar=inputLineFillChar,
	      unsigned pass=0 );
   inputLine( Point p, const char *s=0, char *res=0, int lscr=1, int l=0,
	      const char *reg=0, Collection *hist=0,
	      int fchar=inputLineFillChar,
	      unsigned pass=0 );
   ~inputLine();

   char *regex;
   int len, scrlen, width;
   virtual int 	isType(long typ);
   virtual void setColors( int type );
   virtual int draw( int Redraw=0, int sy=0, int sx=0 );
   virtual Point getCursor( int *hide=0 );
   virtual int initString( const char *s );
   virtual int init( void *data=0 );
   virtual long handleKey( long key, void *&ptr );
   virtual int accept( int flagMessage=1 );
   void addToHistory();
   virtual char *valid( void *data=0 );
   virtual int type() { return TYPE_INPUT; }
   virtual void moveHome() { pos=delta=0; }
   virtual void moveEnd();
   virtual void moveLeft();
   virtual void moveRight();
   void resetFirst() { firstkey=0; }
   void *	getData()		{ return str; }
   int getWidth();
   int setWidth( int w );
   virtual void setRect( Point &p );
   Collection *history;
   unsigned clearFlag:1;
   int getPos() { return pos; }
};

#endif
