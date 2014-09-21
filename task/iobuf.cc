/*
	$Id: iobuf.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "iobuf.h"
#include "task.h"

void *BufList::getNext(void *item) { return (void*) ((Buf*)item)->next; }
void *BufList::getPrev(void *item) { return (void*) ((Buf*)item)->prev; }
void BufList::setNext(void *item, void *next) { ((Buf*)item)->next = (Buf*)next; }
void BufList::setPrev(void *item, void *prev) { ((Buf*)item)->prev = (Buf*)prev; }
void BufList::freeItem(void *item)  { delete (Buf*)item; }
Buf *BufList::item(){ return (Buf*) getCurrent(); }

#define MIN(a,b)	((a)<(b)?(a):(b))

BufList Buf::buffers;

Buf::Buf( int Bflag, int Fflag ):
  next(0), prev(0),
  buf(0), buflen(0), start(0), end(0), nextb(0),
  position(0), nextb_offset(0),
  bflag(Bflag), fflag(Fflag), tflag(0), changed(0),
  _state(Closed)
{
  buffers.insert(this);
}

void Buf::init(int Buflen)
{
  buflen=(Buflen/2)*2;
  if ( buflen < _BUF_BUFMIN_ )
    buflen = _BUF_BUFMIN_;
  int slen, nlen, elen, len=buflen;
  if ( buf )
   {
     slen=start-buf;
     nlen=nextb-buf;
     elen=end-buf;
   }
  else
     slen=nlen=elen=0;

  buf=(char*)realloc(buf, buflen);

  if ( slen>=buflen || nlen>=buflen || elen>=buflen )
   {
     start=nextb=end=buf;
     nextb_offset -= nlen;
   }
  else
   {
     start = buf + slen;
     nextb = buf + nlen;
     end = buf + elen;
   }
}

Buf::~Buf()
{
  if ( buf )
    ::free(buf);
  if ( !buffers.freeFlag )
    buffers.remove(this);
}

int Buf::blockmode(int newmode)
{
  int b = bflag; bflag = newmode; return b;
}

int Buf::flushmode(int newmode)
{
  int b=fflag; fflag=newmode; return b;
}

int Buf::textmode(int newmode)
{
  int b=tflag; tflag=newmode; return b;
}

long Buf::read(void *buf, re_registers *regs, int n, u_long timeout)
{
  if ( seek( regs->start[n] ) < 0 )
     return 0;
  return read( buf, regs->end[n] - regs->start[n], timeout );
}

long Buf::fetch( char *&obuf, u_long timeout )
{
  int b_read = _read( buf, buflen, timeout );
  if ( b_read >= 0 )
   {
     start = nextb = buf;
     nextb_offset = position;
     position += b_read;
     end = buf + b_read;
     obuf = start;
   }
  return b_read;
}

long Buf::halfill( int &eoflag, u_long timeout )
{
  int b_len = end-nextb;
  int left = buflen - b_len;
  if ( b_len > 0 )
     memmove( buf, nextb, b_len );
  start = nextb = buf;
  end = start + b_len;
  int b_read = _read( end, left, timeout );
  if ( b_read < 0 )
     eoflag=b_read;
  else
   {
     eoflag=0;
     end += b_read;
     position += b_read;
   }
  return b_read; // < 0 ? 0 : b_read;
}

/*
long Buf::read(void *addr, long nbytes, u_long timeout)
{
  int b_left = end - nextb;
  int b_transferred = MIN( b_left, nbytes );
  memcpy( addr, nextb, b_transferred );
  nextb += b_transferred;
  nextb_offset += b_transferred;
  if ( b_transferred == nbytes )
     return b_transferred;
  addr += b_transferred;
  nbytes -= b_transferred;
  int b_read, total_b_transferred = b_transferred;
  do {
     if ( (b_read = _read( buf, buflen, timeout )) < 0 )
      {
	_state=Fail;
	return total_b_transferred ? total_b_transferred : -1;
      }
     if ( b_read==0 && timeout != WaitForever && !bflag )
	return total_b_transferred;
     position += b_read;
     start = nextb = buf;
     end = start + b_read;
     b_transferred = MIN( b_read, nbytes );
     memcpy( addr, nextb, b_transferred );
     addr += b_transferred;
     nextb += b_transferred;
     nextb_offset += b_transferred;
     total_b_transferred += b_transferred;
     nbytes -= b_transferred;
   } while ( nbytes && b_read == buflen );
  return total_b_transferred;
}
*/

