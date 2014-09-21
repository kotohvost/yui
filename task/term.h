// $Id: term.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
#ifndef _TERM_H_
#define _TERM_H_

#define BASE_TERMCAP	0
#define BASE_TERMINFO	1

#define CAPFLG	0
#define CAPNUM	1
#define CAPSTR	2

#define VisualBold	1
#define VisualDim	2
#define VisualInverse	4
#define VisualColors	8
#define VisualGraph	16

typedef struct {
	char *tcap;
	char type;	/* CAPFLG, CAPNUM, CAPSTR */
	char *val_bool;
	int  *val_int;
	char **val_ptr;
} CapMap;

typedef struct {
	char *tcap;
	char *str;
	long val;
} KeyMap;

class Term
{
public:
	Term() {}
	~Term() {}
	static char *tname;
	static int rows, cols;
	static short ctab[16], btab[16];
	static unsigned short linedraw[11];

	static char *BS, *BR, *DS, *DR, *NS, *NR;
	static char *GS, *GE, *G1, *G2, *GT, *AC, *AS, *AE;
	static char *CS, *SF, *SR, *EA;

	static char *KS, *KE;

	static char *CL, *CE, *CM, *SE, *SO, *TE, *TI, *VE, *VS,
		*AL, *DL, *IS, *IF, *FS, *MD, *MH, *ME, *MR,
		*CF, *CB, *AF, *AB, *Sf, *Sb, *MF, *MB, *OP;
	static int NF, NB;
	static char MS, C2;

	static char Cy;
	static char *Cs, *Ce, *Ct;

	static int Visuals;
	static int VisualMask;
	static int scrool, rscrool;
	static CapMap captab[];
	static KeyMap keytab[];

	static int init( int base );
	static void destroy();
	static char *makecolor( int c, int b );
	static void initbold();
	static void initgraph();
	static void initcolor( char *fg, char *bg );

	static int hasDim();
	static int hasBold();
	static int hasInverse();
	static int hasColors();
	static int hasGraph();

	/* ввод в стандартном режиме */
	static unsigned char inputTable[256];
	/* для ввода в национальном режиме (трансляция латинских символов) */
	static unsigned char cyrInputTable[256];
	static char cyrOutputTable[256];

	static void sysvgoto( register char *outp, register char *cp, int p1, int p2 );
	static char *Goto (char *cm, int destcol, int destline);

	static void SetTty();
	static void ResetTty();
	static void fill_info();
};

/*
 *      Символы 0-127   считаются латинскими
 *	      128-191 символы псевдографики
 *	      192-255 национальные символы
 *      Такая структура кодовой таблицы совпадает с koi8, но в принципе
 *      позволяет описывать любой алфавит с числом символов <=32
 *
 *      Cтруктура файла описания трансляции символов (termcap entry Ct):
 *
 *      64 символа - трансляция национальных символов при выводе
 *      96 символов - трансляция латинских символов (32-127) в национальные
 *		    при вводе
 *      128 символов - трансляция верхней половины таблицы в символы
 *		     псевдографики/национальные при вводе
 *      64 символа - трансляция символов псевдографики при выводе
 *
 *      - структура файла совместима "вниз" с используемой deco
 *
 *      алгоритм вывода:
 *      для национального символа :
 *	если  Cs != 0  , то надо вывести Cs для переключения терминала
 *	в режим вывода национальных символов и Ce для переключения обратно
 *      для символа псевдографики :
 *	если gs != 0 , то надо вывести gs для переключения терминала
 *	в режим вывода псевдографики и ge для переключения обратно
 *
 *      для символа n выводится outputTable[n]
 *
 */

/*
 * массив linedraw содержит символы для рисования рамок:
 *     0    horisontal line
 *     1    vertical line
 *     2    lower left corner
 *     3    lower center
 *     4    lower right corner
 *     5    left center
 *     6    center cross
 *     7    right center
 *     8    upper left corner
 *     9    upper center
 *     10   upper right corner
 */

#ifdef SCO_SYSV
  #ifndef _STRUCT_WINSIZE
    #define _STRUCT_WINSIZE
struct winsize {
	unsigned short ws_row;		/* rows, in characters */
	unsigned short ws_col;		/* columns, in character */
	unsigned short ws_xpixel;	/* horizontal size, pixels */
	unsigned short ws_ypixel;	/* vertical size, pixels */
};
  #endif

#endif

#endif /* _TERM_H_ */
