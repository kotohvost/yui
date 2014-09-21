/*
	$Id: screen.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "screen.h"

#ifndef DJGPP
#include "term.h"
#else
#include <pc.h>
unsigned short linedraw [11];
#endif

#define STANDOUT        0400
#define BOLD            01000
#define DIM             02000
#define GRAPH           04000

int Screen::shadowColor = FG_WHITE | BG_BLACK;
int Screen::UpperCaseMode = 0;
int Screen::mapLineLen;
int Screen::inited=0;
int GraphAttr=0;

int language=0;		// english
char *lang( char *s1, char *s2 )
{
   return language ? s2 : s1;
}

static unsigned char outbuf[512], *outptr;
static int curx, cury, oldy, oldx;
static int clear;
static unsigned short flgs, oldflgs;
static int hidden;
static int NormalAttr;
static int ClearAttr;

static int updateFlag;
static int beepflag;
static int boldMode = 0;

static   char *helpContextString="index";

char **Screen::ScreenMap = NULL;	// биты знакомест всего экрана
char **Screen::currentMap = NULL;	// биты знакомест текущей области рисования
char **Screen::shadowMap = NULL;	// тени

unsigned char Screen::_HL;
unsigned char Screen::_VL;
unsigned char Screen::_BL;
unsigned char Screen::_BC;
unsigned char Screen::_BR;
unsigned char Screen::_CL;
unsigned char Screen::_CC;
unsigned char Screen::_CR;
unsigned char Screen::_UL;
unsigned char Screen::_UC;
unsigned char Screen::_UR;

int Screen::Lines=25;
int Screen::Columns=80;
int Screen::clipTop, Screen::clipBott, Screen::clipLeft, Screen::clipRight;

short **scr = NULL;
short **oldscr = NULL;

short**	Screen::getScr()	{ return ::scr; }
int	Screen::col()		{ return curx; }
int	Screen::row()		{ return cury; }
void	Screen::move( int y, int x ) { curx = x; cury = y; hidden = 0; }
void	Screen::move( Point p ) 	{ curx = p.x; cury = p.y; hidden = 0; }
Point	Screen::cursor()		{ return Point(cury, curx); }
void	Screen::hideCursor() 	{ hidden=1; }
void	Screen::Clear( Point p, int ny, int nx, int attr, int sym )
	{ Clear( p.y, p.x, ny, nx, attr, sym ); }
void	Screen::Clear( Rect r, int attr, int sym )
	{ Clear(r.a.y, r.a.x, r.b.y-r.a.y+1, r.b.x-r.a.x+1, attr, sym); }
void	Screen::beep() 	{ beepflag=1; }
Rect Screen::center(int NY, int NX)
{
  return Rect(
   Lines/2-(NY)/2,
   Columns/2-(NX)/2,
   Lines/2-(NY)/2+(NY)-1,
   Columns/2-(NX)/2+(NX)-1
   );
}

static void moveTo( int y, int x );

#ifndef DJGPP

static void tputs( char *cp );
static char *skipDelay( char *cp );
static void setAttr( int attr );
static void setColor( int fg, int bg );
static void putChar( unsigned char c );
static int putRawChar( unsigned char c );
static void Flush();

#endif

static int sync_on_exit=0;

void pokeChar( int y, int x, unsigned short c )
{
/*
   if ( y < 0 || y >= Screen::Lines || x < 0 || x >= Screen::Columns )
     {
       fprintf( stderr, "Error in pokeChar, y=%d, x=%d\n", y, x );
       exit(2);
     }
*/
   if ( Screen::currentMap[y][x>>3] & (0x80 >> (x & 7)) )
      scr[y][x] = c;
}

void allocMemScr()
{
  char *err = "Cannot allocate memory\n";
  if ( !(oldscr = (short**)calloc( sizeof(short*), Screen::Lines + 1 ) ) ||
       !(scr    = (short**)calloc( sizeof(short*), Screen::Lines + 1 ) ) ||
       !(Screen::ScreenMap = (char**)calloc( sizeof(char*), Screen::Lines + 1 ) ) ||
       !(Screen::shadowMap = (char**)calloc( sizeof(char*), Screen::Lines + 1 ) ) )
    {
      fprintf( stderr, err );
      exit(1);
    }
  short i, space = ' ' | NormalAttr;
  short *sp, *end;
  for( i=0; i<Screen::Lines; i++ )
   {
     if ( !(scr[i]    = (short*)calloc( sizeof(short), Screen::Columns ) ) ||
	 !(oldscr[i]  = (short*)calloc( sizeof(short), Screen::Columns ) ) ||
	 !(Screen::ScreenMap[i] = (char*)calloc( sizeof(char), Screen::mapLineLen ) ) ||
	 !(Screen::shadowMap[i] = (char*)calloc( sizeof(char), Screen::Columns ) ) )
      {
	fprintf( stderr, err );
	exit(1);
      }
     for( sp=scr[i], end=sp+Screen::Columns; sp < end; sp++ )
	*sp = space;
     for( sp=oldscr[i], end=sp+Screen::Columns; sp < end; sp++ )
	*sp = space;
     memset( Screen::ScreenMap[i], 0xff, Screen::mapLineLen );
   }
}

