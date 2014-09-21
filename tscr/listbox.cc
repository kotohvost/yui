/*
	$Id: listbox.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <stdio.h>
#include <string.h>
#include <reg_expr.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "listbox.h"
#include "dialog.h"
#include "hashcode.h"

unsigned char monoList[11] = {
	FG_HI_WHITE | BG_WHITE,    // active frame (WINDOW)
	FG_BLACK | BG_WHITE,        // normal frame (WINDOW)
	FG_WHITE | BG_BLACK,	    // item, object is active
	FG_WHITE | BG_BLACK,	    // item, not active
	FG_HI_WHITE | BG_BLACK,    // selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // selected item, not active
	FG_HI_WHITE | BG_BLACK,    // current item, object is active
	FG_HI_WHITE | BG_BLACK,    // current item, not actve
	FG_HI_WHITE | BG_BLACK,    // current selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // current selected item, not actve
	FG_HI_WHITE | BG_BLACK	    // info
};

unsigned char colorList[11] = {
	FG_HI_WHITE | BG_WHITE,    // active frame (WINDOW)
	FG_BLACK | BG_WHITE,        // normal frame (WINDOW)
	FG_BLACK | BG_CYAN,         // item, object is active
	FG_BLACK | BG_CYAN,         // item, not active
	FG_HI_YELLOW | BG_CYAN,    // selected item, object is active
	FG_HI_YELLOW | BG_CYAN,    // selected item, not active
	FG_BLACK | BG_GREEN,        // current item, object is active
	FG_HI_WHITE | BG_CYAN,     // current item, not actve
	FG_HI_YELLOW | BG_GREEN,   // current selected item, object is active
	FG_HI_YELLOW | BG_CYAN,    // current selected item, not actve
	FG_HI_WHITE | BG_BLUE      // info
};

unsigned char laptopList[11] = {
	FG_HI_WHITE | BG_WHITE,    // active frame (WINDOW)
	FG_BLACK | BG_WHITE,        // normal frame (WINDOW)
	FG_WHITE | BG_BLACK,        // item, object is active
	FG_WHITE | BG_BLACK,        // item, not active
	FG_HI_WHITE | BG_BLACK,    // selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // selected item, not active
	FG_HI_WHITE | BG_BLACK,    // current item, object is active
	FG_HI_WHITE | BG_BLACK,    // current item, not actve
	FG_HI_WHITE | BG_BLACK,    // current selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // current selected item, not actve
	FG_HI_WHITE | BG_BLACK     // info
};

unsigned char monoInput[4] = {
	FG_WHITE | BG_BLACK,		// active
	FG_WHITE | BG_BLACK,		// normal
	FG_HI_WHITE | BG_BLACK,		// active code
	FG_HI_WHITE | BG_BLACK		// normal code
};

unsigned char colorInput[4] = {
	FG_HI_WHITE | BG_BLUE,
	FG_HI_WHITE | BG_CYAN,
	FG_WHITE | BG_BLUE,
	FG_WHITE | BG_CYAN
};

unsigned char laptopInput[4] = {
	FG_WHITE | BG_BLACK,
	FG_WHITE | BG_BLACK,
	FG_HI_WHITE | BG_BLACK,		// active code
	FG_HI_WHITE | BG_BLACK		// normal code
};

long listBoxKeyMap[] = {
	kbHome,		HASH_cmHome,
	kbEnd,		HASH_cmEnd,
	kbPgDn,		HASH_cmPgDn,
	kbPgUp,		HASH_cmPgUp,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbIns,		HASH_cmIns,
	'+',		HASH_cmSetMask,
	'-',		HASH_cmUnsetMask,
	'*',		HASH_cmInverse,
	kbCtrlB,	HASH_cmMode,
	0
};

long inputLineKeyMap[] = {
	kbHome,		HASH_cmHome,
	kbEnd,		HASH_cmEnd,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbCtrlY,	HASH_cmClear,
	kbDel,		HASH_cmDelete,
	kbBS,		HASH_cmBackspace,
	kbIns,		HASH_cmIns,
	FUNC1('+'),	HASH_cmCopyToClip,
	FUNC1(kbIns),	HASH_cmCopyFromClip,
	kbCtrlB,	HASH_cmMode,
	0
};

Keymap ListBoxKeyMap( listBoxKeyMap );
Keymap InputLineKeyMap( inputLineKeyMap );

#ifndef DJGPP
int inputLineFillChar=32;
#else
int inputLineFillChar=176;
#endif

listItem::listItem( const char *Item, const char *Info, int State ) :
	item(0), info(0), state(State)
{
  if ( Item )
    item = strdup( Item );
  if ( Info )
    info = strdup( Info );
}

listItem::~listItem()
{
  if ( item )
    ::free( item );
  if ( info )
    ::free( info );
}

void listBox::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoList;
	  break;
      case COLOR:
	  clr = colorList;
	  break;
      case LAPTOP:
	  clr = laptopList;
    }
};

void inputLine::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoInput;
	  break;
      case COLOR:
	  clr = colorInput;
	  break;
      case LAPTOP:
	  clr = laptopInput;
    }
}

int  listBox::isType(long typ)
{
  return (typ==HASH_listBox ? 1 : Window::isType(typ));
}

listBox::listBox( Rect r, int Width, int *res, Collection *Items,
	    Collection *State, Collection *Info, int cur, int Delta ) :
	    Window( r ), width(Width),
	    infoLines(0), result(res),
	    countIns(1), manualPos(0), mask(0)
{
   setHelpContext( "ListBox" );
   keyHolder.add( &ListBoxKeyMap );
   separator = Screen::_VL | GraphAttr;
   mask = (char*)malloc( 256 * sizeof(char) );
   if ( mask )
      strcpy( mask, "*" );
   newItems( Items, State, Info, cur, Delta );
   if ( width < 1 )
      width = 1;
   setColors( colorsType );
}

listBox::~listBox()
{
  freeAll();
  if ( mask )
    ::free( mask );
}

int listBox::init( void *data )
{
   Window::init();
   if ( isGetObj )
      box = 0;
   int siz = size.x - (box ? 2 : 0);
   columns = siz / (width + 1) + ((siz % (width+1)) ? 1 : 0);
   if ( columns < 1 )
      columns = 1;

   lines = rect.b.y - rect.a.y - (box?2:0) + 1;
   scrItems = lines * columns;

   if ( current >= count )
      current = count-1;
   if ( current < 0 && count > 0 )
      current = 0;
   if ( current >= 0 )
     {
       if ( current < delta.y ) delta.y = current;
       if ( current-delta.y >= scrItems ) delta.y = current - scrItems + 1;
     }
   if ( delta.y < 0 ) delta.y=0;

   return 1;
}

Point listBox::getCursor( int *hide )
{
   Point p(0,0);
   if ( box ) { p.y=p.x=1; }
   if ( current >= 0 )
    {
      int cur=current-delta.y, col=cur/lines;
      p.y += cur%lines;
      p.x += col*width+col+manualPos;
      if ( hide && p.x > size.x-(box?2:0) )
	 *hide = 1;
    }
   return rect.a + p;
}

void listBox::moveCursor()
{
   int hide=0;
   Point p = getCursor( &hide );
   if ( hide )
       Screen::hideCursor();
   else
       Screen::move( p );
}

long listBox::handleKey( long key, void *&ptr )
{
   long Key = keyHolder.translate( key );

   int *p, i;
   if ( Key>31 && Key<256 && count > 0 )
    {
      int cmp = manualPos ? current : 0;
      char *it = ((listItem*)items[cmp])->item;
      for( i=0; i<count; i++ )
       {
	if ( cmpItem( ((listItem*)items[i])->item, it, Key, manualPos) )
	   {
	      if ( owner )
		 owner->bindCursor=1;
	      current=i;
	      manualPos++;
	      if ( current<delta.y )
		 delta.y = current;
	      else if ( current >= delta.y+scrItems )
		 delta.y = max( current-scrItems+1, 0 );
	      break;
	   }
       }
      if ( i>=count )
	 Screen::beep();
      goto end;
    }

   manualPos = 0;
   if ( owner && !owner->hardMode )
      return key;

   switch( Key )
    {
     case HASH_cmLeft:
	 if ( count > 0 )
	    current = max( current-lines, 0 );
	 if ( current<delta.y )
	    delta.y = max( delta.y-lines, 0 );
	 Key=0; break;
     case HASH_cmUp:
	 if ( current<=0 )
	    return 0;
	 if ( --current < delta.y )
	    delta.y--;
	 Key=0; break;
     case HASH_cmPgUp:
	 delta.y = max( delta.y-scrItems, 0 );
	 if ( count > 0 )
	    current = max( current-scrItems, 0);
	 Key=0; break;
     case HASH_cmRight:
	 if ( count <= 0 ) return 0;
	 current = min( current+lines, getCount()-1 );
	 if ( current >= delta.y+scrItems )
	    delta.y += lines;
	 Key=0; break;
     case HASH_cmDown:
m1:
	 if ( current+1 >= count )
	    return 0;
	 if ( ++current >= delta.y+scrItems )
	    delta.y++;
	 Key=0; break;
     case HASH_cmPgDn:
	 if ( count <= 0 ) return 0;
	 current = min( current+scrItems, getCount()-1 );
	 if ( delta.y+scrItems <= getCount()-1 )
	    delta.y = min( delta.y+scrItems, getCount()-1 );
	 Key=0; break;
     case HASH_cmHome:
	 moveHome();
	 Key=0; break;
     case HASH_cmEnd:
	 if ( count <= 0 ) return 0;
	 moveEnd();
	 Key=0; break;
     case HASH_cmInverse:
	 countIns = 1;
	 for( i=0; i<count; i++ )
	  {
	    p = &((listItem*)items[i])->state;
	    *p = *p ? 0 : countIns++;
	  }
	 Key=0; break;
     case HASH_cmIns:
	 if ( count <= 0 )
	    return 0;
	 p = &((listItem*)items[current])->state;
	 if ( *p >= 0 )
	  {
	    if ( !*p )
	       *p = countIns++;
	    else
	     {
	       decrementState( *p );
	       *p = 0;
	     }
	  }
	 goto m1;
     case HASH_cmSetMask:
	 newMask( 1 );
	 redrawAppl();
	 Key=0; break;
     case HASH_cmUnsetMask:
	 newMask( 0 );
	 redrawAppl();
	 Key=0; break;
     case HASH_cmOK:
	 accept();
     default:
	 return Key;
    }
end:
   if ( !Key && !isGetObj && draw() )
     { MoveCursor(); Screen::sync(); }
   return 0;
}

void listBox::moveHome()
{
   delta.y = 0;
   current = count > 0 ? 0 : -1;
}

void listBox::moveEnd()
{
   current = count-1;
   if ( current >= delta.y+lines )
      delta.y = max( current-scrItems+1, 0 );
}

int listBox::draw( int Redraw, int sy, int sx )
{
   if ( !Window::draw( Redraw ) )
      return 0;
   Point coor = rect.a + Point(sy,sx);
   if ( box )
     { coor.y++; coor.x++; }
   if ( !isGetObj ) {
      Screen::currentMap = scr.bord;
      Screen::Clear( coor.y, coor.x, size.y-(box?2:0), 1, active ? clr[2] : clr[3] );
   }
   normColor = ((active || bind_flag ? clr[2] : clr[3])<<8) & 0x7f00;
   selColor = ((active || bind_flag ? clr[4] : clr[5])<<8) & 0x7f00;
   curColor = ((active || bind_flag ? clr[6] : clr[7])<<8) & 0x7f00;
   curSelColor = ((active || bind_flag ? clr[8] : clr[9])<<8) & 0x7f00;
   int len, i, l;
   for( i=delta.y, l=0; l < lines; i++, l++, coor.y++ ) {
      unsigned short *str = scrLine( i, len );
      Screen::putLimited( coor, str, len );
   }

   char *s=0;
   static char inf[256];
   if ( infoLines <= 0  )
      goto end;

   Screen::Clear( coor, infoLines, size.x, clr[10] );

   if ( current<0 || current>=count || !(s=((listItem*)items[current])->info) )
      goto end;
   for( i=0, l=0; s[i]; i++ ) {
      if ( s[i] != '\n' )
	{ inf[l++]=s[i]; if ( l<size.x ) continue; }
      inf[l]=0;
      Screen::putLimited( coor.y++, coor.x, (const u_char*)inf, size.x, clr[10] );
      l=0;
   }
   inf[l]=0;
   Screen::putLimited( coor.y++, coor.x, (const u_char*)inf, size.x, clr[10] );

end:
   if ( !isGetObj )
      Screen::currentMap = Screen::ScreenMap;

   return 1;
}

unsigned short *listBox::scrLine( int line, int &len )
{
   len = 0;
   static unsigned short screenStr[256];
   unsigned short attr;
   unsigned char *it, symbol;
   int scr_width = size.x - (box ? 2:0);
   for( int pos=line, col=0; col < columns; pos+=lines, col++ ) {
      it = (unsigned char*)(pos<count ? ((listItem*)items[pos])->item : "");
      if ( pos < count && ((listItem*)items[pos])->state )
	  attr = (pos==current ? curSelColor : selColor);
      else
	  attr = (pos==current ? curColor : normColor);
      for( int j=0; len+1<=size.x && j<width; j++ ) {
	  symbol = translate_out( *it );
	  screenStr[len++] = attr | (symbol ? symbol : ' ');
	  if ( *it ) it++;
      }
      if ( len <= scr_width-1 )
	  screenStr[len++] = normColor | (unsigned char)separator | GraphAttr;
   }
   return screenStr;
}

ccIndex listBox::add( const char *str, const char *Info, int State )
{
   if ( !str ) return -1;
   ccIndex ret = insert( new listItem( str, Info, State ) );
   if ( current < 0 )
      current = 0;
   if ( State > 0 )
      countIns++;
   if ( !Info )
      return ret;

   int len = strlen( Info )+1;
   Rect grect = rect;
   if ( !isGetObj )
      grect.grow(-1,-1);
   int ncount = 1, pos = 0, w = grect.b.x - grect.a.x + 1;
   for( int i=0; i<len; i++, pos++ )
    {
      if ( Info[i]=='\n' )
	 { ncount++; pos=0; continue; }
      if ( pos > w )
	 { ncount++; pos=0; }
    }
   if ( ncount > infoLines )
      infoLines = ncount;

   return ret;
}

int listBox::cmpItem( const char *str1, const char *str2, int ch, int pos )
{
  if ( (unsigned char)(str1[pos]) != ch )
     return 0;
  return !memcmp( str1, str2, pos );
}

void listBox::newItems( Collection *Items, Collection *State,
			Collection *Info, int cur, int Delta )
{
   freeAll();
   manualPos = 0;
   if ( Items )
     for( int i=0, end=Items->getCount(); i<end; i++ )
       add( (const char*)Items->at(i),
	    Info && i<Info->getCount() ? (const char*)Info->at(i) : 0,
	    State && i<State->getCount() ? *(int*)State->at(i) : 0 );
   current = cur;
   if ( current < 0 )
      current = 0;
   if ( current >= count )
      current = count-1;
   if ( Delta>=0 )
      delta.y = Delta;
   if ( current >= 0 )
     {
       if ( current < delta.y ) delta.y = current;
       if ( current-delta.y > scrItems ) delta.y = current - scrItems + 1;
     }
   if ( delta.y < 0 ) delta.y=0;
}

char *listBox::getItem( ccIndex index )
{
   if ( index < 0 || index >= count )
      return 0;
   return ((listItem*)items[index])->item;
}

int listBox::setCurrent( ccIndex newcur )
{
   if ( newcur < 0 || newcur >= count )
      return 0;
   current = newcur;
   if ( current < delta.y )
      delta.y = current;
   else if ( current >= delta.y+lines )
      delta.y = max( current-scrItems+1, 0 );
   return 1;
}

void listBox::decrementState( int s )
{
   if ( countIns > 1 )
      countIns--;
   for( int *is=0, i=count-1; i >= 0; i-- )
    {
      is = &((listItem*)items[i])->state;
      if ( *is >= s && *is > 0 )
	 (*is)--;
    }
}

void listBox::freeItem( void *item )
{
  if ( item )
    delete (listItem*)item;
}

int listBox::newMask( int type )
{
  if ( !boxMask( (char*)(type ? " Set " : " Unset ") ) )
     return 0;

  char *fmask = new char[1024];
  Regexpr *ex = new Regexpr;
  re_registers *regs = new re_registers;

  fmask[0] = '^';
  char *m = mask;
  int i, ret=0;
  for( i=1; *m; m++ )
     switch( *m )
      {
	  case '*': fmask[i++]='.'; fmask[i++]='*'; break;
	  case '?': fmask[i++]='.'; break;
	  case '.': fmask[i++]='\\'; fmask[i++]='.'; break;
	  default:
	     fmask[i++] = *m;
      }
  fmask[i++]='$';
  fmask[i]=0;
  char *cp = ex->compile( fmask );
  if ( !cp )
   {
     for( i=0; i<count; i++ )
      {
	char *str = ((listItem*)items[i])->item;
	int len = strlen( str );
	int a = ex->search( str, len, 0, len, regs );
	if ( a != -1 )
	 {
	   int *s = &((listItem*)items[i])->state;
	   if ( !type )
	    {
	      decrementState( *s );
	      *s = 0;
	    }
	   else if ( *s == 0 )
	      *s = countIns++;
	 }
      }
     ret = 1;
   }

  delete fmask;
  delete ex;
  delete regs;

  return ret;
}

int listBox::boxMask( char *tit )
{
   int w = max( width, 7 );
   maskRect.a.y = rect.a.y + (size.y>>1);
   maskRect.b.y = maskRect.a.y + 2;
   maskRect.a.x = rect.a.x + ((size.x-w) >> 1);
   maskRect.b.x = maskRect.a.x + w + 1;
   if ( isGetObj )
     {
       Point d( owner->delta.y, owner->delta.x );
       maskRect.a += owner->rect.a - d;
       maskRect.b += owner->rect.a - d;
     }
   else
     {
       maskRect.a -= Point(1,1);
       maskRect.b -= Point(1,1);
     }
   Dialog *d = new Dialog( maskRect, new Lang(tit) );
   d->redrawAfterExec = 0;
   inputLine *il = new inputLine( Point(0,0), mask, mask, w, 128 );
   d->insert( il );
   long ret = d->exec( isGetObj ? owner->father : father, Executor );
   delete d;
   return HASH_cmOK == ret;
}

int listBox::setWidth( int w )
{
  int oldw = width;
  if ( w > 0 )
     width = w;
  return oldw;
}

/*---------------------------- class inputLine --------------------------*/

