/*
	$Id: window.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "program.h"
#include "modal.h"
#include "menu.h"
#include "status.h"
#include "hashcode.h"

int borderVisible = 1;
Collection clipBoard( 50, 50 );
long Window::WinPid=0;

extern Dialog *getBoxForString( char *prompt, int scrlen,
			Collection *hist, char *initStr, char *outstr,
			char *title, char *help, int fillChar,
			char *status, InputType itype );

extern Dialog *getBoxForPass( char *prompt, int scrlen, char *title,
			char *out, char *help, int fillChar, char *status );

int  Window::isType(long typ)
{
  return (typ==HASH_Window ? 1 : getobj::isType(typ));
}

long windowKeyMap[] = {
	kbEsc,		HASH_cmEsc,
	kbEnter,	HASH_cmOK,
	kbCtrlC,	HASH_cmCancel,
	FUNC1(kbCtrlC),	HASH_cmCancel,
	0
};

Keymap WindowKeyMap( windowKeyMap );

scrInfo::scrInfo() : map(0), orig(0), bord(0)
{
  lines = Screen::Lines;
  cols = Screen::Columns;
  maplen = Screen::mapLineLen;
  alloc_map();
}

scrInfo::~scrInfo()
{
  free_map();
}

void scrInfo::free_map()
{
  int i;
  if ( map ) {
     for( i=0; i < lines; i++ ) {
	 if ( map[i] )
	     ::free( map[i] );
     }
     ::free( map );
     map = 0;
  }
  if ( orig ) {
     for( i=0; i < lines; i++ ) {
	 if ( orig[i] )
	     ::free( orig[i] );
     }
     ::free( orig );
     orig = 0;
  }
  if ( bord ) {
     for( i=0; i < lines; i++ ) {
	 if ( bord[i] )
	     ::free( bord[i] );
     }
     ::free( bord );
     bord = 0;
  }
}

void scrInfo::alloc_map()
{
  if ( map || orig || bord )
     free_map();
  int i=0;
  if ( !(map = (char**)calloc( sizeof(char*), lines )) ||
       !(orig = (char**)calloc( sizeof(char*), lines )) ||
       !(bord = (char**)calloc( sizeof(char*), lines )) )
     goto err_mem;
  if ( maplen > 0 ) {
      for( ; i < lines; i++ ) {
	  if ( !(map[i] = (char*)calloc( sizeof(char), maplen )) ||
	       !(orig[i] = (char*)calloc( sizeof(char), maplen )) ||
	       !(bord[i] = (char*)calloc( sizeof(char), maplen )) )
	  goto err_mem;
      }
  }
  return;
err_mem:
  fprintf( stderr, "Cannot allocate memory\n" );
  exit(1);
}

Window::Window( Rect r, Lang *tit, Lang *id, Lang *st, int Box ) :
	getobj(r), Collection(100,100),
	minSize(3,3), oldDelta(-1,-1), topOffset(0), bottOffset(0),
	changed(1), current(-1), _flags(0),
	drawFrameOnly(0), helpContext(0),
	firstHandle(0), winNo(0),
#if defined(SOLARIS_SPARC) || defined(SOLARIS_X86)
	stk_len(8192),
#else
	stk_len(4096),
#endif
	retCommand(0), selectable(1), redrawAfterExec(1), box(Box),
	flagDelete(1), programRedraw(0), timerMessage(0),
	target_name(0)
{
   winPid = WinPid++;
   keyHolder.add( &WindowKeyMap );
   isGetObj=0;
   size = Point( rect.b - rect.a + Point(1,1) );
   unzoomed = Rect(-1,-1,-1,-1);
   // -------- name, ident and status line ----------
   title = tit ? tit : new Lang("");
   ident = id ? id : new Lang("");
   status = st ? st : new Lang("");
}

Window::~Window()
{
   if ( title ) delete title;
   if ( ident ) delete ident;
   if ( status ) delete status;
   if ( winMenu )
    {
      if ( appl->menuMgr )
	 appl->menuMgr->remove( winMenu );
      delete winMenu;
    }
   if ( appl->topWindow == this )
      appl->topWindow = 0;
   if ( helpContext )
      ::free( helpContext );
   if ( target_name )
      ::free( target_name );
   scr.free_map();
}

void Window::reallocScrMap()
{
  int flag = (Screen::currentMap == scr.map ? 1 : 0);
  scr.free_map();
  scr.lines = Screen::Lines;
  scr.cols = Screen::Columns;
  scr.maplen = Screen::mapLineLen;
  scr.alloc_map();
  if ( flag )
     Screen::currentMap = scr.map;
}

void Window::resizeScreen( Rect &old )
{
  if ( rect == old )
     rect = scrRect();
  init();
}

int Window::init( void *data )
{
   if ( unzoomed.a.y==-1 && unzoomed.a.x==-1 && unzoomed.b.y==-1 && unzoomed.b.x==-1 )
      unzoomed = isType( HASH_Menu ) ? rect : scrRect();
   setOrigScrMap();
   if ( cursor.y>=size.y-2 ) cursor.y=size.y-3;
   if ( cursor.x>=size.x-2 ) cursor.x=size.x-3;
   makeMenu();
   return getobj::init( data );
}

void Window::drawStatus()
{
   if ( appl->statusLine )
      appl->statusLine->draw( status->get() );
   MoveCursor();
}

int Window::scroll( int attr, int flagRedraw )
{
   topDraw = delta.y + topOffset;
   bottDraw= delta.y + size.y - bottOffset - (box ? 3 : 1);
   int aY = rect.a.y + (box ? 1 : 0);
   int bY = rect.b.y - (box ? 1 : 0);
   if ( owner )
    {
      aY -= owner->delta.y;
      bY -= owner->delta.y;
    }
   int dig = (appl->statusLine && appl->statusLine->visible) ? 1 : 0;
   if ( aY < 0 )
      topDraw -= aY;
   if ( bY >= scr.lines - dig )
      bottDraw -= bY - scr.lines + 1 + dig;

   if ( flagRedraw || !active || oldDelta.y < 0 || oldDelta.x != delta.x ||
	this != appl->topWindow  )
      return 1;                         // draw
   if ( rect.a.x > 5 || scr.cols - rect.b.x > 6 )
      return 1;
   if ( BLOCK_EXIST && (block.top>=topDraw && block.top<=bottDraw ||
	     block.bott>=topDraw && block.bott<=bottDraw) )
      return 1;

   long n = oldDelta.y - delta.y;
   if ( !n )                            // no scrolling
      return 0;
   Rect r = rect;
   if ( box )
      r.grow(-1,-1);
   if ( r.a.y < 1 ) r.a.y = 1;
   if ( r.b.y >= scr.lines ) r.b.y = scr.lines - 1;
   if ( abs(n) < (size.y-3)>>1 && Screen::scroll( r, n, attr << 8 ) )
      if ( n > 0 )
	  bottDraw = topDraw + n - 1;
      else
	  topDraw = bottDraw + n + 1;
   return 1;                            // draw
}

void Window::setOrigScrMap()
{
   size = Point( rect.b - rect.a );
   size.x++; size.y++;
   char *sc=0;
   for( int y=0; y < scr.lines; y++ )
    {
       memset( (sc=scr.orig[y]), 0, scr.maplen );
       if ( y >= rect.a.y && y <= rect.b.y )
	 for( int ind, x=max( 0, rect.a.x ); x <= rect.b.x && x < scr.cols; x++ )
	   if ( drawFrameOnly && y > rect.a.y && y < rect.b.y && x > rect.a.x && x < rect.b.x )
	       continue;
	   else
	    {
	       if ( (ind=x>>3) < 0 || ind > scr.maplen )
		 break;
	       sc[ind] |= 0x80 >> (x & 0x7);
	    }
    }
}

void Window::setScrMap()
{
   for( int y=0; y < scr.lines; y++ )
      memcpy( scr.map[y], scr.orig[y], scr.maplen );
}

void Window::nonDraw()
{
   int y = max( rect.a.y, (appl->menuMgr && appl->menuMgr->Visible) ? 1 : 0);
   int Y = min( rect.b.y+1, scr.lines );
   for( ; y < Y; y++ )
       memset( scr.map[y], 0, scr.maplen );
}

int Window::isVisual()
{
   if ( !Screen::isOpen )
      return 0;
   int y	= max( rect.a.y, 0 );
   int bott	= min( scr.lines-1, rect.b.y );
   int x, X	= max( rect.a.x, 0 ) >> 3;
   int right	= min( rect.b.x, scr.cols-1) >> 3;
   for( ; y <= bott; y++ )
    {
      for( x=X; x <= right; x++ )
	if ( scr.map[y][x] )
	  return 1;
    }
   return 0;
}

int Window::draw( int Redraw, int sy, int sx )
{
   if ( !Screen::isOpen )
      return 0;
   if ( programRedraw )
    {
      programRedraw=0;
      if ( !isGetObj )
       {
	 appl->redraw();
	 return 0;
       }
    }
   if ( !canDraw() )
      return 0;
   if ( this == appl->topWindow && appl->menuMgr )
      appl->menuMgr->draw();
   char **saveMap = Screen::currentMap;
   if ( !isGetObj )
     {
       if ( !isVisual() )
	  return 0;
       Screen::currentMap = scr.map;
     }
   if ( box || drawFrameOnly )
     {
       Rect r = Rect( rect.a.y + sy, rect.a.x + sx, rect.b.y + sy, rect.b.x + sx );
       Screen::box( r, (u_char*)winName(), COLOR_FRAME );
     }
   if ( isGetObj )
     {
       Screen::currentMap = saveMap;
       return 1;
     }

   if ( winNo > 0 && box )
      Screen::print( rect.a.y, rect.b.x - 4 - (winNo>9 ? 1:0), 0, "[%d]", winNo );

   Screen::currentMap = saveMap;

   int y=0, end=0;
   for( ; y < scr.lines; y++ )
      memcpy( scr.bord[y], scr.map[y], scr.maplen );
   if ( box )
     {
       if ( rect.a.y >= 0 && rect.a.y < scr.lines )
	  memset( scr.bord[rect.a.y], 0, scr.maplen );
       if ( rect.b.y >= 0 && rect.b.y < scr.lines )
	  memset( scr.bord[rect.b.y], 0, scr.maplen );

       int byte1 = -1, byte2 = -1;
       char and1=0, and2=0;
       if ( rect.a.x >= 0 && rect.a.x < scr.cols )
	 {
	   byte1 = rect.a.x>>3;
	   and1 = ~(0x80 >> (rect.a.x & 7));
	 }
       if ( rect.b.x >= 0 && rect.b.x < scr.cols )
	 {
	   byte2 = rect.b.x>>3;
	   and2 = ~(0x80 >> (rect.b.x & 7));
	 }
       for( y=max( rect.a.y + 1, 1 ), end=min( rect.b.y, scr.lines ); y<end; y++ )
	 {
	   if ( byte1>=0 )
	      scr.bord[y][byte1] &= and1;
	   if ( byte2>=0 )
	      scr.bord[y][byte2] &= and2;
	 }
     }
   return drawFrameOnly ? 0 : 1;
}

void Window::zoom( int flag )
{
/*
   Rect r = flag ? Rect( ((appl->menuMgr && appl->menuMgr->Visible) ? 0 : -1),
			-1,
			scr.lines - ((appl->statusLine && appl->statusLine->visible) ? 1 : 0),
			scr.cols ) : unzoomed;
*/
   Rect r = flag ? scrRect() : unzoomed;
   if ( r.a == rect.a && r.b == rect.b )
      return;
   unzoomed = rect;
   rect = r;
   init();
}

