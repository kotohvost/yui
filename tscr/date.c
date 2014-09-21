/*
	$Id: date.c,v 3.2.2.2 2007/07/24 11:28:11 shelton Exp $
*/

#include <stdlib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define  JULIAN_ADJUSTMENT    1721425L

static char bufDate[5];

static char *day_of_week[8] =
{
   "",
   "Sunday",
   "Monday",
   "Tuesday",
   "Wednesday",
   "Thursday",
   "Friday",
   "Saturday"
};

static char *month_of_year[13] =
{
   "",
   "January",
   "February",
   "March",
   "April",
   "May",
   "June",
   "July",
   "August",
   "September",
   "October",
   "November",
   "December"
};

static int month_tot[] = {
	0, 0,  31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

/*
 * Jan Feb Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
 * 31  28  31   30   31   30   31   31   30   31   30   31
 */

int day_in_year( int year, int month, int day )
{
   /*  Returns */
   /*     >0   The day of the year starting from 1 */
   /*          Ex.    Jan 1, returns  1 */
   /*     -1   Illegal Date */
   int is_leap, month_days ;

   is_leap =  ( (year%4 == 0 && year%100 != 0) || year%400 == 0 ) ?  1 : 0 ;

   month_days = month_tot[ month+1 ] - month_tot[ month] ;
   if ( month == 2 )
	month_days += is_leap ;

   if ( year  < 0  ||
	month < 1  ||  month > 12  ||
	day   < 1  ||  day   > month_days )
	return( -1 ) ;        /* Illegal Date */

   if ( month <= 2 )  is_leap = 0 ;

   return(  month_tot[month] + day + is_leap ) ;
}

long day_to_year( int yr )
{
   /*  Calculates the number of days to the year */
   /*  This calculation takes into account the fact that */
   /*     1)  Years divisible by 400 are always leap years. */
   /*     2)  Years divisible by 100 but not 400 are not leap years. */
   /*     3)  Otherwise, years divisible by four are leap years. */
   /*  Since we do not want to consider the current year, we will */
   /*  subtract the year by 1 before doing the calculation. */
   yr-- ;
   return( yr*365L +  yr/4L - yr/100L + yr/400L ) ;
}

long date_long( char *date_ptr )
{
   /*  Format: DDMMYYYY */
   /*  Returns: */
   /*    >0  -  Julian day */
   /*           That is the number of days since the date  Jan 1, 4713 BC */
   /*           Ex.  Jan 1, 1981 is  2444606 */
   /*     0  -  NULL Date (dbf_date is all blank) */
   /*    -1  -  Illegal Date */
   int  year, month, day, day_year;
   strcpy( bufDate, date_ptr+4 );
   year=atoi( bufDate );

   memcpy( bufDate, date_ptr+2, 2 ); bufDate[2]=0;
   month = atoi( bufDate );

   memcpy( bufDate, date_ptr, 2 ); bufDate[2]=0;
   day = atoi( bufDate );

   day_year = day_in_year( year, month, day );
   if ( day_year < 1 )    /* Illegal Date */
      return -1L ;

   return day_to_year( year ) + day_year + JULIAN_ADJUSTMENT;
}

int date4dow( char *date_ptr )
{
   long date ;
   date = date_long(date_ptr) ;
   if ( date < 0 )
      return 0 ;
   return (int)( ( date + 1 ) % 7 ) + 1 ;
}

char *day( char *date_ptr )
{
   return day_of_week[date4dow(date_ptr)];
}

int date4month( char *date_ptr )
{
   memcpy( bufDate, date_ptr+2, 2 ); bufDate[2]=0;
   return atoi( bufDate );
}

char *month( char *date_ptr )
{
   return month_of_year[date4month( date_ptr )];
}

#ifdef EXAMPLE

#include <stdio.h>
char buf[128];

int main()
{
  while( 1 ) {
      puts( "Enter date (DDMMYYYY) or 'q' to quit:" );
      gets( buf );
      if ( buf[0] == 'q' || buf[0] == 'Q' )
	  break;
      if ( 0 < date_long( buf ) )
	  printf( "It is %s, %s\n\n", day(buf), month(buf) );
      else
	  puts( "Illegal date.\n" );
  }
  return 0;
};

#endif

