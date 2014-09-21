/*
	$Id: getobj.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "dialog.h"
#include "hashcode.h"

static char buf[256];

unsigned char alt2koi8[] = {
 0xe1, 0xe2, 0xf7, 0xe7, 0xe4, 0xe5, 0xf6, 0xfa,
 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
 0xf2, 0xf3, 0xf4, 0xf5, 0xe6, 0xe8, 0xe3, 0xfe,
 0xfb, 0xfd, 0xff, 0xf9, 0xf8, 0xfc, 0xe0, 0xf1,
 0xc1, 0xc2, 0xd7, 0xc7, 0xc4, 0xc5, 0xd6, 0xda,
 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
 0x90, 0x91, 0x92, 0x81, 0x87, 0xb2, 0xb4, 0xa7,
 0xa6, 0xb5, 0xa1, 0xa8, 0xae, 0xad, 0xac, 0x83,
 0x84, 0x89, 0x88, 0x86, 0x80, 0x8a, 0xaf, 0xb0,
 0xab, 0xa5, 0xbb, 0xb8, 0xb1, 0xa0, 0xbe, 0xb9,
 0xba, 0xb6, 0xb7, 0xaa, 0xa9, 0xa2, 0xa4, 0xbd,
 0xbc, 0x85, 0x82, 0x8d, 0x8c, 0x8e, 0x8f, 0x8b,
 0xd2, 0xd3, 0xd4, 0xd5, 0xc6, 0xc8, 0xc3, 0xde,
 0xdb, 0xdd, 0xdf, 0xd9, 0xd8, 0xdc, 0xc0, 0xd1,
 0xb3, 0xa3, 0x99, 0x98, 0x93, 0x9b, 0x9f, 0x97,
 0x9c, 0x95, 0x9e, 0x96, 0xbf, 0x9d, 0x94, 0x9a
};

unsigned char koi82alt[] = {
 0xc4, 0xb3, 0xda, 0xbf, 0xc0, 0xd9, 0xc3, 0xb4,
 0xc2, 0xc1, 0xc5, 0xdf, 0xdc, 0xdb, 0xdd, 0xde,
 0xb0, 0xb1, 0xb2, 0xf4, 0xfe, 0xf9, 0xfb, 0xf7,
 0xf3, 0xf2, 0xff, 0xf5, 0xf8, 0xfd, 0xfa, 0xf6,
 0xcd, 0xba, 0xd5, 0xf1, 0xd6, 0xc9, 0xb8, 0xb7,
 0xbb, 0xd4, 0xd3, 0xc8, 0xbe, 0xbd, 0xbc, 0xc6,
 0xc7, 0xcc, 0xb5, 0xf0, 0xb6, 0xb9, 0xd1, 0xd2,
 0xcb, 0xcf, 0xd0, 0xca, 0xd8, 0xd7, 0xce, 0xfc,
 0xee, 0xa0, 0xa1, 0xe6, 0xa4, 0xa5, 0xe4, 0xa3,
 0xe5, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae,
 0xaf, 0xef, 0xe0, 0xe1, 0xe2, 0xe3, 0xa6, 0xa2,
 0xec, 0xeb, 0xa7, 0xe8, 0xed, 0xe9, 0xe7, 0xea,
 0x9e, 0x80, 0x81, 0x96, 0x84, 0x85, 0x94, 0x83,
 0x95, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
 0x8f, 0x9f, 0x90, 0x91, 0x92, 0x93, 0x86, 0x82,
 0x9c, 0x9b, 0x87, 0x98, 0x9d, 0x99, 0x97, 0x9a
};

unsigned char win2koi8[] = {
 0x20, 0x20, 0x20, 0x20, 0x22, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x3c, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x2c, 0x20, 0x22, 0x20, 0x2d, 0x20,
 0x20, 0x20, 0x20, 0x3e, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0xb3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0x9c, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0xa3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0xe1, 0xe2, 0xf7, 0xe7, 0xe4, 0xe5, 0xf6, 0xfa,
 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
 0xf2, 0xf3, 0xf4, 0xf5, 0xe6, 0xe8, 0xe3, 0xfe,
 0xfb, 0xfd, 0xff, 0xf9, 0xf8, 0xfc, 0xe0, 0xf1,
 0xc1, 0xc2, 0xd7, 0xc7, 0xc4, 0xc5, 0xd6, 0xda,
 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
 0xd2, 0xd3, 0xd4, 0xd5, 0xc6, 0xc8, 0xc3, 0xde,
 0xdb, 0xdd, 0xdf, 0xd9, 0xd8, 0xdc, 0xc0, 0xd1
};

unsigned char koi82win[] = {
 0xad, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x20, 0x20, 0xb7, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xb0, 0x20,
 0x20, 0x20, 0x20, 0xb8, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0xa8, 0x20, 0x20, 0x20, 0x20,
 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
 0xfe, 0xe0, 0xe1, 0xf6, 0xe4, 0xe5, 0xf4, 0xe3,
 0xf5, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee,
 0xef, 0xff, 0xf0, 0xf1, 0xf2, 0xf3, 0xe6, 0xe2,
 0xfc, 0xfb, 0xe7, 0xf8, 0xfd, 0xf9, 0xf7, 0xfa,
 0xde, 0xc0, 0xc1, 0xd6, 0xc4, 0xc5, 0xd4, 0xc3,
 0xd5, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce,
 0xcf, 0xdf, 0xd0, 0xd1, 0xd2, 0xd3, 0xc6, 0xc2,
 0xdc, 0xdb, 0xc7, 0xd8, 0xdd, 0xd9, 0xd7, 0xda
};

unsigned char main2koi8[] = {
 0xb9, 0xba, 0xb6, 0xb2, 0xb4, 0xa7, 0xa6, 0xb7,
 0xaa, 0xa9, 0xa2, 0xad, 0xac, 0xaf, 0xb0, 0xa4,
 0xa5, 0xa8, 0xae, 0xab, 0xa0, 0xa1, 0xb8, 0xb5,
 0xbb, 0xb1, 0xbe, 0x90, 0x91, 0x92, 0xbd, 0xbc,
 0x82, 0x83, 0x85, 0x84, 0x80, 0x81, 0x88, 0x87,
 0x89, 0x86, 0x8a, 0x8d, 0x8c, 0x8e, 0x8f, 0x8b,
 0xe1, 0xe2, 0xf7, 0xe7, 0xe4, 0xe5, 0xf6, 0xfa,
 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
 0xf2, 0xf3, 0xf4, 0xf5, 0xe6, 0xe8, 0xe3, 0xfe,
 0xfb, 0xfd, 0xff, 0xf9, 0xf8, 0xfc, 0xe0, 0xf1,
 0xc1, 0xc2, 0xd7, 0xc7, 0xc4, 0xc5, 0xd6, 0xdc,
 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
 0xd2, 0xd3, 0xd4, 0xd5, 0xc6, 0xc8, 0xc3, 0xde,
 0xdb, 0xdd, 0xdf, 0xd9, 0xd8, 0xdc, 0xc0, 0xd1,
 0xb3, 0xa3, 0x99, 0x98, 0x93, 0x9b, 0x9f, 0x97,
 0x9c, 0x95, 0x9e, 0x96, 0xbf, 0x9d, 0x94, 0x9a
};

unsigned char koi82main[] = {
 0xa4, 0xa5, 0xa0, 0xa1, 0xa3, 0xa2, 0xa9, 0xa7,
 0xa6, 0xa8, 0xaa, 0xaf, 0xac, 0xab, 0xad, 0xae,
 0x9b, 0x9c, 0x9d, 0xf4, 0xfe, 0xf9, 0xfb, 0xf7,
 0xf3, 0xf2, 0xff, 0xf5, 0xf8, 0xfd, 0xfa, 0xf6,
 0x94, 0x95, 0x8a, 0xf1, 0x8f, 0x90, 0x86, 0x85,
 0x91, 0x89, 0x88, 0x93, 0x8c, 0x8b, 0x92, 0x8d,
 0x8e, 0x99, 0x83, 0xf0, 0x84, 0x97, 0x82, 0x87,
 0x96, 0x80, 0x81, 0x98, 0x9f, 0x9e, 0x9a, 0xfc,
 0xee, 0xd0, 0xd1, 0xe6, 0xd4, 0xd5, 0xe4, 0xd3,
 0xe5, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
 0xdf, 0xef, 0xe0, 0xe1, 0xe2, 0xe3, 0xd6, 0xd2,
 0xec, 0xeb, 0xd7, 0xe8, 0xed, 0xe9, 0xe7, 0xea,
 0xce, 0xb0, 0xb1, 0xc6, 0xb4, 0xb5, 0xc4, 0xb3,
 0xc5, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe,
 0xbf, 0xcf, 0xc0, 0xc1, 0xc2, 0xc3, 0xb6, 0xb2,
 0xcc, 0xcb, 0xb7, 0xc8, 0xcd, 0xc9, 0xc7, 0xca
};

long getBoxKeyMap[] = {
	kbHome,		HASH_cmHome,
	kbEnd,		HASH_cmEnd,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbEnter,	HASH_cmOK,
	' ',		HASH_cmSelect,
	kbCtrlB,	HASH_cmMode,
	0
};

Keymap GetBoxKeyMap( getBoxKeyMap );

getobj::getobj( Rect r ) : strError(0), father(0), Executor(0), rect(r),
			owner(0), active(0), bind_flag(0), isGetObj(1),
			winMenu(0), bindList(0), trans_flag(TRANS_none)
{
}

getobj::~getobj()
{
  if ( bindList )
   {
     for( int i=bindList->getCount()-1; i>=0; i-- )
       if ( this == (getobj*)bindList->at(i) )
	{
	  bindList->atRemove( i );
	  break;
	}
     if ( bindList->getCount() == 0 )
      {
	delete bindList;
	bindList = NULL;
      }
   }
}

int  getobj::isType(long typ)
{
  return typ==HASH_getobj;
}

int getobj::init( void *data )
{
   return 1;
}

void getobj::setRect( Rect &nRect )
{
   rect=nRect;
   init();
}

void getobj::setRect( Point &p )
{
   Point dp( p.y - rect.a.y, p.x - rect.a.x );
   rect.a = p;
   rect.b += dp;
   init();
}

int getobj::translate_out( int symbol )
{
	if ( symbol < 128 )
	    return symbol;
	switch( trans_flag ) {
	    case TRANS_alt2koi8:
		symbol = alt2koi8[ symbol & 0x7f ]; break;
	    case TRANS_koi82alt:
		symbol = koi82alt[ symbol & 0x7f ]; break;
	    case TRANS_win2koi8:
		symbol = win2koi8[ symbol & 0x7f ]; break;
	    case TRANS_koi82win:
		symbol = koi82win[ symbol & 0x7f ]; break;
	    case TRANS_main2koi8:
		symbol = main2koi8[ symbol & 0x7f ]; break;
	    case TRANS_koi82main:
		symbol = koi82main[ symbol & 0x7f ]; break;
	}
	return symbol;
}

int getobj::translate_in( int key )
{
	if ( key < 128 )
	    return key;
	switch( trans_flag ) {
	    case TRANS_alt2koi8:
		key = koi82alt[ key & 0x7f ]; break;
	    case TRANS_koi82alt:
		key = alt2koi8[ key & 0x7f ]; break;
	    case TRANS_win2koi8:
		key = koi82win[ key & 0x7f ]; break;
	    case TRANS_koi82win:
		key = win2koi8[ key & 0x7f ]; break;
	    case TRANS_main2koi8:
		key = koi82main[ key & 0x7f ]; break;
	    case TRANS_koi82main:
		key = main2koi8[ key & 0x7f ]; break;
	}
	return key;
}

unsigned char monoHyper[2] = {
	FG_HI_WHITE | BG_BLACK,	// active
	FG_WHITE | BG_BLACK		// normal
};

unsigned char colorHyper[2] = {
	FG_HI_YELLOW | BG_BLUE,
	FG_HI_YELLOW | BG_CYAN
};

unsigned char laptopHyper[2] = {
	FG_HI_WHITE | BG_BLACK,
	FG_WHITE | BG_BLACK
};

getBoxStatus::getBoxStatus( Point p, int st, int cur, Collection *el ) :
		rect(Rect(p,p)), state(st), current(cur), items(el) {;}

/*-------------------------- class hyper ----------------------------*/

