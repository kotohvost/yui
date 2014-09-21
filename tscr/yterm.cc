/*
	$Id: yterm.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(LINUX_X86_LIBC6)
  #define __USE_XOPEN
#endif
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <termios.h>

#if defined(SOLARIS_SPARC) || defined(SOLARIS_X86) || defined(SINIX)
#include <stropts.h>
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <term.h>

#include "yterm.h"
#include "task.h"
#include "iobuf.h"
#include "hashcode.h"
#include "program.h"

#include "modal.h"

static int openPty(int &tty, int &pty);
static pid_t startProgram(const char *program, int tty, int lines, int columns);

// [ static functions


static int openPty(int &tty, int &pty)
{
  int ret = 0;
  struct stat st;
/*  if ( stat( "/dev/pts", &st ) == 0 && S_ISDIR(st.st_mode) ) {
      if ( (pty = open("/dev/ptmx", O_RDWR)) >= 0 ) {
	  grantpt(pty);
	  unlockpt(pty);
	  if ( (tty = open( ptsname(pty), O_RDWR )) >= 0 ) { */
#ifndef LINUX_X86_LIBC6
/*	      ioctl( tty, I_PUSH, "ptem" );
	      ioctl( tty, I_PUSH, "ldterm" );*/
#endif
/*	      return 1;
	  }
      }
  } else { */
      char ptyname[]="/dev/ptyp0";
      char ttyname[]="/dev/ttyp0";
      for( int i='p',j=0; i<'r'; i++ ) {
	  for( j='0'; ; ) {
	      if (j==':') j='a';
	      if (j=='g') break;
	      ptyname[8] = ttyname[8] = i;
	      ptyname[9] = ttyname[9] = j;
	      j++;
	      pty = open( ptyname, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/, 0 );
	      if ( pty < 0 )
		  continue;
	      tty = open( ttyname, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/, 0 );
	      if ( tty < 0 ) {
		  close( pty ); continue;
	      }
	      return 1;
	  }
      }
//  }
#if 0

#if defined(SOLARIS_SPARC) || defined(SOLARIS_X86) || defined(SINIX) \
	|| defined(LINUX_X86_LIBC6)
  if ( (pty = open("/dev/ptmx", O_RDWR)) >= 0 )
   {
     grantpt( pty );
     unlockpt( pty );
     if ( (tty = open( ptsname( pty ), O_RDWR )) >= 0 )
      {
#ifndef LINUX_X86_LIBC6
	ioctl( tty, I_PUSH, "ptem" );
	ioctl( tty, I_PUSH, "ldterm" );
#endif
	return 1;
      }
   }
#else
  char ptyname[]="/dev/ptyp0";
  char ttyname[]="/dev/ttyp0";
  for( int i='p',j=0; i<'r'; i++ )
    for( j='0'; ; )
     {
	if (j==':') j='a';
	if (j=='g') break;
	ptyname[8] = ttyname[8] = i;
	ptyname[9] = ttyname[9] = j;
	j++;
	pty = open( ptyname, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/, 0 );
	if ( pty< 0 )
	   continue;
	tty = open( ttyname, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/, 0 );
	if ( tty < 0 )
	  { close( pty ); continue; }
	return 1;
     }
#endif

#endif /* 0 */

  return 0;
}


static const char *termEntry(int lines, int columns)
{
     static char entry[]=
"\
pc3|yterm|BSD/386 Console:\
cm=\\E[%i%d;%dH:ho=\\E[;H:\
C2:Nf#16:Nb#8:Cf=\\E[=%p1%dF\\E[=%p2%dG:\
gs=:ge=:\
ac=l\\332q\\304k\\277x\\263j\\331m\\300w\\302u\\264v\\301t\\303n\\305:\
g1=\\200\\201\\204\\211\\205\\206\\212\\207\\202\\210\\203:\
g2=\\240\\241\\253\\273\\256\\261\\276\\265\\245\\270\\250:\
f1=\\E[M:f2=\\E[N:f3=\\E[O:f4=\\E[P:f5=\\E[Q:\
f6=\\E[R:f7=\\E[S:f8=\\E[T:f9=\\E[U:f0=\\E[V:\
k1=\\E[M:k2=\\E[N:k3=\\E[O:k4=\\E[P:k5=\\E[Q:\
k6=\\E[R:k7=\\E[S:k8=\\E[T:k9=\\E[U:k0=\\E[V:\
kh=\\E[H:kH=\\E[F:kP=\\E[I:kN=\\E[G:kI=\\E[L:\
ku=\\E[A:kd=\\E[B:kl=\\E[D:kr=\\E[C:\
kb=^H:kB=\\E[w:kC=\\E[E:kD=\\177:\
am:bs:\
md=\\E[1m:mb=\\E[1m:mk=\\E[8m:me=\\E[0m:mr=\\E[7m:\
us=\\E[1m:ue=\\E[0m:\
so=\\E[7m:se=\\E[0m:\
cd=\\E[J:ce=\\E[K:\
al=\\E[L:dl=\\E[M:\
sc=\\E7:rc=\\E8:\
up=\\E[A:do=\\E[B:nd=\\E[C:le=\\E[D:\
UP=\\E[%dA:DO=\\E[%dB:RI=\\E[%dC:LE=\\E[%dD:\
cl=\\Ec:\
:::::::::::::::::::::::::::::\
";

  char *ptr = (char*)strstr(entry, ":::")+3;
  sprintf(ptr, "co#%d:li#%d::::", columns, lines);
  return entry;
}


static const char *termNum(int n)
{
  static char str[10];
  sprintf(str, "%d", n);
  return str;
}

