/*
	$Id: hash.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef HAS_H_H
#define HAS_H_H
#include <stdlib.h>

unsigned long hashstr(const char *str);

inline unsigned long Hashstr( char *str )
  {
    unsigned long ret=hashstr(str); free(str); return ret;
  }
unsigned long hashdouble(double x);
unsigned long multiplicativehash(int x);

inline int longcmp(long l1, long l2)
 { return l1<l2 ? -1 : (l1>l2 ? 1 : 0); }
inline int doublecmp( double d1, double d2)
 { return d1<d2 ? -1 : (d1>d2 ? 1 : 0); }


#endif
