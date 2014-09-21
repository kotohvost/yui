/*
	$Id: gdb.cc,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#include <stdio.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <program.h>
#include <dir.h>
#include "gdb.h"
#include "yui.h"
#include "hashcode.h"

Regexpr Debug::expBpoint;
Regexpr Debug::expLine;
//Regexpr Debug::expDelete;
char Debug::Buf[256];
Task *debug=0;

int Debug::flagInit=1;

int MemRects::compare( void *p1, void *p2 )
{
   return ((editRect*)p1)->e - ((editRect*)p2)->e;
}

Debug::Debug( Rect r ) :
	YTerm( (flagInit ? "gdb -f -q" : "gdb -f -q -n"), 0, r, Screen::Columns, Screen::Lines, DEBUG_LINES ),
	started(0), E(0), startCommands(10,10)
{
   firstHandle = 0;
   setHelpContext( "Debug" );
   title->put( (char*)getHelpContext(), 0 );
   title->put( "Отладчик", 1 );
   status->put( "~r~-run  ~s~-step  ~n~-next  ~u~-until  ~c~-continue", 0 );
   status->put( "~r~-старт  ~s~-шаг  ~n~-следующий  ~u~-пока  ~c~-продолжить", 1 );
   char *ch = expBpoint.compile( "^Breakpoint ([0-9]+) at .*: file (.*), line ([0-9]+)\\." );
   if ( ch )
     { strError = ch; return; }
   ch = expLine.compile( "^(\032\032)([^:]*):([0-9]+):.*" );
   if ( ch )
     { strError = ch; return; }
//   ch = expDelete.compile( "^(gdb) +d +([0-9]+)" );
//   if ( ch )
//     { Valid=0; return; }
   void *ptr=0;
   for( int i=0, end=appl->getCount(); i<end; i++ )
     {
       Window *w = ((winExecutor*)appl->at(i))->win;
       if ( w->handleKey( HASH_cmIsEdit, ptr ) )
	  continue;
       Edit *e = (Edit*)w;
       strcpy( Buf, e->title->get() );
       char *name = (char*)strrchr( Buf, FILESEP );
       if ( name ) name++;
       else name = Buf;
       int len = strlen( name );
       for( int j=0, k=e->Bpoint.getCount(); j < k; j++ )
	{
	  sprintf( name+len, ":%ld\n", ((BPinfo*)e->Bpoint.at(j))->line + 1 );
	  char *ch = new char[strlen(name)+3];
	  sprintf( ch, "b %s", name );
	  startCommands.insert( ch );
	}
     }
}

Debug::~Debug()
{
   Edit::debugEdit = 0;
   debug = 0;
   int i, end;
   for( i=0; i < memRects.getCount(); i++ )
     {
       editRect *er = (editRect*)memRects.at( i );
       er->e->setRect( er->r );
     }
   void *ptr=0;
   for( i=0, end=appl->getCount(); i<end; i++ )
     {
       Window *w = ((winExecutor*)appl->at(i))->win;
       if ( w->handleKey( HASH_cmIsEdit, ptr ) )
	  continue;
       Edit *e = (Edit*)w;
       for( int j=0, k=e->Bpoint.getCount(); j < k; j++ )
	  ((BPinfo*)e->Bpoint.at(j))->number = -1;
     }
}

int Debug::init( void *data )
{
   int ret = YTerm::init( data );
   static int sizeY=0;
   if ( sizeY != size.y-2 )
    {
      sizeY = size.y-2;
      sprintf( Buf, "\nset height %d\n", sizeY );
      writePty( Buf );
    }
   if ( !started )
     for( int i=0; i < startCommands.getCount(); i++ )
       writePty( (char*)startCommands.at(i) );
   started = 1;
   debug = Executor;
   return ret;
}

long Debug::handleKey( long key, void *&ptr )
{
   switch( key )
    {
       case HASH_cmIsDebug:
	   return 0;
       case HASH_cmCloseDebug:
	   return key;
       case HASH_cmSetBreakpoint:
	   setBreakpoint( (char*)ptr );
	   key=0; break;
       case HASH_cmUnsetBreakpoint:
	   unsetBreakpoint( *(int*)ptr );
	   key=0; break;
       default:
	   key = YTerm::handleKey( key, ptr );
    }
   return key;
}

void Debug::processStr( const char *str, int str_len )
{
   static char fileName[256];
   static char fileLine[15];
   static char bpointNum[15];
   static re_registers regs;
   if ( str_len < 6 || !started )
      return;

   Edit *e = 0;
   int flagRedraw=0, process=0;
   if ( expLine.search( str, str_len, 0, str_len, &regs ) >= 0 )
      process = 1;
   else if ( expBpoint.search( str, str_len, 0, str_len, &regs ) >= 0 )
      process = 2;
   if ( !process )
      return;

   int lenName = regs.end[2] - regs.start[2];
   memcpy( fileName, str + regs.start[2], lenName );
   fileName[lenName] = 0;
   char *shortName = (char*)strrchr( fileName, FILESEP );
   if ( process == 2 )
     {
       int lenNum = regs.end[1] - regs.start[1];
       memcpy( bpointNum, str+regs.start[1], lenNum );
       bpointNum[lenNum] = 0;
     }
   int lenLine = regs.end[3] - regs.start[3];
   memcpy( fileLine, str+regs.start[3], lenLine );
   fileLine[lenLine] = 0;
   ccIndex ind = appl->findWindow( HASH_cmShortNameCompare, shortName ? shortName+1 : fileName );
   if ( ind >= 0 && ind < appl->getCount() )
      e = (Edit*)((winExecutor*)appl->at(ind))->win;
   if ( !e )
     {
       e = new Edit( fileName, scrRect() );
       if ( !e ) return;
       if ( e->valid() )
	 { delete e; return; }
       e->changeMode();
       appl->insert( e );
       flagRedraw = 1;
     }
   if ( E != e )
     {
       appl->moveWin( Executor->index, e->Executor->index+1 );
       flagRedraw = 1;
       E = e;
     }
   long line = atol( fileLine );
   if ( process == 1 )
      e->DebugLine( line );
   else
      e->breakpoint( line - 1, atol(bpointNum) );
   Edit::debugEdit = e;
   if ( e->rect.b.y > rect.a.y )
     {
       editRect er( e, e->rect );
       if ( !memRects.search( er, ind ) )
	  memRects.insert( er );
       e->rect.b.y = rect.a.y;
       e->init();
     }
   if ( flagRedraw )
      redrawAppl();
   else
      { e->draw(); Screen::sync(); }
}

void Debug::setBreakpoint( char *str )
{
   if ( !str || !str[0] )
      return;
   sprintf( Buf, "b %s\n", str );
   writePty( Buf );
}

void Debug::unsetBreakpoint( int number )
{
   sprintf( Buf, "d %d\n", number );
   writePty( Buf );
}

void Edit::DebugLine( long line, int flag )
{
   if ( line < 1 || line > BOTT )
      return;
   if ( flag )
      debugLine = --line;
   if ( line < delta.y || line >= delta.y + size.y - 2 )
     {
       cursor.y = 2;
       delta.y = max( 0, line - cursor.y );
       if ( !delta.y )
	  cursor.y = line;
     }
   else
       cursor.y = line - delta.y;
}

int Edit::breakpoint( long line, int number )
{
   if ( line < 0 || line >= BOTT )
      return -1;
   ccIndex ind;
   BPinfo BP( line, -1 );
   if ( Bpoint.search( BP, ind ) )
     {
       if ( debug )
	 {
	   BPinfo *bp = (BPinfo*)Bpoint.at(ind);
	   if ( bp->number >= 0 )
	     {
	       static int num;
	       num = bp->number;
	       Task::sendMessage( debug, new KeyMessage( HASH_cmUnsetBreakpoint, &num ) );
	       Bpoint.atRemove( ind );
	     }
	   else
	       bp->number = number;
	 }
       else
	   Bpoint.atRemove( ind );
       return 0;
     }
   if ( number < 0 && debug )
     {
       static char buf[256];
       strcpy( buf, title->get() );
       char *name = (char*)strrchr( buf, FILESEP );
       if ( name ) name++;
       else name = buf;
       int len = strlen( name );
       sprintf( name+len, ":%ld", line+1 );
       Task::sendMessage( debug, new KeyMessage( HASH_cmSetBreakpoint, name ) );
     }
   else
     {
       BPinfo bp( line, number );
       Bpoint.insert( bp );
       DebugLine( line, 0 );
     }
   return 1;
}

void Edit::clearBpoints()
{
   while( Bpoint.getCount() > 0 )
    {
      BPinfo *bp = (BPinfo*)Bpoint.at(0);
      breakpoint( bp->line );
    }
}
