/*
	$Id: dialog.cc,v 3.2.2.2 2007/07/24 11:28:11 shelton Exp $
*/
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <reg_expr.h>

#include "status.h"
#include "hashcode.h"
#include "dialog.h"
#include "i_lines.h"
#include "program.h"
#include "menu.h"

char Dialog::Buf[maxLineLength];
DataFind Dialog::dataFind;

static Lang notFound( "String not found.", "Строка не найдена." );
#define IS_VISUAL(index)  (objmap[index>>5] & (1 << (31 - index&31)))

unsigned char monoDialog[12] = {
	FG_HI_WHITE | BG_WHITE,		// active frame
	FG_BLACK | BG_WHITE,		// frame
	FG_BLACK | BG_WHITE,		// text
	FG_WHITE | BG_BLACK,		// block
	FG_HI_WHITE | BG_WHITE,		// find
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE		// selectet text
};

unsigned char colorDialog[12] = {
	FG_HI_WHITE | BG_WHITE,		// active frame
	FG_BLACK | BG_WHITE,		// frame
	FG_BLACK | BG_WHITE,		// text
	FG_WHITE | BG_BLUE,		// block
	FG_HI_WHITE | BG_CYAN,		// find
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE		// selectet text
};

unsigned char laptopDialog[12] = {
	FG_HI_WHITE | BG_WHITE,		// active frame
	FG_BLACK | BG_WHITE,		// frame
	FG_BLACK | BG_WHITE,		// text
	FG_WHITE | BG_BLACK,		// block
	FG_HI_WHITE | BG_WHITE,		// find
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE,		// selectet text
	FG_HI_WHITE | BG_WHITE 		// selectet text
};

void Dialog::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoDialog;
	  break;
      case COLOR:
	  clr = colorDialog;
	  break;
      case LAPTOP:
	  clr = laptopDialog;
    }
   for( ccIndex i=count-1; i>=0; i-- )
     ((getobj*)items[i])->setColors( type );
}

long dialogKeyMap[] = {
	kbHome,		HASH_cmHome,
	FUNC1(kbHome),	HASH_cmFirst,
	kbEnd,		HASH_cmEnd,
	FUNC1(kbEnd),	HASH_cmLast,
	kbTab,		HASH_cmNext,
	FUNC1(kbDown),	HASH_cmNext2,
	FUNC1(kbRight),	HASH_cmNext2,
	FUNC1(kbTab),	HASH_cmPrev,
	FUNC1(kbUp),	HASH_cmPrev2,
	FUNC1(kbLeft),	HASH_cmPrev2,
	kbPgUp,		HASH_cmPgUp,
	kbPgDn,		HASH_cmPgDn,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	kbCtrlF,	HASH_cmUp2,
	kbCtrlV,	HASH_cmDown2,
//	kbEnter,	HASH_cmOK,
	FUNC2(kbTab),	HASH_cmMode,
	kbF6,		HASH_cmFind,
	kbCtrlL,	HASH_cmFindDown,
	FUNC1(kbCtrlL),	HASH_cmFindUp,
	kbF7,		HASH_cmBlockLine,
	kbF8,		HASH_cmBlockColumn,
	kbCtrlP,	HASH_cmUnmarkBlock,
	FUNC1('+'),	HASH_cmCopyToClip,
//	kbCtrlC,	HASH_cmCancel,
//	kbEsc,		HASH_cmEsc,
	FUNC1(kbPgUp),	HASH_cmFuncPgUp,
	FUNC1(kbPgDn),	HASH_cmFuncPgDn,
	0
};

/*
CommandDescr dialogCommandDescr[] = {
	{ HASH_cmUp2,		new Lang("Scroll down without cursor moving","Скроллинг вниз без перемещения курсора") },
	{ HASH_cmDown2,		new Lang("Scroll up without cursor moving","Скроллинг вверх без перемещения курсора") },
	{ HASH_cmMode,		new Lang("Hard/soft cursor mode","Жесткий/мягкий режим курсора") },
	{ HASH_cmFind,		new Lang("Find string","Поиск строки") },
	{ HASH_cmFindDown,	new Lang("Find continue forward from the cursor","Продолжение поиска вниз от курсора") },
	{ HASH_cmFindUp,	new Lang("Find continue backward from the cursor","Продолжение поиска вверх от курсора") },
	{ HASH_cmBlockLine,	new Lang("Mark line block","Маркер строчного блока") },
	{ HASH_cmBlockColumn,	new Lang("Mark column block","Маркер колоночного блока") },
	{ HASH_cmUnmarkBlock,	new Lang("Unmark block","Снять маркер блока") },
	{ HASH_cmCopyToClip,	new Lang("Copy block to clipboard","Копирование блока в буфер обмена") },
	{ 0, 0 }
};
*/

Keymap DialogKeyMap( dialogKeyMap/*, new Lang("Dialog window base commands","Базовые команды в окне диалога")*/ );

//-------------------------------------------------------------

BgText::BgText( short *s, int l ) : len(0), allocated(0), str(0)
{
  if ( l > 0 )
   {
     allocated = l + 0xf /* - (l & 0xf)*/;
     str = (unsigned short*)malloc( allocated * sizeof(short) );
   }
  if ( str )
   {
     memcpy( str, s, l * sizeof(short) );
     len = l;
   }
}

BgText::~BgText()
{
  clear();
}

int BgText::putC( int ch, int index, int fIndex, int pos, int graph )
{
  int i = (pos < 0 || pos >= maxLineLength) ? len : pos;
  if ( i + 8 >= allocated )
   {
     allocated = i + 0xf;
     str = (unsigned short*)realloc( str, allocated * sizeof(short) );
   }
  short space = ' ' | ((fIndex<<8) & 0x7f00);
  for( ; i > len; str[len++] = space );
  int symbols = 1;
  if ( ch != '\t' )
   {
     if ( (unsigned)ch < 32 )
	str[i++] = (unsigned char)(ch+64) | (((index+3)<<8) & 0x7f00);
     else
	str[i++] = (unsigned char)ch | ((index<<8) & 0x7f00) | (graph ? 0x8000 : 0);
   }
  else
   {
     int k=0, end=8-(i & 7);
     for( ; k<end; k++ )
       { str[i++] = space; symbols++; }
     symbols--;
   }
  if ( i-1 >= len )
     len = i;
  return symbols;
}

