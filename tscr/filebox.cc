/*
	$Id: filebox.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "hashcode.h"
#include "filebox.h"

int filebox::fileSortOrder = SORT_BY_NAME;
char filebox::filePath[MAXPATHLEN];
char filebox::fileMask[128];

Collection filebox::fileHistory(20,10);

int  filebox::isType(long typ)
{
  return (typ==HASH_filebox ? 1 : Dialog::isType(typ));
}

#ifdef DJGPP
void convertPath( char *s, int c1, int c2 )
{
  for( ; *s; s++ )
    if ( *s == c1 )
       *s = c2;
}
#endif

filebox::filebox( char *Path, char *Mask, char *outn, char *outp, char *outm ) :
	Dialog( Rect(0,0,0,0) ), d(0),
	outname(outn), outpath(0), outmask(0), d_path(0), d_mask(0),
	cur(0), d_sort(fileSortOrder)
{
#if defined(SOLARIS_SPARC) || defined(SOLARIS_X86)
   stk_len = 16384;
#else
 #if defined(LINUX_X86_LIBC6) || defined(LINUX_ALPHA_LIBC6)
   stk_len = 65536;
 #endif
#endif
   str = new char[MAXPATHLEN];
   oldname = new char[MAXPATHLEN];
   item_info = new char[MAXPATHLEN*2];
   d_path = strdup( Path );
   d_mask = strdup( Mask );
   setHelpContext( "Files" );
   struct passwd *pw = getpwuid( uid=getuid() );
   struct group *gr  = getgrgid( gid=getgid() );
   if ( pw && pw->pw_name )
      sprintf( item_info, "%s.", pw->pw_name );
   else
#if defined(LINUX_X86_LIBC5) || defined(LINUX_X86_LIBC6) \
 || defined(LINUX_ALPHA_LIBC6) || defined(FreeBSD)
      sprintf( item_info, "%d.", uid );
#else
      sprintf( item_info, "%ld.", uid );
#endif
   if ( gr && gr->gr_name )
      sprintf( item_info + strlen(item_info), "%s", gr->gr_name );
   else
#if defined(LINUX_X86_LIBC5) || defined(LINUX_X86_LIBC6) \
 || defined(LINUX_ALPHA_LIBC6) || defined(FreeBSD)
      sprintf( item_info + strlen(item_info), "%d", gid );
#else
      sprintf( item_info + strlen(item_info), "%ld", gid );
#endif
   title->put( item_info, 0 );
   status->put( "~^J^F~-sort  ~-~,~+~ -mask  ~Enter~-select", 0 );
   status->put( "~^J^F~-сортировка  ~-~,~+~ -маска  ~Enter~-выбор", 1 );
   strcpy( str, Path );
   if ( str[strlen(str)-1] != FILESEP )
      strcat( str, FILESEPSTR );
   strcat( str, Mask );
#ifdef DJGPP
   convertPath( str, FILESEP, '\\' );
#endif
   il = new inputLine( Point(1,0), str, str, 0, MAXPATHLEN-1, 0, &fileHistory );
   il->init();
   insert( il );
   lb = new listBox( Rect(0,0,0,0), 0, &cur );
   Rect r( scrRect() );
   resizeScreen( r );
   strcpy( lb->mask, fileMask );
   insert( lb );
   oldname[0] = 0;
   current = 0;
   il->moveEnd();
}

filebox::~filebox()
{
   if ( filePath && d->path )
      strcpy( filePath, d->path );
   if ( fileMask && lb->mask )
      strcpy( fileMask, lb->mask );
   if ( str )
      delete str;
   if ( oldname )
      delete oldname;
   if ( item_info )
      delete item_info;
   if ( d )
      delete d;
}

int filebox::init( void *data )
{
   int ret = Dialog::init( data );
   if ( !d )
    {
      d = new directory( d_path, d_mask, d_sort );
      ::free( d_path ); d_path=0;
      ::free( d_mask ); d_mask=0;
      fillBox();
    }
   return ret;
}

void filebox::fillItem( fileItem *f )
{
   memset( item_info, ' ', 512 );
   int width = size.x;
   if ( flagDirs )
     {
       memcpy( item_info+1, f->fname, min(f->namlen-1,width-8) );
       memcpy( item_info+width-8, "<DIR>\n", 6 );
     }
   else
     {
       memcpy( item_info+1, f->fname, min(f->namlen,width-11) );
       sprintf( item_info+width-13, "%10ld\n", (long)f->st.st_size );
     }
   mode_t m = f->st.st_mode;
   int uidf=(uid==f->st.st_uid), gidf=(gid==f->st.st_gid);
   if ( uidf ) gidf=0;
   if ( gidf ) uidf=0;
   int othf = !(uidf || gidf);

   int len = width-2;
   item_info[len++] = ' ';
   item_info[len++] = m & S_IRUSR ? (uidf ? 'R' : 'r') : '-';
   item_info[len++] = m & S_IWUSR ? (uidf ? 'W' : 'w') : '-';
   item_info[len++] = m & S_IXUSR ? (uidf ? 'X' : 'x') : '-';

   item_info[len++] = m & S_IRGRP ? (gidf ? 'R' : 'r') : '-';
   item_info[len++] = m & S_IWGRP ? (gidf ? 'W' : 'w') : '-';
   item_info[len++] = m & S_IXGRP ? (gidf ? 'X' : 'x') : '-';

   item_info[len++] = m & S_IROTH ? (othf ? 'R' : 'r') : '-';
   item_info[len++] = m & S_IWOTH ? (othf ? 'W' : 'w') : '-';
   item_info[len++] = m & S_IXOTH ? (othf ? 'X' : 'x') : '-';

   int l = strlen(f->uidname);
   memcpy( item_info+len+max( 0, 8-l ), f->uidname, min(8,l) );
   len+=8;
   item_info[len++] = '.';
   strncpy( item_info+len, f->gidname, min(8,strlen(f->gidname)) );
   len+=8;
   item_info[len++]=' ';
   if ( f->mtime )
      strcpy( item_info + width*2 - 25, f->mtime + 4 );
   else
      item_info[len-1]=0;

   ccIndex ind = lb->add( f->fname, item_info );
   if ( !strcmp( oldname, f->fname ) )
      lb->setCurrent( ind );

   sizeall += f->st.st_size;
}

void filebox::fillBox()
{
   lb->freeAll();
   lb->manualPos = 0;
   lb->moveHome();
   flagDirs=1;
   int i, end;
   for( i=0, end=d->dirs->getCount(); i < end; i++ )
      fillItem( (fileItem*)d->dirs->at(i) );
   flagDirs=0;
   sizeall = 0;
   for( i=0, end=d->files->getCount(); i < end; i++ )
      fillItem( (fileItem*)d->files->at(i) );

   if ( !oldname[0] )
      return;

   for( i=0, end=d->dirs->getCount(); i < end; i++ )
     if ( !strcmp( oldname, ((fileItem*)d->dirs->at(i))->fname ) )
       {
	 lb->setCurrent( i );
	 break;
       }
}

int filebox::newname()
{
   lb->accept();
   char *name = (char*)lb->getItem(cur);
#ifndef DJGPP
   if ( !name || *name == '.' && *(name+1)==FILESEP && !*(name+2) )
      return 0;
#endif
   strcpy( str, d->path );
   if ( cur >= d->dirs->getCount() )
     {
       if ( str[strlen(str)-1] != FILESEP )
	  strcat( str, FILESEPSTR );
       strcat( str, name );
#ifdef DJGPP
       convertPath( str, FILESEP, '\\' );
#endif
       il->initString( str );
#ifdef DJGPP
       convertPath( str, '\\', FILESEP );
#endif
       il->moveEnd();
       select(0);
       return 1;
     }
   oldname[0]=0;
   strcpy( str, d->path );
   if ( str[strlen(str)-1] == FILESEP )
      str[strlen(str)-1] = 0;
   int flag = 1;
   if ( !memcmp( name, "..", 2 )
#ifndef DJGPP
	&& name[2] == FILESEP
#else
	&& name[2] == '\\'
#endif
	&& !name[3] )
     {
       char *ch = (char*)strrchr( str, FILESEP );
       if ( !ch )
	   strcpy( str, ".." );
       else
	 {
	   strcpy( oldname, ch+1 );
#ifndef DJGPP
	      strcat( oldname, FILESEPSTR );
#else
	      strcat( oldname, "\\" );
#endif
	   if ( ch != str )
	       *ch = 0;
	   else
	       { flag = 0; *(ch+1) = 0; }
	 }
     }
   else
     {
       if ( str[strlen(str)-1] != FILESEP )
	  strcat( str, FILESEPSTR );
       strcat( str, name );
       str[ strlen(str)-1 ] = 0;
     }
   if ( d->newpath( str, d->mask, fileSortOrder ) )
     {
       fillBox();
       if ( flag && str[strlen(str)-1] != FILESEP )
	  strcat( str, FILESEPSTR );
       strcat( str, d->mask );
#ifdef DJGPP
       convertPath( str, FILESEP, '\\' );
#endif
       il->initString( str );
#ifdef DJGPP
       convertPath( str, '\\', FILESEP );
#endif
       il->moveEnd();
     }
   else
     {
       sprintf( item_info, "Can't open directory\n%s", str );
#ifdef DJGPP
       convertPath( item_info, FILESEP, '\\' );
#endif
       modal *m = new modal( ALIGN_CENTER, item_info );
       exec( m );
       delete m;
     }
   oldname[0]=0;
   return 1;
}

int filebox::newpath()
{
   il->accept();
   il->resetFirst();
   TranslatePath( str, MAXPATHLEN );
   int ret = 1;
   char *newp = new char[MAXPATHLEN];
   newp[0]=0;
#ifdef DJGPP
   if ( str[strlen(str)-1] == ':' )
      strcat( str, FILESEPSTR );
   convertPath( str, '\\', FILESEP );
#endif
   char *s = (char*)strrchr( str, FILESEP );
   if ( s )
     {
       memcpy( newp, str, s-str+1 );
       newp[s-str+1]=0;
       if ( *(++s) == 0 )
	 {
	   strcat( str, "*" );
#ifdef DJGPP
	   convertPath( str, FILESEP, '\\' );
#endif
	   il->initString( str );
#ifdef DJGPP
	   convertPath( str, '\\', FILESEP );
#endif
	   il->moveEnd();
	   goto m1;
	 }
     }
   else
       s = str;
   if ( !strchr( s, '*' ) && !strchr( s, '?') )
     {
       struct stat st;
       st.st_mode = 0;
       stat( str, &st );
       if ( !S_ISDIR( st.st_mode ) )
	 { ret=0; goto _end; }
       strcpy( newp, str );
       if ( newp[ strlen(newp)-1 ] != FILESEP )
	  strcat( str, FILESEPSTR );
       s = "*";
       strcat( str, s );
#ifdef DJGPP
       convertPath( str, FILESEP, '\\' );
#endif
       il->initString( str );
#ifdef DJGPP
       convertPath( str, '\\', FILESEP );
#endif
       il->moveEnd();
     }
m1:
   if ( !d->path || !d->mask || strcmp( newp, d->path ) || strcmp( s, d->mask ) )
     {
       if ( d->newpath( newp, s, fileSortOrder ) )
	  fillBox();
       else
	{
	  sprintf( item_info, "Can't open directory\n%s", newp );
#ifdef DJGPP
	  convertPath( item_info, FILESEP, '\\' );
#endif
	  modal *m = new modal( ALIGN_CENTER, item_info );
	  exec( m );
	  delete m;
	  ret = -1;
	}
     }
_end:
   delete newp;
   return ret;
}

long filebox::handleKey( long key, void *&ptr )
{
   key = keyHolder.translate( key );
   int ret=0;
   switch( key )
    {
      case HASH_cmNext:
      case HASH_cmOK:
	  switch( current )
	   {
	     case 0:
		 if ( (ret = newpath()) )
		   {
		     fileHistory.atFree( 0 );
		     if ( ret > 0 )
			select(1);
		     if ( draw() )
			Screen::sync();
		     key=0;
		   }
		 else if ( key == HASH_cmOK )
		   {
		     if ( outname )
			strcpy( outname, getName() );
		     if ( outpath )
			strcpy( outpath, d->path );
		     if ( outmask )
			strcpy( outmask, d->mask );
		   }
		 break;
	     case 1:
		 if ( key == HASH_cmOK )
		   {
		     if ( newname() && draw() )
			Screen::sync();
		     key=0;
		   }
		 break;
	   }
	  break;
      case FUNC1(kbCtrlF):
	  if ( resort() && draw() )
	     Screen::sync();
	  key=0;
	  break;
    }
   key = Dialog::handleKey( key, ptr );
   return key;
}

char *filebox::getName()
{
  return str;
}

char *filebox::getPath()
{
  return d->path;
}

char *filebox::getMask()
{
  return d->mask;
}

int filebox::resort()
{
   int st;
   Dialog *db = new Dialog( Rect(9,31,14,48), new Lang( "Sort", "Сортировка" ) );
   radioBox *rb = new radioBox( Point(0,0), fileSortOrder, &st, fileSortOrder );
   rb->add( lang("Name","Имя") );
   rb->add( lang("Extension ","Расширение") );
   rb->add( lang("Size","Размер") );
   rb->add( lang("Date","Дата") );
   db->insert( rb );
   long ret = exec( db );
   delete db;
   if ( ret == HASH_cmOK )
     {
       if ( !d->resort( st ) )
	  return 0;
       fileSortOrder = st;
       fillBox();
     }
   return 1;
}

int filebox::draw( int Redraw, int sy, int sx )
{
   put( 0, 1, lang(" Name","Имя  " ) );
   put( 2, 1, lang(" Files:       Bytes:               Sort by",
		   "Файлов:        Байт:                Сорт.:") );
   sprintf( item_info, "%-5d", (int)d->files->getCount() );
   put( 2, 9, item_info, -1, 4 );
   sprintf( item_info, "%-11ld", sizeall );
   put( 2, 22, item_info, -1, 4 );

   switch( fileSortOrder )
    {
      case SORT_BY_NAME:
	  strcpy( item_info, lang("Name","Имя  ") );
	  break;
      case SORT_BY_EXT:
	  strcpy( item_info, lang("Ext ","Расш.") );
	  break;
      case SORT_BY_SIZE:
	  strcpy( item_info, lang("Size","Разм.") );
	  break;
      case SORT_BY_TIME:
	  strcpy( item_info, lang("Date","Дата ") );
	  break;
    }
   put( 2, 44, item_info, -1, 4 );
   return Dialog::draw( Redraw, sy, sx );
}

void filebox::resizeScreen( Rect &old )
{
  if ( isGetObj )
     return;
  int dx = scr.cols - scr.cols / 4 - 3;
  int width = 16, cols = dx / width;
  for( int dd=dx%width; dd && dx/width >= cols; dd=dx%(width++) );
  il->setWidth( dx );
  lb->setWidth( width );
  Rect r = Screen::center( scr.lines - scr.lines / 4,
			scr.cols - scr.cols / 4 );
  setRect( r );
  r = Rect( 3, 0, rect.b.y - rect.a.y - 4, dx );
  lb->setRect( r );
  fillBox();
}