long Buf::read( void *addr, long le, u_long timeout )
{
  if ( le < 1 ) return 0;
  int eoflag=0, b_tr=0, left, b_read=0;
  unsigned long _timeout;
  while( le > 0 )
   {
     left=end-nextb;
     if ( left != buflen )
      {
	_timeout = bflag ? timeout : 0;
	b_read=halfill( eoflag, _timeout );
	if ( left<=0 && (b_read < 0 || b_read==0 && !bflag) )
	   break;
	left = end - nextb;
      }
     left = MIN( left, le );
     memcpy( addr, nextb, left );
     addr = (void*)((unsigned long)addr + left);
     nextb += left;
     nextb_offset += left;
     b_tr += left;
     le -= left;
   }
  return b_tr || !eoflag ? b_tr : eoflag;
}


static char *mem2chr( char *start, const char *str, int len )
{
  for( int i=0; i<len; i++, start++ )
    if ( *start==*str )
     {
       if ( i+1 >= len )
	 return 0;
       if ( start[1] == str[1] )
	 return start;
     }
  return 0;
}

long Buf::readstr(void *rbuf, long le, u_long timeout)
{
  if ( le < 1 ) return 0;
  char *endstr, *str;
  int pos=0, left=0, b_read=0, eoflag=0, l;
  while( pos <= le )
  {
    int left = end - nextb;
    if ( left <= 0 )
     {
       if ( halfill(eoflag,timeout) <= 0 )
	  break;
       left=end-nextb;
     }
   if ( tflag )
     endstr = mem2chr( nextb, "\r\n", left );
   else
     endstr = (char*)memchr( nextb, '\n', left );

   if ( endstr )
     {
       if ( (l=endstr-nextb) > 0 )
	{
	  int _l = l;
	  if ( pos + _l > le )
	     _l = le - pos;
	  memcpy( (char*)rbuf+pos, nextb, _l );
	  pos += _l;
	}
       nextb_offset += endstr - nextb + 1;
       nextb = endstr+1;
       break;
     }
   else
     {
       if ( left > 0 )
	{
	  int _l = left;
	  if ( pos + _l > le )
	     _l = le - pos;
	  memcpy( (char*)rbuf+pos, nextb, _l );
	  pos += _l;
	}
       nextb_offset += end - nextb;
       nextb = end;
     }
  }
  ((char*)rbuf)[ pos < le ? pos : le-1 ] = 0;
  return eoflag ? eoflag : pos;
}

long Buf::search( re_pattern_buffer *regexp, re_registers * regs, long pos, long size )
{
  int ret=-1, eoflag=0, _size=0, left;
  if ( pos >= 0 )
     seek( pos );
  if ( size < 0 )
     size=0x7fffffff;

  while( 1 )
   {
     if ( size <= 0 )
       break;
     _size = left = end - nextb;
     if ( left <= 0 )
      {
	while( (left=halfill(eoflag,WaitForever)) <= 0 )
	  if ( left < 0 )
	    return -1;
	_size = left = end - nextb;
      }
     if ( _size > size )
	_size = size;
     ret = re_search( regexp, nextb, left, 0, _size, regs );
     if ( ret < -1 )
	break;
     if ( ret >= 0 )
      {
	long shift = regs->start[0];
	for( int i=0; i < RE_NREGS && regs->start[i] >= 0 ; i++ )
	 {
	   regs->start[i] += nextb_offset;
	   regs->end[i] += nextb_offset;
	 }
	nextb += shift;
	nextb_offset += shift;
	break;
      }
     if ( left == buflen )
      {
	int shift = buflen/4;
	nextb += shift;
	nextb_offset += shift;
	size -= shift;
      }
     if ( halfill( eoflag, WaitForever ) < 0 )
	return -1;
   }
  return ret;
}

