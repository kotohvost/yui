/*
	$Id: yui.h,v 3.2.2.1 2007/07/24 09:58:11 shelton Exp $
*/
#ifndef _YUI_H
#define _YUI_H

#include <sys/stat.h>

#include <window.h>
#include <array.h>

#define FLAG_FIND	( _flags & 0x010 )
#define FLAG_INS	( _flags & 0x020 )
#define FLAG_CHANGE	( _flags & 0x040 )
#define FLAG_SGRAPH	( _flags & 0x080 )
#define FLAG_DGRAPH	( _flags & 0x0f0 )
#define FLAG_MODE	( _flags & 0x100 )
#define FLAG_MAN	( _flags & 0x200 )
#define FLAG_BINARY	( _flags & 0x400 )
#define FLAG_FREEZE	( _flags & 0x800 )

#define SET_FIND	_flags |= 0x010
#define SET_INS		_flags |= 0x020
#define SET_CHANGE	_flags |= 0x040
#define SET_SGRAPH	_flags |= 0x080
#define SET_DGRAPH	_flags |= 0x0f0
#define SET_MODE	_flags |= 0x100
#define SET_MAN		_flags |= 0x200
#define SET_BINARY	_flags |= 0x400
#define SET_FREEZE	_flags |= 0x800

#define CLEAR_FIND	_flags &= ~0x010
#define CLEAR_INS	_flags &= ~0x020
#define CLEAR_CHANGE	_flags &= ~0x040
#define CLEAR_SGRAPH	_flags &= ~0x080
#define CLEAR_DGRAPH	_flags &= ~0x0f0
#define CLEAR_MODE	_flags &= ~0x100
#define CLEAR_MAN	_flags &= ~0x200
#define CLEAR_BINARY	_flags &= ~0x400
#define CLEAR_FREEZE	_flags &= ~0x800

// режим "только чтение" не установлен:
#define RDONLY_0	0
// ставится и снимается из окна руками:
#define RDONLY_1	1
// режим бинарного файла, снимается руками:
#define RDONLY_2	2
// нет прав на файл, снимается при появлении прав и reload:
#define RDONLY_3	3
// устанавливается извне, никогда не снимается:
#define RDONLY_4	4

#define NO_UNDO	0
#define UNDO	1
#define REDO	2

#define FIND_CASE_SENSITIVE	0x01
#define FIND_FROM_END		0x02
#define FIND_WORD_ONLY		0x04
#define FIND_SPECIAL		0x08
#define FIND_REGEXPR		0x10
#define FIND_ALL_WIN		0x20
#define REPL_PROMPT		0x40
#define REPL_CHANGE_ALL		0x80

struct strInfo
{
   char *str;
   unsigned short len:15, map:1;
   strInfo( char *s, u_short l, u_short m=1 ) : str(s), len(l), map(m) {;}
};

struct BPinfo
{
   long line;
   int number;
   BPinfo( long l, int n ) : line(l), number(n) {;}
};

class BpointArray: public SortedArray
{
public:
   BpointArray();
   int sizeOf() { return sizeof(BPinfo); }
   BPinfo & operator [](ccIndex index)
		{ return *(BPinfo*)items[index]; }
   ccIndex insert( BPinfo &item )
		{ return SortedArray::insert( &item ); }
   void remove( BPinfo &item )
		{ SortedArray::remove( &item ); }
   void atInsert( ccIndex index, BPinfo &item )
		{ SortedArray::atInsert( index, &item ); }
   void atPut( ccIndex index, BPinfo &item )
		{ SortedArray::atPut( index, &item ); }
   int search( BPinfo &key, ccIndex &index )
		{ return SortedArray::search( &key, index ); }

protected:
   int compare( void *k1, void *k2 )
		{ return ((BPinfo*)k1)->line - ((BPinfo*)k2)->line; }
};

class TagProcessor;
class CommentTagProcessor;

struct HighLight
{
   SortedCollection *ext;
   TagProcessor *tproc;
   CommentTagProcessor *ctproc;
   int punctuate;
};

struct infoArray: public Array
{
   infoArray( int len, int Delta) : Array( len, Delta ) {;}
   int sizeOf() { return sizeof(strInfo); }
   strInfo & operator []( long index )
		{return *(strInfo*)items[index]; }
   long insert( strInfo &item )
		{return Array::insert( &item ); }
   void remove( strInfo &item )
		{Array::remove( &item ); }
   void atInsert( long index, strInfo &item )
		{Array::atInsert( index, &item ); }
   void atPut( long index, strInfo &item )
		{Array::atPut( index, &item ); }
};

#define LINE		(delta.y+cursor.y)
#define COLUMN		(delta.x+cursor.x)
#define BOTT		(atext ? atext->getCount() : 0)

struct findInfo
{
   long line;
   short start;
   short end;
   int found;
   unsigned replaced;
   findInfo() : found(0), replaced(0) {;}
};

