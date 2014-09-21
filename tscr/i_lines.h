/*
	$Id: i_lines.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _I_LINES_H
#define _I_LINES_H

#include "listbox.h"

#define VAL_STR		0
#define VAL_NON_STR	1

class IntegerInputLine : public inputLine
{
protected:
   virtual void addChar( int ch );
   unsigned negative:2;
   int *intRes;
   char *tmpStr;

public:
   IntegerInputLine( Point p, const char *s=0, char *res=0, int lscr=1,
		int l=0, unsigned Negative=0, Collection *hist=0,
                int fchar=inputLineFillChar );
   IntegerInputLine( Point p, int Val, int *res=0, int lscr=1,
		int l=0, unsigned Negative=0, Collection *hist=0,
                int fchar=inputLineFillChar );
   ~IntegerInputLine();
   virtual char *valid( void *data=0 );
   virtual int accept( int flagMessage=1 );
   virtual int type() { return TYPE_INTEGER; }
   int valType;
};

class FloatInputLine : public inputLine
{
protected:
   virtual void addChar( int ch );
   unsigned negative:2;
   double *doubleRes;
   char *tmpStr;

public:
   FloatInputLine( Point p, const char *s=0, char *res=0, int lscr=1,
		int l=0, unsigned Negative=0, Collection *hist=0,
		int fchar=inputLineFillChar );
   FloatInputLine( Point p, double Val, double *res=0, int lscr=1,
		int l=0, unsigned Negative=0, Collection *hist=0,
                int fchar=inputLineFillChar );
   ~FloatInputLine();
   virtual char *valid( void *data=0 );
   virtual int accept( int flagMessage=1 );
   virtual int type() { return TYPE_FLOAT; }
   int valType;
};


#define FIELD_DAY	1
#define FIELD_MONTH	2
#define FIELD_YEAR	3

extern "C" {
extern long date_long( char *date_ptr );
/*
    date_ptr format: DDMMYYYY
    Returns:
    >0  -  Julian day
           That is the number of days since the date  Jan 1, 4713 BC
           Ex.  Jan 1, 1981 is  2444606
     0  -  NULL Date (dbf_date is all blank)
    -1  -  Illegal Date
*/
}

class DateInputLine : public inputLine
{
protected:
   virtual void addChar( int ch );
   virtual void deleteChar();
   virtual void backspace();
   int checkField( int dir );
   char offD, offM, offY;
   virtual void clearStr();

public:
   DateInputLine( Point p, const char *pattern=0, char *res=0,
		Collection *hist=0, int fchar=inputLineFillChar );
   static char formatDate[11];
   static int Year;
   virtual int init( void *data );
   Point getCursor( int *hide );
   void setFormat( char *format );
   virtual int initString( const char *s );
   virtual int accept( int flagMessage=1 );
   virtual int type() { return TYPE_DATE; }
   virtual char *valid( void *data=0 );
   virtual void moveLeft();
   virtual void moveRight();
   virtual void moveHome();
   virtual void moveEnd();
   virtual void checkDate();
   int valType;
};

#endif