long hyperKeyMap[] = {
	kbEnter,	HASH_cmOK,
	kbCtrlB,	HASH_cmMode,
	0
};

Keymap HyperKeyMap( hyperKeyMap );

hyper::hyper( Point p, const char *l ) : data(0), label(0)
#if 0
	, maxlen(0), str_count(0)
#endif
{
   keyHolder.add( &HyperKeyMap );
   rect.a=rect.b=p;
   init( (void*)l );
   setColors( colorsType );
}

hyper::~hyper()
{
   clear();
}

void hyper::clear()
{
#if 0
   for( int i=0; label && label[i]; i++ )
      ::free( label[i] );
   if ( label )
      ::free( label );
   label = NULL;
   maxlen = str_count = 0;
#else
   if ( label )
      ::free( label );
   label = NULL;
#endif
}

void hyper::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoHyper;
	  break;
      case COLOR:
	  clr = colorHyper;
	  break;
      case LAPTOP:
	  clr = laptopHyper;
    }
}

const char *hyper::getLabel()
{
#if 0
   return (const char*)label[0];
#else
   return (const char*)label;
#endif
}

void hyper::setLabel( const char *l )
{
   clear();
#if 0
   addLabel( l );
#else
   label = (unsigned char*)strdup( l ? l : "" );
   rect.b.x = rect.a.x + strlen((char*)label) - 1;
#endif
}