struct dataFindRepl
{
   int options;
   int direction;
   int where;
   char strFind[256];
   char strRepl[256];
   dataFindRepl() : options(202), direction(0), where(0)
	  { strFind[0] = strRepl[0] = 0; }
};

struct undoItem
{
   long command, dY, longVal;
   u_short _flags, dX, column;
   int cycl;
   u_char cY, cX, sym1, sym2;
   blockInfo bi;
   void *ptr;
   void init() { command=0; cycl=1; sym1=0; sym2=0; ptr=0; }
   void freeData();
};

struct undoInfo
{
   u_short max;
   u_short count;
   u_short fullCount;
   u_short start;
   u_short cur;
   undoInfo( u_short m ) : max(m), count(0), fullCount(0), start(0), cur(0) {;}
};

#define BAK_FILE(opt)		(opt & 1)
#define UNDO_OPTIMIZE(opt)	(opt & (1<<1))
#define AUTOSAVE(opt)		(opt & (1<<2))
#define AUTOWRAP(opt)		(opt & (1<<3))
#define WRAP_IN_WORD(opt)	(opt & (1<<4))
#define EXPAND_STRING(opt)	(opt & (1<<5))
#define STRIP_SPACES(opt)	(opt & (1<<6))
#define OPTIMAL_FILL(opt)	(opt & (1<<7))
#define HIGHLIGHT(opt)		(opt & (1<<8))

/*
#define SET_BAK_FILE(opt)	(opt |= 1)
#define SET_UNDO_OPTIMIZE(opt)	(opt |= (1<<1))
#define SET_AUTOSAVE(opt)	(opt |= (1<<2))
#define SET_AUTOWRAP(opt)	(opt |= (1<<3))
#define SET_WRAP_IN_WORD(opt)	(opt |= (1<<4))
#define SET_EXPAND_STRING(opt)	(opt |= (1<<5))
#define SET_STRIP_SPACES(opt)	(opt |= (1<<6))
#define SET_OPTIMAL_FILL(opt)	(opt |= (1<<7))
#define SET_HIGHLIGHT(opt)	(opt |= (1<<8))

#define RES_BAK_FILE(opt)	(opt &= ^1)
#define RES_UNDO_OPTIMIZE(opt)	(opt &= ^(1<<1))
#define RES_AUTOSAVE(opt)	(opt &= ^(1<<2))
#define RES_AUTOWRAP(opt)	(opt &= ^(1<<3))
#define RES_WRAP_IN_WORD(opt)	(opt &= ^(1<<4))
#define RES_EXPAND_STRING(opt)	(opt &= ^(1<<5))
#define RES_STRIP_SPACES(opt)	(opt &= ^(1<<6))
#define RES_OPTIMAL_FILL(opt)	(opt &= ^(1<<7))
#define RES_HIGHLIGHT(opt)	(opt &= ^(1<<8))
*/

struct EditOptions
{
   int options;
   int tab;
   int timeSave;
   int stepUndo;
   int lineLength;
   int parOffset;
   EditOptions();
};

extern long editKeyMap[];
//extern CommandDescr editCommandDescr[];
extern Keymap EditKeyMap;
extern Menu *EditMenu;
extern Menu *EditMenuGetObj;
extern char *getRealFileName( char *name );
extern int trans_flag_edit;
extern int trans_flag_web;
extern int trans_flag_term;
extern int trans_flag_debug;

class Timer;
class HTMLTagProcessor;

struct FileStat
{
   struct stat s;
   long linesNumber;
   FileStat() : linesNumber(0) {;}
};

class Edit : public Window
{
friend class Appl;
friend class Debug;

protected:
   static char Buf1[maxLineLength];
   static char Buf2[maxLineLength];

   infoArray *atext;

   char *cmap, *getstrbuf;
   FileStat *fInfo;
   void fileInfo();

   int	moveHome	( int flagUndo=UNDO );
   int  moveEnd		( int flagUndo=UNDO );
   int  moveUp		( int flagUndo=UNDO, int scroll=0 );
   int  moveDown	( int flagUndo=UNDO, int scroll=0 );
   int  moveLeft	( int flagUndo=UNDO, int word=0 );
   int  moveRight	( int flagUndo=UNDO, int word=0 );
   void movePgUp	( int flagUndo=UNDO );
   void movePgDn	( int flagUndo=UNDO );
   void moveTop		( int flagUndo=UNDO );
   void moveBott	( int flagUndo=UNDO );
   void moveBackTab	( int flagUndo=UNDO );

   void info();
   void setWinIdent();

   strInfo *STRINFO( long l );
   char *getStr( strInfo *si, int &len, int flagNL=0 );
   int realPos( strInfo *si, int scrPos );
   int scrPos( strInfo *si, int realPos );

