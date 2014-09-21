/*
	$Id: menu.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _MENU_H
#define _MENU_H

#include "hashcode.h"
#include "listbox.h"

#define COMMAND		0
#define MENU		1
#define SEPARATOR	2

class Menu;

struct menuItem
{
   u_char type;
   Lang *label;
   Lang *status;
   char *key;
   char *help;
   union {
       long command;
       Menu *menu;
   };
   menuItem( Lang *msg, Lang *s=0, char *h=0, long command=HASH_cmOK );
   menuItem( Lang *msg, Lang *s=0, char *h=0, Menu *m=0 );
   menuItem( char *l=0, char *s=0, char *h=0, long command=HASH_cmOK );
   menuItem( char *l=0, char *s=0, char *h=0, Menu *m=0 );
   ~menuItem();
   void init( char *h );
   u_char hotkey();
};

extern long menuKeyMap[];
extern Keymap MenuKeyMap;

/*---------------------------------------------------------------*/
class menuColl : public Collection
{
protected:
   void freeItem( void *item ) { delete (menuItem*)item; }
public:
   menuColl() : Collection(10,10) {;}
   ~menuColl() { freeAll(); }
};

class Menu : virtual public Window
{
friend class MenuManager;

protected:
   Rect orig;
   Point langPoint[LANGS];
   menuColl *mcoll;
   int vert, offsetRect, width;
//   menuColor *color;
   void beforeExec();
   void afterExec( long key );
   void startInit( int Box, Lang *tit );
   void setRect();

public:
   Menu( menuColl *it, Point coor, Lang *tit=0, int Vert=1, int Auto=1, int Box=1 );
   Menu( menuColl *it, Point coor1, Point coor2, Lang *tit=0, int Vert=1, int Auto=1, int Box=1 );
   ~Menu();
   virtual void reallocScrMap();
   virtual int isType(long typ);
   virtual void setColors( int type );
   int autoOpen;
   static int state;
   static int curstate;
   int oldState;
   int init( void *data=0 );
   long handleKey( long key, void *&ptr );
   void changeRect( int offset );
   long changeCurrent( long key, int flag=0 );
   int draw( int Redraw=0, int sy=0, int sx=0 );
   void drawStatus();
   void moveCursor();
   void fill( KeyHolder *kmap=0 );
   void setHotKeys( char *mas );
   Point getPoint();
   void *varptr;
};

class MenuManager : virtual public Window
{
protected:
   int offset;
//   menuColor *color;
   virtual void beforeExec();
   virtual void afterExec( long key );

public:
   MenuManager();
   virtual void reallocScrMap();
   virtual int 	isType(long typ);
   virtual void setColors( int type );
   int init( void *data=0 );
   void select( Menu *m, int index=-1 );
   void remove( Menu *m );
   virtual long handleKey( long key, void *&ptr );
   virtual int draw( int Redraw=0, int sy=0, int sx=0 );
   virtual void setScrMap();
   virtual void moveCursor();
   int sepchar;
   int canSwitch;
   char hotKeys[32];
   int hotKey;
   char Visible, visible;
   int isHotKey( long key );
};

class SharedMenuCollection : public SortedPtrCollection
{
protected:
   void freeItem( void *item ) { delete (Menu*)item; }
public:
   SharedMenuCollection() { shouldDelete=1; }
   ~SharedMenuCollection() { freeAll(); }
};

extern SharedMenuCollection sharedMenu;

#endif
