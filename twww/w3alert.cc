extern "C" {
#include <HTAlert.h>
}
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <program.h>
#include <dialog.h>
#include <listbox.h>
#include <screen.h>

#include "hashcode.h"

static Lang Yes( " Yes ", "  Да  " );
static Lang No( "  No  ", "  Нет  " );
static Lang Pass( "Password:", "Пароль:" );

extern "C" PUBLIC void HTAlert ARGS1(CONST char *, Msg)
{
   appl->test( (char*)Msg );
}

extern "C" PUBLIC void HTProgress ARGS1(CONST char *, Msg)
{
//    fprintf(stderr, "   %s ...\n", Msg);
}

extern "C" PUBLIC BOOL HTConfirm ARGS1(CONST char *, Msg)
{
//  long ret = appl->test( Task::getCurrent(), (char*)Msg, (char*)0, HASH_cmOK, Yes.get(), HASH_cmNo, No.get() );
  Window *w = (Window*)((winExecutor*)Task::getCurrent())->wins.at(0);
  long ret = w->test( (char*)Msg, (char*)0, HASH_cmOK, Yes.get(), HASH_cmNo, No.get() );
  return (ret == HASH_cmOK ? YES : NO);
}

extern "C" PUBLIC char * HTPrompt ARGS2(CONST char *, Msg, CONST char *, deflt)
{
    static Collection history;
//    char *str = appl->getString( Task::getCurrent(), (char*)Msg, Screen::Columns - 40, &history, (char*)deflt );
  Window *w = (Window*)((winExecutor*)Task::getCurrent())->wins.at(0);
  char *str = w->getString( (char*)Msg, Screen::Columns - 40, &history, (char*)deflt );
  if ( !str || !*str )
     return 0;
  char *ret = 0;
  StrAllocCopy( ret, str );
  return ret;
}

extern "C" PUBLIC char * HTPromptPassword ARGS1(CONST char *, Msg)
{
//    char *pw = appl->getPassword( Task::getCurrent(), (char*)(Msg ? Msg : Pass.get()) );
  Window *w = (Window*)((winExecutor*)Task::getCurrent())->wins.at(0);
  char *pw = w->getPassword( (char*)(Msg ? Msg : Pass.get()) );
  char *result = NULL;
  StrAllocCopy( result, pw ? pw : "" );
  return result;
}

extern Keymap InputKeyMap;

extern "C" PUBLIC BOOL HTPromptUsernameAndPassword ARGS3(CONST char *,	Msg,
					      char **,		username,
					      char **,		password)
{
//    if (Msg)
//	fprintf(stderr, "WWW: %s\n", Msg);
#if 0
  *username = HTPrompt( lang("User name:","Имя:"), *username );
  *password = HTPromptPassword( Pass.get() );
#else
  BOOL ret = NO;
  char *user = (char*)calloc( 1, 256 );
  char *pass = (char*)calloc( 1, 256 );
  Window *w = (Window*)((winExecutor*)Task::getCurrent())->wins.at(0);
  if ( !Msg )
     Msg = "Authorization required";
  int y=0, len=strlen( Msg ) + 4;
  if ( len >= Screen::Columns - 6 )
     len = Screen::Columns - 6;
  Rect r( Screen::center( 6, len ) );
  Dialog *d = new Dialog( r );
  for( const char *s=Msg; 1; y++ ) {
    if ( (int)strlen( s ) < len-2 ) {
       d->put( y++, 1, s );
       d->put( y++, 0, "" );
       break;
    }
    const char *ss = s + len - 3;
    for( ; ss != s && *ss != ' ' && *ss != '\t'; ss-- );
    if ( ss == s ) {
       d->put( y++, 1, s );
       break;
    }
    d->put( y, 1, "" );
    for( int x=1; s <= ss; s++, x++ )
       d->put( *s );
  }
  d->put( y, 2, "User ID:" );
  inputLine *il = new inputLine( Point( y++, 11 ), *username, user, len-14, 255 );
  il->keyHolder.add( &InputKeyMap );
  d->insert( il );

  d->put( y, 1, "Password:" );
  il = new inputLine( Point( y, 11 ), 0, pass, len-14, 255, 0, 0, inputLineFillChar, 1 );
  d->insert( il );

  r = Screen::center( y+3, len );
  d->setRect( r );

  if ( HASH_cmOK == appl->exec( w->Executor, d ) ) {
     *username = strdup( user );
     *password = strdup( pass );
     ret = YES;
  }
  free( user );
  free( pass );
  return ret;
#endif
}


