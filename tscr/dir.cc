/*
	$Id: dir.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <time.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <reg_expr.h>
#include "dir.h"

struct stat *__readlink( char *p, char *n, Collection *chain )
{
  static struct stat s;
  if ( !p )
     return 0;
  if ( chain )
     chain->freeAll();

  char *ch=0;
  char *path = (char*)malloc( MAXPATHLEN+1 );
  char *name = (char*)malloc( 512 );
  char *buf = (char*)malloc( MAXPATHLEN+1 );
  strcpy( path, p );
  if ( path[ strlen(path)-1 ] != FILESEP && n )
     strcat( path, FILESEPSTR );
  if ( n )
   {
     strcpy( name, n );
     if ( name[ strlen(name)-1 ] == FILESEP )
	name[ strlen(name)-1 ] = 0;
   }
  else if ( (ch = (char*)strrchr( path, FILESEP )) )
   {
     strcpy( name, ch+1 );
     path[ch-path+1]=0;
   }
  else
     name[0]=0;
  int len;
  while( 1 )
   {
     strcpy( buf, path );
     strcat( buf, name );
     if ( (len=readlink( buf, buf, MAXPATHLEN )) <= 0 )
	break;
     buf[len]=0;
     ch = strrchr( buf, FILESEP );
     if ( buf[0] == FILESEP )
      {
	memcpy( path, buf, ch-buf+1 );
	path[ ch-buf+1 ] = 0;
      }
     else if ( ch )
      {
	int l = strlen( path );
	memcpy( path+l, buf, ch-buf+1 );
	path[l+ch-buf+1] = 0;
      }
     strcpy( name, ch && path[0] == FILESEP ? ch+1 : buf );
     if ( !chain )
	continue;
     ch = new char[strlen(path)+strlen(name)+1];
     strcpy( ch, path );
     strcat( ch, name );
     chain->atInsert( 0, ch );
   }
  lstat( buf, &s );
  ::free( path );
  ::free( name );
  ::free( buf );
  return &s;
}


fileItem::fileItem( char *name ) : fname(0), ext(0), mtime(0)
{
  namlen = strlen( name );
  if ( namlen > 0 )
   {
     fname = (char*)malloc( namlen+2 );
     memcpy( fname, name, namlen+1 );
   }
  if ( fname )
   {
     ext=(char*)strrchr( fname, '.' );
     if ( ext )
	ext++;
     else
	ext = fname + namlen;
   }
}

fileItem::~fileItem()
{
  if ( fname )
     ::free( fname );
  if ( mtime )
     ::free( mtime );
}

void fileItem::setTime()
{
  char *strTime = ctime( &(st.st_mtime) );
  if ( !strTime )
     return;
  int len = strlen( strTime );
  mtime = (char*)realloc( mtime, len );
  if ( mtime )
   {
     memcpy( mtime, strTime, len-1 );
     mtime[len-1]=0;
   }
}

fileColl::fileColl() : SortedCollection(50,50)
{
  duplicates=1;
}

void fileColl::freeItem( void *item )
{
  delete (fileItem*)item;
}

int fileColl::compare( void *p1, void *p2 )
{
   int ret=0;
   switch( order )
    {
      case SORT_BY_NAME:
m1:
	  ret = strcmp( ((fileItem*)p1)->fname, ((fileItem*)p2)->fname );
	  break;
      case SORT_BY_EXT:
m2:
	  ret = strcmp( ((fileItem*)p1)->ext, ((fileItem*)p2)->ext );
	  if ( !ret )
	     goto m1;
	  break;
      case SORT_BY_SIZE:
	  ret = ((fileItem*)p1)->st.st_size - ((fileItem*)p2)->st.st_size;
	  if ( !ret )
	     goto m2;
	  break;
      case SORT_BY_TIME:
	  ret = ((fileItem*)p1)->st.st_mtime - ((fileItem*)p2)->st.st_mtime;
	  if ( !ret )
	     goto m2;
    }
   return ret;
}

directory::directory( char *p, char *m, int order ) :
		      d(0), path(0), mask(0)
{
   dirs = new fileColl;
   files = new fileColl;
   newpath( (char*)(p ? p : "/"), m, order );
}

directory::~directory()
{
   if ( dirs )
      delete dirs;
   if ( files )
      delete files;
   if ( d )
      closedir( d );
   if ( path )
      ::free( path );
   if ( mask )
      ::free( mask );
}

int directory::newpath( char *p, char *m, int order )
{
   char *name = (char*)calloc( MAXPATHLEN, sizeof( char ) );
   strcpy( name, p );
#ifdef DJGPP
   if ( name[strlen(name)-1] == ':' )
      strcat( name, FILESEPSTR );
#endif
   DIR *newd = opendir( name );
   if ( !newd  ) {
      ::free( name );
      return 0;
   }

   if ( d )
      closedir( d );
   d=newd;

   if ( path )
      ::free( path );
   path = name;

   name = strdup( m );
   if ( mask )
      ::free( mask );
   mask = name;

   readDir( order );
   return 1;
}

void directory::readDir( int order )
{
   char *bufname = (char*)calloc( MAXPATHLEN, sizeof(char) );
   char *bufpath = (char*)calloc( MAXPATHLEN, sizeof(char) );

   static Regexpr ex;
   static re_registers Regs;

   dirs->freeAll();
   files->freeAll();
   dirs->order = order;
   files->order = order;
   struct dirent *sd;
   int lenpath = strlen( path );
   if ( lenpath>0 )
     {
       memcpy( bufpath, path, lenpath+1 );
       if ( bufpath[lenpath-1] != FILESEP )
	 {
	   bufpath[lenpath] = FILESEP;
	   bufpath[++lenpath] = 0;
	 }
     }
   char *Name = bufpath + lenpath;

   int i, compare = strcmp( mask, "*" );
   if ( compare )
    {
      bufname[0] = '^';
      char *m = mask;
      for( i=1; *m; m++ )
	 switch( *m )
	   {
	     case '*': bufname[i++]='.'; bufname[i++]='*'; break;
	     case '?': bufname[i++]='.'; break;
	     case '.': bufname[i++]='\\'; bufname[i++]='.'; break;
	     default:
		bufname[i++] = *m;
	   }
      bufname[i++]='$';
      bufname[i]=0;
      char *cp = ex.compile( bufname );
      if ( cp )
       {
	 ::free( bufname );
	 ::free( bufpath );
	 return;
       }
    }
   struct passwd *pw=0;
   struct group *gr=0;
   while( (sd = readdir( d )) )
    {
      fileItem *f = new fileItem( sd->d_name );
      struct stat *s = &(f->st);

      memcpy( Name, f->fname, f->namlen+1 );
#ifdef DJGPP
      stat( bufpath, s );
#else
      lstat( bufpath, s );
#endif
#ifndef SINIX
      int ISDIR = S_ISLNK( s->st_mode ) ? S_ISDIR( __readlink( bufpath )->st_mode ) : S_ISDIR( s->st_mode );
#else
      int ISDIR = S_ISDIR( s->st_mode );
#endif
      if ( compare && !ISDIR && -1 == ex.search( f->fname, f->namlen, 0, f->namlen, &Regs ) )
	 continue;

      char *c = findUid( s->st_uid );
      if ( !c )
	{
	  if ( (pw = getpwuid( s->st_uid )) )
	      c = pw->pw_name;
	  else
	    {
	      sprintf( bufname, "%ld", (long)s->st_uid );
	      c = bufname;
	    }
	}
      strncpy( f->uidname, c, UID_GID_LEN );
      f->uidname[UID_GID_LEN] = 0;

      c=findGid( s->st_gid );
      if ( !c )
	{
	  gr = getgrgid( s->st_gid );
	  if ( gr )
	      c = gr->gr_name;
	  else
	    {
	      sprintf( bufname, "%ld", (long)s->st_gid );
	      c = bufname;
	    }
	}
      strncpy( f->gidname, c, UID_GID_LEN );
      f->gidname[UID_GID_LEN] = 0;

      f->setTime();
      if ( ISDIR )
	{
	  if ( !memcmp( f->fname, ".", 2 )
#ifndef DJGPP
	       || !memcmp( f->fname, "..", 3 ) && !memcmp( path, "/", 2 )
#endif
	     )
	     delete f;
	  else
	   {
#ifndef DJGPP
	     strcat( f->fname, FILESEPSTR );
#else
	     strcat( f->fname, "\\" );
#endif
	     f->namlen++;
	     dirs->insert( f );
	   }
	}
      else
	  files->insert( f );
    }
  correctDirs();

  ::free( bufname );
  ::free( bufpath );
}

int directory::resort( int neworder )
{
   if ( neworder == dirs->order )
      return 0;

   fileColl *coll = new fileColl;
   coll->order = neworder;
   int i;
   for( i=dirs->getCount()-1; i>=0; i-- )
      coll->insert( dirs->at(i) );
   dirs->removeAll();
   delete dirs;
   dirs = coll;

   correctDirs();

   coll = new fileColl;
   coll->order = neworder;
   for( i=files->getCount()-1; i>=0; i-- )
      coll->insert( files->at(i) );
   files->removeAll();
   delete files;
   files = coll;
   return 1;
}

char *directory::findUid( uid_t uid )
{
  coll = files;
  f = (fileItem**)coll->getItems();
  int i;
  for( i=coll->getCount()-1; i >= 0; i-- )
     if ( f[i]->st.st_uid == uid )
	return f[i]->uidname;

  coll = dirs;
  f = (fileItem**)coll->getItems();
  for( i=coll->getCount()-1; i >= 0; i-- )
     if ( f[i]->st.st_uid == uid )
	return f[i]->uidname;

  return 0;
}

char *directory::findGid( gid_t gid )
{
  coll = files;
  f = (fileItem**)coll->getItems();
  int i;
  for( i=coll->getCount()-1; i >= 0; i-- )
     if ( f[i]->st.st_gid == gid )
	return f[i]->gidname;

  coll = dirs;
  f = (fileItem**)coll->getItems();
  for( i=coll->getCount()-1; i >= 0; i-- )
     if ( f[i]->st.st_gid == gid )
	return f[i]->gidname;

  return 0;
}

void directory::correctDirs()
{
  if ( dirs->order == SORT_BY_NAME || dirs->order == SORT_BY_EXT )
     return;
  int i, end;
  char buf[4];
  buf[0] = buf[1] ='.';
#ifndef DJGPP
  buf[2]=FILESEP;
#else
  buf[2]='\\';
#endif
  buf[3]=0;
  for( i=0, end=dirs->getCount(); i<end; i++ )
    if ( !memcmp( ((fileItem*)dirs->at(i))->fname, buf, 4 ) )
      {
	fileItem *f = (fileItem*)dirs->at( i );
	dirs->atRemove( i );
	dirs->atInsert( 0, f );
	break;
      }
}

