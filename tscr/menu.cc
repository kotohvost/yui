/*
	$Id: menu.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "menu.h"
#include "status.h"
#include "program.h"

SharedMenuCollection sharedMenu;

long menuKeyMap[] = {
	kbHome,		HASH_cmHome,
	kbEnd,		HASH_cmEnd,
	kbPgUp,		HASH_cmHome,
	kbPgDn,		HASH_cmEnd,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbTab,		HASH_cmNext,
	FUNC1(kbTab),	HASH_cmPrev,
	0
};

Keymap MenuKeyMap( menuKeyMap );

unsigned char monoMenu[4] =
{
   FG_WHITE | BG_BLACK,		// active
   FG_BLACK | BG_WHITE,		// normal
   FG_HI_WHITE | BG_BLACK,	// hotkey, active
   FG_HI_WHITE | BG_WHITE	// hotkey, normal
};

unsigned char colorMenu[4] =
{
   FG_BLACK | BG_GREEN,		// active
   FG_BLACK | BG_WHITE,		// normal
   FG_RED | BG_GREEN,		// hotkey, active
   FG_RED | BG_WHITE		// hotkey, normal
};

unsigned char laptopMenu[4] =
{
   FG_WHITE | BG_BLACK,		// active
   FG_BLACK | BG_WHITE,		// normal
   FG_HI_WHITE | BG_BLACK,	// hotkey, active
   FG_HI_WHITE | BG_WHITE	// hotkey, normal
};

void Menu::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoMenu;
	  break;
      case COLOR:
	  clr = colorMenu;
	  break;
      case LAPTOP:
	  clr = laptopMenu;
    }
}

void MenuManager::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoMenu;
	  break;
      case COLOR:
	  clr = colorMenu;
	  break;
      case LAPTOP:
	  clr = laptopMenu;
    }
}

int Menu::state=0;
int Menu::curstate=0;

menuItem::menuItem( Lang *l, Lang *s, char *h, long com ) :
		    label(l), status(s), key(0), help(0),
		    command(com)
{
   type = COMMAND;
   init( h );
}

menuItem::menuItem( Lang *l, Lang *s, char *h, Menu *m ) :
		    label(l), status(s), key(0), help(0),
		    menu(m)
{
   type = MENU;
   init( h );
}

menuItem::menuItem( char *lstr, char *sstr, char *h, long com ) :
		    status(0), key(0), help(0),
		    command(com)
{
   type = COMMAND;
   label = new Lang( lstr );
   if ( sstr )
      status = new Lang( sstr );
   init( h );
}

menuItem::menuItem( char *lstr, char *sstr, char *h, Menu *m ) :
		    status(0), key(0), help(0),
		    menu(m)
{
   type = MENU;
   label = new Lang( lstr );
   if ( sstr )
      status = new Lang( sstr );
   init( h );
}

void menuItem::init( char *h )
{
   help = (char*)(h ? strdup( h ) : NULL);
}

u_char menuItem::hotkey()
{
   char *s = label->get();
   if ( s )
     for( ; *s; s++ )
       if ( *s == '~' )
	  return lowercase( *(s+1) );
   return 0;
}

menuItem::~menuItem()
{
   if ( help ) ::free( help );
   if ( label ) delete label;
   if ( status ) delete status;
   if ( key ) delete key;
   if ( type == MENU && menu )
      delete menu;
}

int  Menu::isType(long typ)
{
  return (typ==HASH_Menu ? 1 : Window::isType(typ));
}

Menu::Menu( menuColl *it, Point coor, Lang *tit, int Vert, int Auto, int Box ) :
	    mcoll(it), vert(Vert), offsetRect(0),
	    autoOpen(Auto), oldState(0), varptr(0)
{
   for( int i=0; i < LANGS; i++ )
      langPoint[i].x = langPoint[i].y = INT_MAX;
   langPoint[0] = coor;
   startInit( Box, tit );
}

Menu::Menu( menuColl *it, Point coor1, Point coor2, Lang *tit, int Vert, int Auto, int Box ) :
	    mcoll(it), vert(Vert), offsetRect(0),
	    autoOpen(Auto), oldState(0), varptr(0)
{
   for( int i=0; i < LANGS; i++ )
      langPoint[i].x = langPoint[i].y = INT_MAX;
   langPoint[0] = coor1;
   langPoint[1] = coor2;
   startInit( Box, tit );
}

void Menu::reallocScrMap()
{
  Window::reallocScrMap();
  for( int i=mcoll->getCount()-1; i>=0; i-- )
   {
     menuItem *mi = (menuItem*)mcoll->at(i);
     if ( mi->type == MENU )
	mi->menu->reallocScrMap();
   }
}

void Menu::startInit( int Box, Lang *tit )
{
   setHelpContext( "Menu" );
   keyHolder.add( &MenuKeyMap );
   box=Box;
   if ( !mcoll )
      mcoll = new menuColl;
   orig.a = rect.a = getPoint();
   if ( tit )
     {
       if ( title ) delete title;
       title = tit;
     }
   flagDelete = 0;
}

Point Menu::getPoint()
{
   int index = language;
   if ( index < 0 || index >= LANGS )
      index = 0;
   if ( langPoint[index].x == INT_MAX || langPoint[index].y == INT_MAX )
      index = 0;
   return langPoint[index];
}

Menu::~Menu()
{
  if ( mcoll ) delete mcoll;
  freeAll();
}

void Menu::fill( KeyHolder *kh )
{
   for( int i=0, end=mcoll->getCount(); i<end; i++ )
    {
      menuItem *mi = (menuItem*)mcoll->at(i);
      if ( mi->key )
	{ delete mi->key; mi->key=0; }
      if ( mi->type == MENU )
	 mi->menu->fill( kh );
      else if ( !kh )
	 continue;
      for( int j=0; j<kh->len; j++ )
       {
	 long k = kh->kmas[j]->getKey( mi->command );
	 if ( k != mi->command )
	   {
	     const char *Key = _descriptKey( k );
	     mi->key = new char[ strlen(Key)+1 ];
	     strcpy( mi->key, Key );
	     break;
	   }
       }
    }
}

int Menu::init( void *data )
{
   setRect();
   if ( current < 0 && mcoll->getCount() > 0 )
      current = 0;
   return 1;
}

void Menu::drawStatus()
{
   char *ch = 0;
   if ( mcoll->getCount() > 0 )
    {
      Lang *st = ((menuItem*)mcoll->at(current))->status;
      if ( st ) ch = st->get();
    }
   if ( appl->statusLine )
      appl->statusLine->draw( ch );
}

void Menu::beforeExec()
{
   if ( curstate!=state )
      return;
   state++; curstate++;
   redrawAppl();
}

void Menu::afterExec( long key )
{
   if ( key==HASH_cmEsc )
     { state--; curstate--; }
}

long Menu::changeCurrent( long key, int flag )
{
   menuItem *it = (menuItem*)mcoll->at(current);
   long k = 0;
   switch( key )
    {
      case HASH_cmLeft:
	  if ( !vert || flag )
	     { current = current>0 ? current-1 : mcoll->getCount()-1; key = 0; }
	  else
	      k = HASH_cmOK;
	  break;
      case HASH_cmRight:
	  if ( !vert || flag )
	     { current = current<mcoll->getCount()-1 ? current+1 : 0; key = 0; }
	  else
	      k = HASH_cmOK;
	  break;
      case HASH_cmUp:
	  if ( vert || flag )
	     { current = current>0 ? current-1 : mcoll->getCount()-1; key = 0; }
	  else
	      k = HASH_cmOK;
	  break;
      case HASH_cmDown:
	  if ( vert || flag )
	     { current = current<mcoll->getCount()-1 ? current+1 : 0; key = 0; }
	  else
	      k = HASH_cmOK;
	  break;
    }
   if ( k == HASH_cmOK && it->type == MENU )
      key = k;
   else
      setHelpContext( ((menuItem*)mcoll->at(current))->help );

   return key;
}

long Menu::handleKey( long key, void *&ptr )
{
   key = keyHolder.translate( key );

   menuItem *it;
   switch( key )
     {
       case 0:
	   it = (menuItem*)mcoll->at(current);
	   setHelpContext( it->help );
	   if ( curstate < state )
	     {
	       if ( ++curstate == state )
		   break;
	       if ( it->type == MENU )
		   goto m1;
	       else
		   state = curstate;
	     }
	   break;
       case HASH_cmHome:
	   current=key=0;
	   break;
       case HASH_cmEnd:
	   current=mcoll->getCount()-1;
	   key=0;
	   break;
       case HASH_cmLeft:
       case HASH_cmRight:
       case HASH_cmUp:
       case HASH_cmDown:
	   if ( (key=changeCurrent( key )) != HASH_cmOK )
	      break;
       case HASH_cmOK:
	   it = (menuItem*)mcoll->at(current);
m2:        if ( it->type == COMMAND )
	       key = it->command;
	   else
	     {
m1:            key = exec( it->menu );
	       switch( key )
		{
		  case HASH_cmEsc:
		      key=0;
		      break;
		  case HASH_cmLeft:
		  case HASH_cmRight:
		  case HASH_cmUp:
		  case HASH_cmDown:
		      changeCurrent( key, 1 );
		      state--; curstate--;
		      it = (menuItem*)mcoll->at(current);
		      if ( it->type == MENU )
			 goto m1;
		      else
			 key=0;
		}
	     }
	   break;
       default:
	   if ( key>31 && key<256 )
	    {
	      int lowkey = lowercase( key );
	      for( int i=mcoll->getCount()-1; i>=0; i-- )
	       {
		it = (menuItem*)mcoll->at(i);
		if ( it->hotkey() == 0 )
		   continue;
		if ( it->hotkey() == lowkey )
		   { current = i; key=HASH_cmOK; goto m2; }
	       }
	      key = 0;
	    }
     }
   if ( !key )
      redrawAppl();
   return key;
}

void Menu::changeRect( int offset )
{
   offsetRect = offset;
   for( int i=mcoll->getCount()-1; i>=0; i-- )
    {
      menuItem *mi = (menuItem*)mcoll->at(i);
      if ( mi->type == MENU )
	 mi->menu->changeRect( offsetRect );
    }
}

void Menu::setRect()
{
   int i, l=0, end;
   width=0;
   for( i=0, end=mcoll->getCount(); i<end; i++ )
    {
      menuItem *mi = (menuItem*)mcoll->at(i);
      char *ch = mi->label->get();
      if ( ch )
	 for( l=0; *ch; ch++ )
	    if ( *ch != '~' )
	       l++;
      if ( mi->key )
	 l += 3 + strlen( mi->key );
      width = max( width, l + (vert ? 1 : 2) );
      if ( mi->type == MENU )
	 mi->menu->setRect();
    }
   int dY = (vert ? end-1 : 0) + (box ? 2 : 0);
   int dX = (vert ? width : width * end) + (box ? 2 : 0);
   orig.a = rect.a = getPoint();
   rect.a.x += offsetRect;
   rect.b.y = rect.a.y + dY;
   rect.b.x = rect.a.x + dX;
   orig.b.y = orig.a.y + dY;
   orig.b.x = orig.a.x + dX;

   size = Point( rect.b - rect.a + Point(1,1) );
   if ( cursor.y>=size.y-2 ) cursor.y=size.y-3;
   if ( cursor.x>=size.x-2 ) cursor.x=size.x-3;
   char *sc=0;
   for( int y=0; y < scr.lines; y++ )
    {
       memset( (sc=scr.orig[y]), 0, scr.maplen );
       if ( y >= rect.a.y && y <= rect.b.y )
	 for( int ind, x=rect.a.x; x <= rect.b.x; x++ )
	  {
	    if ( (ind = x>>3) < 0 || ind >= scr.maplen )
	       break;
	    sc[ind] |= 0x80 >> (x & 0x7);
	  }
    }
   setScrMap();
}

int Menu::draw( int Redraw, int sy, int sx )
{
   setColors( colorsType );
   char **saveMap = Screen::currentMap;
   Screen::currentMap = scr.map;
   Screen::Clear( rect, clr[1] );
   static short str[256];
   short nC = (clr[1]<<8) & 0x7f00;
   short aC = (clr[0]<<8) & 0x7f00;
   short hnC = (clr[3]<<8) & 0x7f00;
   short haC = (clr[2]<<8) & 0x7f00;
   Point coor = rect.a;
   if ( box )
      coor += Point(1,1);
   for( int i=0, l=0; i < mcoll->getCount(); i++ )
    {
       int len=0, flag=1, attr = (active && current==i ? aC : nC);
       int curattr=attr, space = attr | ' ';
       for( l=0; l <= width; str[l++]=space );
       menuItem *mi = (menuItem*)mcoll->at(i);
       char *ch = mi->label->get();
       for( l=1; *ch && l<width; ch++ )
	 {
	   if ( *ch=='~' )
	     { curattr = active && current==i ? haC : hnC; continue; }
	   str[l++] = curattr | (unsigned char)*ch;
	   curattr = attr;
	 }
       if ( !vert )
	  str[l++] = space;
       else if ( mi->key )
	{
	  char *k = mi->key + strlen(mi->key);
	  for( l=width-1; --k >= mi->key; )
	     str[l--] = curattr | (unsigned char)*k;
	}
       Screen::putLimited( coor, (unsigned short*)str, vert ? width+1 : l );
       if ( vert )
	  coor.y++;
       else
	  coor.x += l;
    }
   if ( box )
      Screen::box( rect, 0, clr[1] );
   Screen::currentMap = saveMap;

   if ( active )
     {
       menuItem *mi = (menuItem*)mcoll->at(current);
       if ( mi->type==MENU && mi->menu->autoOpen )
	  appl->drawWindow( mi->menu );
     }
   Screen::hideCursor();
   return 1;
}

void Menu::moveCursor()
{
   Screen::move( Screen::Lines-1, Screen::Columns-1 );
}

void Menu::setHotKeys( char *mas )
{
   int i=0;
   for( ; i<mcoll->getCount(); i++ )
      mas[i] = ((menuItem*)mcoll->at(i))->hotkey();
   mas[i] = 0;
}

/*--------------------- class MenuManager ------------------------*/

