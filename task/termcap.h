/*
	$Id: termcap.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef _TERMCAP_H_
#define _TERMCAP_H_

/*
const int CAPNUM = 1;
const int CAPFLG = 2;
const int CAPSTR = 3;
*/
#define CAPNUM	1
#define CAPFLG	2
#define CAPSTR	3

struct Captab {
	char tname [3];
	char ttype;
	char tdef;
	char *tc;
	int *ti;
	char **ts;
};

struct Keytab {
	char *tcap;
	char *str;
	long val;
};


class Termcap
{
   friend class Screen;
   static int init;
   static int initTermcap();

public:
   static char *term;
   static Keytab keytab[];

   static char MS, C2;
   static int  NF, NB, Lines, Columns;
   static unsigned short linedraw [11];

   // информация о терминале
   static int hasDim();
   static int hasBold();
   static int hasInverse();
   static int hasColors();
   static int hasGraph();

   static void setColorMap(char * fg, char *bg);
   static short ctab [16], btab [16]; // таблицы трансляции цвета символа/фона
   static int scrool, rscrool;


   /* для ввода в национальном режиме (трансляция латинских символов) */
   static unsigned char cyrInputTable [256];
   /* ввод в стандартном режиме */
   static unsigned char inputTable  [256];
   /* вывод */
   static unsigned char outputTable [256];

   // управляющие последовательности терминала
   static char *AS, *AE, *AC, *GS, *GE, *G1, *G2, *GT;
   static char *CS, *SF, *SR;
   static char *PO, *PF, *POO;
   static char *CL, *CE, *CM, *SE, *SO, *TE, *TI, *VI, *VE, *VS;
   static char *AL, *DL, *FS, *MD, *MH, *ME, *MR;
   static char *CF, *CB, *MF, *MB, *OP;

   static char *MsH, *MsL;

   static char *KS, *KE;

   static char Cy; // терминал поддерживает кириллицу
   static char *Cs, *Ce,
	  *Ct;    // имя файла трансляции

   static char *Goto (char *cm, int destcol, int destline);

   static int Visuals;      // маска видео возможностей дисплея
   static int VisualMask;   // маска разрешенных атрибутов

   static void SetTty();
   static void ResetTty();

};

/*
const int VisualBold = 1;    // есть повышенная яркость (md)
const int VisualDim  = 2;    // есть пониженная яркость (mh)
const int VisualInverse = 4;    // есть инверсия (so или mr)
const int VisualColors  = 8;    // есть цвета (NF, NB, CF, CB, C2, MF, MB)
const int VisualGraph   = 16;   // есть псевдографика (G1, G2, GT)
*/

#define VisualBold	1
#define VisualDim	2
#define VisualInverse	4
#define VisualColors	8
#define VisualGraph	16

/*
      Символы 0-127   считаются латинскими
	      128-191 символы псевдографики
	      192-255 национальные символы
      Такая структура кодовой таблицы совпадает с koi8, но в принципе
      позволяет описывать любой алфавит с числом символов <=32

      Cтруктура файла описания трансляции символов (termcap entry Ct):

      64 символа - трансляция национальных символов при выводе
      96 символов - трансляция латинских символов (32-127) в национальные
		    при вводе
      128 символов - трансляция верхней половины таблицы в символы
		     псевдографики/национальные при вводе
      64 символа - трансляция символов псевдографики при выводе

      - структура файла совместима "вниз" с используемой deco

      алгоритм вывода:
      для национального символа :
	если  Cs != 0  , то надо вывести Cs для переключения терминала
	в режим вывода национальных символов и Ce для переключения обратно
      для символа псевдографики :
	если gs != 0 , то надо вывести gs для переключения терминала
	в режим вывода псевдографики и ge для переключения обратно

      для символа n выводится outputTable[n]

*/

/*
 массив linedraw содержит символы для рисования рамок:
     0    horisontal line
     1    vertical line
     2    lower left corner
     3    lower center
     4    lower right corner
     5    left center
     6    center cross
     7    right center
     8    upper left corner
     9    upper center
     10   upper right corner
*/

#ifdef SCO_SYSV

#ifndef _STRUCT_WINSIZE
#define _STRUCT_WINSIZE
struct winsize {
	unsigned short ws_row;		/* rows, in characters*/
	unsigned short ws_col;		/* columns, in character */
	unsigned short ws_xpixel;	/* horizontal size, pixels */
	unsigned short ws_ypixel;	/* vertical size, pixels */
};
#endif

#endif

#endif