static pid_t startProgram(const char *program, int tty, int lines, int columns)
{

  struct winsize ws;
  ioctl( tty, TIOCGWINSZ, &ws);
  ws.ws_row=lines;
  ws.ws_col=columns;
  ioctl( tty, TIOCSWINSZ, &ws);
#if defined(SCO_SYSV) || defined(AIX_PPC)
  pid_t pid=fork();
#else
  pid_t pid=vfork();
#endif
  if (pid==-1)
      return -1;
  else if ( !pid )
    {
       setsid();
       //setpgrp();
#if !defined(SCO_SYSV) && !defined(SOLARIS_SPARC) && !defined(SOLARIS_X86)\
	&& !defined(AIX_PPC) && !defined(SINIX)
       ioctl(tty, TIOCSCTTY, 0);
#endif
       struct termios tio;
       tcgetattr(tty, &tio);
       tcflush(tty, TCOFLUSH);
       tio.c_cc[VERASE]='H'-'@';
       tio.c_cc[VINTR]='C'-'@';
       tio.c_cc[VKILL]='U'-'@';
       tcsetattr(tty, TCSANOW , &tio);

       dup2(tty,0);
       dup2(tty,1);
       dup2(tty,2);
       close(tty);

       for(int i=3; i<20; i++)
	   close(i);

       setgid(getgid());
       setuid(getuid());
#if !defined(SCO_SYSV) && !defined(SOLARIS_SPARC) && !defined(SOLARIS_X86)\
	&& !defined(AIX_PPC) && !defined(SINIX)
       char lstr[10];
       sprintf(lstr, "%d", lines);
       setenv( "LINES", lstr, 1);
       sprintf(lstr, "%d", columns);
       setenv( "COLUMNS", lstr, 1);
       setenv( "TERM", "pc3", 1 );
       setenv( "TERMCAP", termEntry(lines,columns), 1);
#else
       static char lstr[2048];
       sprintf( lstr, "LINES=%d", lines );
       putenv( lstr );
       sprintf( lstr, "COLUMNS=%d", columns );
       putenv( lstr );
       putenv( "TERM=pc3" );
       sprintf( lstr, "TERMCAP=%s", termEntry(lines,columns) );
       putenv( lstr );
#endif
       if ( strpbrk(program, "*|&><;\"'`$()[]{}\\") )
	 execl( "/bin/sh", "/bin/sh", "-c", program, 0 );
       else
	{
	 // split program to words
	 char *string=strdup(program);
	 char **vect=new char *[64];
	 int i=0;
	 for (char *s=strtok( string, " \t"); s && i<63 ; s=strtok(0, " \t"), i++ )
	   vect[i]=s;
	 vect[i]=0;
	 execvp(vect[0], vect);
	 printf("cannot start program '%s'\n", program);
	 fflush(stdout);
	 execl("/bin/sh", "/bin/sh", "-c", "sleep 2", 0);
	}
       exit(111);
   }

  return pid;
}

// static functions ]{ YTerm common

int  YTerm::isType(long typ)
{
  return (typ==HASH_YTerm ? 1 : Window::isType(typ));
}

long ytermKeyMap[] = {
	FUNC1(kbTab),	HASH_cmMode,
	FUNC2(kbTab),	HASH_cmMode,
	FUNC1(kbCtrlF),	HASH_cmModeKey,
	FUNC2(kbCtrlF),	HASH_cmModeKey,
	kbF7,		HASH_cmBlockLine,
	kbF8,		HASH_cmBlockColumn,
	kbCtrlP,	HASH_cmUnmarkBlock,
	FUNC1( '+' ),	HASH_cmCopyToClip,
	FUNC1( kbIns ),	HASH_cmCopyFromClip,
	0
};

/*
CommandDescr ytermCommandDescr[] = {
	{ HASH_cmMode,		new Lang("Hard/soft cursor mode","Жесткий/мягкий режим курсора") },
	{ HASH_cmModeKey,	new Lang("Change key getting mode (first/last)","Порядок получения клавиши (первый/последний)") },
	{ HASH_cmBlockLine,	new Lang("Mark line block","Маркер строчного блока") },
	{ HASH_cmBlockColumn,	new Lang("Mark column block","Маркер колоночного блока") },
	{ HASH_cmUnmarkBlock,	new Lang("Unmark block","Снять маркер блока") },
	{ HASH_cmCopyToClip,	new Lang("Copy block to clipboard","Копирование блока в буфер обмена") },
	{ HASH_cmCopyFromClip,	new Lang("Copy block from clipboard","Копирование блока из буфера обмена") },
	{ 0, 0 }
};
*/

Keymap YTermKeyMap( ytermKeyMap, new Lang("Terminal window base commands","Базовые команды в термнальном окне") );

YTerm::YTerm( const char *program, char **environ, Rect tr, int width,
	      int scrLines, int fullLines, TermInputTask *Input ):
	      Window( ((tr.a.x==-1 && tr.a.y==-1 && tr.b.x==-1 && tr.b.y==-1) ? scrRect() : tr),
	      new Lang((char*)program), new Lang("Terminal","Терминал"),
	      new Lang("Terminal(pc3)","Терминал(pc3)"), 1 ),
	      input(Input), Columns(width), Lines(scrLines), hardMode(1),
	      flagFinishMessage(0)

{
  setHelpContext( "Terminal" );
  keyHolder.add( &YTermKeyMap/*, ytermCommandDescr*/ );
  firstHandle = 1;
  isBold = 0;
  clearAttr = curattr = FG_WHITE | BG_BLACK;
  soattr = FG_BLACK | BG_WHITE;
  mode = NORMAL;
  Lines=size.y-2;
  if (size.x==80)
    Columns=80;
  else
    Columns=size.x-2;

  if (fullLines<Lines)
      fullLines=Lines;

  FullLines = fullLines;
  FirstLine = FullLines-Lines;
  oldDelta.y = delta.y = FirstLine;

  cury = FirstLine;
  curx = 0;

  vscreen = new ScrElement* [ FullLines ];
  bufEl.a=clearAttr;
  bufEl.c=' ';
  lnum = new int[FullLines];
  for( int i=0; i < FullLines; i++)
   {
    vscreen[i]=new ScrElement[ Columns ];
    for(int j=0; j<Columns; j++)
      vscreen[i][j]=bufEl;
    lnum[i]=i;
   }

  pid_t pid;

  if( !openPty(tty, pty) )
   {
     puts("cannot open pseudo-terminal");
     return;
   }

  if ( (pid=startProgram(program, tty, Lines, Columns)) !=-1 )
   {
     if (!input)
       input=new TermInputTask( pty, pid, this );
     else
       input->initData(pty,pid,this);
     Task::addref(input);
   }
  else if (pid==-1)
   {
     puts("cannot start program");
     puts(program);
     puts(strerror(errno));
   }
  ::close(tty);
}

