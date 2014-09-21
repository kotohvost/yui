/*
	$Id: program.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <term.h>
#include <sys/ioctl.h>

#if defined(SOLARIS_SPARC) || defined(SOLARIS_X86) || defined(SINIX)
#include <termios.h>
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "program.h"
#include "menu.h"
#include "status.h"
#include "winlist.h"
#include "i_lines.h"
#include "modal.h"
#include "keybox.h"

long BackMessage::type() { return HASH_Back; }
long IdentMessage::type() { return HASH_Identify; }
long TimerMessage::type() { return HASH_Timer; }

long programKeyMap[] = {
	kbEsc,		HASH_cmMenu,
//	FUNC1(kbEsc),	HASH_cmMenu,
//	FUNC2(kbEsc),	HASH_cmMenu,
//	FUNC12(kbEsc),	HASH_cmMenu,
	kbF1,		HASH_cmHelpContext,
	FUNC1(kbF1),	HASH_cmHelpPrev,
	FUNC2(kbF1),	HASH_cmHelpIndex,
	FUNC1(kbCtrlC),	HASH_cmCloseWin,
	kbCtrlC,	HASH_cmCancel,
	FUNC1(kbCtrlX),	HASH_cmQuit,
	FUNC1('*'),	HASH_cmChoose,
	FUNC1('0'),	HASH_cmChoose,
	FUNC1('1'),	HASH_cmSelect1,
	FUNC1('2'),	HASH_cmSelect2,
	FUNC1('3'),	HASH_cmSelect3,
	FUNC1('4'),	HASH_cmSelect4,
	FUNC1('5'),	HASH_cmSelect5,
	FUNC1('6'),	HASH_cmSelect6,
	FUNC1('7'),	HASH_cmSelect7,
	FUNC1('8'),	HASH_cmSelect8,
	FUNC1('9'),	HASH_cmSelect9,
	kbCtrlE,	HASH_cmNext,
	kbCtrlW,	HASH_cmPrev,
	FUNC1(kbCtrlE),	HASH_cmNextWin,
	FUNC1(kbCtrlW),	HASH_cmPrevWin,
	FUNC1(kbCtrlD),	HASH_cmSize,
	FUNC1(kbCtrlA),	HASH_cmPosition,
	FUNC1(kbCtrlR),	HASH_cmRefresh,
	FUNC1(kbCtrlV),	HASH_cmZoom,
	0
};

CommandDescr programCommandDescr[] = {
	{ HASH_cmRefresh,	new Lang("Refresh screen", "Обновить экран") },
	{ HASH_cmQuit,		new Lang("Quit","Выход") },
	{ HASH_cmMenu,		new Lang("Menu","Меню") },
	{ HASH_cmHelpContext,	new Lang("Help on context","Помощь по контексту") },
	{ HASH_cmHelpPrev,	new Lang("Previous help","Предыдущая помощь") },
	{ HASH_cmHelpIndex,	new Lang("Help index","Содержание помощи") },
	{ HASH_cmChoose,	new Lang("Windows list","Список окон")},
	{ HASH_cmNext,		new Lang("Next window","Следующее окно") },
	{ HASH_cmPrev,		new Lang("Previous window","Предыдущее окно") },
	{ HASH_cmCloseWin,	new Lang("Close window","Закрыть окно") },
	{ HASH_cmSize,		new Lang("New size for window","Изменить размер окна") },
	{ HASH_cmPosition,	new Lang("New position for window","Переместить окно") },
	{ HASH_cmZoom,		new Lang("Zoom for window","Масштабирование окна") },
	{ 0, 0 }
};

Keymap ProgramKeyMap( programKeyMap, new Lang( "Base commands", "Базовые команды" ) );

unsigned char monoProgram[1] =
{
	FG_HI_WHITE | BG_BLACK		// background
};

unsigned char colorProgram[1] =
{
	FG_HI_WHITE | BG_BLUE		// background
};

unsigned char laptopProgram[1] =
{
	FG_HI_WHITE | BG_BLACK		// background
};

Program *appl=0;
int colorsType = COLOR;
char *wordDelim = " .,\t!\"#$%&'()*+-/:;<=>?@[\\]^`{|}~\n\r";

Collection Program::winStack(5,5);

#ifndef DJGPP
int backgroundSymbol=32;
#else
int backgroundSymbol=176;
#endif

long Timer2::main()
{
   TimerMessage *twm=0;
   while( 1 )
    {
      bsleep( msec );
      if ( closed || !reciever || !(twm=new TimerMessage(command,winPid)) )
	 continue;
      sendMessage( reciever, twm );
    }
  return 0;
}

/*--------------------------- class Program ----------------------------*/

Program::Program() : Collection(20,10), Task( "Program", 4096*8 ),
		current(-1), newCurrent(-1), flagDrawStatus(1),
		started(0), menuMgr(0),
		statusLine(0), menu(0), topWindow(0),
		winLst(0), keyBox(0), quit_empty(0)
{
   menuMgr = new MenuManager;
   if ( (statusLine = new StatusLine) )
      statusLine->init();
   keyHolder.add( &ProgramKeyMap, programCommandDescr );
   setColors( colorsType );
   status.put( "~^J^X~-exit  ~Esc~-menu", 0 );
   status.put( "~^J^X~-выход  ~Esc~-меню", 1 );
   Screen::currentMap = Screen::ScreenMap;
   ::appl = this;
}