char *BgText::putS( char *s, int len, int attr, int backAttr, int fillAttr, int pos, int graph )
{
  int offset = pos >= 0 ? pos : 0;
  int curAttr = attr;
  for( ; 1; s++, len-- )
   {
     if ( len <= 0 )
	break;
     switch( *s )
      {
	case '\n':
	    return s;
	case '\b':
	    curAttr=backAttr;
	    offset--;
	    continue;
      }
     offset += putC( *s, curAttr, fillAttr, offset, GraphAttr ? graph : 0);
     curAttr = attr;
   }
  return 0;
}

void BgText::clear()
{
  if ( str )
    {
      ::free( str );
      str=0;
    }
  len=allocated=0;
}

//-------------------------------------------------------------

int  Dialog::isType(long typ)
{
  return (typ==HASH_Dialog ? 1 : Window::isType(typ));
}

Dialog::Dialog( Rect r, Lang *t, Lang *id, Lang *st, int hmode ) :
		Window( r, t, id, st, 1 ),
		current(-1), activeItem(-1), internalRedraw(0), bottom(0),
		cury(0), curx(0), oldCur(-1), objmap(0), lenmap(0),
		notAccept(0), hardMode(hmode), bindCursor(0),
		topFset(0), fset(0)
{
   keyHolder.add( &DialogKeyMap/*, dialogCommandDescr*/ );
   setHelpContext( "Dialog" );
   setColors( colorsType );
}

int Dialog::init( void *data )
{
   int ret = Window::init( data );
   for( ccIndex i = 0; i < count; i++ ) {
      getobj *g = (getobj*)items[i];
      g->father = father;
      g->Executor = Executor;
      if ( g->trans_flag != trans_flag ) {
	  g->trans_flag = trans_flag;
	  internalRedraw = 1;
      }
      if ( g->isType( HASH_Dialog ) )
	  ((Dialog*)g)->init( data );
   }
   setObjmap();
   return ret;
}

Dialog::~Dialog()
{
  freeAll();
  if ( objmap )
     delete objmap;
  winMenu=0;
}

void Dialog::clear( int flagText )
{
  vis.freeAll();
  if ( flagText )
     text.freeAll();
  freeAll();
  delta.y = delta.x = cursor.y = cursor.x = 0;
  oldCur=-1;
  oldDelta.y = -1;
  bindCursor=0;
  bottom=0;
  cury=0;
  curx=0;
  notAccept=0;
  if ( objmap )
    delete objmap;
  objmap=0;
  lenmap=0;
  if ( topFset )
   {
     delete topFset;
     topFset = fset = NULL;
   }
}

int Dialog::close()
{
   if ( notAccept )
      return 1;
   for( int i=0; i<count; i++ )
      if ( !((getobj*)items[i])->accept() )
	 return notAccept=0;
   return 1;
}

void Dialog::freeItem( void *item )
{
   getobj *g = (getobj*)item;
   if ( winMenu == g->winMenu )
      winMenu = 0;
   delete g;
}

void Dialog::correctCursor()
{
  if ( current < 0 || !hardMode && oldCur == current && !bindCursor )
     return;
  bindCursor=0;
  int hide, corr = box ? 2 : 0;
  cursor = ((getobj*)items[current])->getCursor( &hide ) - Point( delta.y, delta.x );
  if ( cursor.x<0 )
     { delta.x=max(delta.x+cursor.x,0); cursor.x=0; }
  else if ( cursor.x>=size.x-corr )
     { delta.x=max(delta.x+cursor.x-size.x+corr+1,0); cursor.x=size.x-corr-1; }
  if ( cursor.y < 0 || cursor.y >= size.y-corr )
    {
      delta.y = delta.y + cursor.y - corr;
      cursor.y = corr;
      if ( delta.y < 0 )
	{ cursor.y += delta.y; delta.y=0; }
    }
}

void Dialog::select( int no )
{
   if ( no < 0 || no >= count /*|| no==current*/ )
      return;
   getobj *g = 0;
   char *strInvalid=0;
   if ( current>=0 && current < count )
    {
      g = (getobj*)items[current];
      if ( no != current && g->type() == TYPE_DATE )
	 ((DateInputLine*)g)->checkDate();
      if ( hardMode )
       {
	 strInvalid = g->valid();
	 if ( strInvalid )
	   { /*Screen::beep();*/ return; }
       }
      g->active = 0;
      if ( g->bindList )
       {
	 internalRedraw = 1;
	 for( int i=g->bindList->getCount()-1; i>=0; i-- )
	    ((getobj*)g->bindList->at(i))->bind_flag = 0;
       }
    }
   if ( g )
    {
      if ( appl->menuMgr )
	 appl->menuMgr->remove( g->winMenu );
      if ( winMenu == g->winMenu )
	 winMenu = 0;
    }
   g = (getobj*)items[no];
   g->active = 1;
   if ( g->bindList )
    {
      internalRedraw = 1;
      for( int i=g->bindList->getCount()-1; i>=0; i-- )
	 ((getobj*)g->bindList->at(i))->bind_flag = 1;
    }
   if ( appl->menuMgr )
      appl->menuMgr->select( g->winMenu );
   if ( !winMenu )
      winMenu = g->winMenu;
   current=no;
   processSelect( g );		// virtual for chield classes
}

void Dialog::select( getobj *get )
{
   getobj *g = NULL;
   if ( count > 0 && current>=0 && current < count && hardMode )
    {
      g = (getobj*)items[current];
      if ( g != get && g->type() == TYPE_DATE )
	 ((DateInputLine*)g)->checkDate();
      char *strInvalid = g->valid();
      if ( strInvalid )
	{ /*Screen::beep();*/ return; }
    }
   int oldcur = current;
   for( int i=0; i < count; i++ )
      if ( get == (getobj*)items[i] )
	{ current=i; break; }
   if ( oldcur != current )
     {
	g = (getobj*)items[oldcur];
	g->active = 0;
	if ( g->bindList )
	 {
	   internalRedraw = 1;
	   for( int i=g->bindList->getCount()-1; i>=0; i-- )
	      ((getobj*)g->bindList->at(i))->bind_flag = 0;
	 }
	g = (getobj*)items[current];
	g->active = 1;
	if ( g->bindList )
	 {
	   internalRedraw = 1;
	   for( int i=g->bindList->getCount()-1; i>=0; i-- )
	      ((getobj*)g->bindList->at(i))->bind_flag = 1;
	 }
     }
   processSelect( get );	// virtual for chield classes
}

