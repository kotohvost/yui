/*
	$Id: termcap.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "termcap.h"
#include "keycodes.h"

const int BUFSIZE       = 2048;	 // max length of termcap entry
const int MAXHOP	= 32;	   // max number of tc= indirections
const int NKEY		= 100;

static char SHARETERMCAP []     = "/usr/share/misc/termcap";
static char TERMCAP []	  = "/etc/termcap";
static char MYTERMCAP []	= "/usr/lib/deco/termcap";
static char MYLCLTERMCAP [256];

static char *tname;		     // terminal name
static char *tcapfile;		  // termcap file name
static char *tbuf;		      // terminal entry buffer
static char *envtermcap;		// global variable TERMCAP
static hopcount;			// detect infinite loops

static char *tskip (char *);
static char *tdecode (char *, char **);
static int tgetent (char *, char *, char *);
static int tnchktc ();
static int tnamatch (char *);

static int initCap(char *bp);
static void initKey (struct Keytab *map);
static void getCap (struct Captab *t);


Keytab Termcap::keytab[] = {
	{ "kl",	 0,	      meta ('l'),    },
	{ "kr",	 0,	      meta ('r'),    },
	{ "ku",	 0,	      meta ('u'),    },
	{ "kd",	 0,	      meta ('d'),    },
	{ "kN",	 0,	      meta ('n'),   },
	{ "kP",	 0,	      meta ('p'),   },
	{ "kh",	 0,	      meta ('h'),   },
	{ "kH",	 0,	      meta ('e'),   },
	{ "kI",	 0,	      meta ('i'),   },
	{ "kb",	 0,	      cntrl ('H'),  },
	{ "kD",	 0,	      meta ('x'),   },

	{ "k1",	 0,	      meta ('A'), },
	{ "k2",	 0,	      meta ('B'), },
	{ "k3",	 0,	      meta ('C'), },
	{ "k4",	 0,	      meta ('D'), },
	{ "k5",	 0,	      meta ('E'), },
	{ "k6",	 0,	      meta ('F'), },
	{ "k7",	 0,	      meta ('G'), },
	{ "k8",	 0,	      meta ('H'), },
	{ "k9",	 0,	      meta ('I'), },
	{ "k0",	 0,	      meta ('J'), },

	{ "f1",	 0,	      meta ('A'), },
	{ "f2",	 0,	      meta ('B'), },
	{ "f3",	 0,	      meta ('C'), },
	{ "f4",	 0,	      meta ('D'), },
	{ "f5",	 0,	      meta ('E'), },
	{ "f6",	 0,	      meta ('F'), },
	{ "f7",	 0,	      meta ('G'), },
	{ "f8",	 0,	      meta ('H'), },
	{ "f9",	 0,	      meta ('I'), },
	{ "f0",	 0,	      meta ('J'), },

	{ "ME",	 0,	      meta ('S'), }, // begin of mouse event

	{ 0,	    "\0331",	meta ('A'), },
	{ 0,	    "\0332",	meta ('B'), },
	{ 0,	    "\0333",	meta ('C'), },
	{ 0,	    "\0334",	meta ('D'), },
	{ 0,	    "\0335",	meta ('E'), },
	{ 0,	    "\0336",	meta ('F'), },
	{ 0,	    "\0337",	meta ('G'), },
	{ 0,	    "\0338",	meta ('H'), },
	{ 0,	    "\0339",	meta ('I'), },
	{ 0,	    "\0330",	meta ('J'), },
	{ 0,	    "\033l",	meta ('l'),   },
	{ 0,	    "\033r",	meta ('r'),   },
	{ 0,	    "\033u",	meta ('u'),   },
	{ 0,	    "\033d",	meta ('d'),   },
	{ 0,	    "\033n",	meta ('n'),   },
	{ 0,	    "\033p",	meta ('p'),   },
	{ 0,	    "\033h",	meta ('h'),   },
	{ 0,	    "\033e",	meta ('e'),   },
	{ 0,	    "\033i",	meta ('i'),   },

	{ 0,	    "\033\033",     27,	      },
	{ 0,	    0,	      0,	    },
};

static Captab outtab [] = {
	{ "ms", CAPFLG, 0, &Termcap::MS, 0, 0, },
	{ "C2", CAPFLG, 0, &Termcap::C2, 0, 0, },
//#ifdef CYRILLIC
	{ "CY", CAPFLG, 0, &Termcap::Cy, 0, 0, },
//#endif
	{ "li", CAPNUM, 0, 0, &Termcap::Lines,   0, },
	{ "co", CAPNUM, 0, 0, &Termcap::Columns, 0, },
	{ "Nf", CAPNUM, 0, 0, &Termcap::NF, 0, },
	{ "Nb", CAPNUM, 0, 0, &Termcap::NB, 0, },
	{ "cl", CAPSTR, 0, 0, 0, &Termcap::CL, },
	{ "ce", CAPSTR, 0, 0, 0, &Termcap::CE, },
	{ "cm", CAPSTR, 0, 0, 0, &Termcap::CM, },
	{ "se", CAPSTR, 0, 0, 0, &Termcap::SE, },
	{ "so", CAPSTR, 0, 0, 0, &Termcap::SO, },
	{ "Cf", CAPSTR, 0, 0, 0, &Termcap::CF, },
	{ "Cb", CAPSTR, 0, 0, 0, &Termcap::CB, },
	{ "Mf", CAPSTR, 0, 0, 0, &Termcap::MF, },
	{ "Mb", CAPSTR, 0, 0, 0, &Termcap::MB, },
	{ "op", CAPSTR, 0, 0, 0, &Termcap::OP, },

	{ "MH", CAPSTR, 0, 0, 0, &Termcap::MsH, }, // mouse on
	{ "ML", CAPSTR, 0, 0, 0, &Termcap::MsL, }, // mouse off

	{ "md", CAPSTR, 0, 0, 0, &Termcap::MD, },
	{ "mh", CAPSTR, 0, 0, 0, &Termcap::MH, },
	{ "mr", CAPSTR, 0, 0, 0, &Termcap::MR, },
	{ "me", CAPSTR, 0, 0, 0, &Termcap::ME, },
	{ "te", CAPSTR, 0, 0, 0, &Termcap::TE, },
	{ "ti", CAPSTR, 0, 0, 0, &Termcap::TI, },
	{ "vi", CAPSTR, 0, 0, 0, &Termcap::VI, },
	{ "vs", CAPSTR, 0, 0, 0, &Termcap::VS, },
	{ "ve", CAPSTR, 0, 0, 0, &Termcap::VE, },
#ifndef NOKEYPAD
	{ "ks", CAPSTR, 0, 0, 0, &Termcap::KS, },
	{ "ke", CAPSTR, 0, 0, 0, &Termcap::KE, },
#endif
	{ "al", CAPSTR, 0, 0, 0, &Termcap::AL, },
	{ "dl", CAPSTR, 0, 0, 0, &Termcap::DL, },
	{ "fs", CAPSTR, 0, 0, 0, &Termcap::FS, },
	{ "as", CAPSTR, 0, 0, 0, &Termcap::AS, },
	{ "ae", CAPSTR, 0, 0, 0, &Termcap::AE, },
	{ "ac", CAPSTR, 0, 0, 0, &Termcap::AC, },
	{ "gs", CAPSTR, 0, 0, 0, &Termcap::GS, },
	{ "ge", CAPSTR, 0, 0, 0, &Termcap::GE, },
	{ "g1", CAPSTR, 0, 0, 0, &Termcap::G1, },
	{ "g2", CAPSTR, 0, 0, 0, &Termcap::G2, },
	{ "gt", CAPSTR, 0, 0, 0, &Termcap::GT, },
	{ "cs", CAPSTR, 0, 0, 0, &Termcap::CS, },
	{ "sf", CAPSTR, 0, 0, 0, &Termcap::SF, },
	{ "sr", CAPSTR, 0, 0, 0, &Termcap::SR, },
	{ "po", CAPSTR, 0, 0, 0, &Termcap::PO, },
	{ "pf", CAPSTR, 0, 0, 0, &Termcap::PF, },
	{ "pO", CAPSTR, 0, 0, 0, &Termcap::POO, },
//#ifdef CYRILLIC
	{ "Cs", CAPSTR, 0, 0, 0, &Termcap::Cs, },
	{ "Ce", CAPSTR, 0, 0, 0, &Termcap::Ce, },
	{ "Ct", CAPSTR, 0, 0, 0, &Termcap::Ct, },
//#endif
	{ { 0, 0, }, 0, 0, 0, 0, 0, },
};

char *Termcap::AS=0, *Termcap::AE=0, *Termcap::AC=0, *Termcap::GS=0,
     *Termcap::GE=0, *Termcap::G1=0, *Termcap::G2=0, *Termcap::GT=0;
char *Termcap::CS=0, *Termcap::SF=0, *Termcap::SR=0;
char *Termcap::PO=0, *Termcap::PF=0, *Termcap::POO=0;
char *Termcap::CL=0, *Termcap::CE=0, *Termcap::CM=0, *Termcap::SE=0, *Termcap::SO=0,
     *Termcap::TE=0, *Termcap::TI=0, *Termcap::VI=0, *Termcap::VE=0, *Termcap::VS=0;
char *Termcap::AL=0, *Termcap::DL=0, *Termcap::FS=0, *Termcap::MD=0, *Termcap::MH=0,
     *Termcap::ME=0, *Termcap::MR=0;
char *Termcap::CF=0, *Termcap::CB=0, *Termcap::MF=0, *Termcap::MB=0, *Termcap::OP=0;

char *Termcap::MsH=0, *Termcap::MsL=0;

char *Termcap::KS=0, *Termcap::KE=0;

char Termcap::Cy=0;
char *Termcap::Cs=0, *Termcap::Ce=0, *Termcap::Ct=0;
char *Termcap::term=0;

short Termcap::ctab [16], Termcap::btab [16];
int Termcap::scrool=0, Termcap::rscrool=0;

char Termcap::MS=0, Termcap::C2=0;
int  Termcap::NF=0, Termcap::NB=0, Termcap::Lines=0, Termcap::Columns=0;
u_short Termcap::linedraw [11];
int Termcap::Visuals;

int Termcap::VisualMask=0xffffffff;


int Termcap::hasDim ()
	{ return (Visuals & VisualMask & (VisualColors | VisualDim)); }
int Termcap::hasBold ()
	{ return (Visuals & VisualMask & (VisualColors | VisualBold)); }
int Termcap::hasInverse ()
	{ return (Visuals & VisualMask & (VisualColors | VisualInverse)); }
int Termcap::hasColors ()
	{ return (Visuals & VisualMask & VisualColors); }
int Termcap::hasGraph ()
	{ return (Visuals & VisualMask & VisualGraph); }

unsigned char Termcap::cyrInputTable [256] ={
	  0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
	  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,


	  ' ', '!', 252, '/', 44,  ':', '.', 218,
	  '?', '%', ';', '+', 194, '-', 192, '/',
	  '0', '1', '2', '3', '4', '5', '6', '7',
	  '8', '9', 246, 214, 226, '=', 224, '?',
	  '"', 230, 233, 243, 247, 245, 225, 240,
	  242, 251, 239, 236, 228, 248, 244, 253,
	  250, 234, 235, 249, 229, 231, 237, 227,
	  254, 238, 241, 200, '\\', 255, ',', '_',
	  '`', 198, 201, 211, 215, 213, 193, 208,
	  242-32, 251-32, 239-32, 236-32, 228-32, 248-32, 244-32, 253-32,
	  250-32, 234-32, 235-32, 249-32, 229-32, 231-32, 237-32, 227-32,
	  222, 206, 209, 232, '|', 223, '~', 127,

	  0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	  0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	  0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	  0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	  0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	  0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	  0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	  0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,

	  };

unsigned char Termcap::inputTable  [256];
unsigned char Termcap::outputTable [256];


int Termcap::init=0;

static struct termios init_termios;

static char *skipDelay( char *cp )
{
	while (*cp>='0' && *cp<='9')
		++cp;
	if (*cp == '.') {
		++cp;
		while (*cp>='0' && *cp<='9')
			++cp;
	}
	if (*cp == '*')
		++cp;
	return (cp);
}

extern "C" void restore_tty(void)
{
  // восстановление состояния терминального интерфейса
  char *cm = 0;
  tcsetattr(0, TCSAFLUSH, &init_termios);
  // восстановление оригинальных цветовых пар терминала
  if ( Termcap::OP )
     fprintf( stdout, "%s", Termcap::OP );
  // очистка зкрана
  if ( Termcap::CL )
     fprintf( stdout, "%s", Termcap::CL );
  // позиционирование курсора в первую позицию последней строки
  cm = Termcap::Goto( Termcap::CM, 0, Termcap::Lines-1 );
  if ( cm && (cm = skipDelay( cm )) )
     fprintf( stdout, "%s", cm );

  fflush( stdout );
}

void errexit(const char *msg)
{
      fprintf(stderr, "%s", msg);
      fprintf(stderr, " (terminal '%s')\n", tname);
      fflush(stderr);
      exit(3);
}

void Termcap::setColorMap(char * fg, char *bg)
{
  if (NF > 16)
	  NF = 16;
  if (NB > 16)
	  NB = 16;
  if (! MF)
	  MF = "0123456789ABCDEF";
  if (! MB)
	  MB = "0123456789ABCDEF";
  int i, fglen=0;
  for ( i=0; i<16; ++i)
	  ctab [i] = btab [i] = -1;

  if (fg) fglen=strlen(fg);
  int bglen=0;
  if (bg) bglen=strlen(bg);

  for (i=0; i<16 && i<NF; ++i)
       {
	  if (! MF [i])
		  break;
	  else if (MF[i]>='0' && MF[i]<='9')
		  ctab [MF [i] - '0'] = i;
	  else if (MF[i]>='A' && MF[i]<='F')
		  ctab [MF [i] - 'A' + 10] = i;
       }
  short buf[16];
  memcpy(buf,ctab, sizeof(buf));
  for (i=0; i<16 && i<NF && i<fglen; ++i)
       {
	    if (fg[i]>='0' && fg[i]<='9')
	      ctab [i] = buf[fg [i] - '0'];
	    else if (fg[i]>='A' && fg[i]<='F')
	      ctab [i] = buf[fg [i] - 'A' + 10];
       }
  for (i=0; i<16 && i<NB; ++i)
       {
	  if (! MB [i])
		  break;
	  else if (MB[i]>='0' && MB[i]<='9')
		  btab [MB [i] - '0'] = i;
	  else if (MF[i]>='A' && MF[i]<='F')
		  btab [MB [i] - 'A' + 10] = i;
      }
  memcpy(buf,btab, sizeof(buf));
  for (i=0; i<16 && i<NB && i<bglen; ++i)
      {
	    if (bg[i]>='0' && bg[i]<='9')
	      btab [i] = buf[bg [i] - '0'];
	    else if (bg[i]>='A' && bg[i]<='F')
	      btab [i] = buf[bg [i] - 'A' + 10];
      }
  for (i=1; i<8; ++i)
      {
	  if (ctab[i] >= 0 && ctab[i+8] < 0)
		  ctab [i+8] = ctab [i];
	  if (ctab[i+8] >= 0 && ctab[i] < 0)
		  ctab [i] = ctab [i+8];
	  if (btab[i] >= 0 && btab[i+8] < 0)
		  btab [i+8] = btab [i];
	  if (btab[i+8] >= 0 && btab[i] < 0)
		  btab [i] = btab [i+8];
     }
}


int Termcap::initTermcap()
{
  if (init)
    return 0;

  char *buf=new char[4096];
  if ( !initCap(buf) )
      errexit("cannot read termcap entry");

  initKey(Termcap::keytab);

  getCap(outtab);

  char *p = getenv ("LINS");
  if (p && *p)
	Lines = atoi (p);
  p = getenv ("LINES");
  if (p && *p)
	Lines = atoi (p);
  p = getenv ("COLUMNS");
  if (p && *p)
	Columns = atoi (p);
  struct winsize ws;
  if (ioctl( 0, TIOCGWINSZ, &ws) != -1)
   {
     if (ws.ws_row>0)
       Lines = ws.ws_row;
     if (ws.ws_col>0)
       Columns = ws.ws_col;
   }

  if (Lines <= 6 || Columns <= 30)
	errexit("too small screen");
  if (! CM)
	errexit("too dumb terminal (no cursor move capabilitie)");
  scrool = AL && DL;
  if (! (rscrool = SF && SR))
	SF = SR = 0;

  if (ME) {
	if (SO)
		SO = skipDelay (SO);
	else if (MR)
		SO = skipDelay (MR);
  }

  Visuals = 0;
  if (NF>0 && NB>0 && CF && (CB || C2))
	Visuals |= VisualColors;
  if (MH)
	Visuals |= VisualDim;
  if (MD)
	Visuals |= VisualBold;
  if (SO)
	Visuals |= VisualInverse;
  if (G1 || G2 || GT || AC)
	Visuals |= VisualGraph;

  if (hasColors ()) {
	  setColorMap("0123456789ABCDEF", "0123456789ABCDEF");
  } else
	CE = 0;	 /* Don't use clear to end of line.*/

  int i, fd;
  for (i=0; i<256 ; ++i)
	  inputTable [i] = i;
  for (i=0; i<256; ++i)
	  outputTable [i] = i;
  outputTable [127]=0xdf;

  int translateGraphMode=0;

  if (! Cs || ! Ce)
	Cs = Ce = 0;
  if ( Ct && (fd=open (Ct, 0)) >=0 )
   {
     read (fd, outputTable+192, 64);  /* koi8 alpha's out translation*/
     read (fd, cyrInputTable+32, 96+128); // 96-RED CYRILLIC 128 - NORMAL CYRILLIC TRANSLATION
     memcpy(inputTable+128, cyrInputTable+128, 128);
     if ( read (fd, outputTable+128, 64)==64)  //koi8 pseudograph out translation
       translateGraphMode=1;

     close (fd);
   }
  else
      Cs = Ce = 0;

  if (hasGraph ())
    {
	char *g = 0;
	if (G1)
		g = G1;
	else if (G2)
		g = G2;
	else if (GT)
	  {
		GT [1] = GT [0];
		g = GT+1;
	  }
       if (translateGraphMode)
	  { //use RFC standart:
	    //:g1=\200\201\204\211\205\206\212\207\202\210\203:
	    //:g2=\240\241\253\273\256\261\276\265\245\270\250:

		 linedraw[0]=0200;
		 linedraw[1]=0201;
		 linedraw[2]=0204;
		 linedraw[3]=0211;
		 linedraw[4]=0205;
		 linedraw[5]=0206;
		 linedraw[6]=0212;
		 linedraw[7]=0207;
		 linedraw[8]=0202;
		 linedraw[9]=0210;
		 linedraw[10]=0203;
	   }
	else if (g)
		for (i=0; i<11 && *g; ++i, ++g)
			linedraw [i] = *g;
	else if (AC)
	  {
		GS = AS;
		GE = AE;
		for (; AC[0] && AC[1]; AC+=2)
			switch (AC[0]) {
			case 'l': linedraw [8] = AC[1]; break;
			case 'q': linedraw [0] = AC[1]; break;
			case 'k': linedraw [10] = AC[1]; break;
			case 'x': linedraw [1] = AC[1]; break;
			case 'j': linedraw [4] = AC[1]; break;
			case 'm': linedraw [2] = AC[1]; break;
			case 'w': linedraw [9] = AC[1]; break;
			case 'u': linedraw [7] = AC[1]; break;
			case 'v': linedraw [3] = AC[1]; break;
			case 't': linedraw [5] = AC[1]; break;
			case 'n': linedraw [6] = AC[1]; break;
			}
	  }
    }
  else
    {
      linedraw[0]='-';    // 0    horisontal line
      linedraw[1]='|';    // 1    vertical line
      linedraw[2]='+';    // 2    lower left corner
      linedraw[3]='-';    // 3    lower center
      linedraw[4]='+';    // 4    lower right corner
      linedraw[5]='|';    // 5    left center
      linedraw[6]='+';    // 6    center cross
      linedraw[7]='|';    // 7    right center
      linedraw[8]='+';    // 8    upper left corner
      linedraw[9]='-';    // 9    upper center
     linedraw[10]='+';    // 10   upper right corner
    }

  delete buf;

  for(i=0;i<11;i++)
    linedraw[i]&=0xff;
  // инициализация терминального интерфейса

  Termcap::SetTty();

  atexit(restore_tty);

  init = 1;
  return 1;
}

