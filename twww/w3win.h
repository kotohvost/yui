#ifndef HTEXT_WIN_H
#define HTEXT_WIN_H

#include <dialog.h>

extern long w3winKeyMap[];
extern Keymap W3WinKeyMap;

class W3ProgressMessage;

typedef struct _HTRequest HTRequest;
typedef struct _HTParentAnchor HTParentAnchor;
typedef struct _HText HText;
typedef struct _HTAnchor HTAnchor;

class HistoryBucket
{
public:
  HTAnchor *anchor;
  HTParentAnchor *parent_anchor;
  char *tag;
  char *method;
  char *data;		// for method POST
  long current;
  PointL delta;
  Point  cursor;
  HistoryBucket( HTAnchor *Anchor, HTParentAnchor *pan, char *Tag=0,
		 char *Method=0, char *Data=0 );
  ~HistoryBucket();
};

class W3HistoryCollection : public Collection
{
private:
  void freeItem( void *item) { delete (HistoryBucket *)item; }
public:
  ~W3HistoryCollection(){ freeAll(); }
  int topInsert( HistoryBucket *buck );
  HistoryBucket *search( const char *addr );
  HistoryBucket *search( HTAnchor *anch, char *tag );
  void updateCoor( ccIndex index, PointL delta, Point cursor, long current );
};

/*
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
  void insert( FRtype type, void *ptr, W3Win *w );
  void drawSkel( W3Win *w );
  void initFrames();
  void setCoor( Collection *coll, int *&coor, int pix_size, int size );
  void setRect( Rect r );
  int  getRect( FRtype type, int frame_count, Rect &r );
  W3Win *findTarget( char *target );
};
*/

class W3Win: public Dialog
{
friend class AnchorHyper;

protected:
  static W3HistoryCollection history;
  char *mainRef;
  char *startRef;
  char *startDest;
  char *startMethod;
  unsigned updateStatus:1,
	   first:1;
  int testLoaded();

public:
  static Collection historyUrl;
  W3HistoryCollection frameHistory;
  unsigned  cleared:1;

  HTRequest *request;
  HText *text;
  char *base_href;
  char *base_target;
  char *target_name;
  HTParentAnchor *parent_anchor;
  HTAnchor *node_anchor;
  char *submit_data;
  int b_read;
  int b_write;

   W3Win( const char *StartRef=0, Rect r=Rect(-1,-1,-1,-1) );
   ~W3Win();
   virtual int isType(long typ);

   // virtual in parents
   virtual int close();
   virtual long handleKey( long key, void *&ptr );
   virtual void setColors(int Type);

   //messages about request state
   void showProgress ( W3ProgressMessage *msg );
   enum ReloadMode
   {
     AnyVersion,
     ReloadIfModificated,
     ForceReload
   };
   // fill window; href _must_ be malloc'ed or 0
   int newRef( char *addr, HTAnchor *Anchor=0, const char *method=0, PointL *_delta=0, Point *_cursor=0, long *_cur=0 );
   int newAnchor( char *tag, int correctHist=1, PointL *_delta=0, Point *_cursor=0 );
   void newStart( const char* href, const char *method=0, HTParentAnchor *pan=0 );

   // вызывается при установке выбора на объект
   virtual void processSelect( getobj *g );

   int getWidth() { return size.x-(box?2:0); } //возвращает текущую ширину окна
   virtual void makeMenu( Menu *m );

   long runWin( Window *w );
};

extern void W3_Lib_Init();
extern void W3_Lib_End();
extern int  setW3Cache( char *path );

#endif