YTerm::~YTerm()
{
  if (input)
   {
    input->close();
    Task::delref(input);
   }
  for( int i=0; i < FullLines; i++ )
     delete vscreen[i];
  delete vscreen;
  delete lnum;
  winMenu = 0;
}

static char finishMessage[256];

void YTerm::closeTerm()
{
  if ( input )
   {
     Task::delref(input);
     input=0;
   }
  if ( flagFinishMessage && this != appl->topWindow )
   {
     sprintf( finishMessage, lang("Process '%s' finished","Процесс '%s' закончен"), title->get() );
     appl->test( finishMessage );
   }
}

int YTerm::close()
{
  if (input)
   {
     input->close();
     Task::delref(input);
     input=0;
   }
  return 1;
}

void YTerm::setBold( int newBold )
{
  isBold=newBold;
}

int YTerm::getBold( )
{
  return isBold;
}

char *YTerm::curWord()
{
  return "not_implemented";
}

char *YTerm::getStr( int line, int &len )
{
   if ( line < 0 || line >= FullLines )
      return 0;
   static char buf[256];
   int i=0;
   for( ; i<Columns; i++ )
     buf[i] = (vscreen[line][i]).c;
   len = i;
   return buf;
}

void YTerm::copyBlockToClip()
{
   if ( !FLAG_LINE && !FLAG_BLOCKLINE && !FLAG_COL && !FLAG_BLOCKCOL )
      return;
   clipBoard.freeAll();
   blockInfo *b = validBlock();
   int len;
   if ( FLAG_LINE || FLAG_BLOCKLINE )
     {
       for( long i=b->top; i <= b->bott; i++ )
	{
	  char *str = getStr( i, len );
	  char *s = new char[len+1];
	  memcpy( s, str, len ); s[len]=0;
	  clipBoard.insert( s );
	}
       short *type = new short(0);
       clipBoard.atInsert( 0, type );
     }
   else if ( FLAG_COL || FLAG_BLOCKCOL )
     {
       int l, pos;
       for( long i=b->top; i<=b->bott; i++ )
	{
	  char *str = getStr( i, len );
	  for( l=0, pos=b->left; pos<len && pos<=b->right; pos++, l++ );
	  char *s = new char[l+1];
	  memcpy( s, str+b->left, l ); s[l]=0;
	  clipBoard.insert( s );
	}
       short *type = new short( b->right - b->left + 1 );
       clipBoard.atInsert( 0, type );
     }
}

void YTerm::copyBlockFromClip()
{
   if ( !hardMode || clipBoard.getCount() < 2 )
      return;
   int flagCR = clipBoard.getCount() > 2 ? 1 : 0;
   for( ccIndex i=1; i < clipBoard.getCount(); i++ )
    {
      writePty( (char*)clipBoard.at(i) );
      if ( flagCR )
	 writePty( "\r" );
    }
}

unsigned char colorTerm[2] = {
	FG_HI_WHITE | BG_BLACK,
	FG_WHITE | BG_BLACK
};

void YTerm::setColors( int type )
{
	clr = colorTerm;
//	clearAttr = curattr = clr[1];
}

int YTerm::init( void *data )
{
  Window::init( data );
  Lines=size.y-2;
  FirstLine=FullLines-Lines;

  delta.y = max( 0, FullLines-Lines );
  delta.x = 0;

  struct winsize ws;
  //ioctl( tty, TIOCGWINSZ, &ws);
  ioctl( pty, TIOCGWINSZ, &ws);
  ws.ws_row=size.y-2;
  ws.ws_col=(size.x==80)?size.x:(size.x-2);
  //ioctl( tty, TIOCSWINSZ, &ws);
  ioctl( pty, TIOCSWINSZ, &ws);
  if ( input )
     input->start();

  return 1;
}

void YTerm::info()
{
   static char buf[30];
   if ( firstHandle )
     {
       Screen::move( rect.b.y, rect.a.x+2 );
       Screen::put( (u_char*)lang("first","первый"), clr[0] );
     }
   if ( hardMode )
      return;
   int pr = min( 100, (delta.y+size.y)*100/FullLines );
   sprintf( buf, "<%3li><%2li><%3i%%>", delta.y+cursor.y+1,
				      delta.x+cursor.x+1, pr );
   Screen::move( rect.a.y, rect.a.x+2 );
   Screen::put( (u_char*)buf, clr[0] );
}

