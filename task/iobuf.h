/*
	$Id: iobuf.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef IOBUF_H
#define IOBUF_H

#include <stdarg.h>
#include <unistd.h>
#include <limits.h>

#include "list.h"
#include "regexpr.h"


class Buf;

class BufList: public TList
{
  virtual void *getNext(void *item);
  virtual void *getPrev(void *item);
  virtual void setNext(void *item, void *next);
  virtual void setPrev(void *item, void *prev);
  virtual void freeItem(void *item);
public:
  int freeFlag;
  BufList():freeFlag(0){;}
  ~BufList(){freeFlag=1;freeAll();}
  Buf *item();
};

#define _BUF_BUFLEN_	4096
#define _BUF_BUFMIN_	512
#define _BUF_FOREVER_	LONG_MAX

class Buf
{
friend class BufList;

public:
  enum State{
	Good,
	Fail,
	Closed
       };

private:

  Buf *next;
  Buf *prev;

  char *buf;
  int buflen;

  char *start;
  char *end;
  char *nextb;

  long position, nextb_offset;

protected:

  static BufList buffers;   //  буфера ввода-вывода
  unsigned
     bflag:1,    // блокировать при недоступности физического ввода/вывода
     fflag:1,    // флаг строчной буферизации
     tflag:1,    // флаг "текстового" режима (трансляция \r\n <-> \n )
     changed:1;  // буфер изменен по сравнению с файлом
  State _state;

  virtual long _read ( void *buf, long len, u_long timeout ) =0;
  virtual long _write( void *buf, long len, u_long timeout ) =0;
  virtual long _seek( long pos, int whence=SEEK_SET ) =0;

public :
  Buf( int Bflag=1, int Fflag=0 );
  virtual ~Buf();
  void init( int Buflen );

  int get_blockmode() { return bflag; }
  int get_flushmode() { return fflag; }
  int get_textmode () { return tflag; }

  virtual int blockmode( int newmode );
  virtual int flushmode( int newmode );
  virtual int textmode ( int newmode );

  State state() { return _state; }

  long fetch( char *&obuf, u_long timeout=_BUF_FOREVER_ ); // заполняет буфер насколько возможно
  long halfill( int &eoflag, u_long timeout );
  /*
     если буфер заполнен больше чем наполовину,
     переносит последние buflen/2 байт в начало буфера, иначе
     переносит все содержимое буфера в начало
     - пытается заполнить буфер до конца
     - если чтение вернуло <0 взводит eoflag
     - возвращает число байт в буфере
  */

  long read( void *buf, long len, u_long timeout=_BUF_FOREVER_ );
  long read( void *buf, re_registers *regs, int n=0, u_long timeout=_BUF_FOREVER_ );
  long readstr( void *buf, long maxlen, u_long timeout=_BUF_FOREVER_ );
  long search( re_pattern_buffer *regexp, re_registers * regs, long from=-1, long size=-1 );
//  int scan(const char *fmt, ...);

  long write( void *buf, long len, u_long timeout=_BUF_FOREVER_ );
  long writestr( const char *buf, u_long timeout=_BUF_FOREVER_ );
  int print( const char *fmt, ... );
  int printv( const char *fmt, va_list ap );

  long tell();
//  long set_position(long pos) { position=pos; }
  long seek( long pos, int whence=SEEK_SET );
  long seek( re_registers *regs, int n=0, int whence=SEEK_SET );

  void flush(u_long timeout=_BUF_FOREVER_);
  int  inbuf(); // return number of bytes already in buffer

  static int remove( Buf *buf );
};


class FileBuf : public Buf
{
protected:
  int handle;
  int needClose, unlink_flag;
  char *path;

  virtual long _read ( void *buf, long len, u_long timeout );
  virtual long _write( void *buf, long len, u_long timeout );
  virtual long _seek( long pos, int whence=SEEK_SET );

public:
  FileBuf( const char *name, const char *mode="a", int amode=0600 );
  FileBuf( int Handle, int NeedClose=1, int Fflag=0, int Buflen=_BUF_BUFLEN_ );
  ~FileBuf();
  int fd()	{ return handle; }
  int set_fd( int _fd );
  int close();
  int badFd()	{ return (handle=-1); }
  void unlinkFlag( int value )	{ unlink_flag=value; }
  virtual int blockmode( int newmode );
  pid_t pid;  //для связки функций open("|cat>/tmp/asdf","w+") и close() в yucl
};

#endif