int  inputLine::isType(long typ)
{
   return (typ==HASH_inputLine ? 1 : getobj::isType(typ));
}

inputLine::inputLine( Rect r, const char *s, char *res, int l,
		      const char *reg, Collection *hist, int fchar,
		      unsigned pass ) :
	str(0), origStr(0), origP(-1,-1),
	max_len(l), delta(0), pos(0),
	insMode(1), fillChar(fchar),
	result(res), password(pass), regex(0), len(l), history(hist), clearFlag(1)
{
   setColors( colorsType );
   keyHolder.add( &InputLineKeyMap );
   rect = r;
   firstkey=1;
   if ( s )
      origStr=strdup(s);
   if ( reg && strlen(reg) > 0 )
      regex=strdup(reg);
   origScrLen = rect.b.x - rect.a.x + 1;
}

inputLine::inputLine( Point p, const char *s, char *res, int lscr, int l,
		      const char *reg, Collection *hist, int fchar,
		      unsigned pass ) :
	str(0), origStr(0), origP(p),
	max_len(l), origScrLen(lscr), lines(1), delta(0), pos(0),
	insMode(1), fillChar(fchar),
	result(res), password(pass), regex(0), len(l), history(hist), clearFlag(1)
{
   setColors( colorsType );
   keyHolder.add( &InputLineKeyMap );
   firstkey=1;
   if ( s )
      origStr=strdup(s);
   if ( reg && strlen(reg) > 0 )
      regex=strdup(reg);
}