void Termcap::SetTty()
{
  struct termios tm;
  tcgetattr( 0, &tm);
  init_termios=tm;

  tm.c_iflag &= ~(INLCR | ICRNL | IGNCR | ISTRIP );
  //tm.c_oflag &= ~(OLCUC | OCRNL);
  tm.c_lflag &= ~(ECHO | ECHOE | ICANON );
  tm.c_cc [VMIN] = 0; //1;
  tm.c_cc [VTIME] = 0; //1;
  tm.c_cc [VINTR] = 0; //'Q'-'@'; // ^C
  tm.c_cc [VQUIT] = 0;
  /* start/stop symbols (^S/^Q)
  tm.c_cc [VSTART] = 0;
  tm.c_cc [VSTOP] = 0;
  */
  tm.c_cc [VSUSP ] = 0;

#if !defined(LINUX_LIBC5) && !defined(LINUX_LIBC6) && !defined(SCO_SYSV)
  tm.c_cc [VDSUSP] = 0;
#if !defined(SOLARIS_26_X86) && !defined(SOLARIS_25) && !defined(AIX_PPC) && !defined(SINIX)
  tm.c_cc [VSTATUS] = 0;
#endif
#endif

#ifndef SCO_SYSV
  tm.c_cc [VLNEXT] = 0;
#if !defined(AIX_PPC)
  tm.c_cc [VDISCARD] = 0;
  tm.c_cc [VWERASE  ] = 0;
#endif
  tm.c_cc [VREPRINT ] = 0;
#endif
  tm.c_cc [VKILL    ] = 0;

  tcsetattr(0, TCSANOW , &tm);

  if ( Termcap::TI )
	write( 1, Termcap::TI, strlen(Termcap::TI) );
#ifndef NOKEYPAD
  if ( Termcap::KS )
	write( 1, Termcap::KS, strlen(Termcap::KS) );
#endif

}