long Window::test( Collection *Text, char *Title,
	int cm1, char *s1, int cm2, char *s2, int cm3, char *s3,
	int cm4, char *s4, int cm5, char *s5 )
{
   modal *m = new modal( ALIGN_CENTER, Text, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 );
   long ret = exec( m );
   delete m;
   return ret;
}

long Window::test( char *str, char *Title, int cm1, char *s1,
	  int cm2, char *s2, int cm3, char *s3,
	  int cm4, char *s4, int cm5, char *s5 )
{
   modal *m = new modal( ALIGN_CENTER, str, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 );
   long ret = exec( m );
   delete m;
   return ret;
}

char *Window::getString( char *prompt, int scrlen, Collection *hist,
			 char *initStr, char *outStr, char *title,
			 char *help, int fillChar, char *status, InputType itype )
{
   static char buf[maxLineLength];
   if ( !outStr )
      outStr = buf;
   Dialog *d = getBoxForString( prompt, scrlen, hist, initStr, outStr,
		title, help, fillChar, status, itype );
   long ret = exec(d);
   delete d;
   if ( ret != HASH_cmOK )
      return 0;
   return outStr;
}

char *Window::getPassword( char *prompt, int scrlen, char *title, char *help,
		int fillChar, char *status )
{
   static char buf[maxLineLength];
   Dialog *d = getBoxForPass( prompt, scrlen, title, buf, help, fillChar, status );
   long ret = exec(d);
   delete d;
   if ( ret != HASH_cmOK )
      return 0;
   return buf;
}