long Buf::write( void *rbuf, long count, u_long timeout )
{
  char *addr = (char*)rbuf;
  int b_left, b_num, b_writed=0;
  while( count > 0 )
   {
     b_left = buflen - (nextb-buf);
     if ( b_left <= 0 )
      {
	if ( changed )
	   flush();
	int off = nextb - buf;
	memmove( buf, nextb, end-nextb );
	start=nextb=buf;
	end -= off;
	b_left = buflen;
      }
     b_num = MIN( b_left, count );
     memcpy( nextb, addr, b_num );
     changed = 1;
     addr += b_num;
     nextb += b_num;
     nextb_offset += b_num;
     b_writed += b_num;
     if ( nextb > end )
	end = nextb;
     count -= b_num;
   }
  if ( changed && (fflag || !bflag) )
     flush();
  return b_writed;
}

long Buf::writestr(const char *buf, u_long timeout)
{
  int b_writed;
  b_writed = write( (void*)buf, strlen(buf), timeout );
  char *eos = (char*)(tflag ? "\r\n" : "\n");
  b_writed += write( eos, strlen(eos), timeout );
  return b_writed;
}

long Buf::tell()
{
  return nextb_offset;
}


long Buf::seek(long pos, int whence)
{
  if ( changed )
      flush();
  long startPos = nextb_offset - (nextb-start);
  if ( whence || pos < startPos || pos > startPos + (end-nextb) )
   {
      if ( whence == 1 )	// SEEK_CUR
	 _seek( nextb_offset, SEEK_SET );
      nextb_offset = position = _seek( pos, whence );
      start = nextb = end = buf;
   }
  else
   {
      nextb = start + (pos - startPos);
      nextb_offset = pos;
   }
  return nextb_offset;
//return nextb_offset=pos;
}

long Buf::seek( re_registers *regs, int n, int whence )
{
  return seek( regs->start[n], whence );
}

void Buf::flush(u_long timeout)
{
  if ( !changed )
     return;
  int nbytes = nextb_offset - position;
  char *s = nextb - nbytes;
  while( nbytes > 0 )
   {
     int l = _write( s, end-s, timeout );
     if ( l < 0 || l==0 && timeout != WaitForever && !bflag )
	return;
     nbytes -= l;
     s += l;
     position +=l;
   }
  changed = 0;
}

int Buf::remove(Buf *buf)
{
  return buffers.remove(buf);
}




#define INS_CHAR( c, bep,  cc )			\
	{					\
		if ( end < bep ) {		\
			*end++ = c; cc++;	\
		} else write((void*)&c,1,WaitForever);\
	}

int Buf::print( const char *fmt, ... )
{
    va_list ap ;

    va_start( ap, fmt ) ;
    int cc = printv( fmt, ap ) ;
    va_end( ap ) ;
    return( cc ) ;
}


