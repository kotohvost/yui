/*
	$Id: winlist.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <stdio.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "winlist.h"
#include "hashcode.h"
#include "program.h"

long winListKeyMap[] = {
	kbDel,		HASH_cmDelete,
	0
};

Keymap WinListKeyMap( winListKeyMap );

int  winList::isType(long typ)
{
   return (typ==HASH_winList ? 1 : listBox::isType(typ));
}

winList::winList( int *n ) : listBox( Rect(0,0,1,1), 0, n )
{
   keyHolder.add( &WinListKeyMap );
   if ( !title )
      title = new Lang("");
   if ( !status )
      status = new Lang("");
   title->put( " Windows list ", 0 );
   title->put( " Список окон ", 1 );
   status->put( "~Esc~-exit  ~Enter~-select  ~Ins~-marker  ~Del~-close", 0 );
   status->put( "~Esc~-выход  ~Enter~-выбрать  ~Ins-маркер  ~Del~-закрыть", 1 );
   current = appl->getCurrent();
   Rect r = scrRect();
   resizeScreen( r );
}

winList::~winList()
{
   if ( appl->winLst == this )
      appl->winLst = 0;
}

int winList::init( void *data )
{
   freeAll();
   int i, sel;
   for( i=0, sel=0; i<appl->getCount() ; i++ )
     if ( ((Window*)((winExecutor*)appl->at(i))->wins.at(0))->selectable == 0 )
	{ sel=1; break; }
   static char str[256];
   static char name[256];
//   int visLen = width - 6;
   for( i=0; i<appl->getCount(); i++ )
    {
       Window *p = ((winExecutor*)appl->at(i))->win;
       sprintf( str, "%2d %s: ", p->winNo, p->winIdent() );
       const char *ch = p->winName();
       strcpy( name, ch ? ch : "" );
       int j=0, k=0, len=width-strlen(str)-1;
       if ( strlen(name) > (size_t)len )
	 {
	   for( ; len>4 && (size_t)(len-3) <= strlen(name+j); j++ );
	   for( k=j; name[k] && !strchr( wordDelim, name[k] ); k++ );
	 }
       if ( j )
	  strcat( str, "..." );
       strcat( str, name+(name[k] ? k : j) );
//       str[50]=0;
       add( str, 0, (sel ? (p->selectable ? 1 : 0) : 0) );
     }
   return listBox::init( data );
}

void winList::setSelectedWin()
{
   int i=0, sel=0, state=0;
   for( ; i < count; i++ )
     if ( ((listItem*)items[i])->state )
       { sel=1; break; }
   for( i=0; i<appl->getCount(); i++ )
    {
      state = ((listItem*)items[i])->state;
      ((Window*)(((winExecutor*)appl->at(i))->wins.at(0)))->selectable = sel ? (state ? 1 : 0) : 1;
    }
}

long winList::handleKey( long key, void *&ptr )
{
   key = keyHolder.translate( key );
   switch( key )
     {
	case HASH_cmOK:
	case HASH_cmClose:
	    setSelectedWin();
	    break;
	case HASH_cmInit:
	    init();
	    redrawAppl();
	    return 0;
	case HASH_cmDelete:
	    if ( appl->getCount() > 0  )
	       Task::sendMessage( (winExecutor*)appl->at(current), new KeyMessage( HASH_cmCloseWin ) );
	    return 0;
     }
   return listBox::handleKey( key, ptr );
}

void winList::resizeScreen( Rect &old )
{
  if ( isGetObj )
     return;
  int dx = scr.cols - scr.cols / 4;
  Rect r = Screen::center( scr.lines - scr.lines / 4, dx );
  setWidth( dx - 2 );
  setRect( r );
}

char *getHelpContext()
{
   return "WinList";
}