void Window::setStatus(const char *format, ...)
{
   char *bufStatus = new char[2048];
   bufStatus[2047] = 0;
   va_list ap;
   va_start(ap, format);
#if defined(DJGPP) || defined(SCO_SYSV) || defined(SOLARIS_SPARC) ||\
		 defined(AIX_PPC) || defined(SINIX)
   vsprintf( bufStatus, format, ap);
#else
   vsnprintf( bufStatus, 2047, format, ap);
#endif
   va_end(ap);
   status->put( bufStatus, language );
   delete bufStatus;
   if ( appl->topWindow==this )
      drawStatus();
}

void Window::setIdent(const char *format, ...)
{
   char *bufIdent = new char[2048];
   bufIdent[2047] = 0;
   va_list ap;
   va_start(ap, format);
#if defined(DJGPP) || defined(SCO_SYSV) || defined(SOLARIS_SPARC) ||\
		defined(AIX_PPC) || defined(SINIX)
   vsprintf( bufIdent, format, ap);
#else
   vsnprintf( bufIdent, 2047, format, ap);
#endif
   va_end(ap);
   ident->put( bufIdent, language );
   delete bufIdent;
}

void Window::setTitle(const char *format, ...)
{
   char *bufTitle = new char[2048];
   bufTitle[2047]=0;
   va_list ap;
   va_start(ap, format);
#if defined(DJGPP) || defined(SCO_SYSV) || defined(SOLARIS_SPARC) ||\
		defined(AIX_PPC) || defined(SINIX)
   vsprintf( bufTitle, format, ap);
#else
   vsnprintf( bufTitle, 2047, format, ap);
#endif
   va_end(ap);
   title->put( bufTitle, language );
   delete bufTitle;
}