void freeMemScr()
{
  int i;
  if ( scr ) {
     for( i=0; scr[i]; ::free( scr[i++] ) );
     ::free( scr ); scr=0;
  }
  if ( oldscr ) {
     for( i=0; oldscr[i]; ::free( oldscr[i++] ) );
     ::free( oldscr ); oldscr=0;
  }
  if ( Screen::ScreenMap ) {
     for( i=0; Screen::ScreenMap[i]; ::free( Screen::ScreenMap[i++] ) );
     ::free( Screen::ScreenMap ); Screen::ScreenMap=0;
  }
  if ( Screen::shadowMap ) {
     for( i=0; Screen::shadowMap[i]; ::free( Screen::shadowMap[i++] ) );
     ::free( Screen::shadowMap ); Screen::shadowMap=0;
  }
}

extern "C" void reset_Screen()
{
   Screen::Close();
   Screen::Clear( FG_WHITE | BG_BLACK );
   sync_on_exit=1;
   Screen::sync();
   freeMemScr();
}

int Screen::initScreen( int term_base )
{
  if ( !isatty( 0 ) ) {
      fprintf( stderr, "Standard input is not terminal, sorry.\n" );
      exit(1);
  }

//  Screen::Close();

#ifndef DJGPP
  Term::init( term_base );

  _HL = Term::linedraw[0];
  _VL = Term::linedraw[1];
  _BL = Term::linedraw[2];
  _BC = Term::linedraw[3];
  _BR = Term::linedraw[4];
  _CL = Term::linedraw[5];
  _CC = Term::linedraw[6];
  _CR = Term::linedraw[7];
  _UL = Term::linedraw[8];
  _UC = Term::linedraw[9];
  _UR = Term::linedraw[10];
#else
  _HL = linedraw[0] = 196;
  _VL = linedraw[1] = 179;
  _BL = linedraw[2] = 192;
  _BC = linedraw[3] = 193;
  _BR = linedraw[4] = 217;
  _CL = linedraw[5] = 195;
  _CC = linedraw[6] = 197;
  _CR = linedraw[7] = 180;
  _UL = linedraw[8] = 218;
  _UC = linedraw[9] = 194;
  _UR = linedraw[10]= 191;
#endif

  outptr = outbuf;
  hidden = 0;
  flgs = oldflgs = NormalAttr = ClearAttr = 0x700;
  cury = curx = oldy = oldx = 0;
  clear = 1;

  updateFlag=0;
#ifndef DJGPP
  Lines = Term::rows;
  Columns = Term::cols;
#else
  Lines = ScreenRows();
  Columns = ScreenCols();
#endif

  mapLineLen = Columns/8 + ((Columns%8) ? 1 : 0 );

  freeMemScr();
  allocMemScr();

  currentMap = ScreenMap;

  if ( !inited ) {
      signal( SIGPIPE, (void (*)(int))SIG_IGN );
      atexit( reset_Screen );
  }

#ifndef DJGPP
  if ( Term::GS && Term::GE && Term::GS[0] && Term::GE[0] ||
       Term::AS && Term::AE && Term::AS[0] && Term::AE[0] )
     GraphAttr = 0x8000;
#endif

  setClipRegion( Rect( 0, 0, Lines-1, Columns-1 ) );

  Screen::Open();

  return (inited=1);
}

static void moveTo( int y, int x )
{
   if ( oldx==x && oldy==y )
      return;
#ifndef DJGPP
   if ( oldy==y && x>oldx && x<oldx+7 )
     {
       register short i;
       while( oldx < x )
	 {
	    if ( ((i = oldscr[oldy][oldx]) & 0xff00 ) != oldflgs )
		break;
	    putChar( i );
	    ++oldx;
	 }
       if ( oldx == x )
	 return;
     }
   tputs( Term::Goto( Term::CM, x, y ) );
#else
   ScreenSetCursor( y, x );
#endif
   oldx = x;
   oldy = y;
}

#ifndef DJGPP

static char *skipDelay( char *cp )
{
   while( *cp>='0' && *cp<='9' )
       ++cp;
   if ( *cp == '.' )
      for( ++cp; *cp>='0' && *cp<='9'; ++cp );
   if ( *cp == '*' )
      ++cp;
   return cp;
}

