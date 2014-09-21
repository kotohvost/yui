/*
	$Id: filepath.c,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "filepath.h"

static char start_path[MAXPATHLEN+1];
static char work_path[MAXPATHLEN+1];

char *CurWorkPath()
{
  return work_path;
}

char *StartPath( char *path )
{
  getcwd( work_path, MAXPATHLEN-1 );
  work_path[MAXPATHLEN-1] = 0;
  if ( !path )
      return start_path;
  strncpy( start_path, path, MAXPATHLEN-1 );
  start_path[MAXPATHLEN-1] = 0;

  if ( strchr( start_path, '/' ) ) {
      int i=0;
      if ( start_path[0] != '/' ) {
	  getcwd( start_path, MAXPATHLEN-1 );
	  strncat( start_path, "/", 2 );
	  strncat( start_path, path, MAXPATHLEN - strlen(start_path) );
      }
      for( i=strlen(start_path)-1; i>=0 ; i-- ) {
	  if ( start_path[i] == '/' ) {
	      start_path[ i ? i : 1 ]=0;
	      break;
	  }
      }
  } else {
      DIR *dir = NULL;
      struct dirent *ent;
      struct stat st;
      const char *spath = getenv( "PATH" );
      char *cpath = (char*)calloc( MAXPATHLEN+1, 1 );
      int beg=0, found=0, slen=strlen( spath );
      cpath[0] = cpath[MAXPATHLEN] = 0;
      for( ; !found && beg < slen ; beg++ ) {
	  int i=beg, len=0;
	  for( ; i < slen && spath[i] != ':'; len++, i++ );
	  if ( len == 0 ) {
	      strcpy( cpath, "." );
	  } else {
	      len = len < MAXPATHLEN ? len : MAXPATHLEN;
	      memcpy( cpath, spath+beg, len );
	      cpath[len]=0;
	  }
	  beg+=len;
	  if ( !(dir=opendir( cpath )) )
	      continue;
	  while( (ent = readdir(dir)) ) {
	      if ( !strcmp( path, ent->d_name )  ) {
		  strcpy( start_path, cpath );
		  strcat( start_path, "/" );
		  strcat( start_path, path );
		  if ( !stat( start_path, &st ) && S_ISREG( st.st_mode ) ) {
		      found=1;
		      break;
		  }
	      }
	  }
	  closedir(dir);
      }
      strcpy( start_path, found ? cpath : "." );
      free( cpath );
  }

  if ( start_path[0]=='.' && start_path[1]==0 )
      getcwd( start_path, MAXPATHLEN );

  while( 1 ) {
      char *p=0;
      if ( (p=strstr(start_path,"/./")) == 0 )
	  break;
      strcpy( p+1, p+3 );
  }

  while( 1 ) {
      int i, k, nslash;
      char *p = NULL;
      if ( (p=strstr(start_path, ".." )) == 0 )
	  break;
      k = p - start_path + 1;
      i = k;
      nslash = 0;
      for( ; i>=0 ; i-- ) {
	  if ( start_path[i] == '/' )
	      nslash++;
	  if ( nslash > 1 )
	      break;
      }
      if ( nslash != 2 )
	  break;
      strcpy( start_path+i+(i?0:1), path+k+1 );
  }
  return start_path;
}

void TranslatePath( char *path, int plen )
{
#ifndef DJGPP
    #define STARTDIR "^/"
    #define HOMEDIR "~/"
#else
    #define STARTDIR "^\\"
    #define HOMEDIR "~\\"
#endif
  const char *s = 0;
  int i, sl;
  if ( plen < 4 )
      return;
  if ( !memcmp(path, STARTDIR, 1) ) {
      s = StartPath( NULL );
  } else if ( !memcmp(path, HOMEDIR, 1) ) {
#ifndef DJGPP
      s = getenv("HOME");
#else
      s = StartPath();
#endif
  } else {
      return;
  }
  i = strlen( path ) - 1;
  sl = strlen( s );
  for( ; i>=0 && (path[i]==' '||path[i]=='\t'); i-- )
      path[i]=0;
  if ( i+sl >= plen )
      return;
  memmove(path+sl, path+1, i);
  memcpy(path, s, sl);
  path[i+sl]=0;
}