int YTerm::draw( int Redraw, int sy, int sx )
{
  int ret=0;
  if ( Window::draw( Redraw, sy, sx ) ) {
      ret=1;
      Screen::currentMap = scr.map;

      if ( appl->topWindow == this ) {
	  if ( hardMode )
	      scroolTerm(); // скроллинги если надо и возможно
	  else
	      scroll( getAttr(), Redraw );
      }

      Rect r = rect;
      r.grow( -1, -1 );
      int lim=min(size.x-2, Columns-delta.x);
#if 0
      int j=r.a.x, jj=delta.x;
      for( int i=r.a.y, ii=delta.y; i <= r.b.y && ii < FullLines; i++, ii++ ) {
	  Screen::putLimited( i, j, (unsigned short*)&(vscreen[ii][jj]), lim );
      }
#else
      for( int i=r.a.y, ii=delta.y; i <= r.b.y && ii < FullLines; i++, ii++ ) {
	  ScrElement *se = vscreen[ii];
	  for( int j=r.a.x, jj=delta.x; j <= lim; j++, se++ )
	      Screen::_put( i, j, translate_out( se->c ), se->a );
      }
#endif

      int lastx = Columns-delta.x;
      if ( lastx < size.x-2 ) {
	  bufEl.a=clearAttr; bufEl.c=' ';
	  for( int i=r.a.y; i<=r.b.y; i++ ) {
	      for( int j=r.a.x+lastx; j <= r.b.x ; j++ )
		  Screen::_put( i, j, bufEl.c, bufEl.a );
	  }
      }
      int lasty = FullLines-delta.y;
      if ( lasty < size.y-2 ) {
	  bufEl.a=clearAttr; bufEl.c=' ';
	  for( int i=r.a.y+lasty; i<=r.b.y; i++ ) {
	      for( int j=r.a.x; j <= r.b.x ; j++ )
		  Screen::_put( i, j, bufEl.c, bufEl.a );
	  }
      }

      if ( FLAG_COL || FLAG_LINE || FLAG_BLOCKCOL || FLAG_BLOCKLINE ) {
	  correctBlock();
	  blockInfo *b = validBlock();
	  Screen::attrSet( b->top - delta.y + rect.a.y + 1,
			   b->left - delta.x + rect.a.x + 1,
			   b->bott - b->top + 1,
			   b->right - b->left + 1,
			   FG_BLACK | BG_WHITE );
      }

      info();
      Screen::currentMap=Screen::ScreenMap;

      if ( appl->topWindow != this )
	 Screen::attrSet( Screen::shadowMap );

      MoveCursor();
      Screen::sync();
  }

  for( int i=0; i<FullLines; i++)
      lnum[i]=i;

  oldDelta=delta;
  return ret;
}

void YTerm::moveCursor()
{
   int y, x;
   if ( hardMode )
     {
       y = cury - delta.y;
       x = curx - delta.x;
     }
   else
     { y = cursor.y; x = cursor.x; }
   if ( y>=0 && y<Lines && x>=0 && x<size.x-2 )
       Screen::move( y + rect.a.y + 1, x + rect.a.x + 1 );
   else
       Screen::hideCursor();
}

void YTerm::writePty( const char *str)
{
  int le=strlen(str);
  for( int l; le>0 ; le-=l, str+=l )
   {
     l=::write(pty, str, le);
     if (l<0)
       return;
     else if(l!=le)
       Task::yield();
   }
}

int YTerm::procKey( long key, long translatedKey )
{
  key=ONEKEY(key);
  if ( !hardMode )
    {
      switch( translatedKey )
       {
	 case kbUp:
	     if ( cursor.y > 0 )	cursor.y--;
	     else if ( delta.y > 0 )	delta.y--;
	     key=0; break;
	 case kbDown:
	     if ( cursor.y < Lines-1 )		cursor.y++;
	     else if ( delta.y + cursor.y < FullLines-1 )	delta.y++;
	     key=0; break;
	 case kbLeft:
	     if ( cursor.x > 0 )		cursor.x--;
	     key=0; break;
	 case kbRight:
	     if ( cursor.x < Columns-1 )	cursor.x++;
	     key=0; break;
	 case kbPgUp:
	     if ( delta.y > 0 )	delta.y = max( 0, delta.y-Lines );
	     else		cursor.y = delta.x = cursor.x = 0;
	     key=0; break;
	 case kbPgDn:
	     if ( delta.y + Lines <= FullLines-1 ) delta.y += Lines;
	     key=0; break;
	 case kbHome:
	     delta.x = cursor.x = 0;
	     key=0; break;
	 case kbEnd:
	     cursor.x = size.x - 3;
	     key=0; break;
	 case HASH_cmBlockLine:
	     markBlock( BLOCK_LINE );
	     key=0; break;
	 case HASH_cmBlockColumn:
	     markBlock( BLOCK_COL );
	     key=0; break;
	 case HASH_cmUnmarkBlock:
	     unmarkBlock();
	     key=0; break;
	 case HASH_cmCopyToClip:
	     copyBlockToClip();
	     key=0; break;
       }
      if ( !key && draw() )
	 Screen::sync();
      return 0;
    }

  switch( key )
   {
     case kbUp:
	 writePty( "\033[A"); // ku
	 return 0;
     case kbDown:
	 writePty( "\033[B"); // kd
	 return 0;
     case kbRight:
	 if ( hardMode )
	    writePty( "\033[C"); // kr
	 return 0;
     case kbLeft:
	 if ( hardMode )
	    writePty( "\033[D"); // kl
	 return 0;
     case kbHome:
	 writePty( "\033[H"); // kh
	 return 0;
     case kbEnd:
	 writePty( "\033[F"); // kH
	 return 0;
     case kbPgUp:
	 writePty( "\033[I"); // kP
	 return 0;
     case kbPgDn:
	 writePty( "\033[G"); // kN
	 return 0;
     case kbIns:
	 writePty( "\033[L"); // kI
	 return 0;
     case kbDel:
	 writePty( "\177"); // kD
	 return 0;
     case kbF1:
	 writePty( "\033[M"); // f1
	 return 0;
     case kbF2:
	 writePty( "\033[N"); // f2
	 return 0;
     case kbF3:
	 writePty( "\033[O"); // f3
	 return 0;
     case kbF4:
	 writePty( "\033[P"); // f4
	 return 0;
     case kbF5:
	 writePty( "\033[Q"); // f5
	 return 0;
     case kbF6:
	 writePty( "\033[R"); // f6
	 return 0;
     case kbF7:
	 writePty( "\033[S"); // f7
	 return 0;
     case kbF8:
	 writePty( "\033[T"); // f8
	 return 0;
     case kbF9:
	 writePty( "\033[U"); // f9
	 return 0;
     case kbF10:
	 writePty( "\033[V"); // f0
	 return 0;
     default:
	 if ( key<256 ) {
	     unsigned char ch = translate_in( key );
	     write(pty, &ch, 1);
	 }
       return 0;
   }
}