static void tputs (char *cp)
{
	register int c;

	if (! cp)
		return;
	cp = skipDelay (cp);
	while ((c = *cp++))
		putRawChar (c);
}

#if 1
static void setAttr( int attrib )
{
   unsigned short attr = attrib & 0xff00;
   if ( oldflgs == attr )
	return;

   if ( Term::hasColors() )
//   if ( Term::NF )
	setColor( attr>>8 & 0xf, attr>>12 & 0x7 );
   else
    {
      // Set dim/bold/inverse.
      int fg = attr>>8 & 0xf;
      switch( Term::Visuals & Term::VisualMask & (VisualBold|VisualDim|VisualInverse) )
       {
	case VisualBold | VisualDim | VisualInverse:
	     if ( attr & 0x7000 )
		if ( fg > 7 )
		    goto bold_inverse;
		else if (fg < 7)
		    goto dim_inverse;
		else
		    goto inverse;
	     if ( fg > 7 )
		goto bold;
	     if ( fg < 7 )
		goto dim;
	     goto normal;
	case VisualDim | VisualInverse:
	     if ( !(attr & 0x7000) )
		if ( fg >= 7 )
		    goto normal;
		else
		    goto dim;
	     if ( fg >= 7 )
		goto inverse;
	dim_inverse:
	     tputs( Term::MH );
	     tputs( Term::SO );
	     break;
	case VisualBold | VisualInverse:
	     if ( !(attr & 0x7000) )
		if ( fg <= 7 )
		    goto normal;
		else
		    goto bold;
	     if ( fg <= 7 )
		goto inverse;
	bold_inverse:
	     tputs (Term::MD);
	     tputs (Term::SO);
	     break;
	case VisualBold | VisualDim:
	     if ( fg > 7 )
		goto bold;
	     if ( fg < 7 )
		goto dim;
	     goto normal;
	case VisualInverse:
	     if ( !(attr & 0x7000) )
		goto normal;
	inverse:
	     tputs (Term::ME);
	     tputs (Term::SO);
	     break;
	case VisualBold:
	     if ( fg <= 7 )
		goto normal;
	bold:
	     tputs( Term::ME ? Term::ME : Term::SE );
	     tputs( Term::MD );
	     break;
	case VisualDim:
	     if ( fg >= 7 )
		goto normal;
	dim:
	     tputs( Term::ME ? Term::ME : Term::SE );
	     tputs( Term::MH );
	     break;
	case 0:
	normal:
	     tputs( Term::ME ? Term::ME : Term::SE );
	     break;
       }
    }
   if ( (attr ^ oldflgs) && GraphAttr )
    {
/*	if ( Term::GS && Term::GE && Term::hasGraph() )
		tputs( (attr & GraphAttr) ? Term::GS : Term::GE );*/
	if ( Term::AS && Term::AE && Term::hasGraph() )
		tputs( (attr & GraphAttr) ? Term::AS : Term::AE );
	oldflgs ^= GraphAttr;
    }
   oldflgs = attr;
}

#else

#define RESETATTR(a,b,c,f)\
	if (a)\
		tputs (b);\
	else {\
		tputs (c);\
		tputs (f);\
	}\
	break

static void resetattr (int c)
{
	c &= DIM | BOLD | STANDOUT | GRAPH;
	switch (c & (DIM | BOLD | STANDOUT)) {
	case 0:                 RESETATTR (Term::NR, Term::NS, Term::NS, Term::SE);
	case STANDOUT:          RESETATTR (Term::NR, Term::NR, Term::NS, Term::SO);
	case DIM:               RESETATTR (Term::DR, Term::DS, Term::DS, Term::SE);
	case DIM|STANDOUT:      RESETATTR (Term::DR, Term::DR, Term::DS, Term::SO);
	case BOLD:              RESETATTR (Term::BR, Term::BS, Term::BS, Term::SE);
	case BOLD|STANDOUT:     RESETATTR (Term::BR, Term::BR, Term::BS, Term::SO);
	}
	flgs = c;
}

#define SETATTR(a,b,c)\
	if (a)\
		tputs (b);\
	else {\
		if (flgs & STANDOUT) {\
			tputs (Term::SE);\
			tputs (b);\
		} else if ((flgs & (DIM|BOLD)) != (c))\
			tputs (b);\
	}\
	break

#define SETREVA(a,b,c)\
	if (a)\
		tputs (a);\
	else {\
		if ((flgs & (DIM|BOLD)) != (c))\
			tputs (b);\
		if (! (flgs & STANDOUT))\
			tputs (Term::SO);\
	}\
	break

