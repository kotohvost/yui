/*
	$Id: yterm.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef YTERM_H
#define YTERM_H

#include "window.h"
#include "menu.h"
#include "task.h"

class TermInputTask;

struct ScrElement
{
#if defined(SOLARIS_SPARC)	 // byte order !!!
  unsigned char a;
  unsigned char c;
#else
  unsigned char c;
  unsigned char a;
#endif
};

#define _YTERM_STRLEN_	1024
#define _YTERM_NACC_	3	/* max parameters for escape sequence */

//extern CommandDescr ytermCommandDescr[];
extern long ytermKeyMap[];
extern Keymap YTermKeyMap;

class YTerm: public Window
{
public:
   enum Mode{
	NORMAL,
	STANDOUT
	} mode;

private:
friend class TermInputTask;

   int cury, curx;
   int curattr;
   int soattr;
   int isBold;
   int clearAttr;
   int FirstLine;
   int FullLines;
   int tty, pty; // канал ввода/вывода

   ScrElement **vscreen;  // виртуальный экран терминала
   int *lnum;             // перестановки строк
   ScrElement bufEl;

   TermInputTask *input;

   int procKey( long key, long translatedKey );
   Rect calcScrollRegion( int topline, int botline );
   void scroolTerm();

public:
   int Columns, Lines;
   unsigned hardMode:2;

   YTerm( const char *program, char **environ=0,
	  Rect tr=Rect(-1,-1,-1,-1),
	  int width=80,
	  int scrLines=24,
	  int fullLines=100,
	  TermInputTask *Input=0
	);
   ~YTerm();

   /* вызываются из Window/Program */
   virtual int 	isType(long typ);
   virtual int draw( int Redraw=0, int sy=0, int sx=0 );
   void info();
   virtual void moveCursor();
   virtual int close();
   virtual char *curWord();
   virtual long handleKey( long key, void *&ptr );
   virtual int init( void *data=0 );
   virtual void setColors( int type );
   virtual void makeMenu( Menu *m=0 );

   void closeTerm();

   /* собственно терминал */
   void move( int y, int x);
   void moveUp(int count);
   void moveDown(int count);
   void moveLeft(int count);
   void moveRight(int count);
   void Cursor( int &y, int &x );
   void setAttr( int attr );
   void setBold( int newBold );
   int  getBold( );
   int  getAttr();
   void setFG(int fg);
   void setBG(int bg);
   void setFG_SO(int fg);
   void setBG_SO(int bg);
   void setMode( Mode mod){ mode=mod; }
   void put( int sym );
   void put( const char *str );
   void puts( const char *str );
   void insLine( int lineno );
   void delLine( int lineno );
   void clearEOL();
   void clearBEOL();
   void clearSCR();
   void clearBSCR();

   /*
      Для порожденных классов - вызывается всякий раз при приеме
      полной строки (до '\n' или  "\r\n"). Концевые '\r''\n' обрезаются.
   */
   void writePty(const char *str);
   virtual void processStr(const char *str, int str_len){;}
   int flagFinishMessage:1;
   char *getStr( int line, int &len );
   void copyBlockToClip();
   void copyBlockFromClip();
};

class FileBuf;

class TermInputTask: public Task
{
protected:
  friend class YTerm;
  int pty;
  pid_t pid;
  YTerm *yterm;
  FileBuf *in;

  char *str;
  int pos;

  void set_allcolors(int clr, int fg);
  enum state_e
   {
     Normal,
     ESC,
     EBRAC,
     EBRACEQ,
     EBRACEQU,
     Fail
   } stat;    // состояния драйвера
  int	so;		/* standout mode */
  u_short ca;		/* additional current attributes, bold/blink */
  int	acc[_YTERM_NACC_];	/* the escape seq arguments */
  int	*accp;		/* pointer to the current processed argument */
  int savecolor, saverow, savecol;

public:
  TermInputTask(int Pty=-1, pid_t Pid=-1, YTerm *Yterm=0);
  void initData(int Pty, pid_t Pid, YTerm *Yterm);
  ~TermInputTask(){/*destroy();*/}
  virtual void destroy();
  virtual long main();
  virtual void sput(u_char c);
  int close();
};



#endif
