#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <term.h>
#include <program.h>
#include <menu.h>
#include <yterm.h>
#include <filebox.h>

#include "w3win.h"
#include "hashcode.h"

#define cmLanguage	12345

long applKeyMap[] = {
	kbF4,		HASH_cmWWWBrowser,
	kbF9,		HASH_cmTerminal,
	0,
};

CommandDescr applCommandDescr[]=
{
	{ HASH_cmWWWBrowser,	new Lang("Browser as window") },
	{ HASH_cmTerminal,	new Lang("Terminal as window","Терминал как окно") },
	{0, 0}
};

Keymap ApplKeyMap( applKeyMap, new Lang("Application keys","Команды управления окнами") );

class Appl : public Program
{
public:
   Appl();
   void makeMenu();
   long handleKey( long key, void *&ptr );
};

Appl::Appl()
{
  keyHolder.add( &ApplKeyMap, applCommandDescr );
  colorsType = 1;
  setColors( colorsType );
}

long Appl::handleKey( long key, void *&ptr )
{
   switch( key )
    {
      case cmLanguage:
	  language = language ? 0 : 1;
	  redraw();
	  return 0;
      case HASH_cmTerminal:
	  nselect( insert( new YTerm("sh") ) );
	  return 0;
      case HASH_cmWWWBrowser:
//          nselect( insert( new W3Win( "file:/tmp/z.html" ) ) );
//          nselect( insert( new W3Win( "file:b.html" ) ) );
	  nselect( insert( new W3Win( "" ) ) );
	  return 0;
      case kbF5:
	  nselect( insert( new W3Win( "file:list.html" ) ) );
	  return 0;
    }
   return key;
}

//-------------------------------- menu -------------------------------
void Appl::makeMenu()
{
   static Menu *Smenu=0;

   if ( Smenu )
     { menu = Smenu; return; }

   menuColl *global = new menuColl;

   menuColl *coll = new menuColl;
   coll->insert( new menuItem( "~Change language",	0, 0, cmLanguage ) );
   coll->insert( new menuItem( new Lang("~WWW browser",	"~WWW окно"),		0, 0, HASH_cmWWWBrowser ) );
   coll->insert( new menuItem( new Lang("~Terminal",	"~Терминал"),		0, 0, HASH_cmTerminal ) );
   Menu *sub = new Menu( coll, Point(1,0) );
   global->insert( new menuItem( new Lang("~Open","~Открыть"),	0, 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( "~Next",		"Next window",	0, HASH_cmNextWin ) );
   coll->insert( new menuItem( "~Prev",		"Previous window", 0, HASH_cmPrevWin ) );
   coll->insert( new menuItem( "~First",	"First window",	0, HASH_cmFirst ) );
   coll->insert( new menuItem( "~Last",		"Last window",	0, HASH_cmLast ) );
   coll->insert( new menuItem( "~Size",		"Window size",	0, HASH_cmSize ) );
   coll->insert( new menuItem( "~Move",		"Window position", 0, HASH_cmPosition ) );
   coll->insert( new menuItem( "~Zoom",		"Window zoom",	0, HASH_cmZoom ) );
   coll->insert( new menuItem( "~Close",	"Close window",	0, HASH_cmCloseWin ) );
   coll->insert( new menuItem( "~List",		"Windows list",	0, HASH_cmChoose ) );
   coll->insert( new menuItem( "Ma~x size",	"Set maximum size for current window",	0, HASH_cmMaxSize ) );
   sub = new Menu( coll, Point(1,6), Point(1,9) );
   global->insert( new menuItem( "W~indow", "Window control", 0, sub ) );

   global->insert( new menuItem( "Exit", " This is ~exit~", 0, HASH_cmQuit ) );

   Smenu = menu = new Menu( global, Point(0,0), new Lang("Sys"), 0, 1, 0 );
   menu->fill( &keyHolder );
   sharedMenu.insert( menu );
   menuMgr->select( menu );
}

int main()
{
   Screen::initScreen( BASE_TERMINFO );
   W3_Lib_Init();
   setW3Cache( "/tmp/.w3cache" );

   getcwd( filebox::filePath, 255 );
   filebox::filePath[255] = 0;
   StartPath( filebox::filePath );

   strcpy( filebox::fileMask, "*" );

   Appl *a = new Appl;
   TaskHolder k( a );

   TaskHolder key( new InputKeyTask( k.task() ) );
   a->makeMenu();

   Task::run();

   W3_Lib_End();

#ifdef MALLOC_DEBUG
   malloc_shutdown();
#endif

   return 0;
}
