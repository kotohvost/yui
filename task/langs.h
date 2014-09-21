/*
	$Id: langs.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef _LANGS_H_
#define _LANGS_H_

/*
 * multi language support
 */

#define LANGS	2

struct Lang
{
  char **msg;
  Lang( char **arr=0 );
  Lang( const char *s1, const char *s2=0 );
  ~Lang();
  char *get( int index=-1 );
  int put( const char *str, int index );
};

char *lang( char *s1, char *s2 );

#endif