Program::~Program()
{
   if ( menuMgr )
      delete menuMgr;
   if ( statusLine )
      delete statusLine;
}

void Program::resizeScreen()
{
  struct winsize ws;
  if ( ioctl( 0, TIOCGWINSZ, &ws) == -1 )
     return;

  Rect old = scrRect();
  freeMemScr();

  if ( ws.ws_row > 0 )
    Screen::Lines = Term::rows = ws.ws_row;
  if ( ws.ws_col > 0 )
    Screen::Columns = Term::cols = ws.ws_col;

  Screen::mapLineLen = Screen::Columns/8 + ((Screen::Columns%8) ? 1 : 0 );
  Screen::setClipRegion( Rect( 0, 0, Screen::Lines-1, Screen::Columns-1) );

  allocMemScr();

  if ( menuMgr )
   {
     menuMgr->reallocScrMap();
     Rect mmgrRect( 0, 0, 0, Screen::Columns-1 );
     menuMgr->setRect( mmgrRect );
     menuMgr->resizeScreen( old );
   }
  if ( statusLine )
   {
     statusLine->reallocScrMap();
     Rect statusRect( Screen::Lines-1, 0, Screen::Lines-1, Screen::Columns-1 );
     statusLine->setRect( statusRect );
   }

  ExecWindows *ex = 0;
  int i, j;
  for( i=0; i < winStack.getCount(); i++ )
   {
     ex = &((winExecutor*)winStack.at(i))->wins;
     for( j = ex->getCount()-1; j>=0; j-- )
      {
	Window *w = (Window*)ex->at(j);
	w->reallocScrMap();
	w->resizeScreen( old );
      }
   }
  for( i=0; i<count; i++ )
   {
     ex = &((winExecutor*)items[i])->wins;
     for( j = ex->getCount()-1; j>=0; j-- )
      {
	Window *w = (Window*)ex->at(j);
	w->reallocScrMap();
	w->resizeScreen( old );
      }
   }
}

Window *Program::getWin( int index )
{
   if ( index < 0 || index >= count ) return 0;
   winExecutor *we = (winExecutor*)items[index];
   int c = we->wins.getCount();
//   return (Window*)(c > 0 ? we->wins.at(c-1) : 0);
   return (Window*)(c > 0 ? we->wins.at(0) : 0);
}

void Program::setActive( int newcur )
{
   for( int i=0; i < count; i++ )
    {
       winExecutor *we = (winExecutor*)items[i];
       we->index = i;
       Window *w = (Window*)we->wins.at(0);
       if ( i != newcur )
	 { w->active=0; continue; }
       w->active=1;
       if ( menuMgr && winStack.getCount() <= 0 /*&& started*/ )
	 {
	   if ( current>=0 )
	    {
	      Collection *wins = &((winExecutor*)items[current])->wins;
	      for( int j=wins->getCount()-1; j>=0; j-- )
		 menuMgr->remove( ((Window*)wins->at(j))->winMenu );
	    }
	   menuMgr->select( w->winMenu );
	 }
       w->init();
       current = newcur;
    }
}

void Program::nextWin()
{
   if ( count <= 0 || !started )
      return;
   int i = current+1;
   while( i != current )
    {
      if ( i >= count )
	 i=0;
      if ( getWin(i)->selectable )
	 break;
      i++;
    }
   nselect( i );
   redraw();
}

void Program::prevWin()
{
   if ( count <= 0 || !started )
      return;
   int i=current-1;
   while( i != current )
    {
      if ( i< 0 )
	 i=count-1;
      if ( getWin(i)->selectable )
	 break;
      i--;
    }
   nselect( i );
   redraw();
}