MenuManager::MenuManager() : Window( Rect(0,0,0,Screen::Columns-1) ),
			offset(2), canSwitch(1), hotKey(0), Visible(1)
{
   shouldDelete = 0;
   keyHolder.add( &MenuKeyMap );
   sepchar = Screen::_VL | GraphAttr;
   hotKeys[0] = 0;
   flagDelete = 0;
}

void MenuManager::reallocScrMap()
{
  Window::reallocScrMap();
  for( int i=0; i < count; i++ )
     ((Menu*)items[i])->reallocScrMap();
}

int MenuManager::isType(long typ)
{
   return (typ==HASH_Menu ? 1 : Window::isType(typ));
}

int MenuManager::init( void *data )
{
   visible = Visible;
   setScrMap();
   return 1;
}

int MenuManager::isHotKey( long key )
{
   if ( !ISKEY(key) || !ISFUNC1(key) || ONEKEY(key) >= 32 )
      return 0;
   int k = lowercase( ONEKEY(key) + 64 );
   return strchr( hotKeys, k ) ? k : 0;
}

void MenuManager::select( Menu *m, int index )
{
   if ( !m || active )
      return;
   ccIndex i = indexOf( m );
   if ( i >= 0 )
       current = i;
   else
     {
       if ( index < 0 || index > count )
	  index = count;
       atInsert( index, m );
       current = index;
       offset += strlen( (char*)m->winName() ) + 1;
       for( i=0; i<count; i++ )
	  ((Menu*)items[i])->changeRect( offset );
       init();
     }
   ((Menu*)items[current])->setHotKeys( hotKeys );
}