void Dialog::moveCursor()
{
   Point cur = getCursor();
   if ( cur.y == scr.lines - 1 && appl->statusLine->visible )
      Screen::hideCursor();
   else
      Screen::move( cur );
   Screen::move( rect.a+cursor+Point(1,1) );
}

void Dialog::info()
{
   if ( box && !hardMode )
     {
	int pr = bottom ? min( 100, (delta.y+size.y)*100/bottom ) : 100;
	sprintf( Buf, "<%ld><%ld><%d%%>", delta.y+cursor.y+1, delta.x+cursor.x+1, pr );
	Point p = rect.a;
	if ( owner )
	   p += owner->corrPoint;
	Screen::move( p.y, p.x+2 );
	Screen::put( strlen(Buf) < (size_t)size.x ? (u_char*)Buf : (u_char*)"*", COLOR_FRAME );
     }
   MoveCursor();
}

int Dialog::insert( getobj *g, int ind )
{
   g->owner=this;
   g->trans_flag = trans_flag;
   if ( ind < 0 || ind > count )
      ind = count;
   Collection::atInsert( ind, g );
   if ( current<0 )
      { select(0); oldCur=current; }
   internalRedraw=1;
   g->isGetObj = 1;
   g->index = ind;
   g->init();
   ccIndex i;
   for( i = count-1; i > g->index; i-- )
      ((getobj*)items[i])->index++;
 //---------------- корректировка номеров об`ектов -----------------
   for( i=vis.getCount()-1; i>=0; i-- )
    {
      bound *b = (bound*)vis.at(i);
      if ( b->index >= ind )
	 b->index++;
    }
   vis.add( g->rect.a.y, g->rect.b.y, g->index );
 //-----------------------------------------------------------------

   int l = count / ( sizeof(int) * 8 ) + 1;
   if ( l > lenmap )
     {
       if ( objmap ) delete objmap;
       lenmap = l;
       objmap = new int [lenmap];
     }
   bottom = max( bottom, g->rect.b.y );
   return ind;
}

void Dialog::remove( getobj *g, int flagFree )
{
   ccIndex ind = indexOf( g );
   if ( ind < 0 )
      return;
   remove( ind, flagFree );
}

void Dialog::remove( ccIndex index, int flagFree )
{
   if ( index<0 || index >= count )
      return;
   getobj *g = (getobj*)items[index];
   vis.remove( g->rect.a.y, g->rect.b.y, g->index );
   ccIndex i, ind;

   if ( flagFree )
      atFree( index );
   else
      atRemove( index );

   for( i=count-1; i >= g->index; i-- )
    {
      getobj *g = (getobj*)items[i];
      g->index--;
    }
 //---------------- корректировка номеров об`ектов -----------------
   for( i=vis.getCount()-1; i>=0; i-- )
    {
      bound *b = (bound*)vis.at(i);
      if ( b->index>=index )
	 b->index--;
    }
 //-----------------------------------------------------------------
   int l = count/(sizeof(int) * 8) + 1;
   if ( l < lenmap )
     {
       if ( objmap ) delete objmap;
       lenmap = l;
       objmap = new int [lenmap];
     }
   internalRedraw=1;
   if ( current>=index )
      current = max( 0, current-1 );
}

long Dialog::handleKey( long key, void *&ptr )
{
   int flagRedraw=0;
   void *Ptr=0;
   if ( !key )
      goto end;

   if ( count > 0 )
    {
      if ( activeItem >=0 && activeItem < count )
       {
	 key = ((getobj*)items[activeItem])->handleKey( key, Ptr );
	 activeItem = -1;
       }
      else if ( hardMode || ((getobj*)items[current])->rect.contain(Point(curPos.y,curPos.x)) )
	 key = ((getobj*)items[current])->handleKey( key, Ptr );
    }
   if ( !key )
      goto end;

   key = keyHolder.translate( key );
m1:
   switch( key ) {
      case HASH_cmMode:
	  hardMode=!hardMode;
	  if ( hardMode )
	     select( current );
	  key=0; break;
      case HASH_cmUp:
	  if ( hardMode )
	     { key=HASH_cmPrev; goto m1; }
	  if ( !cursor.y && !delta.y )
	     return 0;
	  if ( cursor.y-1<0 ) delta.y--; else cursor.y--;
	  key=0; break;
      case HASH_cmUp2:
	  if ( hardMode )
	     { key=HASH_cmPrev; goto m1; }
	  if ( delta.y <= 0 )
	     return 0;
	  delta.y--;
	  key=0; break;
      case HASH_cmDown:
	  if ( hardMode )
	     { key=HASH_cmNext; goto m1; }
	  if ( delta.y>=bottom )
	     return 0;
	  if ( cursor.y+(box?2:0)+1>=size.y ) delta.y++; else cursor.y++;
	  key=0; break;
      case HASH_cmDown2:
	  if ( hardMode )
	     { key=HASH_cmNext; goto m1; }
	  if ( delta.y >= bottom )
	     return 0;
	  delta.y++;
	  key=0; break;
      case HASH_cmLeft:
	  if ( hardMode )
	     { key=HASH_cmPrev; goto m1; }
	  if ( !cursor.x && !delta.x )
	     return 0;
	  if ( cursor.x-1<0 ) delta.x--; else cursor.x--;
	  key=0; break;
      case HASH_cmRight:
	  if ( hardMode )
	     { key=HASH_cmNext; goto m1; }
	  if ( delta.x+cursor.x+1 >= maxLineLength )
	     return 0;
	  if ( cursor.x+(box?2:0)+1>=size.x ) delta.x++; else cursor.x++;
	  key=0; break;
      case HASH_cmHome:
	  if ( hardMode ) select(0);
	  else delta.x=cursor.x=0;
	  key=0; break;
      case HASH_cmFirst:
	  select(0);
	  key=0; break;
      case HASH_cmEnd:
	  if ( hardMode ) select( count-1 );
	  else moveEnd();
	  key=0; break;
      case HASH_cmLast:
	  select( count-1 );
	  key=0; break;
      case HASH_cmPrev2:
	  if ( isGetObj )
	     break;
      case HASH_cmPrev:
	  nextObj( -1 );
	  key=0; break;
      case HASH_cmNext2:
	  if ( isGetObj )
	     break;
      case HASH_cmNext:
	  nextObj( 1 );
	  key=0; break;
      case HASH_cmPgUp:
	  if ( !hardMode )
	     delta.y = max( delta.y-size.y+(box?2:0), 0 );
	  key=0; break;
      case HASH_cmPgDn:
	  if ( !hardMode )
	     delta.y = min( delta.y+size.y-(box?2:0), bottom );
	  key=0; break;
      case HASH_cmFuncPgUp:
	  if ( !hardMode )
	     delta.y = cursor.y = 0;
	  key=0; break;
      case HASH_cmFuncPgDn:
	  if ( !hardMode )
	    {
	      delta.y = max( 0, bottom-size.y+(box?2:0)+1 );
	      cursor.y = size.y - (box?2:0) - 1;
	    }
	  key=0; break;
      case HASH_cmCancel:
      case HASH_cmEsc:
	  notAccept=1;
	  break;
      case HASH_cmFind:
	  if ( !hardMode && !boxFind() )
	     return 0;
	  key=0; break;
      case HASH_cmFindDown:
	  if ( !hardMode && !find( 1 ) )
	    { test( notFound.get() ); return 0; }
	  key=0; break;
      case HASH_cmFindUp:
	  if ( !hardMode && !find( -1 ) )
	    { test( notFound.get() ); return 0; }
	  key=0; break;
      case HASH_cmBlockLine:
	  markBlock( BLOCK_LINE );
	  key=0; break;
      case HASH_cmBlockColumn:
	  markBlock( BLOCK_COL );
	  key=0; break;
      case HASH_cmUnmarkBlock:
	  unmarkBlock();
	  flagRedraw=1;
	  key=0; break;
      case HASH_cmCopyToClip:
	  copyBlockToClip();
	  key=0; break;
      case HASH_cmTransAltKoi8:
	  trans_flag = TRANS_alt2koi8;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
      case HASH_cmTransKoi8Alt:
	  trans_flag = TRANS_koi82alt;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
      case HASH_cmTransWinKoi8:
	  trans_flag = TRANS_win2koi8;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
      case HASH_cmTransKoi8Win:
	  trans_flag = TRANS_koi82win;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
      case HASH_cmTransMainKoi8:
	  trans_flag = TRANS_main2koi8;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
      case HASH_cmTransKoi8Main:
	  trans_flag = TRANS_koi82main;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
      case HASH_cmTransNone:
	  trans_flag = TRANS_none;
	  if ( !owner ) {
	      init();
	  } else {
	      owner->trans_flag = trans_flag;
	      owner->init();
	  }
	  flagRedraw=1; key=0; break;
   }

end:
   if ( owner ) {
      owner->bindCursor = 1;
   }
   correctCursor();
   curPos = delta + PointL( cursor.y, cursor.x );
   setObjmap(1);
   if ( !isGetObj && !key && draw( flagRedraw ) )
      Screen::sync();
   return key;
}

