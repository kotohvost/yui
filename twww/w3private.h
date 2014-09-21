#ifndef _W3PRIVATE_H_
#define _W3PRIVATE_H_

extern "C" {
#include <HTUtils.h>	/* WWW general purpose macros */
#include <HTFormat.h>
#include <HTTCP.h>	/* TCP/IP utilities */
#include <HTAnchor.h>   /* Anchor class */
#include <HTParse.h>    /* WWW address manipulation */
#include <HTAccess.h>   /* WWW document access network code */
/*#include <HTHistory.h>*/	/* Navigational aids */
#include <HTML.h>	/* For parser */
#include <HTFWriter.h>	/* For non-interactive output */
#include <HTMLGen.h>	/* For reformatting HTML */
#include <HTFile.h>	/* For Dir access flags */
#include <HTRules.h>    /* For loading rule file */
#include <HTError.h>
#include <HTAlert.h>
#include <HTTP.h>

#include <HTStyle.h>
#include <HTPlain.h>
#include <HText.h>
#include "w3style.h"
}

#include <point.h>
#include <collect.h>
#include <listbox.h>
#include <yui.h>
#include "hashcode.h"
#include "w3win.h"

extern "C" void HText_BeginAnchor (HText * text, HTChildAnchor * anc, BOOL haveHREF, const char *a_target);
extern "C" HTStyleSheet * styleSheet;
extern "C" void HText_putListPrefix(HText *text, char *prefix);
extern "C" void HText_appendSoftPara(HText *text);
extern "C" void addSlash(char **str);

/* html-2 forms */
extern "C" void HText_beginForm(HText *text, const char *action, const char *method);
extern "C" void HText_endForm(HText *text);
extern "C" void HText_beginInput(HText *text, const char *name, const char *type,
		const char *value, const char *size,
		const char *maxlength, BOOL checked );
extern "C" void HText_beginTextArea(HText *text, const char *name,
		const char *cols, const char *rows);
extern "C" void HText_endTextArea(HText *text);
extern "C" void HText_beginSelect(HText *text, const char *name,
		const char *size, BOOL multiple);
extern "C" void HText_endSelect(HText *text);
extern "C" void HText_option(HText *text, const char *value, int selected);
extern "C" void HText_base(HText *text, const char *base_href, const char *base_target);

/* html-3 tables */
extern "C" void HText_beginTable(HText *text, BOOL border);
extern "C" void HText_endTable(HText *text);
extern "C" void HText_setCaption(HText *text, const char *data);
extern "C" void HText_row(HText *text);
extern "C" void HText_cell(HText *text, int rowspan, int colspan, const char *align, BOOL isHeader);


extern "C" int w3system(const char *string, HTRequest *request);

struct _HTStream {			/* only know it as object */
    CONST HTStreamClass *	isa;
    /* ... */
};


/* HText interface */
class Form;
class Table;
class TableRow;
class TableData;

struct _HText
{
  /* Output stream for non-interactive usage */
  HTStream *target;
  HTStreamClass targetClass;

  HTStyle *style;
  HTChildAnchor * currAnchor;
  char *anchor_target;	// для использования во фреймах
  Form *form;
  Table *table;
  TableRow *row;
  TableData *cell;

  HTStyle *subStyle;  // только наложение параметров
  HTColor color;
  const char *strBefore;
  const char *strAfter;
  char buf[256]; // word buffer
  int buflen;
  int spaceCount;
  int shift;

  // context state flags
  unsigned waitDigit:1;
  unsigned lateBreak:1;
  unsigned newPara:1;
  unsigned softPara:1;
  unsigned inAnchor:1;
  unsigned inSelect:1;
  unsigned wasBreak:1;

  W3Win *win;
  long cury, curx;

  char * area, *areaname;
  int arearows, areacols, arealen, areapos;

  Collection *sellist;
  Collection *selval;
  Collection *selstate;
  char *selname;
  int selsize, selmul, selsel;

  char *option;
  int optlen, optpos;
  char *option_value;
  int option_state;
  void endOption();

  void _new_line();
  void new_line();
  void New_line();
  void new_paragraph( int isFirst=1 );
  void newIndent();
  void flush(char ch);
  void PUTC(char ch, int graph=0);
  void PUTS(const char *str);
  void PUTBUF();
  void charToArea( char ch );
  int  rightMargin();
};

class AnchorHyper: public hyper
{
public:
  HTChildAnchor *anchor;
  char *_target;
  AnchorHyper(int y, int x, const char *tit, HTChildAnchor *Anchor, HTParentAnchor *parent, char *anchor_target);
  ~AnchorHyper();
  virtual long handleKey(long key, void *&data);
  virtual int isType(long typ);
  virtual void setColors( int Type );
};

class FormElement
{
friend class Form;
friend class W3Win;
protected:
  Form *form;
  char *name;
  char *value;
  int getval_flag;
public:
  const char *getName(){ return name; }
  FormElement( Form *_form, const char *Name, const char *Value );
  virtual ~FormElement();
  virtual char *getValue();
  virtual void reset();
  virtual const char* getLabel();
  virtual void *getObj() { return this; }
  virtual int elementType( long t );
  virtual void startValue() { getval_flag = 1; }
};