int Buf::printv( const char *fmt, va_list ap)
{
   char *b, *e;
   char numbuf[64];
   numbuf[sizeof(numbuf)-1]=0;
   char format[16];
   char *bep=buf+buflen;
   int cc=0, l, L, c, i;
   int Bflag=bflag, flushflag=0, longflag=0;
   bflag=1;
   for( i=0; ; i++ )
    {
      while(*fmt && *fmt!='%')
       {
	 if ( *fmt=='\n' && fflag )
	  {
	   flushflag=1;
	   if (tflag)
	    {
	     c='\r';
	     INS_CHAR(c, bep, cc)
	    }
	  }
	 INS_CHAR(*fmt++,bep,cc)
	 if (flushflag)
	   flush();
	 flushflag=0;
       }
      if (!fmt || !*fmt)
	break;

      b = (char*)strpbrk(fmt+1, "scdixXoufFgGEe%");

      if (!b)
	break;

      memcpy(format, fmt, b-fmt+1);
      format[b-fmt+1]=0;
      longflag=0;
      if (strpbrk(format, "lL"))
	longflag=1;

      switch (*b)
       {
	case 's':
	     {
	     char *s = va_arg(ap,char*);
	     l=strlen(s);
	     L=write( s, l, WaitForever );
	     }
	     if (L<l)
	       goto Ex;
	     cc+=L;
	     break;
	case 'n':
	     {
	     int *ip=va_arg(ap, int*);
	     *ip=cc;
	     }
	     break;
	case '%':
	     {
	     c='%';
	     INS_CHAR( c, bep, cc );
	     }
	     break;
	default:
#if defined(DJGPP) || defined(SOLARIS_25) || defined(SCO_SYSV) || \
    defined(AIX_PPC) || defined(SINIX)
	     vsprintf( numbuf, format, ap );
#else
	     vsnprintf( numbuf, sizeof(numbuf)-1, format, ap );
#endif
	     l=strlen(numbuf);
	     L=write( numbuf, l, WaitForever );
	     if (L<l)
	       goto Ex;
	     cc+=L;
	     switch(*b)
	      {
		case 'p':
		     va_arg(ap, void*);
		     break;
		case 'i':
		case 'd':
		case 'u':
		case 'x':
		case 'X':
		case 'o':
		case 'c':
		    if(longflag)
		      va_arg(ap,long);
		    else
		      va_arg(ap,int);
		    break;
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'e':
		case 'E':
		      va_arg(ap, double);
		    break;
	      }
	    break;
       }
      fmt=++b;

    }

Ex:
   bflag=Bflag;
   return i;
}

int Buf::inbuf()
{
  if ( !start || !end || !nextb )
     return 0;
  return end - nextb > 0 ? 1 : 0;
}

/*
int Buf::scan(const char *fmt, ...)
{
  return 0;
}
*/

FileBuf::FileBuf( const char *Path, const char *m, int cmode ) :
		needClose(1), unlink_flag(0), path(0), pid(-1)
{
    int ret, file=-1;
    unsigned mode, pathlen=0;
    path = strdup( Path );

    // mode make
    mode=0;
    #ifdef __MSDOS__
      if (strchr(m,'t'))
	 mode |= O_TEXT;
      else
	 mode |= O_BINARY;
    #endif
    if (strchr(m,'+')) mode |= O_RDWR;
    if (strchr(m,'w'))
      {
	mode |= O_TRUNC;
	mode |= O_CREAT;
	if (!(mode & O_RDWR)) mode |= O_WRONLY;
      }
    if (strchr(m,'r'))
       {
	 if (!(mode & O_RDWR)) mode |= O_RDONLY;
       }
    if (strchr(m,'a'))
       {
	 mode |= O_APPEND;
	 mode |= O_CREAT;
	 if (!(mode & O_RDWR)) mode |= O_WRONLY;
       }
  #if defined(O_SHLOCK) && defined(O_EXLOCK)
    if (strchr(m,'s'))
	 mode |= O_SHLOCK;
    if (strchr(m,'e'))
	 mode |= O_EXLOCK;
  #endif
   if ( !(path[0]=='|' || ( (pathlen=strlen(path))>0 && path[pathlen-1]=='|') ) )
      file=open(path, mode, cmode);
   else
    {
    unlink_flag=0;
#ifndef unix
    errno=EBADF;
#else
    if (!strcmp(path, "|-"))
      file=open("/dev/tty", O_APPEND, 0);
    else if (!strcmp(path, "-|"))
      file=open("/dev/tty", O_RDONLY, 0);
    else
     {
      // create chield
      int pi[2];

      pipe(pi);
	  /* set O_NONBLOCK to 1*/
      int arg=0;
      if (path[0]=='|') // out pipeline
       {
	 arg = fcntl( pi[1], F_GETFL, arg );
	 fcntl( pi[1], F_SETFL,  arg | O_NONBLOCK );
       }
      else
       {
	 path[pathlen-1]=0;
	 arg = fcntl( pi[0], F_GETFL, arg );
	 fcntl( pi[0], F_SETFL,  arg | O_NONBLOCK );
       }

#ifdef SCO_SYSV
     if ( !fork() )
#else
     if ( !vfork() )
#endif
       {
	if (path[0]=='|') // out pipeline
	  {
	    dup2(pi[0],0);
	    ::close(pi[0]);
	  }
	else
	  {
	    dup2(pi[1],1);
	    dup2(pi[1],2);
	    ::close(pi[1]);
	  }
	 for(int i=3; i<20; i++)
	      ::close(i);

	 setgid(getgid());
	 setuid(getuid());
	 char *p=path+ ((path[0]=='|')?1:0) ;
	 execlp( "sh", "sh", "-c", p, 0 );

	 exit(111);
       }
     if (path[0]=='|')
       {
	file = pi[1];
	::close(pi[0]);
       }
     else
       {
	file = pi[0];
	::close(pi[1]);
       }
     }
#endif
    }

 handle=file;
 if (file>=0)
   {
     int arg=0, Buflen=_BUF_BUFLEN_;
     char *bb = (char*)strchr(m, 'b');
     if ( bb )
	Buflen=atol(bb+1);
     _state=Good;
     init(Buflen);
     arg=fcntl( handle, F_GETFL, arg );
     fcntl(handle, F_SETFL,  arg | O_NONBLOCK);
   }
 else
   _state=Fail;
}