#if 0
void hyper::addLabel( const char *l )
{
   if ( !l )
      return;

   int s_count=1, cycle=1, len=0;
   const char *str = l;
   for( ; cycle; str++ )
    {
      switch( *str )
       {
	 case '\n':	s_count++;	break;
	 case 0:	cycle=0;	break;
	 default:	len++;		continue;
       }
      if ( len > maxlen )
	 maxlen = len;
      len=0;
    }

   label = (unsigned char**)realloc( label, sizeof(char*) * (str_count+s_count+1) );

   for( str=l, cycle=1, len=0; cycle; str++ )
    {
      switch( *str )
       {
	 case 0:
	     cycle=0;
	 case '\n':
	     break;
	 default:
	     buf[len++] = *str;
	     continue;
       }
      buf[len] = 0;
      len=0;
      label[str_count++] = (unsigned char*)strdup( buf );
    }
   label[str_count] = NULL;
   rect.b.x = rect.a.x + maxlen - 1;
   rect.b.y = rect.a.y + str_count - 1;
}
#endif

int  hyper::isType(long typ)
{
  return (typ==HASH_hyper? 1 : getobj::isType(typ));
}

int hyper::init( void *l )
{
   getobj::init();
   if ( !l )
      return 0;
   setLabel( (const char*)l );
   return 1;
}