long Program::main()
{
   restStatus();
   redraw();
   long key;
   int flagBack;
   Message *msg;
   KeyMessage *km;
   void *ptr;
   winExecutor *we=0;
   while( 1 )
    {
       msg = getMessage();
       if ( !msg )
	  continue;
       key=0; flagBack=0; ptr=0;
       switch( msg->type() )
	 {
	   case HASH_Back:
	       ptr = ((BackMessage*)msg)->ptr;
	       flagBack=1;
	   case HASH_Key:
	       ptr = ((KeyMessage*)msg)->ptr;
	       break;
	   case HASH_Ptr:
	       ptr = ((PtrMessage*)msg)->data;
	       goto ptr_message;
	   case HASH_Timer:
	       flagBack=1;
	       break;
	   default:
	       respond(msg);
	       continue;
	 }

       km = (KeyMessage*)msg;
       if ( menuMgr && !menuMgr->active && (menuMgr->hotKey=menuMgr->isHotKey(km->data)) )
	   key = HASH_cmMenu;
       else
	   key = keyHolder.translate( km->data );

       we = (winExecutor*)(winStack.getCount() > 0 ? winStack.at(0) : 0);
       if ( !we && current >= 0 && current < count )
	  we = (winExecutor*)items[current];

       if ( winStack.getCount() > 0 )
	{
	  if ( menuMgr && !menuMgr->active )
	     menuMgr->canSwitch = 0;
	}
       else
	{
	  if ( menuMgr )
	     menuMgr->canSwitch = 1;
	  switch( key )
	   {
	      case HASH_cmSelect1:
		   select( 1 ); redraw(); key=0; break;
	      case HASH_cmSelect2:
		   select( 2 ); redraw(); key=0; break;
	      case HASH_cmSelect3:
		   select( 3 ); redraw(); key=0; break;
	      case HASH_cmSelect4:
		   select( 4 ); redraw(); key=0; break;
	      case HASH_cmSelect5:
		   select( 5 ); redraw(); key=0; break;
	      case HASH_cmSelect6:
		   select( 6 ); redraw(); key=0; break;
	      case HASH_cmSelect7:
		   select( 7 ); redraw(); key=0; break;
	      case HASH_cmSelect8:
		   select( 8 ); redraw(); key=0; break;
	      case HASH_cmSelect9:
		   select( 9 ); redraw(); key=0; break;
	      case HASH_cmNext:
		   if ( we && we->firstHandle() )
		      break;
	      case HASH_cmNextWin:
		   nextWin();
		   key=0; break;
	      case HASH_cmPrev:
		   if ( we && we->firstHandle() )
		      break;
	      case HASH_cmPrevWin:
		   prevWin();
		   key=0; break;
	      case HASH_cmFirst:
		   nselect( 0 ); redraw();
		   key=0; break;
	      case HASH_cmLast:
		   nselect( count-1 ); redraw();
		   key=0; break;
	      case HASH_cmCascade:
		   cascadeWindows();
		   key=0; break;
	      case HASH_cmTile:
		   tileWindows();
		   key=0; break;
	      case HASH_cmChoose:
		   if ( !winLst )
		    {
		      winLst = new winList( &newCurrent );
		      winLst->retCommand = HASH_cmNewCurrent;
		      insertExecWindow( winLst );
		    }
		   key=0; break;
	      case HASH_cmNewCurrent:
		   nselect( newCurrent );
		   redraw();
		   key=0; break;
	      case HASH_cmZoom:
		   if ( count > 0 )
		     {
		       getWin(current)->zoom();
		       redraw();
		     }
		   key=0; break;
	      case HASH_cmMaxSize:
		   if ( count > 0 )
		     {
		       getWin(current)->zoom( 1 );
		       redraw();
		     }
		   key=0; break;
	      case HASH_cmSize:
		   newSize();
		   key=0; break;
	   }
	}

       if ( keyBox && keyBox->flagNewKey && key && !flagBack )
	  goto Forward;

       switch( key )
	{
	    case HASH_cmMenu:
		 if ( !menuMgr )
		   { key=0; break; }
		 if ( menuMgr->active || winStack.getCount() > 0 &&
		      ((Window*)(((winExecutor*)winStack.at(0))->wins.at(0)))->winMenu == 0 )
		    break;
		 if ( count > 0 )
		   {
		     Window *w = getWin(current);
		     if ( w && !w->winMenu && w->Executor->wins.getCount() > 1 )
			break;
		   }
		 insertExecWindow( menuMgr );
		 key=0; break;
	    case HASH_cmCloseWin:
		 if ( we )
		   sendMessage( we, new KeyMessage( key ) );
		 key=0; break;
	    case HASH_cmResizeScreen:
		 Screen::Clear( FG_WHITE | BG_BLACK );
		 resizeScreen();
		 redraw();
		 key=0; break;
	    case HASH_cmRedraw:
		 redraw();
		 key=0; break;
	    case HASH_cmRefresh:
		 Screen::Clear( FG_WHITE | BG_BLACK );
		 redraw();
		 key=0; break;
	    case HASH_cmPosition:
		 newPosition();
		 key=0; break;
	    case HASH_cmKill:
		 if ( winStack.getCount() <= 0 && count <= 0 )
		   { shutDown(); return 1; }
		 break;
	    case HASH_cmKeymap:
		 if ( !keyBox )
		  {
		    keyBox = new KeyBox( &keyHolder, menu );
		    insertExecWindow( keyBox );
		  }
		 key=0; break;
	    case HASH_cmKeymapWindow:
		 if ( !keyBox )
		  {
		   Window *w = winStack.getCount() > 0 ? (Window*)((winExecutor*)winStack.at(0))->wins.at(0) : 0;
		   if ( !w && count > 0 )
		      w = getWin(current);
		   if ( w )
		     {
		       keyBox = new KeyBox( &w->keyHolder, w->winMenu );
		       insertExecWindow( keyBox );
		     }
		  }
		 key=0; break;
	}

       if ( key && menuMgr && !menuMgr->active && (flagBack || !we || !we->firstHandle()) )
	{
ptr_message:
	  void *_ptr = ptr;
	  key = handleKey( key, _ptr );
	  if ( _ptr && _ptr != ptr )
	     ((KeyMessage*)msg)->ptr = _ptr;
	}

       switch( key )
	{
	    case HASH_cmQuit:
		 saveStatus();
		 if ( closeAll() )
		   { shutDown(); return 1; }
		 key=0; break;
	    case HASH_cmHelpContext:
	    case HASH_cmHelpPrev:
	    case HASH_cmHelpOnHelp:
	    case HASH_cmHelpIndex:
		 runHelp( key );
		 key=0; break;
	}

       if ( !key || flagBack )
	  { respond(msg); continue; }
Forward:
       if ( we )
	   forward( we, msg );
       else
	   respond( msg );
    }
   return 0;
}