static void setAttr (int c)
{
	if ( c == flgs )
		return;
	c &= DIM | BOLD | STANDOUT | GRAPH;
	if ((c & GRAPH) != (flgs & GRAPH))
		if (c & GRAPH) {
			resetattr (c);
			tputs (Term::GS);
			return;
		} else
			tputs (Term::GE);
	if ((c & (DIM | BOLD | STANDOUT)) != (flgs & (DIM | BOLD | STANDOUT)))
		switch (c & (DIM | BOLD | STANDOUT)) {
		case 0:                 SETATTR (Term::NR, Term::NS, 0);
		case STANDOUT:          SETREVA (Term::NR, Term::NS, 0);
		case DIM:               SETATTR (Term::DR, Term::DS, DIM);
		case DIM|STANDOUT:      SETREVA (Term::DR, Term::DS, DIM);
		case BOLD:              SETATTR (Term::BR, Term::BS, BOLD);
		case BOLD|STANDOUT:     SETREVA (Term::BR, Term::BS, BOLD);
		}
	flgs = c;
}

#endif


static void setColor( int fg, int bg )
{
	static char buf[128];
	/*
	 * This should be optimized later.
	 * For example, we could have an array of 128 color
	 * switching escape strings, precompiled during Init phase.
	 */
	fg = Term::ctab [fg];
	if ( fg < 0 )
		fg = 7;
	bg = Term::btab [bg];
	if ( bg < 0 )
		bg = 0;

	if ( fg & 0x8 ) {
	    if ( Term::MD && !boldMode ) {
		tputs( Term::MD );
		boldMode = 1;
	    }
	} else if ( Term::ME && boldMode ) {
		tputs( Term::ME );
		boldMode = 0;
	}

	if ( Term::C2 ) {
		tputs( Term::Goto( Term::CF, bg, fg ) );
	} else {
		if ( Term::AF && Term::AB ) {
			tputs( Term::Goto( Term::AF, 0, fg & 7 ) );
			tputs( Term::Goto( Term::AB, 0, bg ) );
		} else if ( Term::Sf && Term::Sb ) {
			tputs( Term::Goto( Term::Sf, 0, fg & 7 ) );
			tputs( Term::Goto( Term::Sb, 0, bg ) );
		}
	}
}

static void putChar( unsigned char c )
{
   if ( Screen::UpperCaseMode )
     {
       if ( c>='a' && c<='z' )
	  c += 'A' - 'a';
       else if ( c>=0300 && c<=0336 )
	  c += 040;
       else if (c == 0337)
	  c = '\'';
     }

   static int cyrOutMode=0;
   // set/reset cyrillic output mode
   if ( Term::Cs ) {
     if ( c >=192 && c<=255  ) {
       if ( !cyrOutMode ) {
	    tputs( Term::Cs );
	    cyrOutMode=1;
       }
     } else {
       if ( cyrOutMode ) {
	   tputs( Term::Ce );
	   cyrOutMode=0;
       }
     }
   }
//   putRawChar( Term::outputTable[c] );
   putRawChar( c );
}

static int putRawChar( unsigned char c )
{
   if ( outptr >= outbuf + sizeof(outbuf) )
      Flush ();
   return *outptr++ = c;
}

static void Flush()
{
   if ( outptr > outbuf )
      write( 1, outbuf, (unsigned)(outptr-outbuf) );
   outptr=outbuf;
}

#endif

void Screen::sync( int y, int flushFlag )
{
//   oldflgs = ClearAttr;
   short *Old=oldscr[y], *New=scr[y], x, l;
#ifdef DJGPP
   ScreenUpdateLine( New, y );
#else
   int columns=Columns-1-(y==Lines-1 ? 1 :0), lines=Lines-1;

   for( x=0; x<=columns && Old[x]==New[x]; x++ );
   if ( x>columns )
      return;

   for( l=columns; Old[l]==New[l]; l-- );

   short *prev = &Old[x],
	 *next = &New[x],
	 *end  = &New[l];

	// Search the first nonblank character from the end of line.

   short *e = end;

   if ( Term::CE )
     {
       short *p = &New[columns];
       if ( (*p & 0xff) == ' ' )
	  while( p>next && p[0] == p[-1] )
	     --p;
       // If there are more than 4 characters to clear, use CE.
       if ( e>p+4 || e>p && y >= lines )
	  e = p-1;
     }

   for( ; next <= e; ++prev, ++next, ++x ) {
       if ( *next == *prev )
	  continue;
       if ( x>=Columns || y>=Lines )
	  return;
       moveTo( y, x );
       setAttr( *next );
       putChar( *prev = *next );
       ++oldx;
   }
	// Clear the end of line.
	// First check if it is needed.
   if ( e < end ) {
       moveTo( y, x );
       setAttr( *next );
       tputs( Term::CE );
       while( next <= end )
	  *prev++ = *next++;
   }

   if ( flushFlag )
     {
       if ( curx<0 || curx>=Columns || cury<0 || cury >= Lines /*- (statusVisible ? 1:0)*/ )
	   moveTo( Lines-1, Columns-1 );
       else
	   moveTo( cury, curx );
       Flush();
     }
#endif
}


