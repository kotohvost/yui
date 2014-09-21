/*
	$Id: i_lines.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "i_lines.h"

static char bufInput[128];

/*--------------------- class IntegerInputLine ------------------*/

IntegerInputLine::IntegerInputLine( Point p, const char *s, char *res,
	int lscr, int l, unsigned Negative, Collection *hist, int fchar ) :
	inputLine( p, s, res, lscr, l, 0, hist, fchar ),
	negative( Negative ), intRes(0), tmpStr(0)
{
   valType = VAL_STR;
}

IntegerInputLine::IntegerInputLine( Point p, int Val, int *res,
	int lscr, int l, unsigned Negative, Collection *hist, int fchar ) :
	inputLine( p, 0, 0, lscr, l, 0, hist, fchar ),
	negative( Negative ), intRes(res), tmpStr(0)
{
   valType = VAL_NON_STR;
   sprintf( (char*)bufInput, "%d", Val );
   tmpStr = new char[strlen((char*)bufInput)+1];
   strcpy( tmpStr, bufInput );
   origStr = tmpStr;
}

IntegerInputLine::~IntegerInputLine()
{
   if ( tmpStr )
     { delete tmpStr; tmpStr=0; }
}

char *IntegerInputLine::valid( void *data )
{
   char *s = data ? (char*)data : str;
   static char buf[64];
   int i, flagN=0, flagD=0;
   for( i=0; s[i]==' '; i++ );
   if ( s[i] == '-' )
    {
      if ( !negative )
	 goto bad_integer;
      flagN=1; i++;
    }
   for( ; s[i]; i++ )
     if ( !isdigit( s[i] ) )
	goto bad_integer;
     else
	flagD=1;
   if ( flagD || !flagN )
      return 0;
bad_integer:
   sprintf( buf, "Bad integer value: '%s'", s );
   return buf;
}

int IntegerInputLine::accept( int flagMessage )
{
   int ret = inputLine::accept( flagMessage );
   if ( ret )
    {
      char *s = str;
      for( ; *s == ' '; s++ );
      if ( intRes )
	 *intRes = atoi( s );
      firstkey=1;
    }
   return ret;
}

void IntegerInputLine::addChar( int ch )
{
   int i, flagL=0, flagR=0;
   for( i=0; i<pos; i++ )
     if ( str[i] && str[i] != ' ' )
	flagL=1;
   for( ; i<len; i++ )
     if ( str[i] && str[i] != ' ' )
	flagR=1;
   if ( !isdigit(ch) && ch!='-' && ch!=' ' ||
	flagL && (ch==' '|| ch=='-'|| str[pos-1]==' '|| !str[pos-1]) ||
	flagR && str[pos]==' ' ||
	ch=='-' && (!negative || str[pos]==ch) ||
	isdigit(ch) && str[pos]=='-' )
      { Screen::beep(); return; }

   inputLine::addChar( ch );
}

/*--------------------- class FloatInputLine ------------------*/

FloatInputLine::FloatInputLine( Point p, const char *s, char *res,
	int lscr, int l, unsigned Negative, Collection *hist, int fchar ) :
	inputLine( p, s, res, lscr, l, 0, hist, fchar ),
	negative( Negative ), doubleRes(0), tmpStr(0)
{
   valType = VAL_STR;
}

FloatInputLine::FloatInputLine( Point p, double Val, double *res,
	int lscr, int l, unsigned Negative, Collection *hist, int fchar ) :
	inputLine( p, 0, 0, lscr, l, 0, hist, fchar ),
	negative( Negative ), doubleRes(res), tmpStr(0)
{
   valType = VAL_NON_STR;
   sprintf( (char*)bufInput, "%g", Val );
   tmpStr = new char[strlen((char*)bufInput)+1];
   strcpy( tmpStr, bufInput );
   origStr = tmpStr;
}

FloatInputLine::~FloatInputLine()
{
   if ( tmpStr )
     { delete tmpStr; tmpStr=0; }
}

char *FloatInputLine::valid( void *data )
{
   char *s = data ? (char*)data : str;
   static char buf[64];
   int i, flagN=0, flagD=0, flagP=0;
   for( i=0; s[i]==' '; i++ );
   if ( s[i] == '-' )
    {
      if ( !negative )
	 goto bad_value;
      flagN=1; i++;
    }
   for( ; s[i]; i++ )
    {
      if ( s[i] == '.' )
       {
	 if ( flagP )
	    goto bad_value;
	 flagP=1;
       }
      else if ( !isdigit( s[i] ) )
	 goto bad_value;
      else
	 flagD=1;
    }
   if ( flagD || !flagN )
      return 0;
bad_value:
   sprintf( buf, "Bad double value: '%s'", s );
   return buf;
}

int FloatInputLine::accept( int flagMessage )
{
   int ret = inputLine::accept( flagMessage );
   if ( ret )
    {
      char *s = str;
      for( ; *s == ' '; s++ );
      if ( doubleRes )
	 *doubleRes = atof( s );
      firstkey=1;
    }
   return ret;
}

