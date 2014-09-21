/*
	$Id: calc.cc,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <listbox.h>
#include "calc.h"
#include "hashcode.h"

static Collection calcHist(10,10);

Calculator::Calculator() :
	Dialog( Screen::center( 8, 44 ), new Lang("Calculator","Калькулятор"), new Lang("  "), new Lang("~F10~-base change","~F10~-смена системы счета") ),
	x(0), y(0), m(0), base(DEC), flagEnter(0), oldKey(0), baseStr(0)
{
   il = new inputLine( Point(2,4), 0, str, 32/*scrlen*/, 32, 0/*regexp*/, &calcHist, ' ' );
   insert( il );
   put( 0, 1, "Y:\n\n X:\n M:", 1 );
   memset( str, '-', 44 ); str[44]=0;
   put( 4, 0, str );
   put( 5, 1, lang("Operations:               bits:",
		   "  Операции:               биты:"), 1 );
   put( 5, 13, "+ - * / ^ %" );
   put( 5, 33, "| & ! ~" );
   setBase();
   setHelpContext( "Calculator" );
}

long Calculator::handleKey( long key, void *&ptr )
{
   switch( key )
    {
      case ' ':
	  return 0;
      case kbEnter:
	  flagEnter=1;
      case '+':
      case '-':
      case '*':
      case '/':
      case '^':
      case '%':
      case '|':
      case '&':
      case '!':
      case '~':
      case kbPgDn:
      case kbPgUp:
	  key = prepare( key );
	  flagEnter=0;
	  break;
      case kbCtrlY:
	  il->initString( 0 );
	  key=0; break;
      case kbF10:
	  setBase( 1 );
	  key=0; break;
    }
   key = Dialog::handleKey( key, ptr );
   if ( flagEnter && key == HASH_cmOK )
      key=0;
   return key;
}

long Calculator::prepare( long key )
{
   x = getInput();
   int flagEmpty = strlen(str) ? 0 : 1;
   if ( flagEmpty )
     {
       if ( key == kbEnter ) return 0;
       if ( key == '-' && oldKey ) return key;
     }
   else
     {
       char ch = str[strlen(str)-1];
       if ( key == '-' && (ch == 'e' || ch == 'E') )
	 { il->resetFirst(); return key; }
     }

   switch( key )
     {
       case kbPgDn:
	   m = flagEmpty ? y : x;
	   setStr( m );
	   clearLine( 3 );
	   put( 3, 1, "M:", 1 );
	   put( 3, 4, str );
	   return 0;
       case kbPgUp:
	   setStr( m );
	   il->initString( str );
	   return 0;
       case '~':
	   oldKey = key;
     }

   clearLine( 0 );
   put( 0, 1, "Y:", 1 );
   char *err = 0;
   unsigned long l;

   switch( oldKey )
    {
      case '+':
	  y += x;
	  break;
      case '-':
	  y -= x;
	  break;
      case '*':
	  y *= x;
	  break;
      case '/':
	  if ( x )
	     y /= x;
	  else
	    { y=0; err = "Error"; }
	  break;
      case '^':
	  y = pow( y, x );
	  break;
      case '%':
	  l = (unsigned long)(y/x);
	  y -= x * (double)l;
	  break;
      case '|':
	  l = (unsigned long)y | (unsigned long)x;
	  y = l;
	  break;
      case '&':
	  l = (unsigned long)y & (unsigned long)x;
	  y = l;
	  break;
      case '!':
	  l = (unsigned long)y ^ (unsigned long)x;
	  y = l;
	  break;
      case '~':
	  y = ~(unsigned long)x;
	  break;
      default:
	  if ( !flagEmpty )
	     y = x;
    }

   if ( !err )
      setStr( y );
   else
      strncpy( str, err, 32 );

   str[32]=0;
   put( 0, 1, "Y:", 1 );
   put( 0, 4, str );
   put( 0, 37, " " );
   put( 0, 38, baseStr, 1 );

   char ch[2]; ch[1]=0;
   switch( key )
    {
      case kbEnter:
	  il->initString( 0 );
      case '~':
	  clearLine( 1 );
	  oldKey=0;
	  break;
      case '-':
      case '+':
      case '*':
      case '/':
      case '^':
      case '%':
      case '|':
      case '&':
      case '!':
	  oldKey = key;
	  ch[0] = (char)key;
	  put( 1, 4, ch );
	  il->initString( 0 );
    }
   return 0;
}

void Calculator::setBase( int p )
{
   x = getInput( 1 );
   switch( base )
    {
      case BIN:
	  if ( !p )
	     baseStr = "BIN";
	  else
	    { base=OCT; baseStr="OCT"; }
	  break;
      case OCT:
	  if ( !p )
	     baseStr = "OCT";
	  else
	    { base=DEC; baseStr="DEC"; }
	  break;
      case DEC:
	  if ( !p )
	     baseStr = "DEC";
	  else
	    { base=HEX; baseStr="HEX"; }
	  break;
      case HEX:
	  if ( !p )
	     baseStr = "HEX";
	  else
	    { base=BIN; baseStr="BIN"; }
    }
   if ( strlen( str ) )
    {
      setStr( x );
      il->initString( str );
    }

   clearLine( 0 );
   clearLine( 3 );
   put( 0, 1, "Y:", 1 );
   setStr( y );
   put( 0, 4, str );
   put( 0, 37, " " );
   put( 0, 38, baseStr, 1 );
   put( 2, 38, baseStr, 1 );
   put( 3, 1, "M:", 1 );
   setStr( m );
   put( 3, 4, str );
}

double Calculator::getInput( int flag )
{
   il->accept();
   if ( flag )
      il->resetFirst();
   if ( base == DEC )
      return strtod( str, NULL );
   return (double)strtoul( str, NULL, base );
}

void Calculator::setStr( double d )
{
   unsigned long Y;
   char *p=0;
   switch( base )
    {
      case BIN:
	  str[0]=0; Y = (unsigned long)d;
	  for( int i=31; i>=0; i-- )
	     strcat( str, Y & (1<<i) ? "1" : "0" );
	  break;
      case OCT:
	  sprintf( str, "%lo", (unsigned long)d );
	  break;
      case DEC:
	  sprintf( str, "%.12f", d );
	  p = (char*)strchr( str, '.' );
	  for( char *e = p+strlen(p+1); e>=p; e-- )
	   {
	     if ( *e != '0' && *e != '.' )
		break;
	     *e = 0;
	   }
	  if ( p && p-str > 16 || strlen(str) > 24 )
	     sprintf( str, "%g", d );
	  break;
      case HEX:
	  if ( d > ULONG_MAX )
	    y = d = ULONG_MAX;
	  sprintf( str, "%lx", (unsigned long)d );
    }
}