void Window::beforeExec()
{
   redrawAppl();
}

long Window::execUp( Window *w )
{
   w->setColors( colorsType );
   return appl->exec( isGetObj ? owner->Executor : Executor, w );
}

long Window::exec( Window *w )
{
   Collection *f = father ? father : (owner ? owner->father : 0);
   winExecutor *we = Executor ? Executor : (owner ? owner->Executor : 0);
   if ( !f || !we )
      return 0;
   w->setColors( colorsType );
   w->programRedraw = 1;
   return w->exec( f, we );
}

long Window::exec( Collection *f, winExecutor *executor )
{
   long key=0, typeMessage;
   active=1;
   if ( (father=f)->at(0) != this )
      f->atInsert( 0, this );
   Executor = executor;
   if ( isGetObj )
      keyHolder.del( &WindowKeyMap );

   if ( father->getCount() > 1 )
     {
       init();
       if ( appl->menuMgr/* && winMenu*/ )
	  appl->menuMgr->remove( ((Window*)father->at(1))->winMenu );
     }
   if ( winMenu && appl->menuMgr &&
	(Executor->type == EXEC || Executor->index == appl->current) )
      appl->menuMgr->select( winMenu );

   Message *msg=0;
   KeyMessage *km=0;
   void *ptr=0;
   Window *execWin;

   appl->topWindowPtr();
   programRedraw=1;
   beforeExec();
   int internalEvent=0;
   while( 1 )
    {
      key=handleKey( key, (key == HASH_cmMessage ? (void*&)msg : ptr) );
      if ( msg )
	 Task::respond( msg );
      if ( internalEvent )
	{ key=0; internalEvent=0; appl->redraw(); }
      if ( key && firstHandle && !ISKEY(key) && key != HASH_cmKill &&
		  key != HASH_cmClose && key != HASH_cmCloseWin )
	{
	  Task::sendMessage( appl, new BackMessage( key ) );
	  goto next;
	}
      if ( !ISKEY(key) && (key != HASH_cmEsc || Executor->type != INSERTED )
	   && close() )
	 break;
next:
      timerMessage = 0;
      msg = Task::getMessage();
      switch( msg->type() )
       {
	 case HASH_Key:
	     km = (KeyMessage*)msg;
	     key = km->data;
	     ptr = km->ptr;
	     Task::respond(msg);
	     if ( key == HASH_cmSetSize || key == HASH_cmSetPosition )
	       { init(); internalEvent=1; }
	     msg = 0;
	     break;
	 case HASH_Ptr:
	     execWin = (Window*)((PtrMessage*)msg)->data;
	     Task::respond(msg);
	     msg = 0;
	     ptr = 0;
	     key = execWin->exec( father, Executor );
	     delete execWin;
	     execWin = 0;
	     break;
	 case HASH_Timer:
	     {
	      TimerMessage *tm = (TimerMessage*)msg;
	      long p = tm->winPid;
	      key = tm->data;
	      ptr = 0;
	      Task::respond( msg );
	      msg = 0;
	      if ( winPid == p )
		 timerMessage = 1;
	      else
	       {
		 for( int i=father->getCount()-1; i>=0; i-- )
		  {
		    Window *w = (Window*)father->at(i);
		    if ( w->winPid == p && !w->timerMessage )
		     {
		       w->timerMessage = 1;
		       w->handleKey( key, ptr );
		       w->timerMessage = 0;
		       break;
		     }
		  }
		 goto next;
	       }
	     }
	     break;
	 default:
	     key = HASH_cmMessage;
       }
    }
   father->atRemove(0);
   appl->topWindowPtr();
   active=0;

   if ( appl->menuMgr )
    {
      appl->menuMgr->remove( winMenu );
      if ( father->getCount() > 0 )
	 appl->menuMgr->select( ((Window*)father->at(0))->winMenu );
    }

   afterExec( key );

   if ( key != HASH_cmKill )
      redrawAppl();
   else
    {
      retCommand = key;
      if ( Executor->type != INSERTED )
	 redrawAppl();
    }

   return key;
}