int hyper::draw( int Redraw, int sy, int sx )
{
   Point p = rect.a + Point( sy, sx );
   int attr = active || bind_flag ? clr[0] : clr[1];
   Screen::Clear( p.y, p.x, 1, strlen((char*)label), attr );
   for( int i=0; label && label[i]; i++, p.x++ ) {
       int symbol = translate_out( (int)label[i] );
       Screen::put( p.y, p.x, symbol, attr );
   }
   return 1;
}

long button::handleKey( long key, void *&ptr )
{
   return (keyHolder.translate(key) == HASH_cmOK) ? command : key;
}

int  button::isType(long typ)
{
  return (typ==HASH_button? 1 : hyper::isType(typ));
}


/*------------------------- class getBox ---------------------------*/

unsigned char monoGetbox[4] = {
	FG_WHITE | BG_BLACK,		// active rect
	FG_WHITE | BG_BLACK,		// normal rect
	FG_HI_WHITE | BG_BLACK,		// select item, active
	FG_HI_WHITE | BG_BLACK		// select item, normal
};

unsigned char colorGetbox[4] = {
	FG_BLACK | BG_GREEN,
	FG_BLACK | BG_CYAN,
	FG_HI_WHITE | BG_GREEN,
	FG_HI_WHITE | BG_CYAN
};

unsigned char laptopGetbox[4] = {
	FG_WHITE | BG_BLACK,
	FG_WHITE | BG_BLACK,
	FG_HI_WHITE | BG_BLACK,
	FG_HI_WHITE | BG_BLACK
};