void Program::drawBackground()
{
   Screen::Clear( Rect( 0, 0, Screen::Lines-1, Screen::Columns-1 ),
		/*backgroundAttr*/ clr[0], backgroundSymbol );
}

void Program::drawWindow( Window *w, int shadow )
{
   if ( !started || !w->canDraw( 0 ) ) return;
   char *c, *C, *sh, *end_sh;
   w->setScrMap();
   int scrBott=Screen::Lines-1;
   int y, Y=max( 0, w->rect.a.y ), bott=min( w->rect.b.y, scrBott );
   for( y=Y; y<=bott; y++ )
     {
       c=w->scr.map[y]; C=Screen::ScreenMap[y];
       for( int x=0; x<Screen::mapLineLen; x++, c++, C++ )
	  *c &= *C;
     }

   w->programRedraw = 0;
   w->draw( 1 );

   drawContinue=0;
   for( y=Y; y<=bott; y++ )
    {
      c=w->scr.map[y], C=Screen::ScreenMap[y], sh=Screen::shadowMap[y];
      end_sh = sh + Screen::Columns - 1;
      for( int x=0; x<Screen::mapLineLen; x++, c++, C++ )
       {
	 if ( *C &= ~*c )
	    drawContinue=1;
       //------------ обнуление на месте окна в map'e теней --------
	 if ( !*c )
	    { sh+=8; continue; }
	 for( int cycl=8; cycl-- && sh < end_sh; sh++ )
	    if ( (*sh & 1) && (*c>>cycl) & 1 )
	       *sh = 0;
       //-----------------------------------------------------------
       }
    }
   if ( shadow )
    {
     //--------------------- обозначение тени ---------------------
     int rbx = w->rect.b.x+1;
     if ( rbx < Screen::Columns )		    // справа от окна
       for( y=max( w->rect.a.y+1, 1 ), bott=min( bott+1, scrBott ); y<=bott; y++, rbx-- )
	{
	  sh=Screen::shadowMap[y];
	  if ( sh[rbx] & 1 ) sh[rbx] = 2;
	  if ( ++rbx<Screen::Columns && sh[rbx] & 1 ) sh[rbx] = 2;
	}
     if ( y > w->rect.b.y && w->rect.b.y < Screen::Lines-1 ) // снизу от окна
      {
	y=w->rect.b.y+1;
	int x=max( w->rect.a.x+2, 0 );
	sh = Screen::shadowMap[y] + x;
	for( int X=min( w->rect.a.x+w->size.x+2, Screen::Columns); x<X; x++, sh++ )
	  if ( *sh & 1 ) *sh = 2;
      }
     //------------------------------------------------------------
    }
   if ( !drawContinue )
     for( y=0; y<scrBott; y++ )
       {
	 if ( y==Y ) { y=bott; continue; }
	 C=Screen::ScreenMap[y];
	 for( int x=0; x<Screen::mapLineLen; x++ )
	   if ( *C++ ) { y=scrBott; drawContinue=1; break; }
       }
}

void Program::drawExecWindows( Collection *wins )
{
   if ( !started ) return;
   for( int i=0; i<wins->getCount(); i++ )
     {
       Window *w=(Window*)wins->at(i);
       if ( !topWindow )
	   topWindow = w;
       if ( drawContinue )
	   drawWindow( w );
       else
	   w->nonDraw();
     }
}

void Program::drawStatus( int fromRedraw )
{
   if ( !started )
     {
       Window::MoveCursor();
       return;
     }
   if ( statusLine && statusLine->visible )
     {
       if ( !topWindow )
	 {
	   statusLine->draw( status.get() );
	   Screen::hideCursor();
	 }
       else if ( flagDrawStatus )
	 {
	   topWindow->drawStatus();
	   Window::MoveCursor();
	 }
     }
/*
   else if ( !fromRedraw )
       redraw();
*/
   else
       Window::MoveCursor();
}

Window *Program::topWindowPtr()
{
   if ( !started ) return 0;
   topWindow = 0;
   if ( winStack.getCount() > 0 )
    {
       winExecutor *we = (winExecutor*)winStack.at(0);
       int c = we->wins.getCount();
       topWindow = (Window*)(c > 0 ? we->wins.at(0) : 0);
    }
   if ( !topWindow )
      topWindow = getWin( current );
   return topWindow;
}

