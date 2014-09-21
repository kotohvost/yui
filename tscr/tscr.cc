/*
	$Id: tscr.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/

#include <stdio.h>
#include <signal.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "term.h"

#include "program.h"
#include "dialog.h"
#include "menu.h"
#include "status.h"
#include "filebox.h"
#include "i_lines.h"
#include "tbrow.h"
#include "tag.h"

#ifndef DJGPP
#include "yterm.h"
#endif

#define cmLanguage	12345

unsigned char colorPDialog[12] = {
	FG_HI_WHITE | BG_YELLOW,	// active frame
	FG_BLACK | BG_YELLOW,		// frame
	FG_WHITE | BG_BLUE,		// block
	FG_HI_WHITE | BG_YELLOW,		// find
	FG_BLACK | BG_YELLOW,		// background
	FG_HI_WHITE | BG_YELLOW,	// selectet text
	FG_HI_WHITE | BG_YELLOW,	// selectet text
	FG_HI_WHITE | BG_YELLOW,	// selectet text
	FG_HI_WHITE | BG_YELLOW,	// selectet text
	FG_HI_WHITE | BG_YELLOW,	// selectet text
	FG_HI_WHITE | BG_YELLOW,	// selectet text
	FG_HI_WHITE | BG_YELLOW		// selectet text
};

long applKeyMap[] = {
	kbF2,		HASH_cmOpen,
	FUNC1(kbF2),	HASH_cmNew,
	kbF3,		HASH_cmBlankModal,
	kbF7,		HASH_cmBlankWindow,
	kbF5,		HASH_cmModalWin,
	kbF6,		HASH_cmListBox,
	kbF8,		HASH_cmTbrowser,
	FUNC1(kbF8),	HASH_cmBrowDialog,
	kbF9,		HASH_cmTerminal,
	kbF10,		HASH_cmGetTermCommand,
	0,
};

CommandDescr applCommandDescr[]=
{
	{ HASH_cmOpen,		new Lang("Open file box","Панель выбора файлов") },
	{ HASH_cmNew,		new Lang("Open file box as window","То же самое как окно") },
	{ HASH_cmBlankModal,	new Lang("Blank as modal window","Бланк") },
	{ HASH_cmBlankWindow,	new Lang("Blank as inserted window","Бланк как окно") },
	{ HASH_cmModalWin,	new Lang("Modal window","Модальное окно") },
	{ HASH_cmListBox,	new Lang("ListBox as window","ListBox как окно") },
	{ HASH_cmTerminal,	new Lang("Terminal as window","Терминал как окно") },
	{0, 0}
};

Keymap ApplKeyMap( applKeyMap, new Lang("Application keys","Команды управления окнами") );

#define LENGTH	40000

char str[LENGTH+1];
char s1[100], s2[100], s3[20];
double doubleVal;

class Pdialog : public Dialog
{
public:
   Pdialog();
   ~Pdialog() { winMenu=0; }
   void setColors( int type );
   long handleKey( long key, void *&ptr );
   void makeMenu( Menu *m=0 );
};

Pdialog::Pdialog() : Dialog( scrRect() )
{
   title->put( "Blank window", 0 );
   title->put( "Пустое окно", 1 );
/*   status->put( " ~F1~-help", 0 );
   status->put( " ~F1~-помощь", 1 );*/
   hardMode=0;
}

void Pdialog::setColors( int type )
{
   clr = colorPDialog;
   for( ccIndex i=count-1; i>=0; i-- )
     ((getobj*)items[i])->setColors( type );
}

long Pdialog::handleKey( long key, void *&ptr )
{
   modal *m;
   long ret;
   switch( key )
    {
     case kbCtrlT:
	  m = new modal( ALIGN_CENTER, "Close ?", " Modal(blocking) ", HASH_cmCancel, " OK ", HASH_cmOK, " Cancel " );
	  ret = execUp( m );
	  switch( ret )
	   {
	     case HASH_cmOK:
		 test( "Unblocking, good!" );
		 break;
	     case HASH_cmCancel:
		 test( "Cancelling, good!" );
		 return ret;
	   }
	  key = 0; break;
     case kbCtrlG:
	  {
	  char *str = getString( "asdfadsf" );
	  if ( str )
	     test( str );
	  }
	  key = 0; break;
     default:
	  key = Dialog::handleKey( key, ptr );
    }
   return key;
}