void Termcap::ResetTty()
{
  if ( Termcap::TE )
	write( 1, Termcap::TE, strlen(Termcap::TE) );
#ifndef NOKEYPAD
  if ( Termcap::KE )
	write( 1, Termcap::KE, strlen(Termcap::KE) );
#endif
  restore_tty();
}

static int initCap(char *bp)
{
	if ( ! (tname = getenv ("TERM")))
		tname = "unknown";
	Termcap::term=tname;
	char *tcap_addfile = getenv( "TERMCAP_ADDFILE" );
	if ( tcap_addfile ) {
		strncpy( MYLCLTERMCAP, tcap_addfile, 255 );
		MYLCLTERMCAP[255]=0; }
	else
		strcpy( MYLCLTERMCAP, "/usr/local/lib/deco/termcap" );
	if (! envtermcap)
		envtermcap = getenv ("TERMCAP");
	if (envtermcap && *envtermcap=='/') {
		tcapfile = envtermcap;
		envtermcap = 0;
	}
	if (! envtermcap)
		envtermcap = "";
	if (! tcapfile || access (tcapfile, 0) < 0)
		tcapfile = SHARETERMCAP;
	if (access (tcapfile, 0) < 0)
		tcapfile = TERMCAP;
	return (tgetent (bp, tname, MYLCLTERMCAP) ||
	    tgetent (bp, tname, MYTERMCAP) ||
	    tgetent (bp, tname, tcapfile));
}