long YTerm::handleKey( long key, void *&ptr )
{
  switch( key )
   {
     case 0:
	 return 0;
     case HASH_cmKill:
     case HASH_cmClose:
     case HASH_cmCloseWin:
     case HASH_cmCancel:
	 if ( input ) {
	     input->close();
	     closeTerm();
	 }
	 return key;
   }
  if ( !input || input->stat==TermInputTask::Fail )
      return HASH_cmCloseWin;

  switch( key ) {
      case FUNC2(kbCtrlT):
	  writePty("setenv TERMCAP \"");
	  writePty(termEntry(Lines, Columns));
	  writePty("\"\nsetenv LINES ");
	  writePty(termNum(Lines));
	  writePty("\nsetenv COLUMNS ");
	  writePty(termNum(Columns));
	  writePty("\n");
	  return 0;
      case FUNC12(kbCtrlT):
	  writePty("TERMCAP=\"");
	  writePty(termEntry(Lines, Columns));
	  writePty("\"\nLINES=");
	  writePty(termNum(Lines));
	  writePty("\nCOLUMNS=");
	  writePty(termNum(Columns));
	  writePty("\n");
	  return 0;
  }

  long translatedKey = keyHolder.translate( key );

  switch( translatedKey ) {
     case HASH_cmModeKey:
	 firstHandle = !firstHandle;
	 translatedKey=0; break;
     case HASH_cmMode:
	 hardMode = !hardMode;
	 if ( hardMode ) {
	     unmarkBlock();
	     init();
	 } else {
	     delta.y = FirstLine;
	     cursor.y = cury - delta.y;
	     cursor.x = curx;
	 }
	 translatedKey=0; break;
     case HASH_cmTransAltKoi8:
	 trans_flag = TRANS_alt2koi8;
	 translatedKey=0; break;
     case HASH_cmTransKoi8Alt:
	 trans_flag = TRANS_koi82alt;
	 translatedKey=0; break;
     case HASH_cmTransWinKoi8:
	 trans_flag = TRANS_win2koi8;
	 translatedKey=0; break;
     case HASH_cmTransKoi8Win:
	 trans_flag = TRANS_koi82win;
	 translatedKey=0; break;
     case HASH_cmTransMainKoi8:
	 trans_flag = TRANS_main2koi8;
	 translatedKey=0; break;
     case HASH_cmTransKoi8Main:
	 trans_flag = TRANS_koi82main;
	 translatedKey=0; break;
     case HASH_cmTransNone:
	 trans_flag = TRANS_none;
	 translatedKey=0; break;
  }

  if ( translatedKey == 0 && key != translatedKey ) {
      if ( draw() )
	  Screen::sync();
      return 0;
  }

  if ( !ISKEY( key ) )
      return key;

  if ( hardMode ) {
      if ( translatedKey == HASH_cmCopyFromClip ) {
	  copyBlockFromClip();
	  if ( draw() )
	      Screen::sync();
	  return 0;
      }
      if ( ISFUNC1(key) )
	  return key;
      if ( ISFUNC12(key) )
	  writePty("\012\013");
      else if ( ISFUNC2(key) )
	  writePty("\013");
  }

  procKey( key, translatedKey );

  return 0;
}

// YTerm common }[ YTerm terminal

void YTerm::move( int y, int x )
{
   if ( y<0 ) y=0;
   if ( y>=Lines ) y=Lines-1;
   if ( y<0 ) y=0;
   if ( x<0 ) x=0;
   if ( x>=Columns ) x=Columns-1;

   cury = FirstLine + y;
   curx = x;
}

void YTerm::moveUp( int count )
{
  cury -= count;
  if ( cury < FirstLine )
    {
/*      if ( !hardMode )
	{
	  int i = FirstLine - cury;
	  delta.y = FirstLine = max( 0, FirstLine-i );
	}*/
      cury = FirstLine;
    }
}

void YTerm::moveDown( int count )
{
  cury += count;
  if ( cury >= FullLines )
     cury = FullLines-1;
/*
  if ( cury - Lines >= FirstLine )
     delta.y = FirstLine = cury - Lines + 1;
*/
}

void YTerm::moveLeft(int count)
{
  curx-=count;
  if (curx<0)
    curx=0;
}

void YTerm::moveRight(int count)
{
  curx+=count;
  if (curx>=Columns)
    curx=Columns-1;
}

void YTerm::Cursor( int &y, int &x )
{
  y = cury-FirstLine;
  x = curx;
}

void YTerm::setAttr( int attr )
{
  if (mode==NORMAL)
    curattr=attr;
  else
    soattr=attr;
}

int YTerm::getAttr()
{
  int ret;
  if (mode==NORMAL)
    ret=curattr;
  else
    ret=soattr;
  if (isBold)
    ret |= 0x8;
  return ret;
}

void YTerm::put( const char *str )
{
  while( *str )
    put( *str++);
}

void YTerm::puts( const char *str )
{
  put( str );
  put( "\r\n" );
}


void YTerm::put( int sym )
{
  ScrElement *s;
  switch( sym )
   {
      case '\r':  curx=0;
		  break;
      case '\n':
		  cury++;
		  break;
      case '\b':
		  curx--;
		  if (curx<0)
		      curx=0;
		  break;
      case '\t':
		  curx= ((curx/8)+1)*8;
		  break;
      case 7:
		  Screen::beep();
		  break;
		  break;
      default:
		  s = & (vscreen[cury][curx++]);
		  s->a = getAttr();
		  if (sym<32 || sym==0x9b)
		    s->c = ' ';
		  else
		    s->c = sym;
		  break;
   }

  if (curx>=Columns)
   { curx=0; cury++; }

  while (cury>=FullLines)
    delLine( -FirstLine );
}

