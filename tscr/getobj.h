/*
	$Id: getobj.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _GETOBJ_H
#define _GETOBJ_H

#define TYPE_UNKNOWN	0
#define TYPE_HYPER	1
#define TYPE_BUTTON	2
#define TYPE_RADIO	3
#define TYPE_HOR_RADIO	4
#define TYPE_CHECK	5
#define TYPE_HOR_CHECK	6
#define TYPE_LIST	7
#define TYPE_INPUT	8
#define TYPE_EDIT	9
#define TYPE_TBROW	10
#define TYPE_DIALOG	11
#define TYPE_INTEGER	12
#define TYPE_FLOAT	13
#define TYPE_DATE	14

#include <collect.h>
#include <keycodes.h>
#include <screen.h>

#include "keymap.h"

#ifdef DJGPP

#ifndef u_char
#define u_char unsigned char
#endif

#ifndef u_short
#define u_short unsigned short
#endif

#ifndef u_long
#define u_long unsigned long
#endif

#endif

#define TRANS_none		0
#define TRANS_koi82alt		1
#define TRANS_koi82win		2
#define TRANS_koi82main		3
#define TRANS_alt2koi8		4
#define TRANS_win2koi8		5
#define TRANS_main2koi8		6

class ExecWindows;
class winExecutor;

extern int colorsType;
extern long getBoxKeyMap[];
extern Keymap GetBoxKeyMap;

extern unsigned char alt2koi8[];
extern unsigned char koi82alt[];
extern unsigned char win2koi8[];
extern unsigned char koi82win[];
extern unsigned char main2koi8[];
extern unsigned char koi82main[];

class Dialog;
class Menu;

class getobj
{
friend class Dialog;
friend class Appl;

public:
   getobj( Rect r=Rect(0,0,0,0) );
   virtual ~getobj();

   KeyHolder keyHolder;
   char *strError;

   Collection *father;
   winExecutor *Executor;
   Rect rect;
   u_short index;                // number in owner collection
   Dialog *owner;
   virtual int		init( void *data=0 );
   virtual int		draw( int Redraw=0, int sy=0, int sx=0 ) { return 1; }
   virtual long		handleKey( long key, void *&ptr ) { return key; }
   virtual int		accept( int flagMessage=1 )	{ return 1; }
   virtual int		type()			{ return TYPE_UNKNOWN; }
   virtual int 		isType(long typ);
   virtual void*	getObj()		{ return this; }

   virtual void		setColor( unsigned char *_clr )	{ clr=_clr; }
   virtual void		setColors( int type )	{;}
   virtual void *	getData()		{ return 0; }
   virtual int 		setData( void *data=0 )	{ return 1; }
   virtual Point 	getCursor( int *hide=0 ){ return Point(-1,-1); }
   virtual void 	setCursor( Point &p )	{;}
   virtual void 	moveCursor()		{;}
   virtual long 	getCurrent()		{ return -1; }
   virtual int		setCurrent( long i )	{ return 1; }
   virtual char *	valid( void *data=0 )	{ return strError; }

   Rect	&	getRect()		{ return rect; }
   virtual void		setRect( Rect &nRect );
   virtual void		setRect( Point &p );
   virtual int		translate_out( int symbol );
   virtual int		translate_in( int key );
   PointL delta;
   Point cursor;
   unsigned active:1;
   unsigned bind_flag:1;
   unsigned isGetObj:1;
   Menu *winMenu;
   unsigned char *clr;
   Collection *bindList;

   int trans_flag;	/* флаг текущей таблицы трансляции ввода/вывода */
};

class hyper : public getobj
{
protected:
   void *data;
#if 0
   unsigned char **label;
   int maxlen, str_count;
#else
   unsigned char *label;
#endif

public:
   hyper( Point p=Point(0,0), const char *l=0 );
   ~hyper();
   int init( void *d=0 );
   void clear();
   virtual int 	isType(long typ);
   virtual void setColors( int type );

   void *	getData()		{ return data; }
   const char * getLabel();
   void 	setLabel( const char *l );
#if 0
   void 	addLabel( const char *l );
#endif
   int 		setData( void *d=0 )	{ data=d; return 1; }