inputLine::~inputLine()
{
   if ( str ) ::free( str );
   if ( origStr ) ::free(origStr);
   if ( regex ) ::free(regex);
}

int inputLine::init( void *data )
{
   if ( origP.y >= 0 )
       rect = Rect( origP.y, origP.x, origP.y, origP.x + origScrLen - (history ? 0 : 1) );
   if ( !str ) {
       initString( origStr ? origStr : "" );
       origStr = 0;
   }
   width = rect.b.x - rect.a.x + (history ? 0 : 1);
   lines = rect.b.y - rect.a.y + 1;
   scrlen = lines*width;
   return 1;
}

int inputLine::initString( const char *s )
{
   if ( s ) {
       int l = strlen( s );
       int ml = max( l, len );
       if ( str == NULL || ml != max_len ) {
	   max_len = ml;
	   str = (char*)realloc( str, max_len + 1 );
	   memset( str, 0, max_len + 1 );
       }
       memcpy( str, s, l );
       str[l] = 0;
   } else {
       max_len = len;
       str = (char*)calloc( len + 1, 1 );
   }
   moveHome();
   return 1;
}

int inputLine::draw( int Redraw, int sy, int sx )
{
   Point coor = rect.a + Point(sy,sx);
   int attr = active || bind_flag ? clr[0] : clr[1];
   int attrCode = active || bind_flag ? clr[2] : clr[3];
   int attrHist = active || bind_flag ? clr[1] : clr[0];
   Screen::Clear( coor.y, coor.x, lines, width, attr, fillChar );

   for( int i=0, d=delta; i<lines; i++, d+=width, coor.y++ ) {
       if ( d > max_len || !str[d] ) {
	   break;
       } else if ( coor.y >= 0 && coor.y < Screen::Lines ) {
	   if ( !password ) {
	       int symbol=0;
	       for( int j=d, w=0, pos=coor.x, l=strlen(str); j<l && w<width; j++, w++ ) {
		   symbol = translate_out( (int)(unsigned char)str[j] );
		   Screen::put( coor.y, pos++, symbol, symbol >= 0 && symbol < ' ' ? attrCode : attr );
	       }
	   } else {
	       for( int j=d, w=0, pos=coor.x, l=strlen(str); j<l && w<width; j++, w++ )
		   Screen::put( coor.y, pos++, '*' );
	   }
       }
   }

   if ( history ) {
       coor.y = rect.a.y + lines - 1 + sy;
       coor.x = coor.x+rect.b.x-rect.a.x;
       if ( coor.y >= 0 && coor.y < Screen::Lines )
	  Screen::put( coor.y, coor.x, '#', attrHist );
   }

   return 1;
}

