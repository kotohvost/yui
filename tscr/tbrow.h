/*
	$Id: tbrow.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _TBROWSE_H_
#define _TBROWSE_H_

#include "window.h"

extern Keymap TbrowKeyMap;

class Column : public Collection
{
public:
	Column( char *_head, char *_foot, int _width, int _align, int _len );
	virtual ~Column();
	Collection head, foot;
	int width, freezed, align;
	virtual const char *getVal( int index );
        virtual void setVal( int ind, const char *data );
        virtual void shiftUp( int n );
        virtual void shiftDown( int n );
        virtual void setHF( Collection *coll, char *str );
};

struct CInfo
{
	void *key;
	Column *column;
        char *format;
        char **trans;
        int transLen;
	CInfo( Column *c, void *k=0 );
	~CInfo();
};

class inputLine;

class Tbrow : public Window
{
protected:
	int showStab, lineHigh, showBeof, bufSize, nrec, needMove;
	void freeItem( void *item ) { if (item) delete (CInfo*)item; }
	int HL, VL, UC, DC, CC, headHigh, footHigh, cursorOffset;
        unsigned boflag:1, eoflag:1;
	Point oldPos;
	static unsigned short screenStr[256];
	unsigned internalRedraw:1;
        int freezedPos;
        inputLine *il;
        char *il_buf;
        long il_com;
        int il_x;
public:
	Tbrow( Rect r, Lang *tit=0, Lang *id=0, Lang *st=0, int Box=1,
	int ShowStab=1, int LineHight=1, int ShowBeof=1,
	int BufSize=Screen::Lines*2, int NeedMove=0 );
	virtual ~Tbrow();
	virtual int type() { return TYPE_TBROW; }

        virtual CInfo *makeColumn( char *head, char *foot, int width, int align, void *key=0 );
        virtual int del( int index );

	virtual void setColors( int type );
	virtual int init( void *data );
	virtual Point getCursor( int *hide );
	virtual void moveCursor();
	virtual int draw( int Redraw=0, int sy=0, int sx=0 );
	virtual long handleKey( long key, void *&ptr );
	virtual int moveHome();
	virtual int moveBof();
	virtual int moveEnd();
	virtual int moveEof();
	virtual int movePgUp( int check=0 );
	virtual int movePgDn( int check=0 );
	virtual int moveLeft();
	virtual int moveRight();
	virtual int moveUp();
	virtual int moveDown();
	virtual void freezeColumn( int index, int fr_flag=0 );
	virtual int switchColumn();
        unsigned short *scrLine( int line, int &len, int head=0 );
	unsigned short *getHorLine( int &len, int flag );
};

#endif /* _TBROWSE_H_ */
