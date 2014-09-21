/*
	$Id: dir.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _DIR_H
#define _DIR_H

#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>

extern "C" {
#include "filepath.h"
}

#include "collect.h"

#define SORT_BY_NAME	0
#define SORT_BY_EXT	1
#define SORT_BY_SIZE	2
#define SORT_BY_TIME	3

#define FILESEP		'/'
#define FILESEPSTR	"/"

#define UID_GID_LEN	12

struct stat *__readlink( char *p, char *n=0, Collection *chain=0 );

struct fileItem
{
   char *fname;		// file name
   char *ext;		// file extension
   char *mtime;
   unsigned short namlen;	// length of file name
   char uidname[UID_GID_LEN+1];
   char gidname[UID_GID_LEN+1];
   struct stat st;
   fileItem( char *name );
   ~fileItem();
   void setTime();
};

class fileColl : public SortedCollection
{
protected:
   void freeItem( void *item );
   int compare( void *p1, void *p2 );
public:
   fileColl();
   ~fileColl() { freeAll(); }
   int order;
};

class directory
{
protected:
   DIR *d;
   char *findUid( uid_t uid );
   char *findGid( gid_t gid );
   void correctDirs();
   fileColl *coll;
   fileItem **f;

public:
   directory( char *p=0, char *m="*", int order=SORT_BY_NAME );
   ~directory();
   int newpath( char *p, char *m="*", int order=SORT_BY_NAME );
   void readDir( int order );
   int resort( int neworder );
   fileColl *dirs;
   fileColl *files;
   char *path;
   char *mask;
};

#endif