void FloatInputLine::addChar( int ch )
{
   int i, flagL=0, flagR=0, flagP=0, posP=-1;
   for( i=0; i<pos; i++ )
     switch( str[i] )
      {
	case 0: case ' ': continue;
	case '.':	flagP=1; posP=i;    // without 'break' !
	default:	flagL=1;
      }
   for( ; i<len; i++ )
     switch( str[i] )
      {
	case 0: case ' ': continue;
	case '.':	flagP=1; posP=i;     // without 'break' !
	default:	flagR=1;
      }
   if ( ch=='.' && flagP )
    {
      if ( posP < len-1 )
	 pos = posP+1;
      if ( pos < delta )
	 delta = pos;
      else if ( pos-delta >= scrlen )
	 delta = pos - scrlen + 1;
      return;
    }
   if ( !isdigit(ch) && ch!='-' && ch!=' ' && ch!='.' ||
	flagL && (ch==' '|| ch=='-'|| str[pos-1]==' '|| !str[pos-1]) ||
	flagR && str[pos]==' ' && (insMode || str[pos+1]==' ' || str[pos+1]=='-') ||
	ch=='-' && (!negative || str[pos]==ch || !insMode && str[pos+1]==ch) ||
	isdigit(ch) && str[pos]=='-' )
      { Screen::beep(); return; }

   inputLine::addChar( ch );
}

/*--------------------- class DateInputLine ------------------*/

#define LEN_DATE	10
int DateInputLine::Year = 37;

char DateInputLine::formatDate[11]={0,0,0,0,0,0,0,0,0,0,0};

DateInputLine::DateInputLine( Point p, const char *pattern, char *res,
		Collection *hist, int fchar ) :
	inputLine( p, pattern, res, LEN_DATE, LEN_DATE, 0, hist, fchar ),
	offD(-1), offM(-1), offY(-1)
{
   valType = VAL_STR;
   firstkey=0;
   if ( !formatDate[0] )
      strcpy( (char*)formatDate, "dd/mm/yyyy" );
   setFormat( formatDate );
}

int DateInputLine::init( void *data )
{
   return inputLine::init( data );
}

Point DateInputLine::getCursor( int *hide )
{
   if ( pos==2 || pos==5 )
      inputLine::moveRight();
   return inputLine::getCursor( hide );
}

void DateInputLine::setFormat( char *fmt )
{
   int lenfmt = strlen( fmt );
   if ( lenfmt != 10 || fmt[2] != fmt[5] ||
	fmt[0] != fmt[1] || fmt[3] != fmt[4] || fmt[6] != fmt[7] ||
	fmt[7] != fmt[8] || fmt[8] != fmt[9] )
      goto default_format;
   if ( fmt[6] != 'y' && fmt[7] != 'Y' )
      goto default_format;
   offY=6;
   if ( (fmt[0]=='d' || fmt[0]=='D') && (fmt[3]=='m' || fmt[4]=='M') )
     { offD=0; offM=3; }
   else if ( (fmt[0]=='m' || fmt[0]=='M') && (fmt[3]=='d' || fmt[4]=='D') )
     { offM=0; offD=3; }

   origScrLen = len = 10;
   return;

default_format:
   offD=0; offM=3; offY=6;
   origScrLen = len = 10;
}

int DateInputLine::initString( const char *pattern )
{
   int i=0, j=0, l=0, bad=0;
   static char buf[LEN_DATE+1];
   if ( !pattern || (l=strlen(pattern))<8 || l>10 )
      bad=1;
   else
    {
      formatDate[2] = formatDate[5] = pattern[2];
      for( i=0, j=0; i < len && pattern[i]; i++ )
       {
	 if ( isdigit(pattern[i]) || i==2 || i==5 )
	  {
	    if ( i == 6 && l < 10 )
	     {
	       if ( l == 9 )
		  buf[j++] = '0';
	       else
		{
		  if ( atoi( pattern+6 ) <= Year )
		   {
		     buf[j++] = '2';
		     buf[j++] = '0';
		   }
		  else
		   {
		     buf[j++] = '1';
		     buf[j++] = '9';
		   }
		}
	     }
	    buf[j++] = (i==2 || i==5 ? formatDate[2] : pattern[i]);
	  }
	 else
	  { bad=1; break; }
       }
    }
   if ( bad )
    {
      memset( buf, ' ', len );
      buf[2] = buf[5] = formatDate[2];
      buf[len]=0;
    }
   else
      buf[j]=0;
   origScrLen = len = 10;
   if ( origP.y >= 0 )
      rect = Rect( origP.y, origP.x, origP.y, origP.x + origScrLen - (history ? 0 : 1) );
   inputLine::initString( buf );
   return 1;
}

int DateInputLine::accept( int flagMessage )
{
   checkDate();
   int ret = inputLine::accept( flagMessage );
   firstkey=0;
   return ret;
}