Point inputLine::getCursor( int *hide )
{
   Point ret = rect.a;
   if ( width >0 )
      ret += Point( (pos-delta)/width, (pos-delta)%width );
   if ( ret.x < rect.a.x )
      ret.x = rect.a.x;
   return ret;
}

long inputLine::handleKey( long key, void *&ptr )
{
   long Key = key;
   key = keyHolder.translate( Key );

   if ( owner && !owner->hardMode )
    {
      if ( history && (key == ' '|| key==kbEnter) && owner->curPos.x == rect.b.x )
	{ runHistory(); return 0; }
      pos = delta + (owner->curPos.y-rect.a.y)*width + owner->curPos.x - rect.a.x;
    }
   if ( key>31 && key<256 || Key == ' ' )
    {
      if ( pos >= len )
	 return 0;
      if ( Key == ' ' ) key = ' ';
      if ( firstkey && clearFlag )
	{
	  memset( str, 0, len+1 );
	  moveHome();
	}
      addChar( key );
      key=0;
      if ( owner )
	 owner->bindCursor=1;
      firstkey=0;
    }
   else if ( key )
      firstkey=0;

   switch( key )
    {
      case HASH_cmIns:
	  insMode=!insMode;
	  key=0; break;
      case HASH_cmDelete:
	  deleteChar();
	  key=0; break;
      case HASH_cmBackspace:
	  backspace();
	  if ( pos<delta ) delta--;
	  if ( owner && !owner->hardMode ) owner->bindCursor=1;
	  key=0; break;
      case HASH_cmCopyToClip:
	  copyToClip();
	  key=0; break;
      case HASH_cmCopyFromClip:
	  copyFromClip();
	  key=0; break;
      case HASH_cmClear:
	  clearStr();
	  moveHome();
	  key=0; break;
    }

   if ( owner && !owner->hardMode )
      return key;

   switch( key )
    {
      case HASH_cmUp:
	  if ( width > 0 && (pos-delta)/width > 0 )
	    { pos-=width; key=0; }
	  break;
      case HASH_cmDown:
	  if ( width > 0 )
	   {
	     if ( (pos-delta)/width + 1 < lines )
	       { pos+=width; key=0; }
	     else if ( history )
	       { runHistory(); key=0; }
	   }
	  break;
      case HASH_cmLeft:
	  moveLeft();
	  key=0; break;
      case HASH_cmRight:
	  moveRight();
	  key=0; break;
      case HASH_cmHome:
	  moveHome();
	  key=0; break;
      case HASH_cmEnd:
	  moveEnd();
	  key=0; break;
    }
   return key;
}