void MenuManager::remove( Menu *m )
{
   ccIndex i = indexOf( m );
   if ( i < 0 || i == current && active )
      return;
   atRemove( i );
   if ( i == current )
      current = current>0 ? current-1 : count-1;
   offset -= strlen( (char*)m->winName() ) + 1;
   for( i=0; i<count; i++ )
      ((Menu*)items[i])->changeRect( offset );
   if ( current >= 0 && current < count )
      ((Menu*)items[current])->setHotKeys( hotKeys );
   init();
}

long MenuManager::handleKey( long key, void *&ptr )
{
   if ( count<=0 )
      return key;
   key = keyHolder.translate( key );
m1:
   Menu::curstate=0;
   Menu *m = (Menu*)items[current];
   if ( !hotKey )
       Menu::state = m->oldState;
   else
     {
       Menu::state = 2;
       for( int i=m->mcoll->getCount()-1; i>=0; i-- )
	 if ( hotKey == ((menuItem*)m->mcoll->at(i))->hotkey() )
	   { m->current = i; break; }
       hotKey = 0;
     }
   key = exec( m );
   switch( key )
    {
      case HASH_cmNext:
	  m->oldState = Menu::state;
	  if ( !canSwitch )
	     goto m1;
	  current = current < count-1 ? current+1 : 0;
	  ((Menu*)items[current])->setHotKeys( hotKeys );
	  draw();
	  goto m1;
      case HASH_cmPrev:
	  m->oldState = Menu::state;
	  if ( !canSwitch )
	     goto m1;
	  current = current > 0 ? current-1 : count-1;
	  ((Menu*)items[current])->setHotKeys( hotKeys );
	  draw();
	  goto m1;
    }
   m->oldState = Menu::state;
   retCommand = key;
   return key;
}