void Program::redraw( int sync )
{
   if ( !Screen::isOpen || !started )
      return;
   for( int y=0; y<Screen::Lines; y++ )
    {
      memset( Screen::ScreenMap[y], 0xff, Screen::mapLineLen );
      memset( Screen::shadowMap[y], 1, Screen::Columns );
    }
   if ( menuMgr && !menuMgr->active && menuMgr->visible )
      drawWindow( menuMgr, 0 );
   if ( statusLine && statusLine->visible )
      drawWindow( statusLine, 0 );
   drawContinue=1;
   topWindow = 0;
   int i;
   winExecutor *we = 0;
   for( i=0; i < winStack.getCount(); i++ )
     {
       we = (winExecutor*)winStack.at(i);
       drawExecWindows( &we->wins );
     }
   if ( current >= 0 && current < count )
     {
       for( i=current; i>=0; i-- )
	 {
	   we = (winExecutor*)items[i];
	   drawExecWindows( &we->wins );
	 }
       for( i = count-1; i > current; i-- )
	 {
	   we = (winExecutor*)items[i];
	   drawExecWindows( &we->wins );
	 }
       if ( !topWindow )
	 topWindow = getWin(current);
     }
   Screen::currentMap = Screen::ScreenMap;
   drawBackground();
   Screen::attrSet( Screen::shadowMap );

   drawStatus( 1 );

   if ( sync )
       Screen::sync();
}

int Program::select( Window *w )
{
   return select( w->winNo );
}

int Program::getIndexByNo( int winNo )
{
//   if ( !started ) return -1;
   for( int i=0; i<count; i++ )
    {
      Collection *coll = &((winExecutor*)items[i])->wins;
      for( int j=coll->getCount()-1; j >= 0; j-- )
	if ( winNo == ((Window*)coll->at(j))->winNo )
	   return i;
    }
   return -1;
}

int Program::getIndexByPid( long winPid )
{
//   if ( !started ) return -1;
   for( int i=0; i<count; i++ )
    {
      Collection *coll = &((winExecutor*)items[i])->wins;
      for( int j=coll->getCount()-1; j>=0; j-- )
	if ( winPid == ((Window*)coll->at(j))->winPid )
	   return i;
    }
   return -1;
}

int Program::select( int winNo )
{
//   if ( !started ) return -1;
   int i = getIndexByNo( winNo );
   if ( i >= 0 )
      setActive( i );
   return current;
}

int Program::nselect( int no )
{
//   if ( !started ) return -1;
   if ( no >=0 && no < count )
      setActive( no );
   return current;
}

void Program::setRect( Window *who, Rect newRect )
{
   if ( count < 1 || !started )
      return;
   if ( indexOf( (void*)who ) < 0 )
      return;
   who->setRect( newRect );
   redraw();
}

void Program::insertExecWindow( Window *w )
{
   winExecutor *we = new winExecutor( w, EXEC, w->stk_len );
   Task::addref( we );
   we->start();
   winStack.atInsert( 0, we );
   w->programRedraw = 1;
}

ccIndex Program::insert( Window *w )
{
   int freenum = count+1;
   for( int num=1; num <= count; num++ )
    {
      int i=0;
      for( ; i < count; i++ )
       {
	 winExecutor *we = (winExecutor*)items[i];
	 int c = we->wins.getCount();
	 Window *_w = (Window*)(c > 0 ? we->wins.at(c-1) : 0);
	 if ( _w && _w->winNo == num )
	    break;
       }
      if ( i >= count )
       {
	 freenum=num;
	 break;
       }
    }
   w->winNo = freenum;
   winExecutor *we = new winExecutor( w, INSERTED, w->stk_len );
   Task::addref( we );
   we->start();
   we->index = Collection::insert( we );
   if ( winLst )
      sendMessage( winLst->Executor, new KeyMessage( HASH_cmInit ) );
   if ( current < 0 && count > 0 )
      current = we->index;
   w->programRedraw = 1;
   return we->index;
}

ccIndex Program::Insert( Window *w )
{
   int freenum = count+1;
   for( int num=1; num <= count; num++ )
     {
       int i=0;
       for( ; i < count; i++ )
	  if ( getWin(i)->winNo == num )
	     break;
	if ( i >= count )
	  {
	    freenum=num;
	    break;
	  }
     }
   w->winNo=freenum;
   int no=current+1;
   winExecutor *we = new winExecutor( w, INSERTED, w->stk_len );
   Task::addref( we );
   we->start();
   we->index = no;
   Collection::atInsert( no, we );
   if ( winLst )
      sendMessage( winLst->Executor, new KeyMessage( HASH_cmInit ) );

   if ( current < 0 && count > 0 )
      current = we->index;
   w->programRedraw = 1;
   return we->index;
}

int Program::closeWin( int pos, int Redraw )
{
   int ret=1;
   if ( pos >=0 && pos < count )
      ret = closeWin( (winExecutor*)items[pos], Redraw );
   if ( quit_empty && count <= 0 )
      sendMessage( this, new BackMessage( HASH_cmKill ) );
   return ret;
}