   int draw( int Redraw=0, int sy=0, int sx=0 );
   long handleKey( long key, void *&ptr ) { return key; }
   Point getCursor( int *hide=0 ) { return rect.a; }
   int type() { return TYPE_HYPER; }
};

class button : public hyper
{
protected:
   long command;
public:
   button( Point p=Point(0,0), const char *l=0, long cmd=-1 ) :
	   hyper(p,l), command(cmd) {;}
   long handleKey( long key, void *&ptr );
   int type() { return TYPE_BUTTON; }
   void *	getData()		{ return &command; }
   virtual int 	isType(long typ);
   void		setCommand( long c )	{ command=c; }
};

struct getBoxStatus
{
   Rect rect;
   int state;
   int current;
   Collection *items;
   getBoxStatus( Point p=Point(0,0), int st=0, int cur=0, Collection *el=0 );
};

class getBox : public getobj
{
protected:
   int state;
   virtual long changeState( long key, Point cur ) { return key; }
   virtual long changeCurrent( long key ) { return key; }
   virtual inline int isSelect( int i ) { return 0; }
   char flag;
   int *result;
public:
   getBox( Point p, int stat, int *res, int cur, Collection *items );
   virtual ~getBox();
   int init( void *d=0 );
   void *getStatus();
   int accept( int flagMessage=1 ) { if (result) *result=state; return 1; }
   long handleKey( long key, void *&ptr );
   long add( const char *str, int flag=1 );
   int del( long index );
   virtual int 	isType(long typ);
   virtual void setColors( int type );
   virtual void calcRect() {;}
   void *	getData()		{ return &state; }

   Collection *items;
   const char *prompt;
   int current;
};

class radioBox : public getBox
{
protected:
   long changeState( long key, Point cur );
   long changeCurrent( long key );
   inline int isSelect( int i ) { return state==i; }
public:
   radioBox( Point p=Point(0,0), int st=0, int *res=0, int cur=0,
	     Collection *items=0 ) :
	     getBox( p, st, res, cur, items ) { prompt=" ( ) "; flag='*'; }
   int draw( int Redraw=0, int sy=0, int sx=0 );
   Point getCursor( int *hide=0 ) { return rect.a + Point( current, 2 ); }
   int type() { return TYPE_RADIO; }
   virtual int 	isType(long typ);
   void calcRect();
};

class horRadioBox : public getBox
{
protected:
   long changeState( long key, Point cur );
   long changeCurrent( long key );
   inline int isSelect( int i ) { return state==i; }
   int width;
public:
   horRadioBox( Point p=Point(0,0), int st=0, int *res=0, int cur=0,
		Collection *items=0 ) :
		getBox( p, st, res, cur, items ) { prompt=" ( ) "; flag='*'; }
   int draw( int Redraw=0, int sy=0, int sx=0 );
   Point getCursor( int *hide=0 ) { return rect.a + Point( 0, current*width+2 ); }
   int type() { return TYPE_HOR_RADIO; }
   virtual int 	isType(long typ);
   void calcRect();
};

class checkBox : public radioBox
{
protected:
   long changeState( long key, Point cur );
   inline int isSelect( int i ) { return state & (1<<i); }
public:
   checkBox( Point p=Point(0,0), int st=0, int *res=0, int cur=0,
	     Collection *items=0 ) :
	     radioBox( p, st, res, cur, items ) { prompt=" [ ] "; flag='X'; }
   int type() { return TYPE_CHECK; }
   virtual int 	isType(long typ);
};

class horCheckBox : public horRadioBox
{
protected:
   long changeState( long key, Point cur );
   inline int isSelect( int i ) { return state & (1<<i); }
public:
   horCheckBox( Point p=Point(0,0), int st=0, int *res=0, int cur=0,
		Collection *items=0 ) :
	    horRadioBox( p, st, res, cur, items ) { prompt=" [ ] "; flag='X'; }
   int type() { return TYPE_HOR_CHECK; }
   virtual int 	isType(long typ);
};

#endif