void MenuManager::beforeExec()
{
   visible = 1;
}

void MenuManager::afterExec( long key )
{
   visible = Visible;
   Window::afterExec( key );
}

int MenuManager::draw( int Redraw, int sy, int sx )
{
   if ( !Screen::isOpen || !visible )
      return 0;
   static int Language = language;
   int i;
   if ( Language != language )
    {
      offset = 2;
      for( i=0; i < count; i++ )
       {
	 Menu *m = (Menu*)items[i];
	 offset += strlen( (char*)m->winName() ) + 1;
	 m->changeRect( 2 );
       }
      for( i=0; i < count; i++ )
	 ((Menu*)items[i])->changeRect( offset );
      Language = language;
    }

   for( i=0; i < count; i++ )
      ((Menu*)items[i])->setRect();

   setColors( colorsType );
   int normColor = (clr[1]<<8) & 0x7f00;
   int curColor =  (clr[3]<<8) & 0x7f00;
   char **saveMap = Screen::currentMap;
   Screen::currentMap = scr.map;
   Screen::ClearLine( 0, clr[1] );
   if ( count<=0 )
     { Screen::currentMap=saveMap; return 1; }
   static unsigned short str[256];
   str[0] = normColor | ' ';
   int len = 1;
   for( int pos=0; pos < count; pos++ )
    {
      char *it = (char*)((Menu*)items[pos])->winName();
      int attr = pos==current ? curColor : normColor;
      for( int j=0; *it && len+1<=size.x; j++, it++ )
	 str[len++] = attr | (unsigned char)*it;
      str[len++] = normColor | ' ';
    }
   str[len++] = normColor | (unsigned char)sepchar | GraphAttr;
   Screen::putLimited( 0, 0, str, len );
   Screen::currentMap = saveMap;
   if ( !active )
     {
	Menu *m = (Menu*)items[current];
	if ( !m->box )
	   m->draw( Redraw );
     }
   return 1;
}