class Form: public getobj, public Collection
{
  void freeItem(void *item);
public:
  char *action;
  char *method;
  W3Win *win;
  Form(W3Win* Win, const char* Action, const char*Method);
  ~Form();
  void submit( FormElement *caller );
  void reset();
};

class FormInputLine: virtual public FormElement, virtual public inputLine
{
protected:
  Collection *hist;
  Collection *vals;
  int valIndex;
public:
  FormInputLine( Form *_form, int y, int x, const char *Name,
		 const char *Value, int Size,
		 int MaxLen, int isPasswd=0 );
  FormInputLine( Form *_form, int y, int x, int Hight, int Size,
		 const char *Name, const char *Value,
		 int MaxLen, int isPasswd=0 );
  FormInputLine( Form *_form, int y, int x, const char *Name,
		 const char *Value, int Size, Collection *Hist,
		 Collection *Vals, int ValIndex );
  ~FormInputLine();

  virtual long handleKey( long key, void *&ptr );
  virtual int draw( int Redraw=0, int sy=0, int sx=0 )
		{ return inputLine::draw( Redraw, sy, sx ); }
  virtual char *getValue();
  virtual void reset();
  virtual const char* getLabel();
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
};

class FormListBox: virtual public FormElement, virtual public listBox
{
protected:
  Collection *selval;
  Collection *selstate;
  int multiple, val_index;
  char *val_buf;
public:
  FormListBox( Form *_form, int y, int x, const char *Name,
		int width, int lines, int Multiple,
		Collection *sellist, Collection *selval,
		Collection *selstate, int cur );
  ~FormListBox();
  virtual long handleKey( long key, void *&ptr );
  virtual char *getValue();
  virtual void reset();
  virtual int elementType(long hash);
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
  virtual void startValue();
};

class FormEdit: virtual public FormElement, virtual public Edit
{
protected:
  Collection *reset_buf;
  char *val_buf;
public:
  FormEdit( Form *_form, int y, int x, const char *name,
		int rows, int cols, char *textarea );
  ~FormEdit();
  virtual long handleKey( long key, void *&ptr );
  virtual char *getValue();
  virtual void reset();
  virtual int elementType(long hash);
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
  virtual void startValue();
  int getWidth() { return size.x - (box ? 2 : 0); }
};

class FormCheck: virtual public FormElement, virtual public hyper
{
protected:
  int  _checked;
  int  checked;
  void setLabel();
public:
  FormCheck( Form *_form, int y, int x, const char *Name,
		 const char *Value, int Checked );

  virtual long handleKey( long key, void *&ptr );
  virtual int draw( int Redraw=0, int sy=0, int sx=0 )
		{ return hyper::draw( Redraw, sy, sx ); }
  virtual char *getValue();
  Point getCursor( int *hide );
  virtual void reset();
  virtual int elementType(long hash);
  virtual const char* getLabel()
		{ return hyper::getLabel(); }
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
};

class FormRadio: virtual public FormElement, virtual public hyper
{
protected:
  int  _checked;
  int  checked;
  FormRadio *prev;
  FormRadio *next;
  void setLabel();
public:
  FormRadio( Form *_form, int y, int x, const char *Name,
		 const char *Value, FormRadio *Prev, int Checked );

  virtual long handleKey( long key, void *&ptr );
  Point getCursor( int *hide );
  virtual char *getValue();
  virtual void reset();
  virtual int elementType(long hash);
  virtual const char* getLabel()
		{ return hyper::getLabel(); }
  virtual int draw( int Redraw=0, int sy=0, int sx=0 )
   { return hyper::draw( Redraw, sy, sx ); }
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
};

/*
class FormHidden: virtual public FormElement, virtual public getobj
{
public:
  FormHidden( Form *_form, const char *Name, const char *Value );
};
*/

class FormResetButton: virtual public FormElement, virtual public hyper
{
public:
  FormResetButton( Form *_form, int y, int x, const char *Name,
		   const char *Value );
  virtual int elementType(long hash);
  virtual long handleKey( long key, void *&ptr );
  virtual int draw( int Redraw=0, int sy=0, int sx=0 )
		{ return hyper::draw( Redraw, sy, sx ); }
  virtual const char* getLabel()
		{ return hyper::getLabel(); }
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
};

class FormSubmitButton: virtual public FormElement, virtual public hyper
{
public:
  FormSubmitButton( Form *_form, int y, int x, const char *Name,
	      const char *Value );
  virtual int elementType(long hash);
  virtual long handleKey( long key, void *&ptr );
  virtual int draw( int Redraw=0, int sy=0, int sx=0 )
		{ return hyper::draw( Redraw, sy, sx ); }
  virtual const char* getLabel()
		{ return hyper::getLabel(); }
  virtual void *getObj(){ return this; }
  virtual void setColors(int Type);
};



#endif