void Dialog::setObjmap( int flagSelect )
{
  memset( objmap, 0, lenmap*sizeof(int) );
  vis.region( delta.y, delta.y+size.y-(box?2:0)-1, objmap );
  if ( !flagSelect )
     return;
  ccIndex ind=0;
  int c, k;
  for( int i=0; i<lenmap; i++ )
   {
     if ( !objmap[i] )
      { ind += 32; continue; }
     for( c=objmap[i], k=0; c; c<<=1, ind++, k++ )
      {
	if ( c>0 )
	   continue;
	if ( ((getobj*)items[ind])->rect.contain(Point(curPos.y,curPos.x)) )
	 {
	   select( ind );
	   return;
	 }
      }
     ind+=32-k;
   }
}

int Dialog::draw( int Redraw, int sy, int sx )
{
   if ( !Window::draw( Redraw, sy, sx ) )
      return 0;

   int hide=0;
   setObjmap();
   correctBlock();
   char **saveMap = Screen::currentMap;
   if ( !isGetObj )
      Screen::currentMap = scr.bord;

   Rect oldRegion = Screen::getClipRegion();
   Rect r = rect;
   r.a.y += sy; r.b.y += sy;
   r.a.x += sx; r.b.x += sx;
   if ( box )
      r.grow( -1, -1 );
   Screen::setClipRegion( r );

   if ( !Redraw && (FLAG_LINE || FLAG_COL) )
      Redraw=1;

   int mustDraw = scroll( clr[2], Redraw );
   int flagDraw = Redraw || changed || mustDraw || internalRedraw;

   if ( flagDraw )
    {
      int y = sy + rect.a.y + topDraw - delta.y + (box ? 1 : 0),
	  x = sx + rect.a.x + (box ? 1 : 0),
	  nx = size.x - (box ? 2 : 0),
	  ny = bottDraw - topDraw + 1;
      Screen::Clear( y, x, ny, nx, clr[2] );
      unsigned char *_line = new unsigned char[nx * sizeof(short)];
      for( ccIndex i=topDraw, end=min( text.getCount(), bottDraw+1 ); i<end; i++, y++ ) {
	 BgText *ch=(BgText*)text.at(i);
	 if ( ch->len > delta.x ) {
	    int graphAttr=0, l = min( nx, ch->len-delta.x );
	    memcpy( _line, ch->str + delta.x, l * sizeof(short) );
#if defined(SOLARIS_SPARC) || defined(AIX_PPC) || defined(SINIX)
	    for( int k=0, j=0; k<l; k++, j+=2 )
#else
	    for( int k=0, j=1; k<l; k++, j+=2 )
#endif
	    {
	       graphAttr = 0;
	       if ( _line[j] & 0x80 ) {
		   graphAttr = 0x80;
		   _line[j] &= 0x7f;
	       }
	       _line[j] = clr[ 2 + _line[j] + (_line[j] ? 2 : 0)] | graphAttr;
#if defined(SOLARIS_SPARC) || defined(AIX_PPC) || defined(SINIX)
	       unsigned char *symbol = _line + j + 1;
#else
	       unsigned char *symbol = _line + j - 1;
#endif
	       *symbol = translate_out( *symbol );
	    }
	    Screen::putLimited( y, x, (unsigned short*)_line, l );
	 }
      }
      delete _line;
    }

   if ( FLAG_COL || FLAG_LINE || FLAG_BLOCKCOL || FLAG_BLOCKLINE )
    {
       blockInfo *b = validBlock();
       Screen::attrSet( b->top - delta.y + rect.a.y + 1,
			b->left - delta.x + rect.a.x + 1,
			b->bott - b->top + 1,
			b->right - b->left + 1,
			COLOR_BLOCK );
    }

   corrPoint = rect.a - Point( delta.y, delta.x ); // для об'ектов !!!
   if ( box )
      corrPoint -= Point( -1, -1 );
   if ( owner )
      corrPoint += owner->corrPoint;
   ccIndex ind=0;
   for( int i=0; i<lenmap; i++ )
    {
      int c, k;
      if ( !objmap[i] )
	 { ind+=32; continue; }
      for( c=objmap[i], k=0; c && ind < count; c<<=1, ind++, k++ )
	{
	  if ( c>0 || ind == current )
	    continue;
	  if ( flagDraw )
	    ((getobj*)items[ind])->draw( 1, corrPoint.y, corrPoint.x );
	}
      ind+=32-k;
    }
   if ( oldCur != current && oldCur>=0 && oldCur<count )
      ((getobj*)items[oldCur])->draw( flagDraw, corrPoint.y, corrPoint.x );
   oldCur=current;
   if ( current >= 0 && current < count && IS_VISUAL( current ) )
      ((getobj*)items[current])->draw( flagDraw, corrPoint.y, corrPoint.x );

   if ( fInfo.found )
    {
       Screen::attrSet( fInfo.line - delta.y + rect.a.y + 1,
			fInfo.start - delta.x + rect.a.x + 1,
			1, fInfo.end - fInfo.start + 1,
			COLOR_FIND );
       fInfo.found = 0;
    }

   if ( !isGetObj )
      Screen::currentMap = scr.map;

   Screen::setClipRegion( oldRegion );

   info();

   if ( !isGetObj )
    {
      if ( hide )
	 Screen::hideCursor();
      Screen::currentMap = saveMap;

      if ( appl->topWindow != this )
	 Screen::attrSet( Screen::shadowMap );
    }

   changed = 0;
   oldDelta = delta;
   internalRedraw = 0;
   return 1;
}

