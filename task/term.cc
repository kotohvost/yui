// $Id: term.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

extern "C" {
#include <tcaps.h>
}

#include "term.h"

char *Term::tname=0;
int Term::rows=0, Term::cols=0;
short Term::ctab[16], Term::btab[16];
unsigned short Term::linedraw[11];

#if 0
#ifdef TIOCSLTC
static struct ltchars oldchars, newchars;
#endif
#endif

#if 0

unsigned char Term::inputTable[256];
char Term::cyrInputTable['~' - ' ' + 1];
char Term::cyrOutputTable[64];

#else

unsigned char Term::cyrInputTable[256] = {
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

#endif

char *Term::BS, *Term::BR, *Term::DS, *Term::DR, *Term::NS, *Term::NR;
char *Term::GS, *Term::GE, *Term::G1, *Term::G2, *Term::GT, *Term::AC,
     *Term::AS, *Term::AE;
char *Term::CS, *Term::SF, *Term::SR, *Term::EA;
char *Term::KS, *Term::KE;
char *Term::CL, *Term::CE, *Term::CM, *Term::SE, *Term::SO, *Term::TE, *Term::TI,
     *Term::VE, *Term::VS, *Term::AL, *Term::DL, *Term::IS, *Term::IF,
     *Term::FS, *Term::MD, *Term::MH, *Term::ME, *Term::MR, *Term::CF,
     *Term::CB, *Term::AF, *Term::AB, *Term::Sf, *Term::Sb, *Term::OP,
     *Term::MF=NULL, *Term::MB=NULL;

int Term::NF, Term::NB;
char Term::MS, Term::C2;
char Term::Cy;
char *Term::Cs, *Term::Ce, *Term::Ct;

int Term::Visuals=0;
int Term::VisualMask=0xffffffff;
int Term::scrool=0, Term::rscrool=0;

static char *tinfo_path[] = {
	"/usr/lib/terminfo",
	"/usr/share/terminfo",
	"/usr/share/lib/terminfo",
	"/usr/lib/share/terminfo"
};

static char *tcaps_path[] = {
	"/usr/local/lib/deco/termcap",
	"/usr/lib/deco/termcap",
	"/usr/share/lib/termcap",
	"/etc/termcap"
};

/* повышенная яркость (md) */
#define VisualBold	1
/* пониженная яркость (mh) */
#define VisualDim	2
/* инверсия (so или mr) */
#define VisualInverse	4
/* цвет (NF, NB, CF, CB, C2, MF, MB) */
#define VisualColors	8
/* псевдографика (G1, G2, GT) */
#define VisualGraph	16

/* max length of 'goto' string */
#define	MAXRETURNSIZE	64

#define ERR(c)          (c)
#define TPARMERR(c)     { strcpy (outp, (c)); return; }

static Terminfo tinfo;

#ifndef CNTRL
	#define CNTRL(c)    (c & 037)
#endif
#ifndef META
	#define META(c)     (c | 0400)
#endif

CapMap Term::captab[] = {
	/* bool */
	{ "ms", CAPFLG, &Term::MS, 0, 0 },
	{ "C2", CAPFLG, &Term::C2, 0, 0  },
	{ "CY", CAPFLG, &Term::Cy, 0, 0  },
	/* num */
	{ "li", CAPNUM, 0, &Term::rows, 0 },
	{ "co", CAPNUM, 0, &Term::cols, 0 },
	{ "Nf", CAPNUM, 0, &Term::NF, 0 },
	{ "Nb", CAPNUM, 0, &Term::NB, 0 },
	/* str */
	{ "cl", CAPSTR, 0, 0, &Term::CL },
	{ "ce", CAPSTR, 0, 0, &Term::CE },
	{ "cm", CAPSTR, 0, 0, &Term::CM },
	{ "se", CAPSTR, 0, 0, &Term::SE },
	{ "so", CAPSTR, 0, 0, &Term::SO },
	{ "Cf", CAPSTR, 0, 0, &Term::CF },
	{ "Cb", CAPSTR, 0, 0, &Term::CB },
	{ "AF", CAPSTR, 0, 0, &Term::AF },
	{ "AB", CAPSTR, 0, 0, &Term::AB },
	{ "Sf", CAPSTR, 0, 0, &Term::Sf },
	{ "Sb", CAPSTR, 0, 0, &Term::Sb },
	{ "op", CAPSTR, 0, 0, &Term::OP },
	{ "md", CAPSTR, 0, 0, &Term::MD },
	{ "mh", CAPSTR, 0, 0, &Term::MH },
	{ "mr", CAPSTR, 0, 0, &Term::MR },
	{ "me", CAPSTR, 0, 0, &Term::ME },
	{ "te", CAPSTR, 0, 0, &Term::TE },
	{ "ti", CAPSTR, 0, 0, &Term::TI },
	{ "vs", CAPSTR, 0, 0, &Term::VS },
	{ "ve", CAPSTR, 0, 0, &Term::VE },
	{ "ks", CAPSTR, 0, 0, &Term::KS },
	{ "ke", CAPSTR, 0, 0, &Term::KE },
	{ "al", CAPSTR, 0, 0, &Term::AL },
	{ "dl", CAPSTR, 0, 0, &Term::DL },
	{ "is", CAPSTR, 0, 0, &Term::IS },
	{ "if", CAPSTR, 0, 0, &Term::IF },
	{ "fs", CAPSTR, 0, 0, &Term::FS },
	{ "eA", CAPSTR, 0, 0, &Term::EA },
	{ "gs", CAPSTR, 0, 0, &Term::GS },
	{ "ge", CAPSTR, 0, 0, &Term::GE },
	{ "as", CAPSTR, 0, 0, &Term::AS },
	{ "ae", CAPSTR, 0, 0, &Term::AE },
	{ "g1", CAPSTR, 0, 0, &Term::G1 },
	{ "g2", CAPSTR, 0, 0, &Term::G2 },
	{ "gt", CAPSTR, 0, 0, &Term::GT },
	{ "ac", CAPSTR, 0, 0, &Term::AC },
	{ "cs", CAPSTR, 0, 0, &Term::CS },
	{ "sf", CAPSTR, 0, 0, &Term::SF },
	{ "sr", CAPSTR, 0, 0, &Term::SR },
	{ "Cs", CAPSTR, 0, 0, &Term::Cs },
	{ "Ce", CAPSTR, 0, 0, &Term::Ce },
	{ "Ct", CAPSTR, 0, 0, &Term::Ct },
	{ 0, 0 }
};

KeyMap Term::keytab[] = {
	{ "kl",		0,		META ('l')	},
	{ "kr",		0,		META ('r')	},
	{ "ku",		0,		META ('u')	},
	{ "kd",		0,		META ('d')	},
	{ "kN",		0,		META ('n')	},
	{ "kP",		0,		META ('p')	},
	{ "kh",		0,		META ('h')	},
	{ "kH",		0,		META ('e')	},
	{ "@7",		0,		META ('e')	},
	{ "kI",		0,		META ('i')	},
	{ "kb",		0,		CNTRL ('H'),	},
	{ "k.",		0,		CNTRL ('G')	},
	{ "kD",		0,		META ('x')	},
	{ "kE",		0,		META ('h')	},
	{ "kS",		0,		META ('e')	},

	{ "k1",		0,		META ('A')	},
	{ "k2",		0,		META ('B')	},
	{ "k3",		0,		META ('C')	},
	{ "k4",		0,		META ('D')	},
	{ "k5",		0,		META ('E')	},
	{ "k6",		0,		META ('F')	},
	{ "k7",		0,		META ('G')	},
	{ "k8",		0,		META ('H')	},
	{ "k9",		0,		META ('I')	},
	{ "k0",		0,		META ('J')	},
	{ "k;",		0,		META ('J')	},

	{ "f1",		0,		META ('A')	},
	{ "f2",		0,		META ('B')	},
	{ "f3",		0,		META ('C')	},
	{ "f4",		0,		META ('D')	},
	{ "f5",		0,		META ('E')	},
	{ "f6",		0,		META ('F')	},
	{ "f7",		0,		META ('G')	},
	{ "f8",		0,		META ('H')	},
	{ "f9",		0,		META ('I')	},
	{ "f0",		0,		META ('J')	},
	{ "F1",		0,		META ('K')	},
	{ "F2",		0,		META ('L')	},

	{ 0,		"\0331",	META ('A')	},
	{ 0,		"\0332",	META ('B')	},
	{ 0,		"\0333",	META ('C')	},
	{ 0,		"\0334",	META ('D')	},
	{ 0,		"\0335",	META ('E')	},
	{ 0,		"\0336",	META ('F')	},
	{ 0,		"\0337",	META ('G')	},
	{ 0,		"\0338",	META ('H')	},
	{ 0,		"\0339",	META ('I')	},
	{ 0,		"\0330",	META ('J')	},
	{ 0,		"\033-",	META ('K')	},
	{ 0,		"\033=",	META ('L')	},

	{ 0,		"\033l",	META ('l')	},
	{ 0,		"\033r",	META ('r')	},
	{ 0,		"\033u",	META ('u')	},
	{ 0,		"\033d",	META ('d')	},
	{ 0,		"\033n",	META ('n')	},
	{ 0,		"\033p",	META ('p')	},
	{ 0,		"\033h",	META ('h')	},
	{ 0,		"\033e",	META ('e')	},

	{ 0,		"\033\033",	27		},
	{ 0,		0,		0		}
};

void errexit(const char *msg)
{
	fprintf( stderr, "%s", msg );
	fprintf( stderr, " (terminal '%s')\n", Term::tname );
	fflush( stderr );
	exit( 3 );
}

static char *skipdelay( char *cp )
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

static char *branchto( register char *cp, int to )
{
	register int c, level;

	level = 0;
	while ((c = *cp++)) {
		if (c == '%') {
			if ((c = *cp++) == to || c == ';') {
				if (level == 0) {
					return cp;
				}
			}
			switch( c ) {
				case '?': level++; break;
				case ';': level--;
			}
		}
	}
	return ERR ("no matching ENDIF");
}

/*
 * Routine to perform parameter substitution.
 * instring is a string containing printf type escapes.
 * The whole thing uses a stack, much like an HP 35.
 * The following escapes are defined for substituting row/column:
 *
 *	%d	print pop() as in printf
 *      %[0]2d  print pop() like %2d
 *      %[0]3d  print pop() like %3d
 *	%c	print pop() like %c
 *
 *	%p[1-0]	push ith parm
 *	%P[a-z] set variable
 *	%g[a-z] get variable
 *	%'c'	char constant c
 *	%{nn}	integer constant nn
 *
 *	%+ %- %* %/ %m		arithmetic (%m is mod): push(pop() op pop())
 *	%& %| %^		bit operations:		push(pop() op pop())
 *	%= %> %<		logical operations:	push(pop() op pop())
 *	%! %~			unary operations	push(op pop())
 *	%b			unary BCD conversion
 *	%d			unary Delta Data conversion
 *
 *	%? expr %t thenpart %e elsepart %;
 *				if-then-else, %e elsepart is optional.
 *				else-if's are possible ala Algol 68:
 *				%? c1 %t %e c2 %t %e c3 %t %e c4 %t %e %;
 *
 * all other characters are ``self-inserting''.  %% gets % output.
 */

void Term::sysvgoto( register char *outp, register char *cp, int p1, int p2 )
{
	register int c, op;
	int vars [26], stack [10], top, sign;

#define PUSH(i)        (stack [++top] = (i))
#define POP()          (stack [top--])

	if (! cp)
		TPARMERR ("null arg");
	top = 0;
	while ((c = *cp++)) {
		if (c != '%') {
			*outp++ = c;
			continue;
		}
		op = stack [top];
		if (*cp == '0')
			++cp;
		switch (c = *cp++) {
		case 'd':               /* PRINTING CASES */
			if (op < 10)
				goto one;
			if (op < 100)
				goto two;
			/* fall into... */
		case '3':
three:
			if (c == '3' && *cp++ != 'd')
				TPARMERR ("bad char after %3");
			*outp++ = (op / 100) | '0';
			op %= 100;
			/* fall into... */
		case '2':
			if (op >= 100)
				goto three;
			if (c == '2' && *cp++ != 'd')
				TPARMERR ("bad char after %2");
two:
			*outp++ = op / 10 | '0';
one:
			*outp++ = op % 10 | '0';
			(void) POP ();
			continue;
		case 'c':
			*outp++ = op;
			(void) POP ();
			break;
		case '%':
			*outp++ = c;
			break;
		/*
		 * %i: shorthand for increment first two parms.
		 * Useful for terminals that start numbering from
		 * one instead of zero (like ANSI terminals).
		 */
		case 'i':
			p1++; p2++;
			break;
		case 'p':       /* %pi: push the ith parameter */
			switch (c = *cp++) {
			case '1': PUSH (p1); break;
			case '2': PUSH (p2); break;
			default: TPARMERR ("bad parm number");
			}
			break;
		case 'P':       /* %Pi: pop from stack into variable i (a-z) */
			vars [*cp++ - 'a'] = POP ();
			break;
		case 'g':       /* %gi: push variable i (a-z) */
			PUSH (vars [*cp++ - 'a']);
			break;
		case '\'':      /* %'c' : character constant */
			PUSH (*cp++);
			if (*cp++ != '\'')
				TPARMERR ("missing closing quote");
			break;
		case '{':       /* %{nn} : integer constant.  */
			op = 0; sign = 1;
			if (*cp == '-') {
				sign = -1;
				cp++;
			} else if (*cp == '+')
				cp++;
			while ((c = *cp++) >= '0' && c <= '9') {
				op = 10*op + c - '0';
			}
			if (c != '}')
				TPARMERR ("missing closing brace");
			PUSH (sign * op);
			break;
		/* binary operators */
		case '+': c = POP (); op = POP (); PUSH (op + c); break;
		case '-': c = POP (); op = POP (); PUSH (op - c); break;
		case '*': c = POP (); op = POP (); PUSH (op * c); break;
		case '/': c = POP (); op = POP (); PUSH (op / c); break;
		case 'm': c = POP (); op = POP (); PUSH (op % c); break;
		case '&': c = POP (); op = POP (); PUSH (op & c); break;
		case '|': c = POP (); op = POP (); PUSH (op | c); break;
		case '^': c = POP (); op = POP (); PUSH (op ^ c); break;
		case '=': c = POP (); op = POP (); PUSH (op = c); break;
		case '>': c = POP (); op = POP (); PUSH (op > c); break;
		case '<': c = POP (); op = POP (); PUSH (op < c); break;
		/* Unary operators. */
		case '!': stack [top] = ! stack [top]; break;
		case '~': stack [top] = ~ stack [top]; break;
		/* Sorry, no unary minus, because minus is binary. */
		/*
		 * If-then-else.  Implemented by a low level hack of
		 * skipping forward until the match is found, counting
		 * nested if-then-elses.
		 */
		case '?':	/* IF - just a marker */
			break;
		case 't':	/* THEN - branch if false */
			if (! POP ())
				cp = branchto (cp, 'e');
			break;
		case 'e':	/* ELSE - branch to ENDIF */
			cp = branchto (cp, ';');
			break;
		case ';':	/* ENDIF - just a marker */
			break;
		default:
			TPARMERR ("bad % sequence");
		}
	}
	*outp = 0;
}

/*
 * Routine to perform cursor addressing.
 * CM is a string containing printf type escapes to allow
 * cursor addressing.  We start out ready to print the destination
 * line, and switch each time we print row or column.
 * The following escapes are defined for substituting row/column:
 *
 *	%d	as in printf
 *	%2	like %2d
 *	%3	like %3d
 *	%.	gives %c hacking special case characters
 *	%+x	like %c but adding x first
 *
 *	The codes below affect the state but don't use up a value.
 *
 *	%>xy	if value > x add y
 *	%r	reverses row/column
 *	%i	increments row/column (for one origin indexing)
 *	%%	gives %
 *	%B	BCD (2 decimal digits encoded in one byte)
 *	%D	Delta Data (backwards bcd)
 *
 * all other characters are ``self-inserting''.
 */

char *Term::Goto( char *CM, int destcol, int destline )
{
	register char *cp, *dp;
	register int c, which, oncol;
	static char result [MAXRETURNSIZE];

#ifdef DEBUG
	printf ("CM='%s'\n", CM);
#endif
	cp = CM;
	if (! cp)
		return "";
	dp = result;
	oncol = 0;
	which = destline;
	while ((c = *cp++)) {
		if (c != '%') {
			*dp++ = c;
			continue;
		}
		switch (c = *cp++) {
		case 'n':
			destcol ^= 0140;
			destline ^= 0140;
			goto setwhich;
		case 'd':
			if (which < 10)
				goto one;
			if (which < 100)
				goto two;
			/* fall into... */
		case '3':
			*dp++ = (which / 100) | '0';
			which %= 100;
			/* fall into... */
		case '2':
two:                    *dp++ = which / 10 | '0';
one:                    *dp++ = which % 10 | '0';
swap:                   oncol = 1 - oncol;
setwhich:               which = oncol ? destcol : destline;
			continue;
		case '>':
			if (which > *cp++)
				which += *cp++;
			else
				cp++;
			continue;
		case '+':
			which += *cp++;
			/* fall into... */
		case '.':
			*dp++ = which;
			goto swap;
		case 'r':
			oncol = 1;
			goto setwhich;
		case 'i':
			destcol++;
			destline++;
			which++;
			continue;
		case '%':
			*dp++ = c;
			continue;
		case '/':
			c = *cp;
			*dp++ = which / c | '0';
			which %= *cp++;
			continue;
		case 'B':
			which = (which/10 << 4) + which%10;
			continue;
		case 'D':
			which = which - 2 * (which%16);
			continue;
		case 'p':
		case 'P':
		case '\'':
		case '{':
		case '?':
			/* string is in tparm format... */
			sysvgoto (dp, cp-2, destline, destcol);
			return (result);
		default:
			return "bad capgoto";
		}
	}
	*dp = 0;
	return (result);
}

static struct termios orig_termios;

void restore_tty()
{
	char *cm = 0;

	/*
	 * восстановление состояния терминального интерфейса
	 */
	orig_termios.c_iflag |= ICRNL;
	orig_termios.c_oflag |= OPOST | ONLCR;
	orig_termios.c_lflag |= ISIG | ICANON | ECHO;
	tcsetattr( 0, TCSAFLUSH, &orig_termios );

	/*
	 * восстановление оригинальных цветовых пар терминала
	 */
	if ( Term::OP )
	    fprintf( stdout, "%s", Term::OP );

	/*
	 * очистка зкрана
	 */
	if ( Term::CL )
	    fprintf( stdout, "%s", Term::CL );

	/*
	 * позиционирование курсора в первую позицию последней строки
	 */
	cm = Term::Goto( Term::CM, 0, Term::rows-1 );
	if ( cm && (cm = skipdelay( cm )) )
	    fprintf( stdout, "%s", cm );

	fflush( stdout );
}

/*
 *      Compare keys. Used in call to qsort.
 *      Return -1, 0, 1 iff a is less, equal or greater than b.
 *      First, check if there is ->str.
 *      Then compare lengths, then strings.
 *      If equal, check ->tcap field.
 */
static int compkeys (const void *arg1, const void *arg2)
{
	const KeyMap *a = (KeyMap*)arg1;
	const KeyMap *b = (KeyMap*)arg2;
	register int cmp;

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

static void cap_init( CapMap *map )
{
	if ( !map )
	    return;
	TermcapBucket *tb = NULL;
	char *s=NULL, flg=0;
	int num=-1;
	for( ; map->tcap; map++ ) {
	    if ( (tb = tgetentry(map->tcap)) == NULL )
		continue;
	    s = NULL;
	    flg = 0;
	    num = -1;
	    switch( tb->type ) {
		case 'B':
		    if ( map->type == CAPFLG )
			*map->val_bool = tinfo.bools[tb->no];
		    break;
		case 'N':
		    if ( map->type == CAPNUM )
			*map->val_int = tinfo.nums[tb->no];
		    break;
		case 'S':
		    if ( map->type == CAPSTR && tinfo.strings[tb->no] >= 0 )
			*map->val_ptr = tinfo.buf + tinfo.strings[tb->no];
		    break;
		case 'K':
		    if ( map->type == CAPSTR && tinfo.keys[tb->no] >= 0 )
			*map->val_ptr = tinfo.buf + tinfo.keys[tb->no];
		    break;
	    }
	}
}

static void key_init( KeyMap *map )
{
	if ( !map )
	    return;
	KeyMap *km=map, *kp=map;
	TermcapBucket *tb = NULL;
	for( ; kp->val; ++kp ) {
	    if ( kp->tcap == NULL || (tb = tgetentry(kp->tcap)) == NULL )
		continue;
	    if ( tb->type == 'K' && tinfo.keys[tb->no] >= 0 )
		kp->str = tinfo.buf + tinfo.keys[tb->no];
	}

	qsort( (char*)km, (unsigned)(kp - km), sizeof(km[0]), compkeys );
}

void Term::destroy()
{
	destroy_Terminfo( &tinfo );
}

int Term::init( int base )
{
	register char *p;
	int i;
	char *tn = getenv( "TERM" );
	static char errbuf[128];
	struct winsize ws;

	if ( tname ) {
	    ::free( tname );
	    tname = NULL;
	}
	if ( tn )
	    tname = (char*)strdup( tn );
	if ( tname == NULL )
	    errexit( "Unknown terminal type" );

	init_Terminfo( &tinfo );
	tinfo.name = tname;


	if ( base == BASE_TERMINFO ) {
	    if ( read_tinfo( 4, (char**)tinfo_path, 1, &tinfo, errbuf, sizeof(errbuf) ) < 0 )
		errexit( errbuf );
	} else if ( base == BASE_TERMCAP ) {
	    if ( read_tcap( 4, (char**)tcaps_path, 1, &tinfo, errbuf, sizeof(errbuf)) < 0 )
		errexit( errbuf );
	} else {
	    errexit( "cannot initialize terminal" );
	}

	cap_init( captab );

	if ( ioctl (0,TIOCGWINSZ, &ws ) >= 0 ) {
	    if ( ws.ws_row > 0 )
		rows = ws.ws_row;
	    if ( ws.ws_col > 0 )
		cols = ws.ws_col;
	}

	if ( rows <= 0 && (p=getenv("LINES")) && *p )
	    rows = strtol( p, 0, 0 );
	if ( cols <= 0 && (p=getenv("COLUMNS")) && *p )
	    cols = strtol( p, 0, 0 );

	if ( rows <= 3 || cols <= 19 )
	    errexit( "too small screen" );

	key_init( keytab );

	if ( !CM )
	    errexit( "too dumb terminal (no cursor move capabilitie)" );

	if ( CF && (C2 || CB) ) {
	    NF = 16;
	    NB = 8;
	} else if ( AF || Sf ) {
	    CF = AF;
	    if ( !CF )
		CF = Sf;
	    CB = AB;
	    if ( !CB )
		CB = Sb;
	    C2 = 0;
	    NF = 16;
	    NB = 8;
	}

	Visuals = 0;

	if ( MH )
	    Visuals |= VisualDim;

	if ( SO )
	    Visuals |= VisualInverse;

	scrool = AL && DL;
	if ( !(rscrool = SF && SR) )
		SF = SR = 0;
	if ( NF >= 4 ) {
	    initcolor( MF, MB );
	    Visuals |= VisualColors;
	} else if ( ME ) {
	    initbold ();
	    Visuals |= VisualBold;
	}
	if ( (G1 || G2 || GT || AC) ) {
	    initgraph ();
	    Visuals |= VisualGraph;
	}

#if 0
	for (i=' '; i<='~'; ++i)
	    cyrInputTable [i-' '] = i;
	for (i=0300; i<=0377; ++i)
	    cyrOutputTable [i-0300] = i;

	if ( !Cs || !Ce )
	    Cs = Ce = 0;
	if (Cy && Ct) {
	    int fd = open (Ct, 0);
	    if (fd >= 0) {
		read (fd, cyrOutputTable, sizeof (cyrOutputTable));
		read (fd, cyrInputTable, sizeof (cyrInputTable));
		close (fd);
	    }
	}
#endif

//	SetTty();
	atexit( restore_tty );

	return (1);
}

void Term::initbold()
{
	if (ME)
		NS = skipdelay (ME);
	if (MD)
		BS = skipdelay (MD);
	if (MH)
		DS = skipdelay (MH);
	if (SO)
		SO = skipdelay (SO);
	else if (MR)
		SO = skipdelay (MR);
}

void Term::initgraph()
{
	register int i;
	register char *g = 0;

	if (G1)
		g = G1;
	else if (G2)
		g = G2;
	else if (GT) {
		GT [1] = GT [0];
		g = GT+1;
	}
	if (g)
		for (i=0; i<11 && *g; ++i, ++g)
			linedraw [i] = *g;
	else if (AC) {
		GS = AS;
		GE = AE;
		for (; AC[0] && AC[1]; AC+=2)
			switch (AC[0]) {
			case 'l': linedraw [8]  = AC[1]; break;
			case 'q': linedraw [0]  = AC[1]; break;
			case 'k': linedraw [10] = AC[1]; break;
			case 'x': linedraw [1]  = AC[1]; break;
			case 'j': linedraw [4]  = AC[1]; break;
			case 'm': linedraw [2]  = AC[1]; break;
			case 'w': linedraw [9]  = AC[1]; break;
			case 'u': linedraw [7]  = AC[1]; break;
			case 'v': linedraw [3]  = AC[1]; break;
			case 't': linedraw [5]  = AC[1]; break;
			case 'n': linedraw [6]  = AC[1]; break;
			}
	}
}

void Term::initcolor( char *fg, char *bg )
{
  if (NF > 16)
	  NF = 16;
  if (NB > 16)
	  NB = 16;

  if ( fg && (!MF || fg != MF) ) {
      if ( MF )
	  ::free( MF );
      MF = strdup( fg );
  }
  if ( !MF )
      MF = strdup( "0123456789ABCDEF" );

  if ( bg && (!MB || bg != MB) ) {
      if ( MB )
	  ::free( MB );
      MB = strdup( bg );
  }
  if ( !MB )
      MB = strdup( "0123456789ABCDEF" );

  int i, fglen=0, bglen=0;
  for ( i=0; i<16; ++i )
       ctab [i] = btab [i] = -1;

  if (fg) fglen=strlen(fg);
  if (bg) bglen=strlen(bg);

  for( i=0; i<16 && i<NF; ++i ) {
      if (! MF [i])
	  break;
      else if ( MF[i]>='0' && MF[i]<='9' )
	  ctab [MF [i] - '0'] = i;
      else if ( MF[i]>='A' && MF[i]<='F' )
	  ctab [MF [i] - 'A' + 10] = i;
  }
/*
  short buf[16];
  memcpy( buf, ctab, sizeof(buf));
  for( i=0; i<16 && i<NF && i<fglen; ++i ) {
      if ( fg[i]>='0' && fg[i]<='9' )
	  ctab [i] = buf[fg [i] - '0'];
      else if (fg[i]>='A' && fg[i]<='F')
	  ctab [i] = buf[fg [i] - 'A' + 10];
  }
*/
  for( i=0; i<16 && i<NB; ++i ) {
     if (! MB [i])
	 break;
     else if (MB[i]>='0' && MB[i]<='9')
	 btab [MB [i] - '0'] = i;
     else if (MF[i]>='A' && MF[i]<='F')
	 btab [MB [i] - 'A' + 10] = i;
  }
/*
  memcpy( buf, btab, sizeof(buf) );
  for( i=0; i<16 && i<NB && i<bglen; ++i ) {
      if ( bg[i]>='0' && bg[i]<='9' )
	  btab [i] = buf[bg [i] - '0'];
      else if ( bg[i]>='A' && bg[i]<='F' )
	  btab [i] = buf[bg [i] - 'A' + 10];
  }
*/
  for( i=1; i<8; ++i ) {
      if ( ctab[i] >= 0 && ctab[i+8] < 0 )
	  ctab [i+8] = ctab [i];
      if ( ctab[i+8] >= 0 && ctab[i] < 0 )
	  ctab [i] = ctab [i+8];
      if ( btab[i] >= 0 && btab[i+8] < 0 )
	  btab [i+8] = btab [i];
      if ( btab[i+8] >= 0 && btab[i] < 0 )
	  btab [i] = btab [i+8];
  }
}

int Term::hasDim ()
{
	return (Visuals & VisualMask & (VisualColors | VisualDim));
}

int Term::hasBold ()
{
	return (Visuals & VisualMask & (VisualColors | VisualBold));
}

int Term::hasInverse ()
{
	return (Visuals & VisualMask & (VisualColors | VisualInverse));
}

int Term::hasColors ()
{
	return (Visuals & VisualMask & VisualColors);
}

int Term::hasGraph ()
{
	return (Visuals & VisualMask & VisualGraph);
}

void Term::SetTty()
{
#define NOCHAR	0
#define CHANNEL	0
	struct termios tio;
	if (tcgetattr (CHANNEL, &orig_termios) < 0)
		return;
	tio = orig_termios;
#ifndef FreeBSD
	tio.c_iflag &= ~(INLCR | ICRNL | IGNCR | ISTRIP | IUCLC);
	tio.c_oflag &= ~(OLCUC | OCRNL | /*OXTABS*/TAB3);
	tio.c_lflag &= ~(ECHO | ICANON | XCASE);
#else
	tio.c_iflag &= ~(INLCR | ICRNL | IGNCR | ISTRIP);
	tio.c_lflag &= ~(ECHO | ICANON);
#endif
	tio.c_cc [VMIN] = 1;         /* break input after each character */
	tio.c_cc [VTIME] = 1;        /* timeout is 100 msecs */
	tio.c_cc [VINTR] = NOCHAR;
	tio.c_cc [VQUIT] = NOCHAR;
	tio.c_cc [VSUSP] = NOCHAR;
#ifdef VDSUSP
	tio.c_cc [VDSUSP] = NOCHAR;
#endif
#ifdef VSWTCH
	tio.c_cc [VSWTCH] = NOCHAR;
#endif
#ifdef VLNEXT
	tio.c_cc [VLNEXT] = NOCHAR;
#endif
#ifdef VDISCARD
	tio.c_cc [VDISCARD] = NOCHAR;
#endif
	tcsetattr (CHANNEL, TCSADRAIN, &tio);

#if 0
#ifdef TIOCSLTC
	ioctl (CHANNEL, TIOCGLTC, (char *) &oldchars);
	newchars = oldchars;
	newchars.t_lnextc = NOCHAR;
	newchars.t_rprntc = NOCHAR;
	newchars.t_dsuspc = NOCHAR;
	newchars.t_flushc = NOCHAR;
	ioctl (CHANNEL, TIOCSLTC, (char *) &newchars);
#endif
#endif
}

void Term::ResetTty()
{
	if ( Term::TE )
	    write( 1, Term::TE, strlen(Term::TE) );
#ifndef NOKEYPAD
	if ( Term::KE )
	    write( 1, Term::KE, strlen(Term::KE) );
#endif
	restore_tty();
}

