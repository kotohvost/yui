#ifndef _W3TABLE_H_
#define _W3TABLE_H_

#include <HTChunk.h>

#include "collect.h"
#include "list.h"

class getobj;

class Element
{
public:
  Element() : _prev(0), _next(0), empty(1), break_before(0) {}
  virtual ~Element() {}
  Element *_prev, *_next;
  unsigned empty:1;
  unsigned break_before:1;
  virtual int width( int recommend_width=-1 ) { return 0; }
  virtual void setWidth( int new_width ) {}
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 ) { return 0; }
};

class ChunkBuf
{
  HTChunk buf;
public:
  ChunkBuf() { buf.size = 0; buf.growby = 128;
	       buf.allocated = 0; buf.data = 0; }
  ~ChunkBuf() { HTChunkClear(&buf); }

  void clear() { HTChunkClear(&buf); }
  void put(char ch) { HTChunkPutc(&buf,ch); }
  void put(const char *s) { HTChunkPuts(&buf, s); }
  void terminate() { HTChunkTerminate(&buf); }
  char *str() { return buf.data; }
  int size() { return buf.size; }
  void ensure(int size) { HTChunkEnsure(&buf, size); }
};

class WordElement:public Element
{
public:
  char *str;
  WordElement(char *Str) : str(Str) {}
  ~WordElement() { if (str) ::free(str); }
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class BreakElement:public Element
{
public:
  BreakElement() {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class AnchorElement: public Element
{
public:
  HTChildAnchor *anchor;
  char *buf, *anchor_target;
  char **text;
  int offset, max_word_width;
  Collection *bindList;
  AnchorElement( HTChildAnchor *Anchor, const char *label, const char *a_target );
  ~AnchorElement();
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class CheckElement : public Element
{
protected:
  FormCheck *el;
public:
  CheckElement( FormCheck *_el ) : el(_el) {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class RadioElement : public Element
{
protected:
  FormRadio *el;
public:
  RadioElement( FormRadio *_el ) : el(_el) {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class InputLineElement : public Element
{
protected:
  FormInputLine *il;
public:
  InputLineElement( FormInputLine *_il ) : il(_il) {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class StyleElement: public Element
{
public:
  HTStyle *style;
  StyleElement( HTStyle *Style ) : style(Style) {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class FormSubmitElement : public Element
{
protected:
  FormSubmitButton *bt;
public:
  FormSubmitElement( FormSubmitButton *_bt ) : bt(_bt) {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class FormResetElement : public Element
{
protected:
  FormResetButton *bt;
public:
  FormResetElement( FormResetButton *_bt ) : bt(_bt) {}
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class TableElement : public Element
{
protected:
  Table *tb;
public:
  TableElement( Table *_tb ) : tb(_tb) { break_before=1; }
  ~TableElement();
  virtual int width( int recommend_width );
  virtual void setWidth( int new_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class ListBoxElement : public Element
{
protected:
  FormListBox *el;
  int lines, el_width;
public:
  ListBoxElement( FormListBox *_el );
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class EditElement : public Element
{
protected:
  FormEdit *el;
  int lines, el_width;
public:
  EditElement( FormEdit *_el );
  virtual int width( int recommend_width );
  virtual getobj *insertTo( HText *text, int index, int recommend_width=-1 );
};

class TableData : public TList
{
friend class TableRow;
  TableData *_next, *_prev;
  virtual void *getNext(void *item) { return (void*)((Element*)item)->_next; }
  virtual void *getPrev(void *item) { return (void*)((Element*)item)->_prev; }
  virtual void setNext(void *item, void *next) { ((Element*)item)->_next=(Element*)next; }
  virtual void setPrev(void *item, void *prev) { ((Element*)item)->_prev=(Element*)prev; }
  virtual void freeItem(void *item) { delete (Element*) item; }
public:
  TableData( int rowspan=0, int colspan=0, int Align=0, int Header=0 );
  ~TableData();
  getobj *first_obj, *last_obj;
  int id;
  Collection *cols;
  int rowspan;
  int align;
  int isHeader;
  HTStyle *style, *subStyle;
  HTColor color;
  TableData *parent;	// если установлен, то ячейка фиктивная
  int hor_line;
  int add_count;
};

class TableRow: public TList
{
protected:
  virtual void *getNext(void *item) { return ((TableData*)item)->_next; }
  virtual void *getPrev(void *item) { return ((TableData*)item)->_prev; }
  virtual void setNext(void *item, void *next) { ((TableData*)item)->_next=(TableData*)next; }
  virtual void setPrev(void *item, void *prev) { ((TableData*)item)->_prev=(TableData*)prev; }
  virtual void freeItem(void *item) { delete (TableData*) item; }
public:
  TableRow() : td_with_obj(0) {}
  ~TableRow() { freeAll(); }
  TableData *td_with_obj; // для вставки getobj в соответствии с порядком ячеек в строке
};

class Table: public Collection
{
protected:
  Collection columns;
  void freeItem(void *item) { delete (TableRow*)item; }
  int min_width, max_width, tbl_width, line;
  int lock_id, hor_flag;
  TableRow *tr_prev;
  char *caption;
public:
  Table( BOOL Border, Table *prev_t=0, TableRow *prev_r=0, TableData *prev_c=0 );
  ~Table();
  BOOL border;
  ChunkBuf caption_buf;
  Table *prev_table;
  TableRow *prev_row;
  TableData *prev_cell;
  Element *parent;
  void init();
  int width() { return tbl_width; }
  void setWidth( int new_width );
  void print( HText *text );
  int printRow( HText *text );
};

#endif
