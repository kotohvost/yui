/*
	$Id: dialog.h,v 3.2.2.2 2007/07/24 11:28:11 shelton Exp $
*/
#ifndef _DIALOG_H
#define _DIALOG_H

#include "window.h"
#include "visual.h"

//extern CommandDescr dialogCommandDescr[];
extern long dialogKeyMap[];
extern Keymap DialogKeyMap;

class BgText
{
public:
  int len;
  int allocated;
  unsigned short *str;
  BgText( short *s, int l );
  ~BgText();
  int putC( int ch, int attr, int fillAttr, int pos, int graph=0 );
  char *putS( char *s, int l, int attr, int backAttr, int fillAttr, int pos, int graph=0 );
  void clear();
};

class TextColl : public Collection
{
protected:
   void freeItem( void *item ) { delete (BgText*)item; }
public:
   TextColl() : Collection(50,50) {;}
   ~TextColl() { freeAll(); }
};

struct FindInfo
{
   long line;
   short start;
   short end;
   unsigned found:1;
   FindInfo() : line(-1), start(-1), end(-1), found(0) {;}
};

struct DataFind
{
  int options;
  int direction;
  char strFind[256];
  DataFind() : options(0), direction(0) { strFind[0] = 0; }
};

enum rcType {
	TYPE_MUL,
	TYPE_NUMBER,
	TYPE_PERCENT,
	TYPE_PERCENT_HARD,
};

enum FRtype {
	TYPE_FRAME,
	TYPE_FRAMESET
};

typedef struct {
	int val;
	rcType type;
} rcInfo;

typedef struct {
	FRtype type;
	void *ptr;
} FRinfo;

class Dialog;

class FrameSet
{
public:
  FrameSet( FrameSet *ptr, Collection *cols, Collection *rows );
  ~FrameSet();
  FrameSet *prev;
  Rect rect;
  Collection *set;
  Collection *cols;
  Collection *rows;
  int fcount, maxframes;
  int *coor_x, *coor_y;
  void insert( FRtype type, void *ptr, Dialog *w );
  void drawSkel( Dialog *w );
  void initFrames();
  void setCoor( Collection *coll, int *&coor, int pix_size, int size );
  void setRect( Rect r );
  int  getRect( FRtype type, int frame_count, Rect &r );
  Dialog *findTarget( char *target );
};

class Dialog : public Window
{
protected:
  static char Buf[maxLineLength];
  long current, activeItem;
  unsigned internalRedraw:1;
  long bottom;
  long cury;
  short curx;
  TextColl text;
  long oldCur;
  void select( int no );
  void select( getobj *get );
  void nextObj( int pr );
  void info();
  void moveEnd();
  void freeItem( void *item );
  visual vis;
  int *objmap, lenmap;
  int notAccept;
  static DataFind dataFind;
  void copyBlockToClip();
  void setObjmap( int flagSelect=0 );
  void correctCursor();

public:
  Dialog( Rect r=Rect( 1, 0, Screen::Lines-2, Screen::Columns-1 ),
     Lang *t=0, Lang *id=0, Lang *st=0, int hmode=1 );
  virtual ~Dialog();
  virtual int 	isType(long typ);
  virtual int	init( void *data=0 );
  virtual void setColors( int type );

  int hardMode;	// 0 - со скроллингом
  int bindCursor;	// флаг коррекции курсора в зав-ти от текущего об'екта
  PointL curPos;
  Point corrPoint;	// для вычисления координат об'ектов на экране

  int close();
  virtual void clear( int flagText=1 );
  virtual int insert( getobj *g, int ind=-1 );
  virtual void remove( getobj *g, int flagFree=1 );
  virtual void remove( long no, int flagFree=1 );
  long handleKey( long key, void *&ptr );
  int draw( int Redraw=0, int sy=0, int sx=0 );
  void moveCursor();
  void getPos( long &y, short &x ) { y=cury; x=curx; }
  void clearLine( long no );   // clear text line
  void removeLine( long no );  // remove text line
  void insertLine( long no, const char *str, int index=0 )
		{ put( no, 0, str, -1, index ); }
  void put( long y, long x, const char *str, int len=-1, int index=0, int graph=0 );
  void put( Point p, const char *str, int index=0, int graph=0 )
		{ put( (long)p.y, (long)p.x, str, -1, index, graph ); }
  void put( const char *str, int len=-1, int index=0, int graph=0 )
		{ put( cury, curx, str, len, index, graph ); }
  void put( int ch, int index=0, int graph=0 );
  void clearText( long y1, long y2 );
  void removeText( long y1, long y2 );
  char *currentWord();
  char *getStr( long line, int &len );
  void setCursor( PointL d, Point c );
  Point getCursor( int *hide=0 );

  virtual void processSelect( getobj *g ) {;}

  FindInfo fInfo;
  int boxFind();
  int find( int offset );

  FrameSet *topFset, *fset;
  int fillRCinfo( const char *str, Collection *coll );
  void beginFrameset( const char *cols, const char *rows );
  void beginFrame( Window *w, const char *name, int noresize );
  void endFrameset();
};

#endif