int DateInputLine::checkField( int dir )
{
   static char buf[5];
   int i=0, l;
   char *fld = pos<=2 ? (dir==1 ? str : str+3) :
			(pos<=5 ? (dir==1 ? str+3 : str+6) : str+6);
   if ( !fld )
      return 1;

   if ( isdigit( fld[0] ) )
      buf[i++] = fld[0];
   else if ( fld[0] != ' ' )
      return 0;

   if ( isdigit( fld[1] ) )
      buf[i++] = fld[1];
   else if ( fld[1] != ' ' )
      return 0;

   buf[i]=0;
   l = atol( buf );
   if ( fld == str+offD )
    {
      if ( l<1 || l>31 )
	 goto bad_field;
    }
   else if ( fld == str+offM )
    {
      if ( l<1 || l>12 )
	 goto bad_field;
    }
   if ( pos < offY )
    {
      sprintf( buf, "%02d", l );
      memcpy( fld, buf, 2 );
    }
   return 1;
bad_field:
//   Screen::beep();
   return 0;
}

char *DateInputLine::valid( void *data )
{
   char *s = data ? (char*)data : str;
   char *c="    ";
   if ( s[2]==s[5] && !memcmp(s,c,2) && !memcmp(s+3,c,2) && !memcmp(s+6,c,4) )
      return 0;
   memcpy( bufInput, s+offD, 2 );
   memcpy( bufInput+2, s+offM, 2 );
   bufInput[4] = 0;
   static char buf[64];
   strcat( bufInput, s+offY );
   long longDate = date_long( bufInput );
   if ( longDate > 0 )
      return 0;
   sprintf( buf, "Invalid date: %s", s );
   return buf;
}

void DateInputLine::addChar( int ch )
{
   if ( !isdigit(ch) )
     { Screen::beep(); return; }
   insMode=0;
   if ( pos != 2 && pos != 5 )
      inputLine::addChar( ch );
   if ( pos==2 || pos==5 )
     if ( checkField( 1 ) )
	inputLine::moveRight();
     else
	inputLine::moveLeft();
}

void DateInputLine::deleteChar()
{
   if ( pos==2 || pos==5 )
      return;
   int end = pos<2 ? 2 : (pos<5 ? 5 : len);
   for( int i=pos+1; i<end; i++ )
      str[i-1] = str[i];
   str[end-1] = (end-1 >= offY ? 0 : ' ');
}

void DateInputLine::backspace()
{
   if ( pos <= 0 )
      return;
   if ( pos == 3 || pos == 6 )
     {
       if ( !checkField( 1 ) )
	  return;
       inputLine::moveLeft();
       inputLine::moveLeft();
       deleteChar();
       return;
     }
   int end = pos<2 ? 2 : (pos<5 ? 5 : len);
   for( int i=pos--; i<end; str[i-1] = str[i++] );
   str[end-1] = (end-1 >= offY ? 0 : ' ');
}

void DateInputLine::moveLeft()
{
  inputLine::moveLeft();
  if ( pos == 2 || pos == 5 )
    if ( checkField( -1 ) )
       inputLine::moveLeft();
    else
       inputLine::moveRight();
}

void DateInputLine::moveRight()
{
  inputLine::moveRight();
  if ( pos == 2 || pos == 5 )
    if ( checkField( 1 ) )
       inputLine::moveRight();
    else
       inputLine::moveLeft();
}

void DateInputLine::moveHome()
{
  if ( checkField( 1 ) )
    inputLine::moveHome();
}

void DateInputLine::moveEnd()
{
  if ( checkField( 1 ) )
    inputLine::moveEnd();
}

void DateInputLine::clearStr()
{
  char br = str[2];
  memset( str, ' ', len );
  str[2] = str[5] = br;
}

void DateInputLine::checkDate()
{
  char *c="    ";
  if ( !memcmp(str,c,2) && !memcmp(str+3,c,2) && !memcmp(str+offY,c,4) )
     return;
  int p = pos;
  pos=1; checkField(1);
  pos=4; checkField(1);
  pos=p;

  int i;
  for( i=LEN_DATE-1; str[i]==' ' && i>=offY; str[i--]=0 );
  for( i=offY; str[i]==' ' && i < LEN_DATE; i++ );
  if ( i < LEN_DATE )
   {
     memmove( str+offY, str+i, LEN_DATE-i );
     str[offY+LEN_DATE-i] = 0;
   }

  int slen = strlen( str ) - offY;
  if ( slen <= 0 )
   {
     time_t t = time(0);
     struct tm *_tm = localtime( &t );
     sprintf( str+offY, "%d", _tm->tm_year );
   }
  int year = atoi( str+offY );
  if ( year > 999 )
     return;
  if ( year > 99 || slen == 3 )
   {
     memmove( str+offY+1, str+offY, 3 );
     str[offY] = '0';
   }
  else
   {
     i = (year > 9 || slen == 2 ? 0 : 1);
     memmove( str+offY+2+i, str+offY, 2-i );
     memcpy( str+offY, (year <= Year ? "20" : "19"), 2 );
     if ( i )
       str[offY+2] = '0';
   }
}