int Program::closeWin( winExecutor *we, int Redraw )
{
   we->selfClose = 0;
   static KeyMessage closeMessage( HASH_cmCloseWin );
   for( int i=0; we->wins.getCount() > 0; i++ )
      sendMessage( we, closeMessage );

   if ( we->wins.getCount() > 0 )
     {
       we->selfClose = 1;
       return 0;
     }

   int kill_flag = 0;
   if ( we->type != INSERTED )
       winStack.remove( we );
   else
    {
      remove( we );
      if ( we->index == current )
	current--;
      if ( current < 0 || current >= count )
	current = count-1;
      kill_flag = 1;
    }

   if ( !we->win->isType( HASH_Menu ) )
    {
      if ( menuMgr && winStack.getCount() > 0 )
	 menuMgr->select( ((Window*)(((winExecutor*)winStack.at(0))->wins.at(0)))->winMenu );
      else
	 setActive( current );
    }

   processClose( we );
   Task::delref( we );

   if ( winLst )
      sendMessage( winLst->Executor, new KeyMessage( HASH_cmInit ) );
   else if ( Redraw )
      redraw();

   if ( quit_empty && kill_flag && count <= 0 )
      sendMessage( this, new BackMessage( HASH_cmKill ) );

   return 1;
}

int Program::closeAll()
{
   winExecutor *we = winStack.getCount() > 0 ? (winExecutor*)winStack.at(0) : 0;
   if ( !we && count > 0 )
      we = (winExecutor*)items[current];
   if ( !we )
      return 1;
   sendMessage( we, new KeyMessage( HASH_cmKill ) );
   return 0;
}

void Program::cascadeWindows()
{
}

void Program::tileWindows()
{
}

void Program::newPosition()
{
   winExecutor *we = (winExecutor*)(winStack.getCount() > 0 ? winStack.at(0) : 0);
   if ( !we )
     {
       if ( current < 0 && current >= count )
	  return;
       we = (winExecutor*)items[current];
     }

   Window *w = (Window*)we->wins.at(0);
   w->drawFrameOnly = 1;
   Rect r=w->rect, unz=w->unzoomed;
   flagDrawStatus = 0;
   for( int done=0; !done; )
     {
       w->setOrigScrMap();
       w->setScrMap();
       redraw( 0 );
       if ( statusLine )
	 statusLine->draw( lang("~Arrow keys~-move window  ~Enter~-set  ~Esc~-cancel",
			"~Стрелки~-перемещение окна  ~Enter~-установка  ~Esc~-выход"), 1 );
       Screen::sync();

       Message *msg = getMessage();
       if ( msg->type() != HASH_Key )
	  { respond(msg); continue; }

       KeyMessage *km = (KeyMessage*)msg;
       respond(msg);

       switch( km->data )
	 {
	   case kbEnter:
	      done=1;
	      break;
	   case kbEsc:
	   case kbCtrlC:
	      done=1;
	      w->rect=r;
	      w->unzoomed=unz;
	      break;
	   case kbUp:
	      if ( w->rect.b.y<2 )
		 continue;
	      w->rect.move(0,-1);
	      break;
	   case kbDown:
	      if ( w->rect.a.y>=Screen::Lines-2 )
		 continue;
	      w->rect.move(0,1);
	      break;
	   case kbLeft:
	      if ( w->rect.b.x<1 )
		 continue;
	      w->rect.move(-1,0);
	      break;
	   case kbRight:
	      if ( w->rect.a.x>=Screen::Columns-1 )
		 continue;
	      w->rect.move(1,0);
	      break;
	   case kbHome:
	      if ( w->rect.b.y<2 || w->rect.b.x<2 )
		 continue;
	      w->rect.move(-2,-1);
	      break;
	   case kbPgUp:
	      if ( w->rect.b.y<2 || w->rect.a.x>=Screen::Columns-2 )
		 continue;
	      w->rect.move(2,-1);
	      break;
	   case kbEnd:
	      if ( w->rect.a.y>=Screen::Lines-2 || w->rect.b.x<2 )
		 continue;
	      w->rect.move(-2,1);
	      break;
	   case kbPgDn:
	      if ( w->rect.a.y>=Screen::Lines-2 || w->rect.a.x>=Screen::Columns-2 )
		 continue;
	      w->rect.move(2,1);
	      break;
	 }
     }
   flagDrawStatus = 1;
   w->drawFrameOnly = 0;
/*
   w->init();
   void *ptr=0;
   w->handleKey( HASH_cmPosition, ptr );
   redraw();
*/
   Task::sendMessage( w->Executor, new KeyMessage( HASH_cmSetPosition ) );
}

