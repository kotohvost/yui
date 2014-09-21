/*
	$Id: iotest.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "iobuf.h"

int main( int argc, char **argv )
{
  if ( argc < 2 )
    { fprintf( stderr, "One argument required\n" ); return 1; }
  int d_in = open( argv[1], O_RDONLY );
  int d_out = -1;

  if ( d_in < 0 )
    { fprintf( stderr, "Cannot open %s\n", argv[1] ); return 1; }
  FileBuf *in = new FileBuf( d_in );
  in->init(512);

  if ( argc > 2 && (d_out=open( argv[2], O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR )) < 0 )
    { fprintf( stderr, "Cannot open %s\n", argv[1] ); return 1; }
  FileBuf *out = 0;
  if ( d_out >= 0 )
   {
     out =  new FileBuf( d_out );
     out->init(512);
   }
  char buf[1024];
  int bt;

//  while( (bt=in->read( buf, 1023 )) >= 0 )
//    write( 1, buf, bt );

#if 1
  while( (bt=in->read( buf, 1023 )) >= 0 )
   {
     if ( out )
	out->write( buf, bt );
     else
	puts( buf );
   }
#else
  struct re_registers regs;
  struct re_init
   {
     re_pattern_buffer exp;
     char fastmap[512];
     re_init() { exp.fastmap=fastmap; exp.buffer=0; exp.allocated=0;
	      exp.translate=0; }
   } regexp;
  char *expr = "task";
  char *ch = re_compile_pattern( expr, strlen(expr), &regexp.exp );
  if ( ch )
     return 0;
  re_compile_fastmap( &regexp.exp );
  long ret = in->search( &regexp.exp, &regs, 0 );
#endif

  if ( in )
     delete in;
  if ( out )
     delete out;

#ifdef MALLOC_DEBUG
   malloc_shutdown();
#endif

  return 0;
}