getBox::getBox( Point p, int st, int *res, int cur, Collection *el ) :
		result(res), items(NULL)
{
   items = new Collection( 5, 5 );
   keyHolder.add( &GetBoxKeyMap );
   getBoxStatus _st( p, st, cur, el );
   init( &_st );
   setColors( colorsType );
}

getBox::~getBox()
{
   if ( items ) {
       items->freeAll();
       delete items;
       items = NULL;
   }
}

int getBox::isType(long typ)
{
  return (typ==HASH_getBox? 1 : getobj::isType(typ));
}

void getBox::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoGetbox;
	  break;
      case COLOR:
	  clr = colorGetbox;
	  break;
      case LAPTOP:
	  clr = laptopGetbox;
    }
}

int getBox::init( void *st )
{
   getobj::init();
   if ( !st )
      return 0;
   getBoxStatus *s=(getBoxStatus*)st;
   rect.a=s->rect.a;
   state=s->state;
   current=s->current;
   if ( s->items )
     for( int i=0, end=s->items->getCount(); i<end; i++ )
	add( (const char*)s->items->at(i), 0 ) ;
   calcRect();
   return 1;
}

long getBox::handleKey( long key, void *&ptr )
{
   key = keyHolder.translate( key );

   switch( key )
    {
       case HASH_cmSelect:
	   return changeState( key, Point(owner->delta.y,owner->delta.x) + owner->cursor );
       case HASH_cmUp:
       case HASH_cmDown:
       case HASH_cmLeft:
       case HASH_cmRight:
	   return changeCurrent( key );
       case HASH_cmHome:
	   if ( owner->hardMode )
	      return current=0;
	   break;
       case HASH_cmEnd:
	   if ( owner->hardMode )
	     { current=items->getCount()-1; return 0; }
	   break;
    }
   return key;
}

void *getBox::getStatus()
{
   static getBoxStatus status;
   status.rect=rect;
   status.state=state;
   status.current=current;
   status.items=items;
   return (void*)&status;
}

ccIndex getBox::add( const char *str, int flag )
{
   if ( !str /*|| !str[0]*/ )
      return -1;
   int len = min( strlen(str), 122 ) + 6;
   char *p=new char[len];
   int lenpr = strlen( prompt );
   memcpy( p, prompt, lenpr );
   memcpy( p+lenpr, str, len - lenpr );
   p[len-1]=0;
   ccIndex ret = items->insert(p);
   if ( flag )
       calcRect();
   return ret;
}

int getBox::del( ccIndex index )
{
   if ( index<0 || index >= items->getCount() )
       return 0;
   items->atFree( index );
   calcRect();
   return 1;
}

/*-------------------------- class radioBox --------------------------*/

int radioBox::isType(long typ)
{
  return (typ==HASH_radioBox ? 1 : getBox::isType(typ));
}

int radioBox::draw( int Redraw, int sy, int sx )
{
   Point coor = rect.a + Point(sy,sx);
   int normal = active || bind_flag ? clr[0] : clr[1];
   int len, width=rect.b.x-rect.a.x+1, count=items->getCount();
   Screen::Clear( coor.y, coor.x, count, width, normal );
   static unsigned char str[128];
   str[width]=0;
   for( int i=0; 1; coor.y++, i++ ) {
       if ( i >= count )
	   break;
       unsigned char *ch = (unsigned char*)items->at( i );
       memset( (char*)str, ' ', width );
       len = strlen( (char*)ch );
       for( int j=0; j < len; j++ )
	   str[j] = translate_out( (int)ch[j] );
       int attr = normal;
       if ( isSelect(i) ) {
	   str[2]=flag;
	   attr = active ? clr[2] : clr[3];
       }
       Screen::put( coor, str, attr );
    }
   return 1;
}

void radioBox::calcRect()
{
   int i=0, count=items->getCount(), len=0, maxlen=0;
   for( ; i < count; i++ )
       if ( (len=strlen((char*)items->at(i))) > maxlen )
	   maxlen=len;
   rect.b.y = rect.a.y + (count ? count-1 : 0);
   rect.b.x=rect.a.x+maxlen;
}

