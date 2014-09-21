/*
	$Id: appl.h,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#ifndef _APPL_H
#define _APPL_H

#include <program.h>

struct statusYui
{
   char name[128];
   Rect rect;
   PointL delta;
   Point cursor;
   short _flags;
   int active;
   int readOnly;
   int trans_flag;
};

#define STATUS_FILE(opt)	(opt & 1)
#define STATUS_BAR(opt)		(opt & (1<<1))
#define MENU_BAR(opt)		(opt & (1<<2))
#define WINDOW_BORDER(opt)	(opt & (1<<3))
#define GDB_AUTOLOAD(opt)	(opt & (1<<4))

struct ApplOptions
{
   int options;
   int bg_symbol;
   int fill_char;
   int hist_limit;
   int term_limit;
   char *lang;
   char *monitor;
   char *term_base;
   char *shell;
   char *trans_edit;
   char *trans_web;
   char *trans_term;
   char *trans_debug;
   char *url;
   char *w3cache;
   ApplOptions();
   ~ApplOptions();
};

class Edit;

extern long applKeyMap[];
extern Keymap ApplKeyMap;
extern CommandDescr commandDescr[];
extern long *applKeys;
extern long *programKeys;

extern KeyHolder editKeyHolder;
extern KeyHolder dialogKeyHolder;
#ifndef DJGPP
extern KeyHolder ytermKeyHolder;
#endif

class Appl : public Program
{
protected:
   void find( int flag );
   void replace();
   void systemCommand( Collection *coll, int type=0 );
   void userMenu();

   int saveStatus();
   int restStatus();
   int saveOptions();
   int restOptions();

   void readOptions();
   void readColormap();
   void readColortrans();
   void readKeymap();
   void readMacro();

#ifndef DJGPP
   void getManWord();
   void getCtagWord();
   void manualPage( char *str=0 );
   void gdbInitFile();
   void openDebug();
#endif
   void openEdit( char *name, long command );
   void openShare();

public:
   Appl();
   static ApplOptions aOpt;
   void makeMenu();
   long handleKey( long key, void *&ptr );
   void runHelp( long type );
   static void send_mail_function( char *email );
   void findCtag( char *ctag );
   void setTransFlag( char *str, int *flag );
};

extern Collection *makeCopyright();
extern int returnStatus;
extern int trans_flag_edit;
extern int trans_flag_web;
extern int trans_flag_term;
extern int trans_flag_debug;

#endif