void Screen::_put( unsigned char c )
{
   if (c<32)
     pokeChar( cury, curx++, (c | 0x40) | HI(flgs) );
   else
     pokeChar( cury, curx++, c | flgs );
}


void Screen::put( unsigned char c )
{
  if ( cury < clipTop || cury > clipBott || curx < clipLeft || curx > clipRight )
     return;
  _put(c);
}

void Screen::put( int c, int attr )
{
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   put( (unsigned char)c );
}

void Screen::_put( int c, int attr )
{
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   _put( (unsigned char)c );
}


void Screen::put( int y, int x, int ch, int attr )
{
   move( y, x );
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   put( (unsigned char)ch );
}

void Screen::_put( int y, int x, int ch, int attr )
{
   move( y, x );
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   _put( (unsigned char)ch );
}

void Screen::put( const unsigned char *str, int attr )
{
   if ( cury < clipTop || cury > clipBott )
      return;
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   for( int x=curx; *str; x++, str++ )
    {
      if ( x < clipLeft )
       {
	 curx++;
	 continue;
       }
      if ( x > clipRight )
	 break;
      put( *str );
    }
}

void Screen::_put( const unsigned char *str, int attr )
{
   if ( cury < clipTop || cury > clipBott )
      return;
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   for( int x=curx; *str; x++, str++ )
    {
      if ( x < clipLeft )
       {
	 curx++;
	 continue;
       }
      if ( x > clipRight )
	 break;
      _put( *str );
    }
}

void Screen::putLimited( int y, int x, const unsigned char *str, int lim, int attr )
{
   if ( y < clipTop || y > clipBott )
      return;
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   move( y, x );
   for( int x=curx; --lim >= 0 && *str; x++, str++ )
    {
      if ( x < clipLeft )
       {
	 curx++;
	 continue;
       }
      if ( x > clipRight )
	 break;
      _put( *str );
    }
}


void Screen::putLimited( int y, int x, const unsigned short *str, int lim )
{
   if ( y < clipTop || y > clipBott )
      return;
   move( y, x );
   for( ; --lim>=0; x++, str++ )
    {
      if ( x < clipLeft )
       {
	 curx++;
	 continue;
       }
      if ( x > clipRight )
	 break;
      pokeChar( y, x, *str );
    }
}

int Screen::isOpen=0;

void Screen::Open ()
{
#ifndef DJGPP
	Term::SetTty();
	if ( Term::IS )
		tputs( Term::IS );
	if ( Term::TI )
		tputs( Term::TI );
	if ( Term::VE )
		tputs( Term::VE );
	if ( Term::KS )
		tputs( Term::KS );
	if ( Term::EA )
		tputs( Term::EA );
#endif
	isOpen=1;
}

void Screen::Close()
{
#ifndef DJGPP
	if ( oldscr )
		setAttr( NormalAttr );
	if ( Term::FS )
		tputs( Term::FS );
	if ( Term::VE )
		tputs( Term::VE );
	if ( Term::KE )
		tputs( Term::KE );
	Flush ();
	Term::ResetTty ();
#endif
	isOpen=0;
}



void Screen::Clear( int attr )
{
#ifndef DJGPP
   if ( !Term::hasColors() )
//   if ( !Term::NF )
	ClearAttr = NormalAttr;
   else
#endif
   if (attr)
       { ClearAttr = attr<<8 /*&& 0x7f00*/; flgs=ClearAttr; }
   else
	ClearAttr = flgs;
   for( int y=0; y<Lines; y++ )
     {
	short *end = &scr[y][Columns];
	for( short *sp=scr[y]; sp<end; sp++ )
	  if ( *sp != (' ' | ClearAttr) )
		*sp = ' ' | ClearAttr;
     }
   curx = cury = 0;
   hidden = 0;
   clear = 1;
}

void Screen::Clear( int y, int x, int ny, int nx, int attr, int sym )
{
   if ( attr )
      flgs = attr<<8 & 0x7f00;
   sym = sym | flgs;
   for( ; --ny>=0; ++y )
     if ( y>=clipTop && y<=clipBott )
	for( int i=nx, j=x; --i>=0; j++ )
	  if ( j>=clipLeft && j<=clipRight )
	     pokeChar( y, j, sym );
}

void Screen::ClearLine( int y, int attr )
{
   if ( y < clipTop || y > clipBott )
      return;
   Clear( y, 0, 1, Columns, attr );
}