long radioBox::changeState( long key, Point cur )
{
   if ( owner->hardMode )
      { state=current; return 0; }
   int x=rect.a.x+2, y=rect.a.y;
   for( int i=0, j=items->getCount(); i<j && y<=rect.b.y ; i++, y++ )
      if ( Point(y,x) == cur )
	 { state=i; return 0; }
   return key;
}

long radioBox::changeCurrent( long key )
{
   if ( !owner->hardMode )
      return key;
   switch( key )
    {
       case HASH_cmUp:
	   current = current-1 < 0 ? items->getCount()-1 : current-1;
	   return 0;
       case HASH_cmDown:
	   current = current+1==items->getCount() ? 0 : current+1;
	   return 0;
    }
   return key;
}

/*---------------------------- class horRadioBox ---------------------*/
int horRadioBox::isType(long typ)
{
  return (typ==HASH_horRadioBox ? 1 : getBox::isType(typ));
}

int horRadioBox::draw( int Redraw, int sy, int sx )
{
   Point coor = rect.a + Point(sy,sx);
   int normal = active || bind_flag ? clr[0] : clr[1];
   int count = items->getCount(), len;
   Screen::Clear( coor.y, coor.x, 1, rect.b.x-rect.a.x+1, normal );
   static unsigned char str[128];
   str[width]=0;
   for( int i=0; 1; coor.x+=width, i++ ) {
       if ( i >= count )
	   break;
       unsigned char *ch = (unsigned char*)items->at( i );
       memset( (char*)str, ' ', width );
       len = strlen( (char*)ch );
       for( int j=0; j < len; j++ )
	   str[j] = translate_out( (int)ch[j] );
       int attr = normal;
       if ( isSelect(i) ) {
	   str[2]=flag;
	   attr = active ? clr[2] : clr[3];
       }
       Screen::put( coor, str, attr );
   }
   return 1;
}

void horRadioBox::calcRect()
{
   int i=0, count=items->getCount(), maxlen=0;
   for( ; i<count; i++ )
      maxlen = max( maxlen, strlen((char*)items->at(i)) );
   rect.b.y = rect.a.y;
   rect.b.x = rect.a.x + maxlen * count /*+ 2*/;
   width = count > 0 ? (rect.b.x-rect.a.x+1)/count : 0;
}

long horRadioBox::changeState( long key, Point cur )
{
   if ( owner->hardMode ) {
       state = current;
       return 0;
   }
   for( int i=0, count=items->getCount(); i<count; i++ )
       if ( Point( rect.a.y, rect.a.x + width*i + 2 ) == cur ) {
	   state = i;
	   return 0;
       }
   return key;
}

long horRadioBox::changeCurrent( long key )
{
   if ( !owner->hardMode )
      return key;
   switch( key )
    {
       case HASH_cmLeft:
	   current = current-1>=0 ? current-1 : items->getCount()-1;
	   return 0;
	   break;
       case HASH_cmRight:
	   current = current+1==items->getCount() ? 0 : current+1;
	   return 0;
    }
   return key;
}

/*----------------------- class checkBox ----------------------*/
int checkBox::isType(long typ)
{
  return (typ==HASH_checkBox ? 1 : radioBox::isType(typ));
}


long checkBox::changeState( long key, Point cur )
{
   int c=current;
   if ( owner->hardMode ) {
m1:    long mask = 1 << c;
       if ( state & mask )
	   state &= ~mask;
       else
	   state |= mask;
       return 0;
   }
   int x=rect.a.x+2, y=rect.a.y;
   for( int i=0, count=items->getCount(); i<count && y<=rect.b.y ; i++, y++ )
      if ( Point(y,x) == cur )
	 { c=i; goto m1; }
   return key;
}

/*----------------------- class horCheckBox ----------------------*/
int horCheckBox::isType(long typ)
{
  return (typ==HASH_horCheckBox ? 1 : horRadioBox::isType(typ));
}


long horCheckBox::changeState( long key, Point cur )
{
   int c=current;
   if ( owner->hardMode ) {
m1:    long mask = 1 << c;
       if ( state & mask )
	   state &= ~mask;
       else
	   state |= mask;
       return 0;
   }
   for( int i=0, count=items->getCount(); i<count; i++ )
     if ( Point( rect.a.y, rect.a.x + width*i + 2 ) == cur )
       { c=i; goto m1; }
   return key;
}

