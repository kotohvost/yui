/*
	$Id: filebox.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _FILEBOX_H
#define _FILEBOX_H

#include "modal.h"
#include "listbox.h"
#include "dir.h"

#ifdef DJGPP
extern void convertPath( char *path, int c1, int c2 );
#endif

class filebox : public Dialog
{
protected:
   directory *d;
   listBox *lb;
   inputLine *il;
   static int fileSortOrder;
   char *outname, *outpath, *outmask, *d_path, *d_mask;
   char *str, *oldname, *item_info;
   int cur, d_sort;
   int newname();
   int newpath();
   void fillItem( fileItem *f );
   void fillBox();
   long sizeall;
   int flagDirs;
   uid_t uid;
   gid_t gid;

public:
   filebox( char *Path, char *Mask="*", char *outn=0, char *outp=0, char *outm=0 );
   ~filebox();
   virtual void resizeScreen( Rect &old );
   virtual int 	isType(long typ);
   static char filePath[MAXPATHLEN];
   static char fileMask[128];

   virtual int init( void *data=0 );
   virtual int draw( int Redraw=0, int sy=0, int sx=0 );
   virtual long handleKey( long key, void *&ptr );
   char *getPath();
   char *getMask();
   char *getName();
   int resort();

   static Collection fileHistory;
};

#endif