void Dialog::clearLine( ccIndex no )
{
   if ( no >= 0 && no < text.getCount() )
      ((BgText*)text.at(no))->clear();
   changed=1;
}

void Dialog::removeLine( ccIndex no )
{
   if ( no<0 || no >= text.getCount() )
      return;
   text.atFree( no );
   changed=1;
}

void Dialog::put( int ch, int index, int graph )
{
   if ( ch == '\n' )
     { cury++; curx=0; return; }
   BgText *bg=0;
   if ( cury < text.getCount() )
       bg = (BgText*)text.at(cury);
   else while( cury >= text.getCount() )
       text.insert( (bg=new BgText( 0, 0 )) );
   curx += bg->putC( ch, index, 0, curx, GraphAttr ? graph : 0 );
   changed=1;
   bottom=max( bottom, text.getCount()-1 );
}

void Dialog::put( ccIndex y, ccIndex x, const char *str, int len, int index, int graph )
{
   if ( x<0 || y<0 )
      return;
   cury=y; curx=x;
   if ( !str )
      return;
   int lenStr = len >= 0 ? len : strlen( str );
   BgText *bg=0;
   char *newStr = (char*)str;
   while( newStr )
    {
      if ( cury < text.getCount() )
	  bg = (BgText*)text.at(cury);
      else while( cury >= text.getCount() )
	  text.insert( (bg=new BgText( 0, 0 )) );
      char *nstr = bg->putS( newStr, lenStr, index, index+3, 0, curx, GraphAttr ? graph : 0 );
      if ( nstr )
	{
	  nstr++;
	  lenStr -= nstr - newStr;
	  cury++;
	  curx=0;
	  newStr = nstr;
	}
      else
	{
	  newStr = 0;
	  curx += lenStr;
	}
    }
   changed=1;
   bottom=max( bottom, text.getCount()-1 );
}

void Dialog::clearText( ccIndex y1, ccIndex y2 )
{
   if ( y1 > y2 )
      return;
   if ( y1 < 0 ) y1=0;
   if ( y2 >= text.getCount() ) y2 = text.getCount() - 1;
   for( ccIndex line=y1; line<=y2; line++ )
      clearLine( line );
}

void Dialog::removeText( ccIndex y1, ccIndex y2 )
{
   if ( y1 > y2 )
      return;
   if ( y1 < 0 ) y1=0;
   if ( y2 >= text.getCount() ) y2 = text.getCount() - 1;
   for( long i = y2-y1+1; i>=0; i-- )
      removeLine( y1 );
}

char *Dialog::currentWord()
{
   static char buf[256];
   buf[0] = 0;
   int len;
   char *s = getStr( delta.y + cursor.y, len );
   if ( !s )
      return buf;
   int i, j, end, pos = delta.x + cursor.x;
   if ( pos >= len || strchr( wordDelim, s[pos] ) )
      return buf;
   for( i=pos; i >= 0 && !strchr( wordDelim, s[i] ); i-- );
   i++;
   for( j=0, end=min(255,len); j<end && !strchr( wordDelim, s[i] ); i++, j++ )
	buf[j] = s[i];
   buf[j]=0;
   return buf;
}

char *Dialog::getStr( long line, int &len )
{
   if ( line < 0 || line >= text.getCount() )
      return 0;
   BgText *ch = (BgText*)text.at(line);
   int i;
   for( i=0; i < ch->len; i++ )
      Buf[i] = ch->str[i];
   Buf[i]=0;
   len = ch->len;
   return Buf;
}

void Dialog::setCursor( PointL d, Point c )
{
   if ( d.y < 0 ) d.y = 0;
   if ( d.x < 0 ) d.x = 0;
   if ( c.y < 0 ) c.y = 0;
   if ( c.x < 0 ) c.x = 0;
   delta=d; cursor=c;
}

Point Dialog::getCursor( int *hide )
{
  Point cur = rect.a + cursor;
  if ( box )
     cur += Point(1,1);
  return cur;
}

