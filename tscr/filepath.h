/*
	$Id: filepath.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/

#ifndef _FILEPATH_H_
#define _FILEPATH_H_

char *StartPath( char *path );
char *CurWorkPath();
void TranslatePath( char *path, int plen );

#endif
