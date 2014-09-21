/*
	$Id: tbrow.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/

#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "program.h"
#include "tbrow.h"
#include "hashcode.h"
#include "dialog.h"
#include "listbox.h"

#define SPACE_SYMBOL	141

unsigned char monoTbrow[9] = {
	FG_HI_WHITE | BG_WHITE,	// активная рамка
	FG_BLACK | BG_WHITE,		// обычная рамка
	FG_BLACK | BG_WHITE,		// заголовки столбцов и рамки
	FG_WHITE | BG_BLACK,		// обычный элемент
	FG_BLACK | BG_WHITE,		// текущий элемент
	FG_HI_WHITE | BG_WHITE,	// текущая строка
	FG_WHITE | BG_BLACK,		// обычный элемент в замороженном
	FG_BLACK | BG_WHITE,		// текущий элемент в замороженном
	FG_HI_WHITE | BG_WHITE		// текущая строка в замороженном
};

unsigned char colorTbrow[9] = {
	FG_HI_WHITE | BG_CYAN,		// активная рамка
	FG_BLACK | BG_CYAN,		// обычная рамка
	FG_HI_WHITE | BG_CYAN,		// заголовки столбцов и рамки
	FG_BLACK | BG_CYAN,		// обычный элемент
	FG_WHITE | BG_BLUE,		// текущий элемент
	FG_BLACK | BG_GREEN,		// текущая строка
	FG_HI_YELLOW | BG_CYAN,	// обычный элемент в замороженном
	FG_WHITE | BG_BLUE,		// текущий элемент в замороженном
	FG_HI_YELLOW | BG_GREEN	// текущая строка в замороженном
};

unsigned char laptopTbrow[9] = {
	FG_HI_WHITE | BG_WHITE,	// активная рамка
	FG_BLACK | BG_WHITE,		// обычная рамка
	FG_BLACK | BG_WHITE,		// заголовки столбцов и рамки
	FG_WHITE | BG_BLACK,		// обычный элемент
	FG_BLACK | BG_WHITE,		// текущий элемент
	FG_HI_WHITE | BG_WHITE,	// текущая строка
	FG_WHITE | BG_BLACK,		// обычный элемент в замороженном
	FG_BLACK | BG_WHITE,		// текущий элемент в замороженном
	FG_HI_WHITE | BG_WHITE		// текущая строка в замороженном
};

long tbrowKeyMap[] = {
	kbHome,		HASH_cmHome,
	FUNC1(kbHome),	HASH_cmFirst,
	kbEnd,		HASH_cmEnd,
	FUNC1(kbEnd),	HASH_cmLast,
	kbPgUp,		HASH_cmPgUp,
	kbPgDn,		HASH_cmPgDn,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	kbCtrlF,	HASH_cmUp2,
	kbCtrlV,	HASH_cmDown2,
	FUNC1(kbPgUp),	HASH_cmFuncPgUp,
	FUNC1(kbPgDn),	HASH_cmFuncPgDn,
	kbF10,		HASH_cmFreeze,
	kbTab,		HASH_cmSwitch,
	0
};

Keymap TbrowKeyMap( tbrowKeyMap, new Lang("Table browser base commands","Базовые команды в таблице") );
unsigned short Tbrow::screenStr[256];

//-------------------------- class Column ------------------------------

Column::Column( char *_head, char *_foot, int _width, int _align, int len ) :
	Collection(len,0), head(5,5), foot(5,5), width(_width),
	freezed(0), align(_align)
{
  Collection *coll = &head;
  char *str = _head;
  int i, l;
  for( i=0; i < 2; i++ )
   {
     setHF( coll, str );
     coll = &foot;
     str = _foot;
   }
  for( i=0; i < len; i++ )
     insert(0);
  if ( align < 0 || align > 2 )
     align = 0;
}

Column::~Column()
{
  head.freeAll();
  freeAll();
}

void Column::setHF( Collection *coll, char *str )
{
  coll->freeAll();
  if ( !str )
     return;
  char *old = str;
  str = strchr(str,'\n');
  for( int i=0; 1; str=strchr(str,'\n') )
   {
     i = str ? str-old : strlen( old );
     char *ch = new char[ i + 1 ];
     memcpy( ch, old, i ); ch[i] = 0;
     coll->insert( ch );
     if ( !str )
	return;
     old = ++str;
   }
}

const char *Column::getVal( int index )
{
  static char buf[256];
  if ( !items || index < 0 || index >= count )
     return 0;
  unsigned char *val = (unsigned char*)items[index];
  int len = val ? strlen( (char*)val ) : 0;
  int i, j, k, d=width-len;
  if ( !align || !val || len == width )
     return (const char*)val;
  if ( d > 0 )		// дополняем пробелами
   {
     switch( align )
      {
	case 1:					// к правому краю
	    memset( buf, ' ', d );
	    memcpy( buf+d, val, len );
	    buf[ d + len ] = 0;
	    break;
	case 2:					// по центру
	    i = d >> 1;	// разделить на 2
	    j = d & 1;  // остаток от деления на 2
	    memset( buf, ' ', i );
	    memcpy( buf+i, val, len );
	    memset( buf+i+len, ' ', i+j );
	    buf[ d + len + j ] = 0;
	    break;
      }
   }
  else			// обрезаем
   {
     d = -d;
     switch( align )
      {
	case 1:		// к правому краю
	    memcpy( buf, val+d, width );
	    buf[ width ] = 0;
	    break;
	case 2:		// по центру
	    memcpy( buf, val+(d>>1), width );
	    buf[ width ] = 0;
	    break;
      }
   }
  return (const char*)buf;
}

void Column::setVal( int ind, const char *data )
{
  if ( !items || ind < 0 || ind >= count )
     return;
  if ( items[ind] )
     ::free( (char*)items[ind] );
  items[ind] = (void*)(data ? strdup( data ) : 0);
}

void Column::shiftUp( int n )
{
  if ( count <= 0 )
     return;
  for( int i=0; i < n; i++ )
   {
     char *ch = (char*)items[ 0 ];
     atRemove( 0 );
     if ( ch )
      { ::free( ch ); ch = 0; }
     insert( ch );
   }
}

void Column::shiftDown( int n )
{
  if ( count <= 0 )
     return;
  for( int i=0; i < n; i++ )
   {
     char *ch = (char*)items[count-1];
     atRemove( count-1 );
     if ( ch )
      { ::free( ch ); ch = 0; }
     atInsert( 0, ch );
   }
}

//-------------------------- Column Info ------------------------------

CInfo::CInfo( Column *c, void *k ) :
		key(k), column(c), format(0), trans(0), transLen(0)
{
}

CInfo::~CInfo()
{
  if ( column )
     delete column;
  if ( format )
     ::free( format );
  if ( trans )
   {
     for( int i=0; trans[i]; i++ )
       if ( trans[i] )
	  ::free( trans[i] );
     ::free( trans );
   }
}

//-------------------------- class Tbrow ------------------------------

Tbrow::Tbrow( Rect r, Lang *tit, Lang *id, Lang *st, int Box,
		int ShowStab, int LineHigh, int ShowBeof,
		int BufSize, int NeedMove ) :
	Window( r, tit, id, st, Box ),
	showStab(ShowStab), lineHigh(LineHigh), showBeof(ShowBeof),
	bufSize(BufSize), nrec(0), needMove(NeedMove), headHigh(0),
	footHigh(0), cursorOffset(0), boflag(0), eoflag(0), oldPos(-1,-1),
	internalRedraw(0), freezedPos(0), il(0), il_buf(0), il_com(0)
{
  keyHolder.add( &TbrowKeyMap );
  setTitle( "Table browser" );
  HL = Screen::_HL | GraphAttr;
  VL = Screen::_VL | GraphAttr;
  UC = Screen::_UC | GraphAttr;
  DC = Screen::_BC | GraphAttr;
  CC = Screen::_CC | GraphAttr;
}

Tbrow::~Tbrow()
{
  freeAll();
}

void Tbrow::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoTbrow;
	  break;
      case COLOR:
	  clr = colorTbrow;
	  break;
      case LAPTOP:
	  clr = laptopTbrow;
    }
}

CInfo *Tbrow::makeColumn( char *head, char *foot, int width, int align, void *key )
{
  Column *column = new Column( head, foot, width, align, bufSize );
  headHigh = max( headHigh, column->head.getCount() );
  footHigh = max( footHigh, column->foot.getCount() );
  return new CInfo( column, key );
}

int Tbrow::del( int index )
{
  if ( index < 0 || index > count )
     return 0;
  atFree( index );
  return 1;
}

int Tbrow::init( void *data )
{
  if ( isGetObj )
     box = 0;
  return Window::init( data );
}

Point Tbrow::getCursor( int *hide )
{
   if ( il )
      return rect.a + il->getCursor();

   int i, j, y=cursor.y+headHigh+1, x=0;
   for( i=delta.x, j=delta.x+cursor.x; i < j; i++ )
     x += ((CInfo*)items[i])->column->width+1;
   if ( j < count && cursorOffset > 0 && cursorOffset < ((CInfo*)items[j])->column->width )
     x += cursorOffset;
   if ( box )
     { y++; x++; }
   if ( hide && (count <= 0 || x > size.x-(box?2:0)) )
      *hide = 1;
   return rect.a + Point(y,x);
}

void Tbrow::moveCursor()
{
   int hide=0;
   Point p = getCursor( &hide );
   if ( hide || nrec <= 0 )
       Screen::hideCursor();
   else
       Screen::move( p );
}


unsigned short *Tbrow::scrLine( int line, int &len, int head )
{
  len = 0;
  unsigned short attrH = (clr[2] << 8) & 0x7f00;
  unsigned short attrI = (clr[3] << 8) & 0x7f00;
  unsigned short attrF = (clr[6] << 8) & 0x7f00;
  int i, wd=0, saveLen=0, size_x = size.x - (box ? 2 : 0);
  if ( head )	// строка заголовка
   {
     unsigned short sym=0, spaceH = attrH | ' ';
     unsigned char *ch=0, *ch2;
     for( i=0; i <= delta.x && i < count; i++ )
      {
	Column *col = ((CInfo*)items[i])->column;
	Collection *coll = (head == 1 ? &col->head : &col->foot);
	ch2 = (line<0 || line>=coll->getCount()) ? 0 :
			(unsigned char*)coll->at( line );
	if ( ch && i != delta.x )
	   wd += col->width+1;
	if ( ch2 && ch2[0] )
	 {
	   if ( i == delta.x )
	     { ch=0; break; }
	   ch=ch2;
	   wd = col->width + 1 + (ch2[0]=='\b' ? 1 : 0);
	 }
      }
     if ( ch && strlen((char*)ch) > (size_t)(wd+1) )
      {
	for( ch += wd; *ch; ch++ )
	   screenStr[len++] = attrH | *ch;
      }
     for( i=delta.x; i < count; i++ )
      {
	Column *col = ((CInfo*)items[i])->column;
	Collection *coll = (head == 1 ? &col->head : &col->foot);
	ch = (line<0 || line>=coll->getCount()) ? 0 :
			(unsigned char*)coll->at( line );
	if ( ch && *ch == '\b' )
	 { sym=VL; ch++; }
	if ( sym && i > delta.x && saveLen > 0 )
	 {
	   screenStr[saveLen-1] = attrH | sym;
	   len = saveLen;
	 }
	sym = 0;
	for( wd=0; ch && *ch; wd++ )
	 {
	   if ( len >= size_x )
	     goto _end;
	   if ( ch && *ch == '\t' )
	    {
	      if ( *(ch+1) != 0 )
		*ch = ' ';
	      else
		{ sym=VL; break; }
	    }
	   if ( !wd && saveLen > 0 )
	     len = saveLen;
	   screenStr[len++] = attrH | *ch++;
	 }
	saveLen += col->width + 1;
	while( len <= saveLen && len < size_x )
	   screenStr[len++] = spaceH;
      }
     while( len < size_x )
	screenStr[len++] = spaceH;
   }
  else for( i=delta.x; i < count; i++ )
   {
     if ( i > delta.x )
	screenStr[len++] = attrH | VL;
     Column *col = ((CInfo*)items[i])->column;
     unsigned char c, *ch = (unsigned char*)col->getVal( line );
     unsigned short attr = col->freezed ? attrF : attrI;
     for( wd=0; wd < col->width; wd++ )
      {
	if ( len >= size_x )
	  goto _end;
	c = ch && *ch ? *ch++ : ' ';
	if ( c < ' ' )
	   c = SPACE_SYMBOL;
	screenStr[len++] = attr | c;
      }
   }
_end:
  return screenStr;
}

unsigned short *Tbrow::getHorLine( int &len, int flag )
{
  char *ch = "";
  if ( !flag )
   {
     if ( nrec <= 0 )
	ch = "<empty>";
     else if ( delta.y + cursor.y == 0 && boflag )
	ch = "<bof>";
     else if ( delta.y + cursor.y == nrec-1 && eoflag )
	ch = "<eof>";
   }
  int ch_len = strlen( ch );
  int high = flag ? footHigh : headHigh;
  len=0;
  unsigned short sym=0, attr = (clr[2] << 8) & 0x7f00;
  unsigned short attrHL = attr | HL;
  int size_x = size.x - (box ? 2 : 0);
  for( int i=delta.x, j=0; i < count; i++ )
   {
     Column *col = ((CInfo*)items[i])->column;
     Collection *coll = flag ? &col->foot : &col->head;
     unsigned char *c=0;
     if ( high > 0 && coll->getCount() == high )
      {
	c = (unsigned char*)coll->at( high-1 );
	if ( *c == '\b' )
	   sym = CC;
      }
     if ( i > delta.x )
      {
	if ( !sym )
	   sym = flag ? DC : UC;
	screenStr[len++] = attr | sym;
      }
     sym=0;
     if ( c && (j=strlen((char*)c)) > 0 && c[j-1]=='\t' )
	sym = CC;
     c = (unsigned char*)((ch_len <= col->width) ? ch : "");
     for( int wd=0; wd < col->width || (i==count-1 && *c); wd++ )
      {
	if ( len >= size_x )
	  goto _end;
	screenStr[len++] = attr | (*c ? *(c++) : HL);
      }
   }
  while( len < size_x )
    screenStr[len++] = attrHL;
_end:
  return screenStr;
}

int Tbrow::draw( int Redraw, int sy, int sx )
{
  if ( !Window::draw( Redraw, sy, sx ) )
     return 0;

  Point coor = rect.a + Point(sy,sx);
  if ( box )
    { coor.y++; coor.x++; }
  if ( !isGetObj )
      Screen::currentMap = scr.bord;
  topOffset = headHigh + 1;
  bottOffset = footHigh > 0 ? footHigh + 1 : 0;
  int mustDraw = 0;
  if ( internalRedraw )
   {
     topDraw = delta.y + topOffset;
     bottDraw= delta.y + size.y - bottOffset - (box ? 3 : 1);
   }
  else
     mustDraw = scroll( clr[3], Redraw );
  int i, j=0, k, _y=coor.y, y=coor.y+topDraw-delta.y, x=coor.x,
      size_x = size.x - (box ? 2 : 0);
  unsigned short *str;
  if ( Redraw || mustDraw || internalRedraw )
   {
     internalRedraw = 0;
     Screen::Clear( _y, x, headHigh+1, size_x, clr[3] );
     for( i=0; i < headHigh; i++ )
      {
	str = scrLine( i, j, 1 );
	Screen::putLimited( _y+i, x, str, j );
      }
     str = getHorLine( j, 0 );
     Screen::putLimited( _y + headHigh, x, str, j );
     Screen::Clear( y, x, bottDraw-topDraw+1, size_x, clr[3] );
     for( _y=y, i=topDraw-topOffset, k=bottDraw-topOffset; i<=k; i++, _y++ )
      {
	str = scrLine( i, j );
	Screen::putLimited( _y, x, str, j );
      }
     if ( footHigh > 0 )
      {
	str = getHorLine( j, 1 );
	_y = coor.y + size.y - (box?2:0) - footHigh - 1;
	Screen::Clear( _y, x, footHigh+1, size_x, clr[3] );
	Screen::putLimited( _y++, x, str, j );
	for( i=0; i < footHigh; i++ )
	 {
	   str = scrLine( i, j, 2 );
	   Screen::putLimited( _y+i, x, str, j );
	 }
      }
   }
  else
   {
     str = getHorLine( j, 0 );
     Screen::putLimited( _y + headHigh, x, str, j );
     if ( footHigh > 0 )
      {
	str = getHorLine( j, 1 );
	Screen::putLimited( _y + size_x - footHigh - 1, x, str, j );
      }
   }

  int _x=x, pos_y = delta.y+cursor.y, pos_x = delta.x+cursor.x;
  if ( oldPos.y >= delta.y && oldPos.y <= delta.y + size.y - (headHigh+1) - (footHigh>0 ? footHigh+2 : 0) - (box?2:0) )
   {
     y = coor.y+headHigh+oldPos.y-delta.y+1;
     Screen::attrSet( y, x, 1, size_x, clr[3] );
     for( i=delta.x, _x=x; i < count && _x-coor.x < size_x; i++ )
      {
	Column *col = ((CInfo*)items[i])->column;
	if ( col->freezed )
	   Screen::attrSet( y, _x, 1, col->width, clr[6] );
	if ( i > delta.x )
	   Screen::attrSet( y, _x-1, 1, 1, clr[2] );
	_x += col->width + 1;
      }
     if ( i < count && _x <= size_x + 1 )
	Screen::attrSet( y, _x-1, 1, 1, clr[2] );
   }

  y = coor.y + headHigh + pos_y - delta.y + 1;
  for( i=delta.x, _x=x; i < count && _x-coor.x < size_x; i++ )
   {
     Column *col = ((CInfo*)items[i])->column;
     if ( !il || i != il_x )
      {
	int attr = col->freezed ? (i==pos_x && col->freezed!=2 ? clr[7] : clr[8]) : (i==pos_x ? clr[4] : clr[5]);
	int put_size = min( col->width, size_x-(_x-coor.x) );
	Screen::attrSet( y, _x, 1, put_size, attr );
      }
     _x += col->width + 1;
   }

  if ( il )
     il->draw( 1, sy + rect.a.y, sx + rect.a.x );

  if ( !isGetObj )
   {
     Screen::currentMap=Screen::ScreenMap;
     if ( appl->topWindow != this )
	Screen::attrSet( Screen::shadowMap );
   }

  oldDelta = delta;
  oldPos.y = pos_y; oldPos.x = pos_x;
  MoveCursor();
  return 1;
}

int Tbrow::moveHome()
{
  while( moveLeft() );
  return 1;
}

int Tbrow::moveBof()
{
  delta.y = cursor.y = 0;
  return 1;
}

int Tbrow::moveEnd()
{
  while( moveRight() );
  return 1;
}

int Tbrow::moveEof()
{
  cursor.y = size.y - headHigh - 2 - (footHigh>0 ? footHigh+1 : 0) - (box?2:0);
  delta.y = nrec - cursor.y - 1;
  if ( delta.y < 0 )
   {
     cursor.y += delta.y;
     if ( cursor.y < 0 )
	cursor.y = 0;
     delta.y=0;
   }
  return 1;
}

int Tbrow::movePgUp( int check )
{
  int offset = size.y - (headHigh + footHigh + 2) - (box?2:0);
  if ( check && (delta.y - offset) < 0 )
     return 0;
  if ( !delta.y )
     cursor.y = 0;
  else
   {
     delta.y -= offset;
     if ( delta.y < 0 )
	delta.y=0;
   }
  return 1;
}

int Tbrow::movePgDn( int check )
{
  int offset = size.y - (headHigh + footHigh + 2) - (box?2:0);
  if ( check && (delta.y + offset*2) >= nrec )
     return 0;
  if ( delta.y >= nrec )
   {
     delta.y = nrec-1;
     cursor.y = 0;
   }
  else
   {
     delta.y += offset;
     if ( delta.y >= nrec )
	delta.y = nrec-1;
     if ( delta.y + cursor.y >= nrec )
	cursor.y = nrec - delta.y - 1;
   }
  return 1;
}

int Tbrow::moveLeft()
{
  CInfo *ci = (CInfo*)items[delta.x+cursor.x];
  int d_x=delta.x, c_x=cursor.x;
m1:
  if ( !cursor.x && (!delta.x || ci->column->freezed) )
   {
     delta.x = d_x;
     cursor.x = c_x;
     return 0;
   }
  int ret=0;
  if ( cursor.x <= 0 )
   { delta.x--; ret=1; }
  else
   {
     CInfo *ci2 = (CInfo*)items[delta.x+cursor.x-1];
     if ( ci->column->freezed || !ci2->column->freezed || !delta.x )
      { cursor.x--; ret=1; }
     else if ( delta.x > 0 )
      {
	for( int i=delta.x; i < count; i++ )
	 {
	   ci2 = (CInfo*)items[i];
	   if ( !ci2->column->freezed )
	      break;
	   atRemove( i );
	   atInsert( i-1, ci2 );
	 }
	delta.x--;
	ret=1;
      }
   }
  ci = (CInfo*)items[delta.x+cursor.x];
  if ( ci->column->freezed == 2 )
     goto m1;
  if ( ci->column->freezed )
     freezedPos = cursor.x;
  return ret;
}

int Tbrow::moveRight()
{
  int d_x=delta.x, c_x=cursor.x;
m1:
  if ( delta.x + cursor.x >= count-1 )
   {
     delta.x = d_x;
     cursor.x = c_x;
     return 0;
   }
  int i, j=-1, wd=0, size_x=size.x-(box?2:0), pos_x=delta.x+cursor.x,
	fr_width=0, ret=0;
  CInfo *ci=0;
  for( i=delta.x; i <= pos_x+1; i++ )
   {
     wd += (ci=(CInfo*)items[i])->column->width+1;
     if ( ci->column->freezed )
      {
	fr_width += ci->column->width+1;
	j = i;
      }
   }
  size_x -= fr_width;
  wd -= fr_width;
  if ( wd <= size_x+1 || !wd )
    { cursor.x++; ret=1; }
  else if ( !ci->column->freezed )
   {
     j = fr_width > 0 ? j+1 : delta.x;
     for( i=0; wd > size_x && j <= pos_x; i++ )
      {
	wd -= ((CInfo*)items[ j++ ])->column->width+1;
	if ( i )
	   cursor.x--;
	ret = 1;
      }
     if ( !i )
      {
	if ( size_x > 0 )
	  cursor.x++;
	ret = 1;
      }
     else
      {
	if ( fr_width > 0 )
	 {
	   for( i=0; (ci=(CInfo*)items[delta.x])->column->freezed; i++ )
	    {
	      atRemove( delta.x );
	      atInsert( j-1, ci );
	    }
	   j -= i;
	 }
	delta.x = j;
      }
   }
  ci = (CInfo*)items[delta.x+cursor.x];
  if ( ci->column->freezed == 2 )
     goto m1;
  if ( ci->column->freezed )
     freezedPos = cursor.x;
  return ret;
}

int Tbrow::moveUp()
{
  if ( !delta.y && !cursor.y )
     return 0;
  if ( cursor.y > 0 )
     cursor.y--;
  else
     delta.y--;
  return 1;
}

int Tbrow::moveDown()
{
  if ( delta.y + cursor.y >= nrec - 1 )
     return 0;
  if ( cursor.y < size.y - (box?2:0) - headHigh - 2 - (footHigh>0 ? footHigh+1 : 0) )
     cursor.y++;
  else
     delta.y++;
  return 1;
}

long Tbrow::handleKey( long key, void *&ptr )
{
  if ( owner && !owner->hardMode )
     return key;
  key = keyHolder.translate( key );
  switch( key )
   {
     case HASH_cmHome:
	 moveHome();
	 key=0; break;
     case HASH_cmFirst:
     case HASH_cmFuncPgUp:
	 moveBof();
	 key=0; break;
     case HASH_cmEnd:
	 moveEnd();
	 key=0; break;
     case HASH_cmLast:
     case HASH_cmFuncPgDn:
	 moveEof();
	 key=0; break;
     case HASH_cmPgUp:
	 movePgUp();
	 key=0; break;
     case HASH_cmPgDn:
	 movePgDn();
	 key=0; break;
     case HASH_cmLeft:
	 moveLeft();
	 key=0; break;
     case HASH_cmRight:
	 moveRight();
	 key=0; break;
     case HASH_cmUp:
	 moveUp();
	 key=0; break;
     case HASH_cmDown:
	 moveDown();
	 key=0; break;
     case HASH_cmUp2:
	 key=0; break;
     case HASH_cmDown2:
	 key=0; break;
     case HASH_cmFreeze:
	 freezeColumn( delta.x + cursor.x );
	 key=0; break;
     case HASH_cmSwitch:
	 switchColumn();
	 key=0; break;
   }
  if ( !key && !isGetObj && draw() )
     { MoveCursor(); Screen::sync(); }
  return key;
}

void Tbrow::freezeColumn( int pos_x, int fr_flag )
{
  if ( pos_x < 0 || pos_x >= count )
     return;
  int i;
  CInfo *ci = (CInfo*)items[ pos_x ];
  if ( ci->column->freezed )	// разморозить
   {
     atRemove( pos_x );
     for( i=pos_x; i < count; i++ )
       if ( !((CInfo*)items[i])->column->freezed )
	  break;
     atInsert( i, ci );
     cursor.x = i - delta.x;
     int wd=0, size_x=size.x-(box?2:0);
     for( i=delta.x, pos_x=delta.x+cursor.x; i < pos_x; i++ )
       if ( (wd+=((CInfo*)items[i])->column->width+1) >= size_x )
	 { cursor.x = i - delta.x; break; }
     ci->column->freezed = 0;
     if ( freezedPos > 0 )
	freezedPos--;
   }
  else for( i=delta.x; i <= pos_x && i < count; i++ )	// заморозить
   {
     if ( ((CInfo*)items[i])->column->freezed )
	continue;
     if ( i != pos_x )
      {
	atRemove( pos_x );
	atInsert( i, ci );
	cursor.x = i - delta.x;
      }
     ci->column->freezed = fr_flag > 0 ? 2 : 1;
     if ( ci->column->freezed == 2 && !moveRight() )
	moveLeft();
     break;
   }
  internalRedraw = 1;
}

int Tbrow::switchColumn()
{
  CInfo *ci = (CInfo*)items[delta.x+cursor.x];
  if ( !ci->column->freezed )
     cursor.x = freezedPos;
  else
   {
     int frPos = cursor.x;
     while( moveRight() && ((CInfo*)items[delta.x+cursor.x])->column->freezed );
     freezedPos = frPos;
   }
  return 0;
}