void Window::afterExec( long key )
{
}

void Window::redrawAppl()
{
   Task::sendMessage( appl, new BackMessage( HASH_cmRedraw ) );
}

void Window::MoveCursor()
{
   if ( !appl->topWindow || appl->topWindow->drawFrameOnly ||
	!appl->topWindow->canDraw() )
     { Screen::hideCursor(); return; }
   appl->topWindow->moveCursor();
}

void Window::markBlock( int type )
{
   switch( type )
    {
      case BLOCK_LINE:
	  if ( FLAG_LINE )
	   { CLEAR_BLOCK; SET_BLOCKLINE; }
	  else
	   {
	     CLEAR_BLOCK; SET_LINE;
	     block.top = block.bott = delta.y + cursor.y;
	     block.left = 0;
	     block.right = SHRT_MAX;
	   }
	  break;
      case BLOCK_COL:
	  if ( FLAG_COL )
	   { CLEAR_BLOCK; SET_BLOCKCOL; }
	  else
	   {
	     CLEAR_BLOCK; SET_COL;
	     block.top = block.bott = delta.y + cursor.y;
	     block.left = block.right = delta.x + cursor.x;
	   }
    }
}

void Window::unmarkBlock()
{
   CLEAR_BLOCK;
}

void Window::correctBlock()
{
   if ( !FLAG_COL && !FLAG_LINE )
      return;
   block.bott = delta.y + cursor.y;
   if ( FLAG_COL )
      block.right = delta.x + cursor.x;
}

blockInfo *Window::validBlock()
{
   static blockInfo b;
   b.top=block.top; b.bott=block.bott; b.left=block.left; b.right=block.right;
   if ( b.top > b.bott )
     {
       long top = b.bott;
       b.bott = b.top;
       b.top = top;
     }
   if ( b.left > b.right )
     {
       short left = b.right;
       b.right = b.left;
       b.left = left;
     }
   return &b;
}

const char *Window::setHelpContext( const char *newContext )
{
   if ( !newContext || strlen( newContext ) <= 0 )
     {
       if ( helpContext )
	 ::free( helpContext );
       return (helpContext=0);
     }
   helpContext = (char*)realloc( helpContext, strlen(newContext)+1 );
   strcpy( helpContext, newContext );
   return helpContext;
}

void Window::setRect( Rect &nRect )
{
   programRedraw = 1;
   getobj::setRect( nRect );
   size = Point( rect.b - rect.a + Point(1,1) );
}

void Window::setRect( Point &p )
{
   programRedraw = 1;
   getobj::setRect( p );
}

Rect scrRect()
{
   int top = (appl->menuMgr && appl->menuMgr->Visible) ? 1 : 0;
   int bott1 = (appl->statusLine && appl->statusLine->visible) ? 1 : 0;
   int bott2 = borderVisible ? 1 : 0;
   int left = borderVisible ? 0 : -1;
   int right = borderVisible ? 1 : 0;
   return Rect( top, left, Screen::Lines-bott1-bott2, Screen::Columns-right );
}
