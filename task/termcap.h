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

   // ���������� � ���������
   static int hasDim();
   static int hasBold();
   static int hasInverse();
   static int hasColors();
   static int hasGraph();

   static void setColorMap(char * fg, char *bg);
   static short ctab [16], btab [16]; // ������� ���������� ����� �������/����
   static int scrool, rscrool;


   /* ��� ����� � ������������ ������ (���������� ��������� ��������) */
   static unsigned char cyrInputTable [256];
   /* ���� � ����������� ������ */
   static unsigned char inputTable  [256];
   /* ����� */
   static unsigned char outputTable [256];

   // ����������� ������������������ ���������
   static char *AS, *AE, *AC, *GS, *GE, *G1, *G2, *GT;
   static char *CS, *SF, *SR;
   static char *PO, *PF, *POO;
   static char *CL, *CE, *CM, *SE, *SO, *TE, *TI, *VI, *VE, *VS;
   static char *AL, *DL, *FS, *MD, *MH, *ME, *MR;
   static char *CF, *CB, *MF, *MB, *OP;

   static char *MsH, *MsL;

   static char *KS, *KE;

   static char Cy; // �������� ������������ ���������
   static char *Cs, *Ce,
	  *Ct;    // ��� ����� ����������

   static char *Goto (char *cm, int destcol, int destline);

   static int Visuals;      // ����� ����� ������������ �������
   static int VisualMask;   // ����� ����������� ���������

   static void SetTty();
   static void ResetTty();

};

/*
const int VisualBold = 1;    // ���� ���������� ������� (md)
const int VisualDim  = 2;    // ���� ���������� ������� (mh)
const int VisualInverse = 4;    // ���� �������� (so ��� mr)
const int VisualColors  = 8;    // ���� ����� (NF, NB, CF, CB, C2, MF, MB)
const int VisualGraph   = 16;   // ���� ������������� (G1, G2, GT)
*/

#define VisualBold	1
#define VisualDim	2
#define VisualInverse	4
#define VisualColors	8
#define VisualGraph	16

/*
      ������� 0-127   ��������� ����������
	      128-191 ������� �������������
	      192-255 ������������ �������
      ����� ��������� ������� ������� ��������� � koi8, �� � ��������
      ��������� ��������� ����� ������� � ������ �������� <=32

      C�������� ����� �������� ���������� �������� (termcap entry Ct):

      64 ������� - ���������� ������������ �������� ��� ������
      96 �������� - ���������� ��������� �������� (32-127) � ������������
		    ��� �����
      128 �������� - ���������� ������� �������� ������� � �������
		     �������������/������������ ��� �����
      64 ������� - ���������� �������� ������������� ��� ������

      - ��������� ����� ���������� "����" � ������������ deco

      �������� ������:
      ��� ������������� ������� :
	����  Cs != 0  , �� ���� ������� Cs ��� ������������ ���������
	� ����� ������ ������������ �������� � Ce ��� ������������ �������
      ��� ������� ������������� :
	���� gs != 0 , �� ���� ������� gs ��� ������������ ���������
	� ����� ������ ������������� � ge ��� ������������ �������

      ��� ������� n ��������� outputTable[n]

*/

/*
 ������ linedraw �������� ������� ��� ��������� �����:
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