void inputLine::clearStr()
{
   memset( str, 0, len );
}

void inputLine::addChar( int ch )
{
   int i;
   for( i=pos-1; i>=0 && str[i]==0; str[i--]=' ' );
   if ( insMode )
       for( i=len-1; i>0 && i>pos; i-- )
	   str[i]=str[i-1];
   str[pos] = translate_in( (int)(unsigned int)ch );
   if ( pos<len-1 )
       if ( ++pos-delta >= scrlen )
	   delta++;
}

void inputLine::deleteChar()
{
   for( int i=pos+1; i<len; i++ )
      str[i-1] = str[i];
   str[len-1] = 0;
}

void inputLine::backspace()
{
   if ( pos <= 0 )
      return;
   for( int i=pos--; i<len; str[i-1] = str[i++] );
   str[len-1]=0;
}

void inputLine::moveLeft()
{
   if ( pos > 0 && --pos < delta )
      delta--;
}

void inputLine::moveRight()
{
   if ( pos < len-1 && ++pos-delta >= scrlen )
      delta++;
}

void inputLine::moveEnd()
{
   for( pos=len-1; pos>0 && str[pos-1]==0; pos-- );
     if ( pos-delta >= scrlen )
	delta = pos-scrlen+1;
}

char *inputLine::valid( void *data )
{
   if ( !regex )
      return 0;

   static Regexpr ex;
   static re_registers regs;

   char *cp = ex.compile( regex );
   if ( cp )
      return cp;

   char *s = data ? (char*)data : str;
   int len = strlen( s );
   if ( -1 != ex.search( s, len, 0, len, &regs ) )
      return 0;

   static char buf[256];
   sprintf( buf, "Invalid string, regular expression:\n%s", regex );

   return buf;
}