void fillDialog( Dialog *d, int i, Collection &coll )
{

   d->put( 0, 0, "xxx", 0, 1 );
   d->put( i+4, 42, "L\bLi\big\bgh\bht\bt t\bte\bex\bxt\bt(\b(b\bba\bac\bck\bks\bsp\bpa\bac\bce\bes\bs)\b)\nрус" );
   d->put( i, 1, "Float1:" );
   d->insert( new FloatInputLine( Point(i,8), "-.5", s1, 10, 10, 1 ) );
   d->put( i+1, 1, "Float2:" );
   d->insert( new FloatInputLine( Point(i+1,8), 23.009, &doubleVal, 10, 10, 1 ) );

   d->insert( new hyper( Point(i+2,11), " This is hyper " ) );

   d->put( i+6, 55, "Date inputLine" );
   d->insert( new DateInputLine( Point(i+7,55), "22/08/38", s3 ) );

   d->put( i, 30, "integer inputLine(+)" );
   d->insert( new IntegerInputLine( Point(i+1,30), "3", s1, 8, 12 ) );
   d->put( i+2, 30, "integer inputLine(-)" );
   d->insert( new IntegerInputLine( Point(i+3,30), "4", s1, 8, 12, 1 ) );

   d->insert( new button( Point(i+8,30), " This\n is button \n (Cancel)", HASH_cmCancel ) );

   d->put( i+4, 4, "RadioBox" );
   radioBox *rb=new radioBox( Point(i+5,4) );
   rb->add( "qwer" );
   rb->add( "asdf" );
   rb->add( "zxcv" );
   d->insert( rb );

   d->put( i+5, 20, "HorRadioBox" );
   horRadioBox *hrb=new horRadioBox( Point(i+6,20) );
   hrb->add( "qwer" );
   hrb->add( "asdf" );
   hrb->add( "zxcv" );
   d->insert( hrb );

   d->put( i+8, 15, "CheckBox" );
   checkBox *cb=new checkBox( Point(i+9,15), 2 );
   cb->add( "qwer" );
   cb->add( "asdf" );
   cb->add( "zxcv" );
   d->insert( cb );

   d->put( i+10, 40, "HorCheckBox" );
   horCheckBox *hcb=new horCheckBox( Point(i+11,40), 5 );
   hcb->add( "qwer" );
   hcb->add( "asdf" );
   hcb->add( "zxcv" );
   d->insert( hcb );

   d->put( i+12, 3, "ListBox (1 col)" );
   listBox *lb=new listBox( Rect( i+13, 3, i+17, 7 ), 5 );
   int j;
   for( j=0; j<coll.getCount(); j++ )
      lb->add( (char*)coll.at(j) );
   d->insert( lb );

   d->put( i+12, 20, "ListBox (5 col)" );
   lb=new listBox( Rect( i+13, 20, i+17, 45 ), 4 );
   for( j=0; j<coll.getCount(); j++ )
      lb->add( (char*)coll.at(j) );
   d->insert( lb );

   d->put( i+16, 20, "inputLine" );
   d->insert( new inputLine( Point(i+17,50), "asdf", s1, 8, 6, 0, &coll ) );

   d->put( i, 55, "inputLine(2 lines)" );
   d->insert( new inputLine( Rect(i+1,55,i+2,65), "", s2, 25, 0, &coll ) );

}