void Dialog::moveEnd()
{
   ccIndex line = delta.y + cursor.y;
   if ( line<0 )
      return;
   int len=0;
   memset( objmap, 0, lenmap*sizeof(int) );
   vis.region( line, line, objmap );
   int c, k;
   for( int i=0, ind=0; i<lenmap; i++ )
     {
       if ( !objmap[i] )
	  { ind+=32; continue; }
       for( c=objmap[i], k=0; c; c<<=1, ind++, k++ )
	 if ( c<0 )
	   len = max( len, ((getobj*)items[ind])->rect.b.x + 1 );
       ind+=32-k;
     }
   if ( line < text.getCount() )
      len = max( len, ((BgText*)text.at(line))->len );
   if ( len < delta.x || len >= delta.x + size.x - (box?2:0) )
      delta.x = max( len - size.x + (box?2:0) + 1, 0 );
   cursor.x = len - delta.x;
}

int Dialog::boxFind()
{
   static Collection history(10,10);
   int opt = dataFind.options;
   int dir = dataFind.direction;
   Rect r( Screen::center( 9, 52 ) );
   Dialog *d = new Dialog( r, new Lang("Find","Поиск") );

   static char word[256];
   char *wr = currentWord();
   if ( wr[0] )
       strncpy( word, wr, 255 );
   else
       strncpy( word, dataFind.strFind, 255 );
   word[255] = 0;

   inputLine *il = new inputLine( Point(1,1), word, word, 47, 255, 0 , &history );
   if ( !il )
      return 0;
   d->insert( il );

   d->put( Point(3,3), lang("Options","Опции") );
   getBox *gb = new checkBox( Point(4,1), opt, &opt );
   if ( !gb )
      return 0;
   gb->add( lang("Case-sensitive",	"Чувство к регистру") );
   gb->add( lang("Words only",		"Только слово") );
   gb->add( lang("Regular expression  ", "Регулярное выражение") );
   d->insert( gb );

   d->put( Point(3,31), lang("Direction","Направление") );
   gb = new radioBox( Point(4,29), dir, &dir );
   if ( !gb )
      return 0;
   gb->add( lang("Forward",	"Вперед") );
   gb->add( lang("Backward",	"Назад") );
   gb->add( lang("From begin    ","От начала     ") );
   d->insert( gb );

   long ret = d->exec( father, Executor );
   delete d;

   if (  ret != HASH_cmOK )
      return 0;

   dataFind.options = opt;
   dataFind.direction = dir;
   strcpy( dataFind.strFind, word );
   if ( !find( dataFind.direction == 1 ? -1 : 0 ) )
     {
       test( notFound.get() );
       return 0;
     }
   return 1;
}

int Dialog::find( int offset )
{
   long line, end = text.getCount()-1, Line = delta.y + cursor.y;
   int pr = 1, pos = SHRT_MAX;
   if ( offset >= 0 )		// forward
       line = dataFind.direction == 2 ? 0 : (Line > end ? max( 0, end ) : Line);
   else				// backward
     {
       line = Line > end ? max( 0, end ) : Line;
       end=0; pr = -1;
     }

   if ( dataFind.options & 4 )		// regular expression
       strcpy( Buf, dataFind.strFind );
   else
     {
       int p, c;
       for( p=0, c=0; dataFind.strFind[p]; p++, c++ )
	 {
	    if ( strchr( "+.*?[()|\\/^$", dataFind.strFind[p] ) )
	       Buf[c++]='\\';
	    Buf[c] = dataFind.strFind[p];
	 }
       Buf[c]=0;
     }

   static Regexpr ex;
   static re_registers regs;
   char *ch = ex.compile( Buf, !(dataFind.options & 1) );
   if ( ch )
     { test( ch ); return -1; }

   int len, fullWord = dataFind.options & 2;
   fInfo.found = 0;
   for( ; !fInfo.found && (!dataFind.direction && line<=end ||
				dataFind.direction==1 && line>=end); line+=pr )
    {
       char *s = getStr( line, len );
       if ( pos )
	  pos = delta.x + cursor.x + offset;
       if ( pos >= len )
	  { pos=0; continue; }

       while( 1 )
	{
	  int ret = ex.search( s, len, pos, len - pos, &regs );
	  if ( ret < 0 )
	     { pos = 0; break; }

	  fInfo.line  = line;
	  fInfo.start = regs.start[0];
	  fInfo.end   = regs.end[0]-1;
	  if ( fullWord && ( fInfo.start > 0 && !strchr( wordDelim, s[fInfo.start - 1] ) ||
	       fInfo.end < len && !strchr( wordDelim, s[fInfo.end + 1] ) ) )
	    { pos++; continue; }

	  fInfo.found = 1;
	  int d = fInfo.start - delta.x;
	  if ( d<0 || d >= size.x-(box?2:0) )
	     delta.x = max( 0, fInfo.start - size.x + (box?2:0) + 1);
	  cursor.x = fInfo.start - delta.x;

	  if ( line < delta.y || line >= delta.y + size.y - (box?2:0) )
	    {
	      cursor.y = size.y/2;
	      delta.y = max( 0, line - cursor.y );
	      if ( !delta.y )
		 cursor.y = line;
	    }
	  else
	      cursor.y = line - delta.y;
	  pos = 0;
	  correctBlock();
	  break;
	}
    }
   return fInfo.found;
}

void Dialog::copyBlockToClip()
{
   if ( !FLAG_LINE && !FLAG_BLOCKLINE && !FLAG_COL && !FLAG_BLOCKCOL )
      return;
   clipBoard.freeAll();
   blockInfo *b = validBlock();
   int len;
   if ( FLAG_LINE || FLAG_BLOCKLINE )
     {
       for( long i=b->top; i <= b->bott && i < text.getCount(); i++ )
	{
	  char *str = getStr( i, len );
	  char *s = new char[len+1];
	  memcpy( s, str, len ); s[len]=0;
	  clipBoard.insert( s );
	}
       short *type = new short(0);
       clipBoard.atInsert( 0, type );
     }
   else if ( FLAG_COL || FLAG_BLOCKCOL )
     {
       int l, pos;
       for( long i=b->top; i<=b->bott && i < text.getCount(); i++ )
	{
	  char *str = getStr( i, len );
	  for( l=0, pos=b->left; pos<len && pos<=b->right; pos++, l++ );
	  char *s = new char[l+1];
	  memcpy( s, str+b->left, l ); s[l]=0;
	  clipBoard.insert( s );
	}
       short *type = new short( b->right - b->left + 1 );
       clipBoard.atInsert( 0, type );
     }
}