static int tgetent (char *bp, char *name, char *termcap)
{
	int c;
	int tf = -1;
	char *cp = envtermcap;
	tbuf = bp;

	// TERMCAP can have one of two things in it. It can be the
	// name of a file to use instead of /etc/termcap. In this
	// case it must start with a "/". Or it can be an entry to
	// use so we don't have to read the file. In this case it
	// has to already have the newlines crunched out.
	if (cp && *cp) {
		envtermcap = "";
		tbuf = cp;
		c = tnamatch (name);
		tbuf = bp;
		if (c) {
			strcpy (bp, cp);
			return (tnchktc ());
		}
	}
	if (tf < 0)
		tf = open (termcap, 0);
	if (tf < 0)
		return (0);
	char *ibuf = new char [BUFSIZE];
	int i = 0;
	int cnt = 0;
	for (;;) {
		cp = bp;
		for (;;) {
			if (i == cnt) {
				cnt = read (tf, ibuf, BUFSIZE);
				if (cnt <= 0) {
					close (tf);
					delete ibuf;
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > bp && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+BUFSIZE) {
				errexit ("Termcap entry too long");
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		// The real work for the match.
		if (tnamatch (name)) {
			close (tf);
			delete ibuf;
			return (tnchktc ());
		}
	}
}

static int tnchktc ()
{
	register char *p, *q;
	char tcname [16];       // name of similar terminal
	char *tcbuf;
	char *holdtbuf = tbuf;
	int l;

	p = tbuf + strlen(tbuf) - 2;    // before the last colon
	while (*--p != ':')
		if (p<tbuf) {
			errexit("Bad termcap entry");
			return (0);
		}
	p++;
	// p now points to beginning of last field
	if (p[0] != 't' || p[1] != 'c')
		return (1);
	strcpy (tcname,p+3);
	q = tcname;
	while (*q && *q != ':')
		q++;
	*q = 0;
	if (++hopcount > MAXHOP) {
		errexit ("Infinite tc= loop");
		return (0);
	}
	tcbuf = new char [BUFSIZE];
	if (! tgetent (tcbuf, tcname, tcapfile)) {
		hopcount = 0;	   // unwind recursion
		delete tcbuf;
		return (0);
	}
	for (q=tcbuf; *q != ':'; q++);
	l = p - holdtbuf + strlen(q);
	if (l > BUFSIZE) {
		errexit ("Termcap entry too long\n");
		q [BUFSIZE - (p-tbuf)] = 0;
	}
	strcpy (p, q+1);
	tbuf = holdtbuf;
	hopcount = 0;		   // unwind recursion
	delete tcbuf;
	return (1);
}

static int tnamatch (char *np)
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return (0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			continue;
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
			return (1);
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
			return (0);
		Bp++;
	}
}

static char *tskip (char *bp)
{
	while (*bp && *bp != ':')
		bp++;
	if (*bp == ':')
		bp++;
	return (bp);
}

static void getCap (struct Captab *t)
{
	register char *bp;
	register struct Captab *p;
	register i, base;
	char name [2];
	char *area, *begarea;

	if (! tbuf)
		return;
	bp = tbuf;
	area = begarea = new char [BUFSIZE];
	for (;;) {
		bp = tskip(bp);
		if (! bp[0] || ! bp[1])
			break;
		if (bp[0] == ':' || bp[1] == ':')
			continue;
		name[0] = *bp++;
		name[1] = *bp++;
		for (p=t; p->tname[0]; ++p)
			if (p->tname[0] == name[0] && p->tname[1] == name[1])
				break;
		if (! p->tname[0] || p->tdef)
			continue;
		p->tdef = 1;
		if (*bp == '@')
			continue;
		switch (p->ttype) {
		case CAPNUM:
			if (*bp != '#')
				continue;
			bp++;
			base = 10;
			if (*bp == '0')
				base = 8;
			i = 0;
			while (*bp>='0' && *bp<='9')
				i = i * base, i += *bp++ - '0';
			*(p->ti) = i;
			break;
		case CAPFLG:
			if (*bp && *bp != ':')
				continue;
			*(p->tc) = 1;
			break;
		case CAPSTR:
			if (*bp != '=')
				continue;
			bp++;
			*(p->ts) = tdecode (bp, &area);
			break;
		}
	}
	bp = new char [area - begarea];
	memcpy (bp, begarea, area - begarea);
	for (p=t; p->tname[0]; ++p)
		if (p->ttype == CAPSTR && *(p->ts))
			*(p->ts) += bp - begarea;
	delete begarea;
#ifdef DEBUG
	for (p=t; p->tname[0]; ++p) {
		printf ("%c%c", p->tname[0], p->tname[1]);
		switch (p->ttype) {
		case CAPNUM:
			printf ("#%d\n", *(p->ti));
			break;
		case CAPFLG:
			printf (" %s\n", *(p->tc) ? "on" : "off");
			break;
		case CAPSTR:
			if (*(p->ts))
				printf ("='%s'\n", *(p->ts));
			else
				printf ("=NULL\n");
			break;
		}
	}
#endif
}

static char *tdecode (char *str, char **area)
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (c>='0' && c<='9') {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && *str>='0' && *str<='9');
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}


static int compkeys (struct Keytab *a, struct Keytab *b)
{
	int cmp;

	if (! a->str) {
		if (! b->str)
			return (0);
		return (1);
	}
	if (! b->str)
		return (-1);
	cmp = strcmp (a->str, b->str);
	if (cmp)
		return (cmp);
	if (! a->tcap) {
		if (! b->tcap)
			return (0);
		return (1);
	}
	if (! b->tcap)
		return (-1);
	return (0);
}

static void initKey (struct Keytab *map)
{
	struct Captab tab [NKEY];
	struct Keytab *kp;
	struct Captab *t;

	for (t=tab, kp=map; kp->val && t<tab+NKEY-1; ++kp, ++t) {
		if (! kp->tcap)
			continue;
		t->tname[0] = kp->tcap[0];
		t->tname[1] = kp->tcap[1];
		t->ttype = CAPSTR;
		t->tdef = 0;
		t->tc = 0;
		t->ti = 0;
		t->ts = &kp->str;
	}
	kp->val = 0;
	t->tname[0] = 0;
	getCap (tab);
	qsort ((char *) map, (unsigned) (kp - map), sizeof (map[0]), (int(*)(const void*,const void*))compkeys);

}