void Program::newSize()
{
   if ( current<0 || count<=0 )
      return;
   Window *w = getWin(current);
   w->drawFrameOnly = 1;
   Rect r=w->rect, unz=w->unzoomed;
   flagDrawStatus=0;
   for( int done=0; !done; )
     {
       w->setOrigScrMap();
       w->setScrMap();
       redraw( 0 );
       if ( statusLine )
	 statusLine->draw( lang("~Arrow keys~-change size  ~Enter~-select  ~Esc~-cancel",
				"~Стрелки~-изменение размера  ~Enter~-установка  ~Esc~-выход"), 1 );
       Screen::sync();

       Message *msg = getMessage();
       if ( msg->type() != HASH_Key )
	  { respond(msg); continue; }

       KeyMessage *km = (KeyMessage*)msg;
       respond(msg);

       switch( km->data )
	 {
	   case kbEnter:
	      done=1;
	      w->unzoomed = r;
	      break;
	   case kbEsc:
	   case kbCtrlC:
	      done=1;
	      w->rect=r;
	      w->unzoomed=unz;
	      break;
	   case kbUp:
	      if ( w->rect.b.y > w->rect.a.y )
		 w->rect.b.y--;
	      break;
	   case kbDown:
	      w->rect.b.y++;
	      break;
	   case kbLeft:
	      if ( w->rect.b.x > w->rect.a.x )
		 w->rect.b.x--;
	      break;
	   case kbRight:
	      w->rect.b.x++;
	      break;
	    case kbPgDn:
	      if ( w->size.x+1 >= w->maxSize.x )
		 continue;
	      if ( w->size.y >= w->maxSize.y )
		 continue;
	      w->rect.b.x+=2;
	      w->rect.b.y++;
	      break;
	    case kbPgUp:
	      if ( w->size.x+1 >= w->maxSize.x )
		 continue;
	      if ( w->size.y <= w->minSize.y )
		 continue;
	      w->rect.b.x+=2;
	      w->rect.b.y--;
	      break;
	    case kbEnd:
	      if ( w->size.x-1 <= w->minSize.x )
		 continue;
	      if ( w->size.y >= w->maxSize.y )
		 continue;
	      w->rect.b.x-=2;
	      w->rect.b.y++;
	      break;
	    case kbHome:
	      if ( w->size.y <= w->minSize.y )
		 continue;
	      if ( w->size.x-1 <= w->minSize.x )
		 continue;
	      w->rect.b.y--;
	      w->rect.b.x-=2;
	 }
     }
   flagDrawStatus = 1;
   w->drawFrameOnly = 0;
/*
   w->init();
   void *ptr=0;
   w->handleKey( HASH_cmSize, ptr );
   redraw();
*/
   Task::sendMessage( w->Executor, new KeyMessage( HASH_cmSetSize ) );
}

long Program::test( /*Task *task,*/ Collection *Text, char *Title,
	int cm1, char *s1, int cm2, char *s2, int cm3, char *s3,
	int cm4, char *s4, int cm5, char *s5 )
{
   modal *m = new modal( ALIGN_CENTER, Text, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 );
//   if ( task )
//      return exec( task, m );
   insertExecWindow( m );
   return 0;
}

long Program::test( /*Task *task,*/ char *str, char *Title,
	int cm1, char *s1, int cm2, char *s2, int cm3, char *s3,
	int cm4, char *s4, int cm5, char *s5 )
{
   modal *m = new modal( ALIGN_CENTER, str, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 );
//   if ( task )
//      return exec( task, m );
   insertExecWindow( m );
   return 0;
}

long Program::exec( Task *task, Window *w )
{
   winExecutor *we = insertToStack( w );
   we->selfClose = 0;
   long ret = task->spawn( we );
   winStack.remove( we );
   return ret;
}

winExecutor *Program::insertToStack( Window *w )
{
   winExecutor *we = new winExecutor( w, EXEC, w->stk_len );
   winStack.atInsert( 0, we );
   return we;
}

char getStringBuf[1024];

// InputType : INPUT, INT, U_INT, FLOAT, U_FLOAT, DATE

Dialog *getBoxForString( char *prompt, int scrlen,
			Collection *hist, char *initStr, char *outstr,
			char *title, char *help, int fillChar,
			char *status, InputType itype )
{
   int lenP = prompt ? min( 80, strlen( prompt ) ) : 0;
   int pos = lenP ? lenP+2 : 0;
   int len = pos + scrlen;
   char *Str = initStr ? initStr : (hist && hist->getCount() > 0 ? (char*)hist->at(0) : 0);
   char *outStr = outstr ? outstr : getStringBuf;
   inputLine *il=0;
   int negative = 1;
   Point p( 0, pos );
   switch( itype )
    {
      case INPUT:
	 il = new inputLine( p, Str, outStr, scrlen, 255, 0, hist, fillChar );
	 break;
      case U_INT:
	 negative = 0;
      case INT:
	 il = new IntegerInputLine( p, Str, outStr, scrlen, 255, negative, hist, fillChar );
	 break;
      case U_FLOAT:
	 negative = 0;
      case FLOAT:
	 il = new FloatInputLine( p, Str, outStr, scrlen, 255, negative, hist, fillChar );
	 break;
      case DATE:
	 il = new DateInputLine( p, Str, outStr, hist, fillChar );
    }
   if ( !il )
      return 0;
   Rect r( Screen::center( 3, len + 2 + (lenP>0?1:0) + (hist?1:0) ) );
   Dialog *d = new Dialog( r, new Lang(title), 0, new Lang(status) );
   if ( d )
    {
      if ( lenP > 0 )
	 d->put( 0, 1, prompt );
      d->insert( il );

      if ( help )
	 d->setHelpContext( help );
    }
   return d;
}

char *Program::getString( long command, /*Task *task,*/ char *prompt, int scrlen,
			Collection *hist, char *initStr, char *outstr,
			char *title, char *help, int fillChar, char *status,
			InputType itype )
{
   Dialog *d = getBoxForString( prompt, scrlen, hist, initStr,
		outstr, title, help, fillChar, status, itype );
   if ( !d )
      return 0;
   d->retCommand = command ? command : HASH_cmGetString;
   insertExecWindow( d );
   return 0;
}

//char getPasswordBuf[1024];