/*
void *Dialog::getState()
{
   return (void*) new DialogState( delta, cursor, current, hardMode );
}

void Dialog::setState( void *st )
{
   DialogState *s = (DialogState*)st;
   if ( s->delta.y >= 0 && s->delta.y <= bottom )
      delta.y = s->delta.y;
   if ( s->delta.x >= 0 && s->delta.x <= 256 )
      delta.x = s->delta.x;
   if ( s->cursor.y >= 0 && s->cursor.y < size.y - (box?2:0) )
      cursor.y = s->cursor.y;
   if ( s->cursor.x >= 0 && s->cursor.x < size.x - (box?2:0) )
      cursor.x = s->cursor.x;
   if ( s->current >= 0 && s->current < count )
      select( s->current );
//   oldCur = current;
   hardMode = s->hardMode;
}
*/

void Dialog::nextObj( int pr )
{
   if ( count <= 0 )
      return;
   curPos = delta + PointL( cursor.y, cursor.x );
   if ( hardMode || ((getobj*)items[current])->rect.contain(Point(curPos.y,curPos.x)) )
     {
       if ( pr > 0 )
	  select( current+1 >= count ? 0 : current+1 );
       else
	  select( current-1 >= 0 ? current-1 : count-1 );
       return;
     }
   long startline = delta.y+cursor.y;
   for( long line=startline; (pr>0?line<=bottom+1:line>=0); line+=pr )
     {
       memset( objmap, 0, lenmap*sizeof(int) );
       vis.region( line, line, objmap );
       ccIndex ind=0;
       int c, k;
       for( int i=0; i<lenmap; i++ )
	{
	  if ( !objmap[i] )
	    { ind+=32; continue; }
	  for( c=objmap[i], k=0; c; c<<=1, ind++, k++ )
	    {
	      if ( c>0 )
		 continue;
	      getobj *g = (getobj*)items[ind];
	      if ( line == startline && g->rect.b.x < curPos.x )
		 continue;
	      select( ind );
	      bindCursor=1;
	      return;
	    }
	  ind+=32-k;
	}
     }
}

int Dialog::fillRCinfo( const char *str, Collection *coll )
{
  if ( !str || strlen( str ) == 0 )
     return 0;
  int i=0, percent=0, mul=0, ret=0;
  char *buf = (char*)calloc( 32, 1 );
  for( ; 1; str++ )
   {
     switch( *str )
      {
       case '0'...'9':
	   if ( !percent && !mul )
	      buf[i++] = *str;
	   continue;
       case '*':
	   buf[i]=0;
	   mul = 1;
	   continue;
       case '%':
	   buf[i]=0;
	   percent = 1;
	   continue;
       case 0:
       case ',':
	   buf[i]=0;
	   break;
       default:
	   mul=percent=1;	// генерируем ошибку
      }
     if ( mul && percent )
	break;
     rcInfo *rc = (rcInfo*)calloc( 1, sizeof(rcInfo) );
     rc->type = mul ? TYPE_MUL : (percent ? TYPE_PERCENT : TYPE_NUMBER);
     rc->val = atoi( buf );
     if ( rc->type == TYPE_MUL && rc->val == 0 )
	rc->val = 1;
     coll->insert( rc );
     i = mul = percent = 0;
     if ( *str == 0 )
      {
	ret = 1;
	break;
      }
   }
  ::free( buf );
  return ret;
}

void Dialog::beginFrameset( const char *Cols, const char *Rows )
{
  Collection *cols = new Collection(3,3);
  Collection *rows = new Collection(3,3);
  if ( !fillRCinfo( Cols, cols ) )
   {
     delete cols;
     cols = NULL;
   }
  if ( !fillRCinfo( Rows, rows ) )
   {
     delete rows;
     rows = NULL;
   }
  if ( !cols && !rows )
     return;
  FrameSet *fs = new FrameSet( fset, cols, rows );
  if ( fset )
   {
     fset->insert( TYPE_FRAMESET, fs, this );
     fset = fs;
   }
  else
   {
     fs->setRect( Rect( 0, 0, size.y - (box?2:0), size.x - (box?2:0) ) );
     fset = topFset = fs;
   }
  fset->drawSkel( this );
}

void Dialog::beginFrame( Window *w, const char *name, int noresize )
{
  if ( !fset || !w )
     return;
  w->box = 0;
  w->owner = owner ? owner : this;
  w->father = father;
  w->Executor = Executor;
  if ( name )
     w->target_name = strdup( name );
  fset->insert( TYPE_FRAME, w, this );
}

void Dialog::endFrameset()
{
  if ( !fset )
     return;
  fset = fset->prev ? fset->prev : topFset;
}

//---------------------- FrameSet class -----------------------------

FrameSet::FrameSet( FrameSet *ptr, Collection *Cols, Collection *Rows ) :
		prev(ptr), set(0), cols(Cols), rows(Rows),
		fcount(0), maxframes(0), coor_x(0), coor_y(0)
{
  rect = Rect(0,0,0,0);
}

FrameSet::~FrameSet()
{
  int i=0;
  if ( set )
   {
     for( i=set->getCount()-1; i>=0; i-- )
      {
	FRinfo *fr = (FRinfo*)set->at(i);
	if ( fr->type == TYPE_FRAMESET )
	   delete ((FrameSet*)fr->ptr);
	::free( fr );
	set->atRemove( i );
      }
     delete set;
     set = NULL;
   }

  if ( cols )
   {
     for( i=cols->getCount()-1; i>=0; i-- )
      {
	::free( cols->at( i ) );
	cols->atRemove( i );
      }
     delete cols;
     cols = NULL;
   }

  if ( rows )
   {
     for( i=rows->getCount()-1; i>=0; i-- )
      {
	::free( rows->at( i ) );
	rows->atRemove( i );
      }
     delete rows;
     rows = NULL;
   }

  if ( coor_x )
   {
     ::free( coor_x );
     coor_x = NULL;
   }

  if ( coor_y )
   {
     ::free( coor_y );
     coor_y = NULL;
   }
}

void FrameSet::initFrames()
{
  if ( set )
     return;
  if ( coor_x )
     ::free( coor_x );
  if ( coor_y )
     ::free( coor_y );
  fcount = 0;
  int c_count = cols ? cols->getCount() : 0;
  int r_count = rows ? rows->getCount() : 0;
  maxframes = c_count * r_count;
  if ( maxframes <= 0 )
     maxframes = c_count > 0 ? c_count : (r_count > 0 ? r_count : 1);
  int size_x = rect.b.x - rect.a.x + 1;
  int size_y = rect.b.y - rect.a.y + 1;
  int x_pix = Screen::Columns * 8;
  int y_pix = Screen::Lines * 16;
  setCoor( cols, coor_x, x_pix, size_x );
  setCoor( rows, coor_y, y_pix, size_y );
}

