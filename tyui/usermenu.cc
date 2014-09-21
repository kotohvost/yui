/*
	$Id: usermenu.cc,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#include <stdio.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "usermenu.h"
#include "hashcode.h"

UserMenu::UserMenu( char *fName, Collection *Commands, int *current ) :
		listBox( Screen::center(Screen::Lines-Screen::Lines/3,50), 48, current ),
		fileName(fName), commands(Commands), allCommands(20,10),
		curCommand(current)
{
   title->put( "User menu", 0 );
   title->put( "Меню пользователя", 1 );
   status->put( "~Enter~-execute", 0 );
   status->put( "~Enter~-исполнить", 1 );
   fill();
   retCommand = HASH_cmExecCommands;
   setHelpContext( "UserMenu" );
}

long UserMenu::handleKey( long key, void *&ptr )
{
   key = listBox::handleKey( key, ptr );
   switch( key )
    {
      case HASH_cmOK:
	  {
	  char *com = getItem( *curCommand );
	  if ( *com == ' ' )
	     com++;
	  int i;
	  for ( i=0; i < allCommands.getCount(); i++ )
	     if ( !strcmp( com, (char*)allCommands.at(i) ) )
	       {
		 if ( com[0] == '>' )
		    retCommand = HASH_cmExecCommandsInWindow;
		 i++; break;
	       }
	  while( i < allCommands.getCount() )
	   {
	     char *s = (char*)allCommands.at(i);
	     if ( *s != ' ' && *s != '\t' )
		 break;
	     commands->insert( s );
	     allCommands.atRemove( i );
	   }
	  }
	  break;
    }
   return key;
}

void UserMenu::fill()
{
   FILE *f=fopen( fileName, "rt" );
   if ( !f )
      return;
   static char Buf[256]; Buf[255]=0;
   while( fgets( Buf, 254, f ) )
     {
       int len=strlen(Buf)+1;
       if ( len && Buf[len-2]=='\n' )
	  Buf[(--len)-1]=0;
       char *ch=new char[len];
       if ( !ch )
	  { fclose(f); return; }
       memcpy( ch, Buf, len );
       allCommands.insert( ch );
       if ( *ch && *ch != '\n' && *ch != ' ' && *ch != '\t' )
	{
	  if ( *ch != '>' )
	    { Buf[0] = ' '; strcpy( Buf+1, ch ); }
	  else
	     strcpy( Buf, ch );
	  add( Buf );
	}
     }
   fclose( f );
   setCurrent( *curCommand );
}