void Screen::sync()
{
   if ( clear )
     {
       clearScreen();
       clear = 0;
     }

   for( int y=0; y < Lines; y++ )
      sync( y );

   if ( !sync_on_exit )
     {
       if ( curx<0 || curx>=Columns || cury<0 || cury >= Lines /*- (statusVisible ? 1:0)*/ )
	   hidden = 1;

       if ( !hidden )
	   moveTo( cury, curx );
       else
	   moveTo( Lines-1, Columns-1 );
     }
   else moveTo( Lines-1, 0 );

#ifndef DJGPP
   if ( beepflag )
     {
       putRawChar ('\007');
       beepflag = 0;
     }
   Flush();
#else
   if ( beepflag )
     {
       sound(1000);
       usleep(100000);
       nosound();
       beepflag = 0;
     }
#endif
}

void Screen::clearScreen()
{
#ifndef DJGPP
	// Reset screen attributes to "normal" state.
//   if ( Term::hasGraph() )
   if ( Term::GE )
       tputs( Term::GE );
   if ( Term::hasColors() ) {
//   if ( Term::NF ) {
       setColor( ClearAttr>>8 & 15, ClearAttr>>12 & 7 );
       oldflgs=flgs=ClearAttr;
   } else {
       tputs (Term::ME ? Term::ME : Term::SE);
       oldflgs = ClearAttr = NormalAttr;
   }
   // Clear screen.
   tputs( Term::CL );
#endif
   oldy = oldx = 0;
   for( int y=0; y<Lines; ++y )
      for( int x=0; x<Columns; x++ )
	 oldscr[y][x]= short(' ') | ClearAttr;
}

void Screen::setClipRegion( Rect r )
{
  if ( r.a.y > r.b.y || r.a.x > r.b.x )
     return;
  clipTop	= r.a.y;
  clipLeft	= r.a.x;
  clipBott	= r.b.y;
  clipRight	= r.b.x;

  if ( clipTop < 0 )
     clipTop = 0;
  else if ( clipTop >= Lines )
     clipTop = Lines-1;

  if ( clipBott < 0 )
     clipBott = 0;
  else if ( clipBott >= Lines )
     clipBott = Lines-1;

  if ( clipLeft < 0 )
     clipLeft = 0;
  else if ( clipLeft >= Columns )
     clipLeft = Columns - 1;

  if ( clipRight < 0 )
     clipRight = 0;
  else if ( clipRight >= Columns )
     clipRight = Columns - 1;
}

Rect Screen::getClipRegion()
{
  return Rect( clipTop, clipLeft, clipBott, clipRight );
}