void FrameSet::setCoor( Collection *coll, int *&coor, int pix_size, int size )
{
  if ( !coll )
     return;
  int i=0, j=0, n_mul=0, n_perc=0, n=0;
  rcInfo *rc=0;
  for( i=0, j=coll->getCount(); i < j; i++ )
   {
     rc = (rcInfo*)coll->at( i );
     switch( rc->type )
      {
	case TYPE_MUL:
	    n_mul += rc->val;
	    continue;
	case TYPE_PERCENT:
	    n_perc += rc->val;
	    continue;
	case TYPE_NUMBER:
	    n += rc->val;
	    continue;
	default:
	    continue;
      }
   }

  // козффициент для корректировки пиксельных полей
  double corr = n > pix_size ? ((double)n / pix_size) : 0;
  for( n=0, i=0, j=coll->getCount(); i < j; i++ )
   {
     rc = (rcInfo*)coll->at( i );
     if ( rc->type != TYPE_NUMBER )
	continue;
     rc->type = TYPE_PERCENT_HARD;
     if ( corr )
	rc->val = int(rc->val / corr);
     rc->val = (int)rint(rc->val * 100 / pix_size);
     n += rc->val;
   }

  int n_ost = 100 - n;
  if ( n_ost > 0 )	// осталось место для остальных полей
   {
     corr = (double)n_ost / 100; // корректировка процентных полей
     for( i=0, j=coll->getCount(); i < j; i++ )
      {
	rc = (rcInfo*)coll->at( i );
	if ( rc->type != TYPE_PERCENT )
	   continue;
	rc->type = TYPE_PERCENT_HARD;
	rc->val = (int)rint(rc->val * corr);
	n += rc->val;
      }
     n_ost = 100 - n;
     if ( n_ost > 0 )		// осталось место для остальных полей
	n = n_ost / n_mul;	// величина поля для одной "*"
   }

  int c_count = coll->getCount();
  if ( c_count < 1 )
     coor = 0;
  else
   {
     coor = (int*)calloc( c_count, sizeof(int) );
     for( i=0; i < c_count; i++ )
      {
	rc = (rcInfo*)coll->at( i );
	switch( rc->type )
	 {
	   case TYPE_PERCENT_HARD:
	       coor[i] = rc->val * (size-c_count+1) / 100;
	       break;
	   case TYPE_MUL:
	       coor[i] = rc->val * n * (size-c_count+1) / 100;
	       break;
	   default:
	       continue;
	 }
	if ( coor[i] < 1 )
	   coor[i] = 1;
	if ( i > 0 )
	   coor[i] += coor[i-1] + 1;
      }
     coor[c_count-1]=0;
   }
}

void FrameSet::setRect( Rect r )
{
  rect = r;
  initFrames();
}

int FrameSet::getRect( FRtype type, int frame_count, Rect &r )
{
  if ( frame_count < 0 || frame_count >= maxframes )
     return 0;
  int c_count = cols ? cols->getCount() : 0;
  int r_count = rows ? rows->getCount() : 0;
  int row = c_count > 0 ? frame_count / c_count : frame_count;
  int col = c_count > 0 ? frame_count % c_count : 0;
  int d = type == TYPE_FRAME ? 1 : 0;

  r.a.y = row > 0 ? coor_y[row-1] + 1 : rect.a.y;
  r.b.y = row >= r_count-1 ? rect.b.y-d : coor_y[row]-d;
  r.a.x = col > 0 ? coor_x[col-1] + 1 : rect.a.x;
  r.b.x = col >= c_count-1 ? rect.b.x-d : coor_x[col]-d;
  return 1;
}

void FrameSet::insert( FRtype type, void *ptr, Dialog *w )
{
  Rect r;
  if ( getRect( type, fcount, r ) == 0 || !set && !(set = new Collection( 3, 3 ) ) )
     return;
  FRinfo *fr = (FRinfo*)calloc( 1, sizeof(FRinfo) );
  if ( !fr )
     return;
  switch( type )
   {
     case TYPE_FRAME:
	 {
	   Dialog *wptr = (Dialog*)ptr;
	   wptr->setRect( r );
	   w->insert( wptr );
	   wptr->Executor = w->Executor;
	 }
	 break;
     case TYPE_FRAMESET:
	 ((FrameSet*)ptr)->setRect( r );
	 break;
   }
  fr->type = type;
  fr->ptr = ptr;
  set->insert( fr );
  fcount++;
}

void FrameSet::drawSkel( Dialog *w )
{
  char buf[2], buf2[2];
  buf[1] = buf2[0] = 0;
  buf[0]  = Screen::_VL;
  buf2[0] = Screen::_CC;
  int *i=0, j=0, l;
  int size_y = rect.b.y - rect.a.y;
  int size_x = rect.b.x - rect.a.x;

  for( i=coor_x; i && *i; i++ )
    for( j=0; j < size_y; j++ )
	w->put( (long)(j + rect.a.y), (long)(*i + rect.a.x), buf, -1, 0, 1 );

  buf[0] = Screen::_HL;
  for( i=coor_y; i && *i; i++ )
    for( j=0, l=0; j < size_x; j++ )
     {
       char *str = buf;
       if ( j > 0 && coor_x && coor_x[l] == j )
	{
	  str = buf2;
	  l++;
	}
       w->put( (long)(*i + rect.a.y), (long)(j + rect.a.x), str, -1, 0, 1 );
     }
}

Dialog *FrameSet::findTarget( char *name )
{
  Dialog *w=0, *ww=0;
  for( int i=0, j=set->getCount(); i < j; i++ )
   {
     FRinfo *fr = (FRinfo*)set->at(i);
     if ( fr->type == TYPE_FRAME )
      {
	ww = (Dialog*)fr->ptr;
	if ( !ww->target_name )
	   continue;
	if ( !strcmp( name, ww->target_name ) )
	 {
	   w = ww;
	   break;
	 }
      }
     else if ( (w=((FrameSet*)fr->ptr)->findTarget( name )) )
	break;
   }
  return w;
}