void YTerm::insLine( int n )
{
	if (n < -FirstLine)
	  n=-FirstLine;
	if (n>=Lines)
	  n=Lines-1;
	n+=FirstLine;
	int fins=0;
	if (n==FirstLine)
	   { n=0; fins=1; }
	ScrElement *end, *temp = vscreen[FullLines-1];
	for (int y=FullLines-1; y>n; --y) {
		vscreen[y] = vscreen[y-1];
		lnum [y] = lnum [y-1];
	}
	lnum [n] = -1;
	vscreen [n] = temp;
	bufEl.a= getAttr();// clearAttr;
	bufEl.c=' ';
	for ( end = temp+Columns; temp<end; temp++ )
	  *temp=bufEl;
	if (fins)
	 {
	   for ( temp=vscreen[FirstLine], end = temp+Columns; temp<end; temp++ )
	    *temp=bufEl;
	   n=FirstLine;
	 }

	if (cury>n)
	  cury++;
}

void YTerm::delLine( int n )
{
	if (n < -FirstLine)
	  n=-FirstLine;
	if (n>=Lines)
	  n=Lines-1;
	n+=FirstLine;
	if (n==FirstLine)
	   n=0;
	ScrElement *temp = vscreen[n];
	for (int y=n; y < FullLines-1; y++) {
		vscreen[y] = vscreen[y+1];
		lnum [y] = lnum [y+1];
	}
	vscreen[FullLines-1] = temp;
	lnum [FullLines-1] = -1;
	bufEl.a= getAttr();//clearAttr;
	bufEl.c=' ';
	for (ScrElement *end = temp+Columns; temp<end; temp++ )
	  *temp=bufEl;

	if (cury > n)
	  cury--;
}



void YTerm::scroolTerm ()
{
	int line, n, topline, botline;

	for (line=0; line<FullLines; ++line)
	{
		// find next range to scrool

		// skip fresh lines
		while (line < FullLines && lnum [line] < 0)
			++line;

		// last line reached - no range to scrool
		if (line >= FullLines)
			break;

		// top line found
		topline = line;

		// skip range of old lines
		while (line < FullLines-1 && lnum [line] + 1 == lnum [line+1])
			++line;

		// bottom line found
		botline = line;

		// compute number of scrools, >0 - forward
		n = topline - lnum [topline];

		if (n == 0)
			continue;
		else if (n > 0)
			topline = lnum [topline];
		else if (n < 0)
			botline = lnum [botline];

		// do scrool
		Rect scrollRect = calcScrollRegion(topline, botline);
		Screen::scroll( scrollRect, n, getAttr()<<8 );
	}
}


Rect YTerm::calcScrollRegion( int topline, int botline )
{
  static Rect r;
  r=rect;
  r.grow(-1,-1);

  if (topline>delta.y+size.y-2)
    { r.b.y=r.a.y; return r; }

  if (topline>delta.y)
    r.a.y+=topline-delta.y;

  if (botline<=topline)
    { r.b.y=r.a.y; return r; }

  if (botline<delta.y+size.y-2)
    r.b.y-=delta.y+size.y-2-botline;

  return r;
}



void YTerm::clearEOL()
{
  bufEl.a=getAttr();
  bufEl.c=' ';
  if (cury>=FullLines)
    cury=FullLines-1;
  for(int i=curx; i<Columns; i++ )
    vscreen[cury][i] = bufEl;
}

void YTerm::clearBEOL()
{
  bufEl.a=getAttr();
  bufEl.c=' ';
  if (cury>=FullLines)
    cury=FullLines-1;
  for(int i=0; i<curx; i++ )
    vscreen[cury][i] = bufEl;
}

void YTerm::clearSCR()
{
  int i,j;
  bufEl.a=getAttr();
  bufEl.c=' ';
  if (cury>=FullLines)
    cury=FullLines-1;
  for( i=curx; i<Columns; i++ )
    vscreen[cury][i] = bufEl;
  for( j=cury+1; j<FullLines; j++ )
   {
    lnum[j]=-1;
    for( i=0; i<Columns; i++ )
      vscreen[j][i] = bufEl;
   }
}

void YTerm::clearBSCR()
{
  int i,j;
  bufEl.a=getAttr();
  bufEl.c=' ';
  if (cury>=FullLines)
    cury=FullLines-1;
  for( j=0; j<cury; j++ )
   {
    lnum[j]=-1;
    for( i=0; i<Columns; i++ )
      vscreen[j][i] = bufEl;
   }
  for( i=0; i<curx; i++ )
    vscreen[cury][i] = bufEl;
}

void YTerm::setFG(int c)
{
	curattr = (curattr & 0xf0) | c;
}

void YTerm::setBG(int c)
{
	curattr = (curattr & 0xf) | (c << 4);
}

void YTerm::setFG_SO(int c)
{
	soattr = (soattr & 0xf0) | c;
}

void YTerm::setBG_SO(int c)
{
	soattr = (soattr & 0xf) | (c << 4);
}