   int insertChar( int ch, int Ins, int flagUndo=UNDO );
   int deleteChar( int flagUndo=UNDO );
   int backspace( int flagUndo=UNDO );
   void enter( int flagUndo=UNDO );
   void deleteLine( long line, int flagUndo=UNDO );
   void truncateStr();
   int glueStrings( long topline, int spaces=0, int flagUndo=UNDO );
/*   void str_copy( strInfo *si, char *str, int len );
   void str_cat( strInfo *si, char *str, int len );
   void str_cat( strInfo *si, int ch, char *str, int len );
   void str_set( strInfo *si, int offset, int ch, int len );*/
   void replaceStr( long line, char *str, int len );
   void insertStr( long line, char *str, int len );
   void insertStr( long line, strInfo *si );
   int addStrings( int flagUndo );
   void clearText( int flag=1 );

   long debugLine;

   void markBlock( int type, int flagUndo=UNDO );
   void unmarkBlock( int flagUndo );
   void copyBlock( int flagUndo=UNDO );
   void moveBlock( long line, int column=0, int flagUndo=UNDO );
   void deleteBlock( int flagCorrectCursor=1, int flagUndo=UNDO, undoItem *ui=0 );
   void copyToBuf( Collection &buf, blockInfo *b );
   int copyFromBuf( long line, int sPos, Collection &buf, short len, int flagUndo=UNDO, int flagMove=0 );
   void copyToClip( int freeBefore=1 );
   void moveToClip( int freeBefore=1 );
   void addToClip() { copyToClip( 0 ); }
   void copyFromClip( Collection *clip = 0 );

   findInfo find_Info;
   static dataFindRepl dFindRepl;
#ifndef DJGPP
   BpointArray Bpoint;
   int breakpoint( long line, int number=-1 );
   void clearBpoints();
#endif
   void correctShared();

   void undo_redo( int type );
   undoInfo *uInfo;
   undoItem *Undo;
   undoItem *undoOptimize( long command );
   undoItem *saveUndo( long command, int canOptimize=0 );
   void restoreCursor( undoItem *u );
   void incUndo();
   virtual void setChange( int flag );
   int compress( char *src, char *dst, int pos, int len, int tab );
   char *uncompress( char *str, int len );
   int format( char *str, int pos, char *&s1, char *&s2, int smartFlag );
   long formatParagraph( long startLine, int flagUndo, int skip, int smartFlag );
   void formatText();

   Collection *externCollection;
   Timer *saveTimer;
   virtual blockInfo *validBlock();
   TagProcessor *tproc;		// ключевые слова
   CommentTagProcessor *ctproc;	// комментарии
   HTMLTagProcessor *htproc;
   int punctuate, comment, hash_is_comment;

   void setTprocByExt( char *ext );
   void setHTprocByExt( char *ext );

public:
   Edit( const char *title="", Rect r=Rect( 0, 0, Screen::Lines-1, Screen::Columns-1), Lang *Status=0, int flag_binary=0 );
   Edit( Collection *ed, const char *title="", Rect r=Rect( 0, 0, Screen::Lines-1, Screen::Columns-1), Lang *Status=0 );
   Edit( Edit *e, Rect r=Rect( 0, 0, Screen::Lines-1, Screen::Columns-1) );
   ~Edit();

   int readOnlyMode;
   Collection *shared;
   short *changed, *saved;
   static HTMLTagProcessor HTproc;
   static SortedCollection HTMLextension;
   static Collection highlight;
   static int showSpace;
#ifndef DJGPP
   static Edit *debugEdit;
#endif
   static EditOptions eOpt;
   static int stepTab;

   int init( void *data=0 );
   int readFile( char *name, int flag_binary=0 );
   int readCollection();
   virtual int saveText( char *name=0 );
   int saveCollection();
   int readBlock();
   int saveBlock();
   virtual void reload( int flag=1 );
   int close();
   int draw( int Redraw=0, int sy=0, int sx=0 );
   void drawLine( long line, int sync=1, int fromDraw=0 );
   short *scrLine( long line, int &len, int &isReload, int full=0 );
   long handleKey( long key, void *&ptr );
   int accept();
   void moveCursor();
   Point getCursor( int *hide=0 );
   void makeMenu( Menu *m=0 );
   void setColors( int type );

   char *currentWord();
   static Collection histFind;
   static Collection histRepl;
   static Collection histGoto;
   int find( int offset=1, int flagStatus=1, int flagStart=0, int flagUndo=UNDO );
   int replace( int flagStart=0, unsigned *replaced=0 );
   int findMatch( int direction, int flagUndo=UNDO );
   void boxGoto();
   long boxFind();
   long boxReplace();
   int gotoLine( long line, int flagUndo=1 );
   void markGotoLine();
#ifndef DJGPP
   void DebugLine( long line, int flag=1 );
#else
   void printText();
#endif
   void showManMode( int warning=1 );
   void changeMode( int warning=1 );
};

extern Collection blockBuf;
extern KeyHolder editKeyHolder;

#ifdef DJGPP
extern Lang sorry_dos;
#endif

/*
#ifdef _USE_MMAP_
extern Edit *currentEdit;
void reload_by_SIGSEGV( int signo );
#endif
*/

#endif