void MenuManager::setScrMap()
{
   memset( scr.map[0], 0xff, scr.maplen );
}

void MenuManager::moveCursor()
{
   Screen::move( Screen::Lines-1, Screen::Columns-1 );
}

/*---------------------- class StatusLine ------------------------*/

unsigned char monoStatus[2] =
{
   FG_BLACK | BG_WHITE,		// normal
   FG_HI_WHITE | BG_WHITE	// light
};

unsigned char colorStatus[2] =
{
   FG_BLACK | BG_WHITE,		// normal
   FG_RED | BG_WHITE		// light
};

unsigned char laptopStatus[2] =
{
   FG_WHITE | BG_BLACK,		// normal
   FG_HI_WHITE | BG_BLACK	// light
};

StatusLine::StatusLine() : Window( Rect(Screen::Lines-1,0,Screen::Lines-1,Screen::Columns-1) ),
		visible(1)
{
   box=0;
}

int StatusLine::isType( long typ )
{
   return (typ==HASH_StatusLine ? 1 : Window::isType(typ));
}

void StatusLine::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoStatus;
	  break;
      case COLOR:
	  clr = colorStatus;
	  break;
      case LAPTOP:
	  clr = laptopStatus;
    }
}

int StatusLine::init( void *data )
{
   setScrMap();
   return 1;
}

void StatusLine::putStr( const char *str )
{
   if ( str == NULL )
       return;
   for( int i=0, lightFlag=0, tilda=0; str[i] && i-tilda < scr.cols-1; i++ )
       if ( str[i] == '~' )
	  { tilda++; lightFlag=!lightFlag; }
       else
	   Screen::put( str[i], lightFlag ? clr[1] : clr[0] );
}

int StatusLine::draw( char *str, int ignoreVisible )
{
   if ( !Screen::isOpen || !ignoreVisible && !visible )
      return 0;

   char **saveMap = Screen::currentMap;

   Screen::currentMap = scr.map;
   Screen::ClearLine( Screen::Lines-1, clr[0] );
   Screen::move( Screen::Lines-1, 0 );

   putStr( language == 0 ? " ~F1~-help" : " ~F1~-помощь" );
   if ( str ) {
       putStr( "   " );
       putStr( str );
   }
   Screen::currentMap = saveMap;
   return 1;
}

void StatusLine::setScrMap()
{
  memset( scr.map[scr.lines-1], 0xff, scr.maplen );
}
