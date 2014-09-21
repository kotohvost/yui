/*
	$Id: keymap.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _KEYMAP_H
#define _KEYMAP_H

#include <stdlib.h>
#include <langs.h>

// для строк ввода тип
enum InputType { INPUT, INT, U_INT, FLOAT, U_FLOAT, DATE };

struct CommandDescr
{
   long command;
   Lang *descr;
   ~CommandDescr() { if ( descr ) delete descr; }
};

extern int qsort_compare( const void *key1, const void *key2 );
extern int qsort_compare2( const void *key1, const void *key2 );

class Keymap
{
public:
   Keymap( long *m=0, Lang *mIdent=0 );
   ~Keymap();
   long *mas;		// sorted by key
   long *mas2;		// sorted by command
   int len, size;
   Lang *mapIdent;
   void init( long *m );
   long translate( long key );
   long getKey( long command );
};

/*-------------------------------------------------------------*/
class KeyHolder
{
public:
   Keymap **kmas;
   CommandDescr **descr;
   char **ident;
   int len;
   int add( Keymap *km, CommandDescr *cd=0 );
   int del( Keymap *km );
   Keymap *item( int index );
   void changeDescr( Keymap *km, CommandDescr *cd );
   long translate( long key );
   KeyHolder();
   ~KeyHolder();
};

#endif