void Pdialog::makeMenu( Menu *m )
{
   static Menu *menu=0;

   if ( menu )
     { winMenu = menu; return; }

   menuColl *global = new menuColl;

   menuColl *coll = new menuColl;
   coll->insert( new menuItem( "~Reload",	0, 0, HASH_cmReload ) );
   coll->insert( new menuItem( "S~hare",	0, 0, HASH_cmShare ) );
   coll->insert( new menuItem( "R~ead only",	0, 0, HASH_cmMode ) );
   coll->insert( new menuItem( "K~eymap (window)",	0, 0, HASH_cmKeymapWindow ) );
   Menu *sub = new Menu( coll, Point(1,0) );
   global->insert( new menuItem( "~One",	0, 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( "~Add to buffer",	0, 0, HASH_cmAddToClip ) );
   coll->insert( new menuItem( "~Edit buffer",		0, 0, HASH_cmClipboard ) );
   coll->insert( new menuItem( "~Load block",		0, 0, HASH_cmLoadBlock ) );
   coll->insert( new menuItem( "~Save block",		0, 0, HASH_cmSaveBlock ) );
   sub = new Menu( coll, Point(1,5) );
   global->insert( new menuItem( "~Two",	0, 0, sub ) );

   global->insert( new menuItem( "T~hree", " This is ~test menu~", 0, HASH_cmMode ) );

   menu = winMenu = new Menu( global, Point(0,0), new Lang("Test","Тест"), 0, 1, 0 );
   winMenu->fill( &keyHolder );
   sharedMenu.insert( winMenu );
}

class Appl : virtual public Program
{
public:
   Appl();
   void makeMenu();
   long handleKey( long key, void *&ptr );
};

Collection coll(100,10);

Appl::Appl()
{
  keyHolder.add( &ApplKeyMap, applCommandDescr );
  char buf[5]; buf[4]=0;
  for( int j=0; j<100; j++ )
   {
     sprintf( buf, "%d", j );
     char *ch=new char[ strlen(buf) + 1 ];
     strcpy( ch, buf );
     coll.insert( ch );
   }
//  quit_empty = 1;
}

static char termCommand[512];

long Appl::handleKey( long key, void *&ptr )
{
   switch( key )
    {
      case cmLanguage:
	  language = language ? 0 : 1;
	  redraw();
	  return 0;
      case HASH_cmNew:
	  nselect( insert( new filebox( filebox::filePath ) ) );
	  return 0;
      case HASH_cmOpen:
	  insertExecWindow( new filebox( filebox::filePath ) );
	  return 0;
      case HASH_cmBlankModal:
      case HASH_cmBlankWindow:
	  {
	  Dialog *d=new Dialog;
	  for( int i=0, j=0; j<1; i+=20, j++ )
	    fillDialog( d, i, coll );
/*
	  Pdialog *d2 = new Pdialog;
	  Rect r( 21, 10, 26, 40 );
//	  d2->box = 0;
	  d2->setRect( r );
	  fillDialog( d2, 0, coll );
	  d->insert( d2 );
*/
	  if ( key == HASH_cmBlankWindow )
	     nselect( insert( d ) );
	  else
	     insertExecWindow( d );
	  }
	  return 0;
      case HASH_cmTbrowser:
	  {
	    Tbrow *tb = new Tbrow( scrRect() );
	    if ( !tb )
	       return 0;
	    tb->insert( tb->makeColumn( "column 1", 0, 10, 0 ) );
	    tb->insert( tb->makeColumn( "column 2", 0, 12, 0 ) );
	    tb->insert( tb->makeColumn( "column 3", 0, 15, 0 ) );
	    tb->insert( tb->makeColumn( "column 4", 0, 9, 0 ) );
	    tb->insert( tb->makeColumn( "column 5", 0, 14, 0 ) );
	    tb->insert( tb->makeColumn( "column 6", 0, 13, 0 ) );
	    tb->insert( tb->makeColumn( "column 7", 0, 10, 0 ) );
	    tb->insert( tb->makeColumn( "column 8", 0, 12, 0 ) );
	    tb->insert( tb->makeColumn( "column 9", 0, 15, 0 ) );
	    tb->insert( tb->makeColumn( "column 10", 0, 9, 0 ) );
	    tb->insert( tb->makeColumn( "column 11", 0, 14, 0 ) );
	    tb->box=0;
	    nselect( insert( tb ) );
	  }
	  return 0;
      case HASH_cmBrowDialog:
	  {
	    Tbrow *tb = new Tbrow( Rect(5,5,15,75) );
	    if ( !tb )
	       return 0;
	    tb->insert( tb->makeColumn( "column 1", 0, 10, 0 ) );
	    tb->insert( tb->makeColumn( "column 2", 0, 12, 0 ) );
	    tb->insert( tb->makeColumn( "column 3", 0, 15, 0 ) );
	    tb->insert( tb->makeColumn( "column 4\nasdf", 0, 9, 0 ) );
	    tb->insert( tb->makeColumn( "column 5", 0, 14, 0 ) );
	    tb->insert( tb->makeColumn( "column 6", 0, 13, 0 ) );
	    tb->insert( tb->makeColumn( "column 7", 0, 10, 0 ) );
	    tb->insert( tb->makeColumn( "column 8", 0, 12, 0 ) );
	    tb->insert( tb->makeColumn( "column 9", 0, 15, 0 ) );
	    tb->insert( tb->makeColumn( "column 10", 0, 9, 0 ) );
	    tb->insert( tb->makeColumn( "column 11", 0, 14, 0 ) );
	    Pdialog *d=new Pdialog;
	    d->insert( tb );
	    nselect( insert( d ) );
	  }
	  return 0;
      case HASH_cmListBox:
	  {
	  listBox *lb = new listBox( Rect( 6, 5, 18, 15 ), 3 );
	  for( int j=0; j<coll.getCount(); j++ )
	     lb->add( (char*)coll.at(j) );
	  nselect( insert( lb ) );
	  }
	  return 0;
      case HASH_cmModalWin:
	  test( "gakfhahdfkadffajdhkhgfkhjdfasdf\nasdf\nasdf\nadjfhaldfhaljdfhkahdfkahdkjfhgakdjhgfkaghfkhdgfkjahgfkjhgfkjasdf\nasdf\nasdf", " Modal window ",
//          test( "qwer\nasdf\nzxcv", " Modal window ",
	     HASH_cmOK, " OK ", HASH_cmNext, " Next ", HASH_cmPrev, " Prev ",
	     HASH_cmCancel, " Cancel ", HASH_cmQuit, " Quit " );
	  return 0;
#ifndef DJGPP
      case HASH_cmTerminal:
	  nselect( insert( new YTerm("/bin/sh") ) );
	  return 0;
      case HASH_cmTermCommand:
	  {
	  YTerm *yt=new YTerm( (char*)termCommand );
	  yt->flagFinishMessage = 1;
	  nselect( insert( yt ) );
	  }
	  return 0;
#endif
      case HASH_cmGetTermCommand:
	  getString( HASH_cmTermCommand, "String:", 30, 0, "asdf", termCommand, " Title " );
	  return 0;
      case HASH_cmString:
	  {
	  char *str = getString( "String:", 30, 0, "asdf", 0, " Title " );
	  if ( str )
	     test( str );
	  }
	  return 0;
      case HASH_cmGetString:
	  test( (char*)getStringBuf, " getString() " );
	  return 0;
/*
      case HASH_cmPassword:
	  {
	  char *str = getPassword( "Password:", 16, " Title ", 0, ' ' );
	  if ( str )
	     test( str );
	  }
	  return 0;
      case HASH_cmGetPassword:
	  test( (char*)getPasswordBuf, " getPassword() " );
	  return 0;
*/
      case HASH_cmMenuVisible:
	  if ( menuMgr )
	   {
	     menuMgr->Visible = !menuMgr->Visible;
	     menuMgr->init();
	   }
	  redraw();
	  return 0;
      case HASH_cmStatusVisible:
	  if ( statusLine )
	    statusLine->visible = !statusLine->visible;
	  redraw();
	  return 0;
      case HASH_cmBorderVisible:
	  borderVisible = !borderVisible;
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
   coll->insert( new menuItem( new Lang("~File dialog",	"~Файлы"),		0, 0, HASH_cmOpen ) );
   coll->insert( new menuItem( new Lang("F~ile dialog as window","^^^^^ как окно"),0, 0, HASH_cmNew ) );
   coll->insert( new menuItem( new Lang("~Blank",	"~Бланк"),		0, 0, HASH_cmBlankModal ) );
   coll->insert( new menuItem( new Lang("B~lank as window","Б~ланк как окно"),	0, 0, HASH_cmBlankWindow ) );
   coll->insert( new menuItem( new Lang("Table browser"),	0, 0, HASH_cmTbrowser ) );
   coll->insert( new menuItem( new Lang("Table browser(insert)"),	0, 0, HASH_cmBrowDialog ) );
   coll->insert( new menuItem( new Lang("~Modal window","~Модальное окно"),	0, 0, HASH_cmModalWin ) );
   coll->insert( new menuItem( new Lang("~Listbox(insert)","~ListBox как окно"),	0, 0, HASH_cmListBox ) );
   coll->insert( new menuItem( new Lang("~Terminal",	"~Терминал"),		0, 0, HASH_cmTerminal ) );
   coll->insert( new menuItem( new Lang("~Terminal(with command)","Терминал с ~командой"),	0, 0, HASH_cmGetTermCommand ) );
   coll->insert( new menuItem( new Lang("Get~String box","~Ввод строки"),	0, 0, HASH_cmString ) );
   coll->insert( new menuItem( new Lang("Get~Password box","Ввод ~пароля"),	0, 0, HASH_cmPassword ) );
   coll->insert( new menuItem( new Lang("~Keymap (global)","~Клавиатура(общ)"),	0, 0, HASH_cmKeymap ) );
   coll->insert( new menuItem( new Lang("K~eymap (window)","~Клавиатура(окно)"),0, 0, HASH_cmKeymapWindow ) );
   Menu *sub = new Menu( coll, Point(1,0) );
   global->insert( new menuItem( new Lang("~Open","~Открыть"),	0, 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( "~Next",		"Next window",	0, HASH_cmNextWin ) );
   coll->insert( new menuItem( "~Prev",		"Previous window",	0, HASH_cmPrevWin ) );
   coll->insert( new menuItem( "~First",	"First window",	0, HASH_cmFirst ) );
   coll->insert( new menuItem( "~Last",		"Last window",	0, HASH_cmLast ) );
   coll->insert( new menuItem( "~Size",		"Window size",	0, HASH_cmSize ) );
   coll->insert( new menuItem( "~Move",		"Window position",	0, HASH_cmPosition ) );
   coll->insert( new menuItem( "~Zoom",		"Window zoom",	0, HASH_cmZoom ) );
   coll->insert( new menuItem( "~Close",	"Close window",	0, HASH_cmCloseWin ) );
   coll->insert( new menuItem( "~List",		"Windows list",	0, HASH_cmChoose ) );
   coll->insert( new menuItem( "Ma~x size",	"Set maximum size for current window",	0, HASH_cmMaxSize ) );
   sub = new Menu( coll, Point(1,6), Point(1,9) );
   global->insert( new menuItem( "W~indow", "Window control", 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( "Menu visible",		"Menu visible",	0, HASH_cmMenuVisible ) );
   coll->insert( new menuItem( "Status visible",	"Status visible",	0, HASH_cmStatusVisible ) );
   coll->insert( new menuItem( "Border visible",	"Frame visible",	0, HASH_cmBorderVisible ) );
   sub = new Menu( coll, Point(1,14), Point(1,17) );
   global->insert( new menuItem( "Scree~n", "menu, status, frames", 0, sub ) );

   global->insert( new menuItem( "Exit", " This is ~exit~", 0, HASH_cmQuit ) );

   Smenu = menu = new Menu( global, Point(0,0), new Lang("SYS"), 0, 1, 0 );
   menu->fill( &keyHolder );
   sharedMenu.insert( menu );
   if ( menuMgr )
     menuMgr->select( menu );
}
//---------------------------------------------------------------------

static KeyMessage km( HASH_cmResizeScreen );
static struct sigaction act;

void resize_windows( int sig )
{
   appl->messToQueue( &km );
}

int main( int argc, char *argv[] )
{
   act.sa_handler = resize_windows;
   sigaction( SIGWINCH, &act,0 );
#if defined(FreeBSD)
   Screen::initScreen( BASE_TERMCAP );
#else
   Screen::initScreen( BASE_TERMINFO );
#endif

   StartPath( argv[0] );
   strcpy( filebox::filePath, StartPath( NULL ) );

   strcpy( filebox::fileMask, "*" );

   Appl *a = new Appl;
   TaskHolder k( a );
   TaskHolder key( new InputKeyTask( k.task() ) );
   a->makeMenu();
   km.setReceiver( ::appl->getNo() );
   km.setSender( ::appl->getNo() );
   Task::run();
   TagProcessor tp(TAG_PROCESSOR_LEN);
   HTMLTagProcessor htp;
   CommentTagProcessor ctp;

   delete a;
   sharedMenu.freeAll();
   Term::destroy();

   exit(0);
}