int inputLine::accept( int flagMessage )
{
   char *message = valid();
   if ( message )
    {
      if ( owner && flagMessage )
	 owner->test( message, lang("Input line","Строка ввода") );
      return 0;
    }
   firstkey=1;
   if ( result )
      strcpy( result, str );
   addToHistory();
   return 1;
}

void inputLine::addToHistory()
{
   if ( !history )
       return;
   int l=strlen( str )+1;
   for( int i=0; i<history->getCount(); i++ ) {
       char *ch=(char*)history->at(i);
       if ( !memcmp( str, ch, l ) ) {
	  history->atRemove( i );
	  history->atInsert( 0, ch );
	  return;
       }
   }
   char *ch = new char[l];
   memcpy( ch, str, l );
   history->atInsert( 0, ch );
}

int inputLine::runHistory()
{
   int res=-1;
   if ( !history || !owner )
      return res;
   Point dd = Point( owner->delta.y, owner->delta.x );
   Point a = owner->rect.a - dd + rect.a + Point( 2, 1 );
   Point b = owner->rect.a - dd + rect.b + Point( 8, 2 );
   if ( b.y>=Screen::Lines-1 ) {
       int i = b.y - Screen::Lines + 2;
       a.y-=i; b.y-=i;
   }
   listBox *lb = new listBox( Rect(a,b), width, &res, history );
   long ret = lb->exec( owner->father, owner->Executor );
   if ( ret == HASH_cmOK && res>=0 && res<history->getCount() )
      initString( lb->getItem(res) );
   else
      res = -1;
   delete lb;
   return res;
}

void inputLine::copyToClip()
{
   clipBoard.freeAll();
   int len = strlen( str ) + 1;
   if ( len <= 1 )
      return;
   short *type = new short(0);
   clipBoard.insert( type );
   char *s = new char[ len ];
   memcpy( s, str, len );
   clipBoard.insert( s );
}

void inputLine::copyFromClip()
{
   if ( clipBoard.getCount() < 2 )
      return;
   char *s = (char*)clipBoard.at( 1 );
   for( ; *s; s++ )
      addChar( *s );
}

void inputLine::setRect( Point &p )
{
  origP=p;
  init();
}

int inputLine::getWidth()
{
  return width + (history ? 1:0);
}

int inputLine::setWidth( int w )
{
  int oldw = origScrLen;
  if ( w > 0 )
   {
     origScrLen = w;
     rect.b.x = rect.a.x + origScrLen - 1;
     init();
   }
  return oldw;
}