int Screen::scroll( Rect &r, int n, int clearAttr )
{
#ifdef DJGPP
   return 0;
#else
   int y, topline = r.a.y, botline = r.b.y,
       left=max(r.a.x,0), right=max(Columns-r.b.x-1,0);
   if ( Term::rscrool && !Term::scrool /*&& !Term::CS*/ )
     {
	// cannot scrool small regions if no scrool region
       if ( 2 * (botline - topline) < Lines-2 )
	    return 0;
       topline = 1;
       botline = Lines-2;
     }
   short *buf1, *buf2, *temp1, *temp2;
   static short ll[256], rr[256];
   if ( topline < 1 ) topline=1;
   if ( botline > Lines-1 ) botline=Lines-1;
   if ( topline >= botline)
     return 0;
   // update in-core screen image
   int sym = ' ' | clearAttr, off=Columns-right,
       lsize=left*sizeof(short), rsize=right*sizeof(short);
   if ( n > 0 )
     {
       for( int i=n; i>0; --i )
	{
	  temp1 = oldscr[botline]; temp2 = scr[botline];
	  buf1 = scr[topline];
	  memcpy( (char*)ll, buf1, lsize );
	  memcpy( (char*)rr, buf1+off, rsize );
	  for( y=topline+1; y<=botline; y++ )
	    {
	      buf1 = scr[y]; buf2 = scr[y-1];
	      memcpy( buf2, buf1, lsize );
	      memcpy( buf2+off, buf1+off, rsize );
	    }
	  for( y=botline; y>topline; y-- )
	    {
	      scr[y] = scr[y-1];
	      oldscr[y] = oldscr[y-1];
	    }
	  memcpy( temp2, ll, lsize );
	  memcpy( temp2+off, rr, rsize );
	  for( short *end = &temp1[Columns]; end>temp1; *(--end)=sym );
	  oldscr[topline] = temp1; scr[topline] = temp2;
	}
     }
   else
     {
       for( int i=n; i<0; i++ )
	{
	  temp1 = oldscr[topline]; temp2 = scr[topline];
	  buf1 = scr[botline];
	  memcpy( (char*)ll, buf1, lsize );
	  memcpy( (char*)rr, buf1+off, rsize );
	  for( y=botline-1; y>=topline; y-- )
	    {
	      buf1 = scr[y]; buf2 = scr[y+1];
	      memcpy( buf2, buf1, lsize );
	      memcpy( buf2+off, buf1+off, rsize );
	    }
	  for( y=topline; y<botline; y++ )
	    {
	      scr[y] = scr[y+1];
	      oldscr[y] = oldscr[y+1];
	    }
	  memcpy( temp2, ll, lsize );
	  memcpy( temp2+off, rr, rsize );
	  for( short *end = &temp1[Columns]; end>temp1; *(--end)=sym );
	  oldscr[botline] = temp1; scr[botline] = temp2;
	}
     }

/*
   if ( Term::CS )                  // set scrool region
      tputs( Term::Goto( Term::CS, botline, topline ) );
*/
	    // do scrool n lines forward or backward
   if ( n > 0 )
     {
       if ( /*Term::CS ||*/ !Term::scrool )
	 {
	   moveTo( /*Term::CS ? topline :*/ 0, 0 );
	   setAttr (clearAttr);
	   while( --n >= 0 )
	      tputs( Term::SR );
	 }
       else while( --n >= 0 )
	 {
	   moveTo (botline, 0);
	   setAttr (clearAttr);
	   tputs (Term::DL);
	   moveTo (topline, 0);
	   setAttr (clearAttr);
	   tputs (Term::AL);
	 }
     }
   else
     {
       if ( /*Term::CS ||*/ !Term::scrool )
	 {
	   moveTo( /*Term::CS ? botline :*/ Lines-1, 0);
	   setAttr (clearAttr);
	   while( ++n <= 0 )
	       tputs (Term::SF);
	 }
       else while( ++n <= 0 )
	 {
	   moveTo (topline, 0);
	   setAttr (clearAttr);
	   tputs (Term::DL);
	   moveTo (botline, 0);
	   setAttr (clearAttr);
	   tputs (Term::AL);
	 }
     }
/*
   if ( Term::CS )
      tputs( Term::Goto (Term::CS, Lines-1, 0) );
*/
   oldx = oldy = -1;
   return 1;
#endif
}

void Screen::attrSet( int y, int x, int ny, int nx, int attr )
{
   if ( x<0 ) { nx+=x; x=0; }
   if ( y<0 ) { ny+=y; y=0; }
   if ( nx<=0 || ny<=0 )
	return;
   attr = (attr ? attr : shadowColor)<<8 & 0x7f00;
   for( ; --ny>=0; ++y )
    {
      if ( y < clipTop )
	 continue;
      if ( y > clipBott )
	 break;
      char *ch=currentMap[y];
      short *sc=scr[y];
      for( int X=x; X < nx+x; X++ )
       {
	if ( X < clipLeft )
	   continue;
	if ( X > clipRight )
	   break;
	if ( ch[X>>3] & 0x80 >> (X & 0x7) )
	  {
//	    sc[X] &= sc[X] & GraphAttr ? GraphAttr | 0x00ff : 0x00ff;
	    sc[X] &= 0x80ff;
	    sc[X] |= attr;
	  }
       }
    }
}

void Screen::attrSet( char **shMap, int attr )
{
   if ( !attr )
      attr = shadowColor<<8 & 0x7f00;
   char *sh=0;
   short *sc=0;
   for( int x, y=clipTop; y <= clipBott; y++ )
    {
      sh=shMap[y];
      sc=scr[y];
      for( x=clipLeft; x <= clipRight; x++ )
	if ( *sh++ & 2 )
	  {
//	    sc[x] &= sc[x] & GraphAttr ? GraphAttr | 0x00ff : 0x00ff;
	    sc[x] &= 0x80ff;
	    sc[x] |= attr;
	  }
    }
}

void Screen::frame( int y, int x, int ny, int nx, int attr )
{
   horLine( y, x+1, nx-2, attr );
   horLine( y+ny-1, x+1, nx-2, attr );
   vertLine( x, y+1, ny-2, attr );
   vertLine( x+nx-1, y+1, ny-2, attr );
#ifndef DJGPP
   unsigned short *sym = Term::linedraw;
#else
   unsigned short *sym = linedraw;
#endif
   unsigned short lineAttr = GraphAttr | flgs;
   int right=x+nx-1, bott=y+ny-1;
   if ( y>=clipTop && y<=clipBott )
     {
       if ( x>=clipLeft && x<=clipRight )
	  pokeChar( y, x, lineAttr | (unsigned char)sym[8] );      // ULC
       if ( right>=clipLeft && right<=clipRight )
	  pokeChar( y, right, lineAttr | (unsigned char)sym[10] ); // URC
     }
   if ( bott>=clipTop && bott<=clipBott )
     {
       if ( x>=clipLeft && x<=clipRight )
	  pokeChar( bott, x, lineAttr | (unsigned char)sym[2] );   // LLC
       if ( right>=clipLeft && right<=clipRight )
	  pokeChar( bott, right, lineAttr | (unsigned char)sym[4] );  // LRC
     }
}