Dialog *getBoxForPass( char *prompt, int scrlen, char *title, char *out,
			char *help, int fillChar, char *status )
{
   int lenP = prompt ? min( 80, strlen( prompt ) ) : 0;
   int pos = lenP ? lenP+2 : 0;
   int len = pos + scrlen;
   Rect r( Screen::center( 3, len + 2 + (lenP > 0 ? 1 : 0) ) );
   inputLine *il = new inputLine( Point( 0, pos ), 0, out, scrlen, 255, 0, 0, fillChar, 1 );
   Dialog *d = new Dialog( r, new Lang(title), 0, new Lang(status) );
   if ( d )
    {
      d->insert( il );
      if ( lenP > 0 )
	 d->put( 0, 1, prompt );
      if ( help )
	 d->setHelpContext( help );
    }
   return d;
}

/*
char *Program::getPassword( char *prompt, int scrlen, char *title, char *help,
			int fillChar, char *status )
{
   Dialog *d = getBoxForPass( prompt, scrlen, title, help, fillChar, status );
   if ( d )
    {
      d->retCommand = HASH_cmGetPassword;
      insertExecWindow( d );
    }
   return 0;
}
*/

int Program::moveWin( int src, int dst )
{
   if ( src == dst || src < 0 || src >= count || dst < 0 || dst > count )
      return 0;
   winExecutor *executor = (winExecutor*)items[src];
   atRemove( src );
   if ( src < dst )
      dst--;
   atInsert( dst, executor );
   nselect( dst );
   return 1;
}

ccIndex Program::findWindow( long command, void *ptr )
{
   static IdentMessage imsg;
   imsg.data = command;
   imsg.ptr = ptr;
   Task *executor = 0;
   if ( sendAll(imsg) == 1 && (executor = findTask( imsg.getReceiver() )) )
      return indexOf( executor );
   return -1;
}

void Program::setColors( int type )
{
   if ( !Term::hasColors() )
      colorsType = type = MONO;
   switch( type )
    {
      case MONO:
	  clr = monoProgram;
	  break;
      case COLOR:
	  clr = colorProgram;
	  break;
      case LAPTOP:
	  clr = laptopProgram;
    }
   for( int i=0; i<count; i++ )
    {
      winExecutor *we = (winExecutor*)items[i];
      for( int j=0; j < we->wins.getCount(); j++ )
	 ((Window*)we->wins.at(j))->setColors( type );
    }
   if ( statusLine )
     statusLine->setColors( type );
}

/*------------------------- class winExecutor ----------------------*/

winExecutor::winExecutor( Window *w, unsigned Type, int stk_len ) :
		Task( "Window", stk_len ),
		we(0), win(w), resultKey(0), index(-1),
		selfClose(1), type(Type)
{
   if ( !win )
      return;
   win->isGetObj = 0;
   win->Executor = this;
   win->init();
   win->setColors( colorsType );
   wins.insert( win );
}

winExecutor::~winExecutor()
{
   int flag = 1;
   while( wins.getCount() > 0 )
    {
      Window *w = (Window*)wins.at(0);
      if ( w->flagDelete )
	 delete w;
      flag = 0;
    }
   if ( win && flag && win->flagDelete )
      delete win;
}

long winExecutor::main()
{
   if ( !win )
      return 0;
   resultKey = win->exec( &wins, this );
   if ( selfClose )
      appl->closeWin( this, 0 );

   if ( resultKey )
    {
      Message *msg=0;
      switch( resultKey )
       {
	 case HASH_cmKill:
	     msg = new KeyMessage( HASH_cmKill /*win->retCommand*/ );
	     break;
	 case HASH_cmEsc:
	 case HASH_cmClose:
	 case HASH_cmCancel:
	     break;
	 default:
/*
	     if ( win->isType( HASH_Menu ) )
		msg = new KeyMessage( win->retCommand );
	     else
		msg = new BackMessage( win->retCommand );
*/
	     msg = new KeyMessage( win->retCommand );
       }
      if ( msg )
	 sendMessage( appl, msg );
    }
   return resultKey;
}

int winExecutor::firstHandle()
{
   return wins.getCount() > 0 ? ((Window*)wins.at(0))->firstHandle : 0;
}

Task::MessPath winExecutor::handleMessage( Message *msg )
{
   switch( msg->type() )
    {
      case HASH_Identify:
	 {
	   IdentMessage *imsg = (IdentMessage*)msg;
	   if ( !((Window*)wins.at(0))->handleKey( imsg->data, imsg->ptr ) )
	      return PROCESSED;
	 }
    }
   return QUEUE;
}

Window *winExecutor::message( long command, void *ptr, long winPid )
{
  Window *w = 0;
  int wcount=wins.getCount();
  KeyMessage km( command, ptr );
  for( int i=0; i<wcount; i++ )
   {
     w = (Window*)wins.at(i);
     if ( winPid >=0 )
      {
	if ( winPid != w->winPid )
	   continue;
	if ( (wcount <= 1 || i==wcount-1) && Task::getCurrent() != this )
	   Task::sendMessage( this, km );
	else
	   w->handleKey( command, ptr );
	return w;
      }
     if ( !w->handleKey( command, ptr ) )
	return w;
   }
  return 0;
}
