/*
	$Id: screen.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef _SCREEN_H
#define _SCREEN_H

#include <unistd.h>
#include <stdarg.h>
#include "point.h"
#include "langs.h"
#include <string.h>

#ifdef DJGPP
extern unsigned short linedraw [11];
#endif

extern int GraphAttr;
extern int language;

#define __COLORS

#if 0

#define FG_BLACK	0
#define FG_BLUE		0x1
#define FG_GREEN	0x2
#define FG_CYAN		0x3
#define FG_RED		0x4
#define FG_MAGENTA	0x5
#define FG_YELLOW	0x6
#define FG_WHITE	0x7

#define FG_HI_BLACK	0x8
#define FG_HI_BLUE	0x9
#define FG_HI_GREEN	0xa
#define FG_HI_CYAN	0xb
#define FG_HI_RED	0xc
#define FG_HI_MAGENTA	0xd
#define FG_HI_YELLOW	0xe
#define FG_HI_WHITE	0xf

#define BG_BLACK	0
#define BG_BLUE		0x10
#define BG_GREEN	0x20
#define BG_CYAN		0x30
#define BG_RED		0x40
#define BG_MAGENTA	0x50
#define BG_YELLOW	0x60
#define BG_WHITE	0x70

#else

#define FG_BLACK	0
#define FG_RED		0x1
#define FG_GREEN	0x2
#define FG_YELLOW	0x3
#define FG_BLUE		0x4
#define FG_MAGENTA	0x5
#define FG_CYAN		0x6
#define FG_WHITE	0x7

#define FG_HI_BLACK	0x8
#define FG_HI_RED	0x9
#define FG_HI_GREEN	0xa
#define FG_HI_YELLOW	0xb
#define FG_HI_BLUE	0xc
#define FG_HI_MAGENTA	0xd
#define FG_HI_CYAN	0xe
#define FG_HI_WHITE	0xf

#define BG_BLACK	0
#define BG_RED		0x10
#define BG_GREEN	0x20
#define BG_YELLOW	0x30
#define BG_BLUE		0x40
#define BG_MAGENTA	0x50
#define BG_CYAN		0x60
#define BG_WHITE	0x70

#endif

#define HI(x)		((x) | 0x8)
/*#define BACK(x)		(0xf0 & ((x)<<4))
#define isHI(x)		((x) & 0x8)
#define BLINK		0x80
#define ATR(x)		(x)
#define FORE(x)		(x)
#define INVCOLOR(x)	(((x)&0x88)|(((x)&0x70)>>4)|(((x)&0x07)<<4))*/

inline void outerr( char *str ) { write( 2, str, strlen(str) ); }
int lowercase( int c );

class Screen
{
protected:
   static int inited;
   static int clipTop;
   static int clipBott;
   static int clipLeft;
   static int clipRight;
public:
   static int initScreen( int term_base );
   static int isOpen;
   static int shadowColor;
   static int UpperCaseMode;

   static char **ScreenMap;        // биты знакомест всего экрана
   static char **currentMap;       // биты знакомест текущей области рисования
   static char **shadowMap;        // тени

   static unsigned char _HL;
   static unsigned char _VL;
   static unsigned char _BL;
   static unsigned char _BC;
   static unsigned char _BR;
   static unsigned char _CL;
   static unsigned char _CC;
   static unsigned char _CR;
   static unsigned char _UL;
   static unsigned char _UC;
   static unsigned char _UR;

   static short **getScr();
   static int Lines, Columns, mapLineLen;

   static Rect center(int NY, int NX);

   static void Open();
   static void Close();

   static int col();
   static int row();
   static void sync( int y, int flushFlag=0 );
   static void sync();

   static void Clear( int attr=0 );
   static void Clear( int y, int x, int ny, int nx, int attr=0, int sym=' ' );
   static void Clear( Point p, int ny, int nx, int attr=0, int sym=' ' );
   static void Clear( Rect r, int attr=0, int sym=' ' );
   static void ClearLine( int y, int attr=0 );
   static void clearScreen();
   static void setClipRegion( Rect r );
   static Rect getClipRegion();

   static void move( int y, int x );
   static void move( Point p );
   static Point cursor();
   static void hideCursor();

   static void put( unsigned char c );
   static void _put( unsigned char c);
   static void put( int c, int attr=0 );
   static void _put( int c, int attr=0 );
   static void put( int y, int x, int ch, int attr=0 );
   static void _put( int y, int x, int ch, int attr=0 );
   static void put( const unsigned char *str, int attr=0 );
   static void _put( const unsigned char *str, int attr=0 );
   static void put( int y, int x, const unsigned char *str, int attr=0 )
	{ move( y, x ); put( str, attr ); }
   static void _put( int y, int x, const unsigned char *str, int attr=0 )
	{ move( y, x ); _put( str, attr ); }
   static void put( Point p, const unsigned char *str, int attr=0 )
	{ move( p.y, p.x ); put( str, attr ); }
   static void _put( Point p, const unsigned char *str, int attr=0 )
	{ move( p.y, p.x ); _put( str, attr ); }
   static void putLimited( int y, int x, const unsigned char *str, int lim, int attr=0 );
   static void putLimited( int y, int x, const unsigned short *str, int lim );
   static void putLimited( Point p, const unsigned short *str, int lim )
	{ putLimited( p.y, p.x, str, lim ); }
   static void attrSet( int y, int x, int ny, int nx, int attr=0 );
   static void attrSet( Rect r, int attr=0 )
	{ attrSet( r.a.y, r.a.x, r.b.y-r.a.y+1, r.b.x-r.a.x+1, attr ); }
   static void attrSet( char **shadowMap, int attr=0 );
   static void print( int attr, const char *fmt, ... );
   static void print( int y, int x, int attr, const char *fmt, ... );
   static void printVect( int attr, const char *fmt, va_list vect );
   static void printVect( int y, int x, int attr, const char *fmt, va_list vect )
	{ move( y, x ); printVect( attr, fmt, vect ); }

   static void horLine  ( int y, int x, int nx, int attr=0 );
   static void vertLine ( int x, int y, int ny, int attr=0 );
   static void frame( int y, int x, int ny, int nx, int attr=0 );
   static void frame( Rect r, int attr=0 );
   static void box( Rect rect, const unsigned char *title=0, int attr=0 );
   static void beep();

   static int scroll( Rect &r, int n, int attr );
/*
   static void statusLine( const char *str, int color=statusLineColor,
			   int light=statusLineLight );
*/
};

extern short **scr;
extern short **oldscr;
extern void allocMemScr();
extern void freeMemScr();

#endif