FileBuf::FileBuf( int Handle, int NeedClose, int Fflag, int Buflen ) :
		handle(Handle), needClose(NeedClose), unlink_flag(0),
		path(0), pid(-1)
{
  init(Buflen);
  fflag=Fflag;
  int arg=0;
  if ( (arg=fcntl(handle, F_GETFL, arg) ) ==-1 )
    _state=Fail;
  else
    {
      fcntl(handle, F_SETFL,  arg | O_NONBLOCK);
      _state=Good;
    }
}

FileBuf::~FileBuf()
{
  close();
  if ( path )
   {
     if ( unlink_flag )
	::unlink( path );
     ::free( path );
   }
}

int FileBuf::set_fd( int _fd )
{
  if ( handle >= 0 )
    { changed=1; flush(); }
  ::close( handle );
  int old_fd = handle;
  handle = _fd;
  return old_fd;
}

int FileBuf::close()
{
  if ( handle >= 0 ) {
      changed = 1;
      flush();
  }
  int ret = -1;
  if ( needClose && handle >= 0 ) {
      ret = ::close(handle);
      handle = -1;
  }
  return ret;
}

int FileBuf::blockmode( int newmode )
{
  int arg=0, ret = Buf::blockmode( newmode );
  arg = fcntl( handle, F_GETFL, arg );
/*  fcntl( handle, F_SETFL, bflag ? (arg | O_NONBLOCK) : (arg & ~O_NONBLOCK) );*/
  fcntl( handle, F_SETFL, (arg | O_NONBLOCK) );
  return ret;
}

long FileBuf::_read (void *rbuf, long len, u_long timeout)
{
   if ( _state!=Good )
     return -1;
   int ret = Task::waitRead( handle, timeout );
   if ( ret > 0 && (ret=::read( handle, rbuf, len ))==0 && len != 0 )
      return -2;	//eof
   return ret;
}

long FileBuf::_write(void *rbuf, long len, u_long timeout)
{
   if (_state!=Good)
     return -1;

   int w = Task::waitWrite(handle, timeout);
   if ( w > 0 )
     return ::write(handle, rbuf, len);

   return w;
}

long FileBuf::_seek( long pos, int whence )
{
   if ( _state!=Good )
     return -1;
   else
     return ::lseek(handle, pos, whence);
}
