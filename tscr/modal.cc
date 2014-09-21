/*
	$Id: modal.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "hashcode.h"
#include "modal.h"

long modalKeyMap[]={
	kbHome,		HASH_cmHome,
	FUNC1(kbHome),	HASH_cmFirst,
	kbEnd,		HASH_cmEnd,
	FUNC1(kbEnd),	HASH_cmLast,
	kbTab,		HASH_cmNext,
	FUNC1(kbDown),	HASH_cmNext,
	FUNC1(kbRight),	HASH_cmNext,
	FUNC1(kbTab),	HASH_cmPrev,
	FUNC1(kbUp),	HASH_cmPrev,
	FUNC1(kbLeft),	HASH_cmPrev,
	kbLeft,		HASH_cmLeft,
	kbRight,	HASH_cmRight,
	kbUp,		HASH_cmUp,
	kbDown,		HASH_cmDown,
	' ',	HASH_cmNext,
	0
};

Keymap ModalKeyMap( modalKeyMap );

int  modal::isType(long typ)
{
   return (typ==HASH_modal ? 1 : Dialog::isType(typ));
}

modal::modal( int Align, Collection *Text, char *Title, int cm1, char *s1,
	  int cm2, char *s2, int cm3, char *s3,
	  int cm4, char *s4, int cm5, char *s5 ) : align(Align)
{
   keyHolder.add( &ModalKeyMap );
   fill( Text, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 );
}

modal::modal( int Align, char *str, char *Title, int cm1, char *s1,
	  int cm2, char *s2, int cm3, char *s3,
	  int cm4, char *s4, int cm5, char *s5 ) : align(Align)
{
   keyHolder.add( &ModalKeyMap );
   int len;
   Collection *ss = new Collection( 10, 10 );
   if ( str )
    {
      for( char *s=(char*)strchr( str, '\n' ); s; s=(char*)strchr( str, '\n' ) )
	{
	  len = s - str;
	  char *ch = new char[len+1];
	  memcpy( ch, str, len );
	  ch[len]=0;
	  ss->insert( ch );
	  str=s+1;
	}
      len = strlen( str )+1;
      char *ch = new char[len];
      memcpy( ch, str, len );
      ss->insert( ch );
    }
   fill( ss, Title, cm1, s1, cm2, s2, cm3, s3, cm4, s4, cm5, s5 );
   delete ss;
}

void modal::fill( Collection *Text, char *Title, int cm1, char *s1,
	  int cm2, char *s2, int cm3, char *s3,
	  int cm4, char *s4, int cm5, char *s5 )
{
   title->put( Title, language );
   int b=0, len=0;
   if ( cm1 ) { b++; len += s1 ? strlen(s1) : 0; }
   if ( cm2 ) { b++; len += s2 ? strlen(s2) : 0; }
   if ( cm3 ) { b++; len += s3 ? strlen(s3) : 0; }
   if ( cm4 ) { b++; len += s4 ? strlen(s4) : 0; }
   if ( cm5 ) { b++; len += s5 ? strlen(s5) : 0; }

   int maxlen=len, c=0, c2=0, i, j;
   char *ch;
   if ( Text )
     {
       c = c2 = Text->getCount();
       for( i=0; i < c; i++ )
	{
	  ch = (char*)Text->at(i);
m1:
	  int flag=0;
	  for( j=0; *ch; ch++, j++ )
	    if ( *ch == '\n' )
	      { c2++; flag=1; break; }
	  maxlen = max( maxlen, j );
	  if ( flag )
	     { ch++; goto m1; }
	}
       for( i=0, cury=1; i < c; i++, cury++ )
	{
	  ch = (char*)Text->at(i);
m2:
	  int flag=0;
	  for( j=0; *ch; ch++, j++ )
	    if ( *ch == '\n' )
	      { flag=1; break; }
	  if ( align < 0 )
	     curx = 3 + (( maxlen - j ) >> 1);
	  else
	     curx = align;
	  for( ; j>0; j-- )
	     put( *(ch-j) );
	  if ( flag )
	     { ch++; goto m2; }
	}
     }
   rect.a.y = max( 1, (Screen::Lines - c2 - 4) >> 1 );
   rect.b.y = min( Screen::Lines-2, rect.a.y + c2 + 4 );
   rect.a.x = max( 0, (Screen::Columns - maxlen - 7) >> 1 );
   rect.b.x = min( Screen::Columns, rect.a.x + maxlen + 7 );

   int y = c2+2;
   if ( !b )
     {
       insert( new button( Point(y, (rect.b.x-rect.a.x-6)>>1 ), "  OK  ", HASH_cmOK ) );
       return;
     }
   int delta=1, w = (rect.b.x-rect.a.x-len-1)/(b+1);
   if ( w>0 ) delta=w;
   int x = delta + (b%2 ? 1 : 0);

   insert( new button( Point(y,x), s1 ? s1 : " ", cm1 ) );
   x += delta + (s1 ? strlen(s1) : 1);

   if ( !cm2 )
      return;
   insert( new button( Point(y,x), s2 ? s2 : " ", cm2 ) );
   x += delta + (s2 ? strlen(s2) : 1);

   if ( !cm3 )
      return;
   insert( new button( Point(y,x), s3 ? s3 : " ", cm3 ) );
   x += delta + (s3 ? strlen(s3) : 1);

   if ( !cm4 )
      return;
   insert( new button( Point(y,x), s4 ? s4 : " ", cm4 ) );
   x += delta + (s4 ? strlen(s4) : 1);

   if ( !cm5 )
      return;
   insert( new button( Point(y,x), s5 ? s5 : " ", cm5 ) );
}