void Screen::frame( Rect r, int attr )
{
   frame( r.a.y, r.a.x, r.b.y-r.a.y+1, r.b.x-r.a.x+1, attr );
}

void Screen::horLine( int y, int x, int nx, int attr )
{
   if ( y < clipTop || y > clipBott )
      return;
   if ( attr )
       flgs = attr<<8 & 0x7f00;
#ifndef DJGPP
   unsigned short sym = _HL | flgs | GraphAttr;
#else
   unsigned short sym = flgs | (unsigned char)(linedraw[0]) | GraphAttr;
#endif
   for( ; --nx >=0; x++ )
    {
      if ( x < clipLeft )
	 continue;
      if ( x > clipRight )
	 break;
      pokeChar( y, x, sym );
    }
}

void Screen::vertLine( int x, int y, int ny, int attr )
{
   if ( x < clipLeft || x > clipRight )
      return;
   if ( attr )
      flgs = attr<<8 & 0x7f00;
#ifndef DJGPP
   unsigned short sym = _VL | flgs | GraphAttr;
#else
   unsigned short sym = flgs | (unsigned char)(linedraw[1]) | GraphAttr;
#endif
   for( ; --ny >=0; y++ )
    {
      if ( y < clipTop )
	 continue;
      if ( y > clipBott )
	 break;
      pokeChar( y, x, sym );
    }
}

void Screen::box( Rect rect, const unsigned char *title, int attr )
{
   if ( rect.b.x<rect.a.x || rect.b.y<rect.a.y )
      return;
   frame( rect, attr );
   if ( !title || !title[0] )
      return;
   int width=rect.b.x-rect.a.x+1;
   int len=strlen( (char*)title );
   if ( len>width-2 )
       len = width-2;
   putLimited( rect.a.y, rect.a.x+((width-len)>>1), title, len, 0 );
}

void Screen::printVect( int attr, const char *fmt, va_list vect )
{
   if ( cury < clipTop || cury > clipBott  )
      return;
   static char buf[512];
   vsprintf( buf, fmt, vect );
   int offset=0;
   if ( curx<0 )
     {
       unsigned cx = -curx;
       if ( cx >= strlen( buf ) )
	  return;
       offset += cx;
       curx=0;
     }
   else if ( curx>=Columns )
       return;
   put( (const unsigned char*)buf+offset, attr );
}

void Screen::print( int attr, const char *fmt, ... )
{
   va_list ap;
   va_start(ap, fmt);
   printVect( attr, fmt, ap/*&fmt + 1*/ );
   va_end(ap);
}

void Screen::print( int y, int x, int attr, const char *fmt, ... )
{
   move( y, x );
   va_list ap;
   va_start(ap, fmt);
   printVect( attr, fmt, ap/*&fmt + 1*/ );
   va_end(ap);
}

int lowercase( int c )
{
  if ( c>='A' && c<='Z' )
      return c + 'a' - 'A';
  if ( c>=0xe0 && c<=0xff )
      return c - 0x20;
  return c;
}

/*
 * multi language support
 */

Lang::Lang( char ** arr ) : msg(0)
{
  msg = (char**)calloc( LANGS, sizeof(char*) );
  if ( arr )
     for( int i=0; arr[i] && i < LANGS; i++ )
	put( arr[i], i );
}

Lang::Lang( const char *s1, const char *s2 )
{
  msg = (char**)calloc( LANGS, sizeof(char*) );
  if ( s1 )
     put( s1, 0 );
  if ( s2 && LANGS > 1 )
     put( s2, 1 );
}

Lang::~Lang()
{
  for( int i=0; i < LANGS; i++ )
     if ( msg[i] )
	::free( msg[i] );
  ::free( msg );
}

char *Lang::get( int index )
{
  if ( index < 0 )
    index = language;
  if ( index < 0 || index >= LANGS )
     return 0;
  if ( !msg[index] )
     index=0;
  return msg[index];
}

int Lang::put( const char *str, int index )
{
  if ( index < 0 || index >= LANGS || !str )
     return 0;
  msg[index] = (char*)realloc( msg[index], strlen( str ) + 1 );
  if ( msg[index] )
     strcpy( msg[index], str );
  return 1;
}