void YTerm::makeMenu( Menu *m )
{
   static Menu *menu = 0;
   if ( menu )
     { winMenu = menu; return; }

   menuColl *global = new menuColl;

   menuColl *coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Cursor","~Курсор"),		new Lang("Hard/soft cursor mode","Жесткий/мягкий режим перемещения курсора"), 0, HASH_cmMode ) );
   coll->insert( new menuItem( new Lang("~Keyboard","К~лавиатура"),	new Lang("Change key getting mode (first/last)","Порядок получения клавиши (первый/последний)"), 0, HASH_cmModeKey ) );

   Menu *sub = new Menu( coll, Point(1,0) );
   global->insert( new menuItem( new Lang("~Mode","Режим"), 0, 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Line block marker","Маркер с~трочный"),	new Lang("Set line block marker",	"Поставить маркер строчного блока"), 0, HASH_cmBlockLine ) );
   coll->insert( new menuItem( new Lang("~Column block marker","Маркер ~колоночный"),	new Lang("Set column block marker",	"Поставить маркер колоночного блока"), 0, HASH_cmBlockColumn ) );
   coll->insert( new menuItem( new Lang("~Unmark block","~Снять маркер"),		new Lang("Unmark block",		"Снять маркер блока"), 0, HASH_cmUnmarkBlock ) );
   coll->insert( new menuItem( new Lang("Copy ~to Clipboard","~Копировать в буфер"),	new Lang("Copy block to Clipboard",	"Копировать блок в буфер"), 0, HASH_cmCopyToClip ) );
   coll->insert( new menuItem( new Lang("Copy ~from Clipboard","Копировать ~из буфера"),new Lang("Copy block from Clipboard",	"Копировать блок из буфера"), 0, HASH_cmCopyFromClip ) );

   sub = new Menu( coll, Point(1,6), Point(1,7) );
   global->insert( new menuItem( new Lang("~Block","Блок"), 0, 0, sub ) );

   coll	= new menuColl;
   coll->insert( new menuItem( new Lang("~1) original"),	0, 0, HASH_cmTransNone ) );
   coll->insert( new menuItem( new Lang("~2) 866  -> koi8"),	0, 0, HASH_cmTransAltKoi8 ) );
   coll->insert( new menuItem( new Lang("~3) 1251 -> koi8"),	0, 0, HASH_cmTransWinKoi8 ) );
   coll->insert( new menuItem( new Lang("~4) main -> koi8"),	0, 0, HASH_cmTransMainKoi8 ) );
   coll->insert( new menuItem( new Lang("~5) koi8 -> 866"),	0, 0, HASH_cmTransKoi8Alt ) );
   coll->insert( new menuItem( new Lang("~6) koi8 -> 1251"),	0, 0, HASH_cmTransKoi8Win ) );
   coll->insert( new menuItem( new Lang("~7) koi8 -> main"),	0, 0, HASH_cmTransKoi8Main ) );
   sub = new Menu( coll, Point(1,13), Point(1,13) );
   global->insert( new menuItem( new Lang("~View","Кодировка"),	0, 0,	sub )	);

   winMenu = menu = new Menu( global, Point(0,0), new Lang("Term","Терм"), 0, 1, 0 );
   winMenu->fill( &keyHolder );
   sharedMenu.insert( winMenu );
}

// YTerm terminal ]{ TermInputTask

TermInputTask::TermInputTask(int Pty, pid_t Pid, YTerm *Yterm):
	  Task("YTERM", 4096*3 ), pty(Pty), pid(Pid),
	  yterm(Yterm)
{
 stat = Normal;
 in = (pty == -1) ? 0 : new FileBuf(pty,0);
 str = new char [_YTERM_STRLEN_+1];
 str[_YTERM_STRLEN_]=0;
 pos=0;

 for(int i=0; i<_YTERM_NACC_; i++)
  acc[i]=0;
 accp=acc;
 ca = 0;
 so = 0;
}

void TermInputTask::initData(int Pty, pid_t Pid, YTerm *Yterm)
{
  pty=Pty;
  pid=Pid;
  yterm=Yterm;
  in = new FileBuf(pty,0);
}

void TermInputTask::destroy()
{
   if (in)
      { delete in; in = 0;}
   if (str)
      { delete str; str=0; }
   if (pty>=0)
      { ::close(pty); pty=-1; }
   if (this->pid!=-1)
    {
      int status;
      waitpid( this->pid, &status, WNOHANG );
      this->pid=-1;
    }
    //::kill(this->pid, SIGKILL);
}

int TermInputTask::close()
{
  if (yterm)
  {
    yterm=0;
    in->badFd();
    WakeUp();
  }
  return 1;
}

long TermInputTask::main()
{
  int le;
  char *buf;

  // terminal "input" driver
  while( in )
   {
      if ( !yterm || stat==Fail )
       {
	 in->badFd();
	 yterm=0;
	 break;
       }
      le=in->fetch(buf);
      if ( !yterm || stat==Fail )
       {
	 in->badFd();
	 yterm=0;
	 break;
       }
      if ( le>0 )
       {
	 for( ; le ; le-- )
	   sput( *buf++);
	 yterm->draw();
       }
//      else if (le==0)
//         continue;
      else
       {
	 stat=Fail;
	 in->badFd();
	 if (yterm)
	  {
	    yterm->closeTerm();
	    Task::sendMessage( yterm->Executor, new KeyMessage( HASH_cmClose ) );
	  }
	 yterm = 0;
	 break;
       }
   }
  return 0;
}


#define	BOLD	0x08		/* high intensity, foreground */
#define	BLINK	0x80		/* blink (or high intensity backgrnd) */
#define	BGHIGH	0x80		/* blink (or high intensity backgrnd) */

#define	cx	acc[0]		/* the first escape seq argument */
#define	cy	acc[1]		/* the second escape seq argument */

static char sco_to_cga[] = {
	0,	/* black */
	4,	/* red */
	2,	/* green */
	6,	/* brown */
	1,	/* blue */
	5,	/* magenta */
	3,	/* cyan */
	7,	/* grey */
};




void TermInputTask::sput(u_char c)
{
	int i, count, row, col;

	switch (stat) {
	case Normal:
		switch (c) {

		case 0x0:		/* Ignore pad characters */
		case 0xff:		/* Ignore 0xff */
			break;

		case 0x1B:
			stat = ESC;
			break;

		default:
			str[pos++]=c;
			if (c=='\n')
			 {
			   str[--pos]=0;
			   if (pos>0 && str[pos-1]=='\r')
			     str[--pos]=0;
			   yterm->processStr(str, pos);
			   pos=0;
			 }
			else
			 {
			   if (pos>=_YTERM_STRLEN_)
			     pos=0;
			 }
			yterm->put(c);
			break;
		}
		break;

	case EBRAC:
		if ((count = cx) < 1)
			count = 1;

		/*
		 * In this stat, the action at the end of the switch
		 * on the character type is to go to Normal stat,
		 * and intermediate states do a return rather than break.
		 */
		switch (c) {

		case 'm': {
		    int *mode;

		    for (mode = acc; mode <= accp; mode++)
			switch (*mode) {
			case 0:	/* normal */
				ca = 0;
				so = 0;
				set_allcolors( sco_to_cga[7], 1);
				set_allcolors( sco_to_cga[0], 0);
				yterm->setMode(YTerm::NORMAL);
				yterm->setBold(0);
				break;
			case 1:	/* bold */
				ca |= BOLD;
				yterm->setBold(1);
				break;
			case 5:	/*blink*/
				ca |= BLINK;
				break;
			case 7:	/* inverse */
				so = 1;
				yterm->setMode(YTerm::STANDOUT);
				break;
			case 8:	/* invisible */
				yterm->setAttr( (yterm->getAttr() & 0xf0) | (yterm->getAttr() >>4) );
				break;

			default:
				/* set current (norm/so) fg, other bg */
				if ((i = *mode) >= 30 && i <= 37) {
					set_allcolors( sco_to_cga[i - 30], 1);
					break;
				}

				/* set current (norm/so) bg, other fg */
				if (i >= 40 && i <= 47) {
					set_allcolors( sco_to_cga[i - 40], 0);
					break;
				}
				break;
			}
		    }
//no_setcolor:
		    break;


		case 'A': /* back col(s) */
			yterm->moveUp(count);
			break;

		case 'B': /* down col(s) */
			yterm->moveDown(count);
			break;

		case 'C': /* right cursor */
			yterm->moveRight(count);
			break;

		case 'D': /* left cursor */
			yterm->moveLeft(count);
			break;

		case 'J': /* Clear to end of display */
			switch (cx) {
			case 0:
				yterm->clearBSCR();
				break;
			case 1:
				yterm->clearSCR();
				break;
			case 2:
				yterm->move(0,0);
				yterm->clearSCR();
				break;
			}
			break;

		case 'K': /* Clear to EOL */
			switch (cx) {
			case 0:
				yterm->clearEOL();
				break;
			case 1:
				yterm->clearBEOL();
				break;
			case 2:
				int y,x;
				yterm->Cursor(y,x);
				x=0;
				yterm->move(y,x);
				yterm->clearEOL();
				break;
				}
			break;

		case 'f':
		case 'H': /* Cursor move */
			if (cx > yterm->Lines)
			   cx =  yterm->Lines;
			if (cy > yterm->Columns)
			   cy = yterm->Columns;
			if (cx == 0)
			   cx = 1;
			if (cy == 0)
			   cy = 1;

			yterm->move(cx-1, cy-1);
			break;

		case 'L':	/* Insert row */

			if ( count<=0 ) break;
			yterm->Cursor(row,col);
			for(; count>0; count--)
			  yterm->insLine(row);
			break;


		case 'M':	/* Delete row */

			if ( count<=0 ) break;
			yterm->Cursor(row,col);
			for(; count>0; count--)
			  yterm->delLine(row);

			break;

		case ';': /* Switch params in cursor def */
			if (accp < & acc[_YTERM_NACC_-1])
			  { accp++; return; }
			break;	/* cancel escape */

		case '=': /* ESC[= color change */
			stat = EBRACEQ;
			return;

		default: /* Only numbers valid here */
			if ((c >= '0') && (c <= '9')) {
				*(accp) *= 10;
				*(accp) += c - '0';
				return;
			} else
				break;
		}
		stat = Normal;
		break;

	case EBRACEQ: {
		//u_char *colp;

		/*
		 * In this stat, the action at the end of the switch
		 * on the character type is to go to Normal stat,
		 * and intermediate states do a return rather than break.
		 */

		/*
		 * Set foreground/background color
		 * for normal mode, standout mode
		 */
		switch (c) {
		case 'D':
			/**ATTR_ADDR(curcolor) &= ~BGHIGH;*/
			ca &= ~BGHIGH;
			break;
		case 'E':
			/*enable_bg_intens();*/
			break;
		case 'F':
			yterm->setFG  ( cx & 0xf );
			break;
		case 'G':
			yterm->setBG  ( cx & 0xf );
			break;
		case 'H':
			yterm->setFG_SO(cx & 0xf );
			break;
		case 'I':
			yterm->setBG_SO(cx & 0xf );
			break;
		case 'S':
			savecolor = yterm->getAttr();
			break;
		case 'R':
			yterm->setAttr(savecolor);
			break;
		case 's': /* save cursor position */
			yterm->Cursor(saverow, savecol);
			break;
		case 'u': /* restore cursor position */
			yterm->move(saverow, savecol);
			break;
		default: /* Only numbers valid here */
			if ((c >= '0') && (c <= '9')) {
				cx *= 10;
				cx += c - '0';
				return;
			} else
				break;
		}
		stat = Normal;
	    }
	    break;

	case ESC:
		switch (c) {
		case 'c':	/* Clear screen & home */
			yterm->move(0,0);
			ca=0;
			so=0;
			yterm->clearSCR();
			stat = Normal;
			break;
		case '7': /* save cursor position */
			yterm->Cursor(saverow, savecol);
			stat = Normal;
			break;
		case '8': /* restore cursor position */
			yterm->move(saverow, savecol);
			stat = Normal;
			break;
		case '[':	/* Start ESC [ sequence */
			stat = EBRAC;
			memset (acc, 0, sizeof(acc));
			accp = acc;
			stat = EBRAC;
			break;
		default: /* Invalid, clear stat */
			stat = Normal;
			break;
		}
		break;
	case Fail:
	case EBRACEQU:
		break;
	}

}

/*
 * Set normal/standout/graphic foreground or background color.
 * Standout color changes in the opposite way; if currently in standout,
 * reverse the change fg vs. bg.
 */
void TermInputTask::set_allcolors( int clr, int fg )
{
   if (so)
    {
      if (fg)
	yterm->setBG_SO(clr);
      else
	yterm->setFG_SO(clr);
    }
   else
    {
      if (fg)
	yterm->setFG(clr);
      else
	yterm->setBG(clr);
   }
}


// }
