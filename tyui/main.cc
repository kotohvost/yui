/*
	$Id: main.cc,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#include <fcntl.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#if defined(bsdi) || defined(__FreeBSD__)
  #include <unistd.h>
#else
  #if defined(SOLARIS_SPARC) || defined(SOLARIS_X86)
     #include <stdlib.h>
  #else
     #include <getopt.h>
  #endif
#endif

#if defined(SOLARIS_X86) || defined(SOLARIS_SPARC) || defined(SINIX)
#include <sys/termio.h>
#endif

#if defined(AIX_PPC)
#include <sys/termio.h>
#include <strings.h>
#endif

#ifdef _USE_MMAP_
#include <signal.h>
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <menu.h>
#include <status.h>
#include <filebox.h>
#include <tag.h>

#ifndef DJGPP
#include <term.h>
#endif

#include <keybox.h>
#include <i_lines.h>

#ifdef _USE_WWW_
  #include <w3win.h>
#endif

#ifndef DJGPP
#include "gdb.h"
#endif

#include "yui.h"
#include "hashcode.h"
#include "appl.h"
#include "usermenu.h"
#include "calc.h"

#include "version.h"

#ifdef SINIX
extern "C" int strcasecmp( const char *s1, const char *s2 );
extern "C" int strncasecmp( const char *s1, const char *s2, size_t n );
#endif

static char *yui_path = NULL;
static char *name_status = NULL;
static char *name_options = NULL;
static char *name_options_common = NULL;
static char *name_colormap = NULL;
static char *name_colormap_common = NULL;
static char *name_colortrans = NULL;
static char *name_colortrans_common = NULL;
static char *name_keymap = NULL;
static char *name_keymap_common = NULL;
static char *name_macro = NULL;
static char *name_macro_common = NULL;
static char *startCtag = NULL;
Collection *startInfo=0;

static char buf1024[1024];

/*
int trans_flag_edit = TRANS_none;
int trans_flag_web = TRANS_none;
int trans_flag_term = TRANS_none;
int trans_flag_debug = TRANS_none;
*/

KeyHolder editKeyHolder;
KeyHolder dialogKeyHolder;
#ifndef DJGPP
KeyHolder ytermKeyHolder;
#endif

#define T_EDIT 		0
#define T_EDIT_MAN	1
#define T_EDIT_RO	2
#define T_EDIT_BIN	3
#define T_WEB		4
#define T_WEB_MAN	5

static InputKeyTask *inputKeyTask = NULL;

struct StartInfoStruct
{
  char type;
  char *name;
};

int returnStatus=0;

unsigned char copyright [] = {
#include "copyrigh.h"
};

Collection *makeCopyright()
{
   Collection *coll = new Collection(25,5);
   static char buf[256];
   char *strCod = "sprintf(fileIn,\"%i5\",num1,sqwo2)";
   int lines = copyright[0], pos=1, count=0;
   for( int i=0; i<lines; i++ )
     {
	int j=0, chars = copyright[pos++];
	for( ; j<chars; j++, pos++ )
	  {
	     if ( !strCod[count] )
		 count=0;
	     buf[j] = copyright[pos] + 256 - strCod[count++];
	  }
	buf[j]=0;
	int len = strlen( buf ) + 1;
	char *ch=new char[ len ];
	memcpy( ch, buf, len );
	coll->insert( ch );
     }
   return coll;
}

static void check_file( const char *user_file, const char *common_file )
{
	if ( !access( user_file, F_OK ) )
	    return;
	creat( user_file, S_IRUSR | S_IWUSR );
	if ( access( common_file, R_OK ) )
	    return;
	FILE *fsrc = fopen( common_file, "r" );
	FILE *fdst = fopen( user_file, "w+" );
	if ( fsrc && fdst ) {
	    int c=0;
	    while( (c=fgetc( fsrc )) != EOF ) {
		fputc( c, fdst );
	    }
	}
	fclose( fsrc );
	fclose( fdst );
}

ApplOptions::ApplOptions() : options(14),
	bg_symbol(backgroundSymbol), fill_char(inputLineFillChar),
	hist_limit(20), term_limit(100)
{
	lang = strdup( "english" );
	monitor = strdup( "color" );
#if defined(FreeBSD)
	term_base = strdup( "termcap" );
#else
	term_base = strdup( "terminfo" );
#endif
	shell = strdup( "/bin/sh" );
	trans_edit = strdup( "none" );
	trans_web = strdup( "none" );
	trans_term = strdup( "none" );
	trans_debug = strdup( "none" );
	/*url = strdup( "http://127.0.0.1" );*/
	url = strdup( "" );
	w3cache = strdup( "~/.yui/cache.web" );
}

ApplOptions::~ApplOptions()
{
	if ( lang ) ::free( lang );
	if ( monitor ) ::free( monitor );
	if ( term_base ) ::free( term_base );
	if ( shell ) ::free( shell );
	if ( trans_edit ) ::free( trans_edit );
	if ( trans_web ) ::free( trans_web );
	if ( trans_term ) ::free( trans_term );
	if ( trans_debug ) ::free( trans_debug );
	if ( url ) ::free( url );
	if ( w3cache ) ::free( w3cache );
}

ApplOptions Appl::aOpt;

static Collection historySystemCommand( 20, 5 );
static Collection historySystemWindow( 20, 5 );

Lang sorry( "Sorry, not implemented.", "Извините, не реализовано." );

#ifdef DJGPP
Lang sorry_dos( "Sorry, under DOS not implemented.", "Извините, для DOS не реализовано." );
#endif

long applKeyMap[] = {
	kbF3,		HASH_cmOpen,
//	kbF2,		HASH_cmSave,
	FUNC1(kbF3),	HASH_cmNew,
	kbCtrlSlash,	HASH_cmSystemCommand,
	FUNC1('.'),	HASH_cmSystemCommand,
	FUNC1('\\'),	HASH_cmSystemWindow,
	FUNC2('.'),	HASH_cmSystemWindow,
	kbCtrl_,	HASH_cmUserMenu,
	FUNC1(kbF10),	HASH_cmUserMenu,
	FUNC1('/'),	HASH_cmUserMenu,
	FUNC1(kbF5),	HASH_cmManPrompt,
	FUNC2(kbF5),	HASH_cmCtagPrompt,
	FUNC1(kbF7),	HASH_cmOpenTerm,
	FUNC1(kbF8),	HASH_cmOpenDebug,
	FUNC1(kbF9),	HASH_cmOpenWeb,
	FUNC2(kbCtrlP),	HASH_cmOpenCalc,
	FUNC2(kbCtrlX),	HASH_cmQuitError_1,
	0
};

Keymap ApplKeyMap( applKeyMap, new Lang("Hot keys for windows","Горячие клавиши для окон") );

/*
CommandDescr applCommandDescr[] = {
	{ HASH_cmQuitError_1,	new Lang("Quit without status saving and return error code","Выход без сохранения статуса с кодом ошибки") },
	{ HASH_cmOpen,		new Lang("Open existst file","Открыть существующий файл") },
	{ HASH_cmSave,		new Lang("Save current text","Сохранить текст") },
	{ HASH_cmNew,		new Lang("Open empty window","Открыть пустое окно") },
	{ HASH_cmSystemCommand,	new Lang("System command","Системная команда") },
	{ HASH_cmSystemWindow,	new Lang("Terminal window with command","Терминальное окно с командой") },
	{ HASH_cmUserMenu,	new Lang("User menu","Меню пользователя") },
	{ HASH_cmOpenTerm,	new Lang("Terminal window (pc3)","Терминальное окно (pc3)") },
	{ HASH_cmGetManWord,	new Lang("Manual pages","Страницы 'man'") },
	{ HASH_cmOpenDebug,	new Lang("Debug window","Окно отладчика") },
	{ HASH_cmOpenWeb,	new Lang("Web client window","Окно Web клиент") },
	{ HASH_cmOpenCalc,	new Lang("Ariphmetic calculator","Арифметический калькулятор") },
	{ 0, 0 }
};
*/

/*
#ifdef _USE_MMAP_
void reload_by_SIGSEGV( int signo )
{
   if ( !currentEdit )
     {
       fputs( "Internal error, immediately quit. Sorry.", stderr );
       exit(-1);
     }
   *currentEdit->validMap = 0;
   currentEdit->reload( 0 );
   currentEdit = 0;
}
#endif
*/

Appl::Appl()
{
   keyHolder.add( &ApplKeyMap/*, applCommandDescr*/ );
   editKeyHolder.add( &EditKeyMap/*, editCommandDescr*/ );
   dialogKeyHolder.add( &DialogKeyMap/*, dialogCommandDescr*/ );
#ifndef DJGPP
   ytermKeyHolder.add( &YTermKeyMap/*, ytermCommandDescr*/ );
#endif
   restOptions();
}

/*
Appl::~Appl()
{
}
*/

static char openFileName[512];
static char sysCommand[512];
static Collection sysCommandCollection(10,5);
Lang getStrStatus("Input command and press ~Enter~",
		  "Введите команду и нажмите ~Enter~");

long Appl::handleKey( long key, void *&ptr )
{
   long ret;
   switch( key )
    {
      case HASH_cmNew:
	  {
	  Edit *e = new Edit( "", scrRect() );
	  if ( !e->valid() )
	      nselect( insert( e ) );
	  else
	      delete e;
	  }
	  key=0; break;
      case HASH_cmOpen:
	  {
	  filebox *f = new filebox( filebox::filePath, filebox::fileMask, openFileName );
	  f->retCommand = HASH_cmOpenFile;
	  insertExecWindow( f );
	  }
	  key = 0; break;
      case HASH_cmOpenFile:
      case HASH_cmOpenFileNew:
      case HASH_cmOpenFileLink:
	  openEdit( openFileName, key );
	  key = 0; break;
      case HASH_cmSystemCommand:
	  {
	  char *s = 0;
	  if ( historySystemCommand.getCount() > 0 )
	     s = (char*)historySystemCommand.at(0);
	  getString( HASH_cmExecCommand, 0, Screen::Columns - 20, &historySystemCommand, s, sysCommand, lang("System command","Системная команда"), "SystemCommand", ' ', getStrStatus.get() );
	  }
	  key=0; break;
      case HASH_cmExecCommand:
	  {
	    int len = strlen( sysCommand ) + 1;
	    if ( len > 1 )
	      {
		char *com=new char[ len ];
		memcpy( com, sysCommand, len );
		sysCommandCollection.insert( com );
		systemCommand( &sysCommandCollection );
	      }
	  }
	  key=0; break;
      case HASH_cmExecCommands:
	  systemCommand( &sysCommandCollection );
	  key=0; break;
      case HASH_cmExecCommandsInWindow:
	  systemCommand( &sysCommandCollection, 1 );
	  key=0; break;
      case HASH_cmSystemWindow:
	  {
	  char *s = 0;
	  if ( historySystemWindow.getCount() > 0 )
	     s = (char*)historySystemWindow.at(0);
	  getString( HASH_cmExecSysWindow, 0, Screen::Columns - 20, &historySystemWindow, s, sysCommand, lang("Terminal with command","Терминал с командой"), "SystemWindow", ' ', getStrStatus.get() );
	  }
	  key=0; break;
      case HASH_cmOpenCalc:
	  {
	  Calculator *calc = new Calculator;
	  nselect( insert( calc ) );
	  }
	  key=0; break;
      case HASH_cmExecSysWindow:
#ifdef DJGPP
	  test( sorry_dos.get() );
#else
	  if ( strlen( sysCommand ) > 0 )
	    {
	      Rect tr = scrRect();
	      if ( !borderVisible ) tr.a.y--;
	      YTerm *yt=new YTerm( sysCommand, 0, tr,
				tr.b.x-tr.a.x-2, tr.b.y-tr.a.y-2,
				aOpt.term_limit );
	      yt->flagFinishMessage = 1;
	      nselect( insert( yt ) );
	    }
#endif
	  key=0; break;
      case HASH_cmOpenTerm:
#ifdef DJGPP
	  test( sorry_dos.get() );
#else
	  {
	  Rect tr = scrRect();
	  if ( !borderVisible ) tr.a.y--;
	  nselect( insert( new YTerm( aOpt.shell, 0, tr,
				tr.b.x-tr.a.x-2, tr.b.y-tr.a.y-2,
				aOpt.term_limit ) ) );
	  }
#endif
	  key=0; break;
      case HASH_cmManPrompt:
#ifdef DJGPP
	  test( sorry_dos.get() );
#else
	  getManWord();
#endif
	  key=0; break;
      case HASH_cmOpenMan:
#ifdef DJGPP
	  test( sorry_dos.get() );
#else
	  manualPage( (char*)getStringBuf );
#endif
	  key=0; break;
      case HASH_cmOpenDebug:
#ifdef DJGPP
	  test( sorry_dos.get() );
#else
	  openDebug();
	  redraw();
#endif
	  key=0; break;
      case HASH_cmGdbInitFile:
#ifdef DJGPP
	  test( sorry_dos.get() );
#else
	  gdbInitFile();
#endif
	  key=0; break;
      case HASH_cmUserMenu:
	  userMenu();
	  key=0; break;
      case HASH_cmOpenWeb:
#if defined(_USE_WWW_)
	  nselect( insert( new W3Win( aOpt.url, scrRect() ) ) );
#elif !defined(DJGPP)
	  test( sorry.get() );
#else
	  test( sorry_dos.get() );
#endif
	  key=0; break;
      case HASH_cmFindString:
	  find( 0 );
	  key=0; break;
      case HASH_cmFindStringNext:
	  find( 1 );
	  key=0; break;
      case HASH_cmReplaceString:
	  replace();
	  key=0; break;
#ifndef DJGPP
      case HASH_cmManual:
	  manualPage();
	  key=0; break;
#endif
      case HASH_cmOpenShare:
	  openShare();
	  key=0; break;
#if 0
      case HASH_cmGlobalOptions:
	  boxOptions();
	  key=0; break;
      case HASH_cmAcceptOptions:
	  {
	  int refreshFlag = strcmp( aOpt.fgMap, _aOpt.fgMap ) ||
			    strcmp( aOpt.bgMap, _aOpt.bgMap ) ? 1 : 0;
	  memcpy( &aOpt, &_aOpt, sizeof(ApplOptions) );
	  memcpy( &Edit::eOpt, &_eOpt, sizeof(EditOptions) );
	  if ( colorsType != aOpt.color ) {
	       colorsType = aOpt.color;
	       setColors( colorsType );
	  }
	  Edit::stepTab = (1 << (Edit::eOpt.tab+1)) - 1;
	  if ( Edit::eOpt.lineLength < 10 ) Edit::eOpt.lineLength = 10;
	  if ( Edit::eOpt.timeSave < 10 ) Edit::eOpt.timeSave = 10;
	  backgroundSymbol = aOpt.backgrSymbol;
	  inputLineFillChar = aOpt.fillChar;
	  language = aOpt.lang;
	  menuMgr->Visible = MENU_BAR(aOpt.options) ? 1 : 0;
	  menuMgr->init();
	  statusLine->visible = STATUS_BAR(aOpt.options) ? 1 : 0;
	  borderVisible = WINDOW_BORDER(aOpt.options) ? 1 : 0;
	  Term::initcolor( aOpt.fgMap, aOpt.bgMap );
	  void *ptr=0;
	  for( int i=0; i<count; i++ )
	    {
	      Window *w = getWin(i);
	      if ( w->handleKey( HASH_cmIsEdit, ptr ) )
		 continue;
	      Timer *timer = ((Edit*)w)->saveTimer;
	      if ( AUTOSAVE(Edit::eOpt.options) ) {
		  timer->setTime( Edit::eOpt.timeSave );
		  timer->open();
	      } else {
		  timer->close();
	      }
	    }
	  if ( refreshFlag )
	     Screen::Clear( FG_WHITE | BG_BLACK );
	  }
	  redraw();
	  key=0; break;
#else
	case HASH_cmModifyOptions:
	    check_file( name_options, name_options_common );
	    openEdit( name_options, HASH_cmOpenFileNew );
	    key=0; break;
	case HASH_cmModifyColormap:
	    check_file( name_colormap, name_colormap_common );
	    openEdit( name_colormap, HASH_cmOpenFileNew );
	    key=0; break;
	case HASH_cmModifyColortrans:
	    check_file( name_colortrans, name_colortrans_common );
	    openEdit( name_colortrans, HASH_cmOpenFileNew );
	    key=0; break;
	case HASH_cmModifyKeymap:
	    check_file( name_keymap, name_keymap_common );
	    openEdit( name_keymap, HASH_cmOpenFileNew );
	    key=0; break;
	case HASH_cmModifyMacro:
	    check_file( name_macro, name_macro_common );
	    openEdit( name_macro, HASH_cmOpenFileNew );
	    key=0; break;
	case HASH_cmReloadOptions:
	    readOptions();
	    Screen::Clear( FG_WHITE | BG_BLACK );
	    redraw();
	    key=0; break;
	case HASH_cmReloadColormap:
	    readColormap();
	    Screen::Clear( FG_WHITE | BG_BLACK );
	    redraw();
	    key=0; break;
	case HASH_cmReloadColortrans:
	    readColortrans();
	    Screen::Clear( FG_WHITE | BG_BLACK );
	    redraw();
	    key=0; break;
	case HASH_cmReloadKeymap:
	    readKeymap();
	    key=0; break;
	case HASH_cmReloadMacro:
	    readMacro();
	    key=0; break;
#endif

#if 0
      case HASH_cmFormatOptions:
	  boxFormat();
	  key=0; break;
      case HASH_cmAcceptFormat:
	  memcpy( &dataFmt, &DataFmt, sizeof(dataFormat) );
	  key=0; break;
      case HASH_cmSaveKeymap:
      case HASH_cmSaveOptions:
	  saveOptions();
	  key=0; break;
#endif

      case HASH_cmCloseDebug:
#ifndef DJGPP
	  if ( debug )
	     closeWin( findWindow( HASH_cmIsDebug ) );
#endif
	  key=0; break;
      case HASH_cmClearBpointAll:
#ifndef DJGPP
	  {
	  void *ptr=0;
	  for( int i=0; i<count; i++ )
	   {
	     Window *w = ((winExecutor*)items[i])->win;
	     if ( !w->handleKey( HASH_cmIsEdit, ptr ) )
		((Edit*)w)->clearBpoints();
	   }
	  }
	  redraw();
#endif
	  key=0; break;
      case HASH_cmKeymapEdit:
	  insertExecWindow( new KeyBox( &editKeyHolder, EditMenu ) );
	  key=0; break;
      case HASH_cmKeymapDialog:
	  insertExecWindow( new KeyBox( &dialogKeyHolder, 0 ) );
	  key=0; break;
      case HASH_cmKeymapTerm:
#ifndef DJGPP
	  insertExecWindow( new KeyBox( &ytermKeyHolder, 0 ) );
#else
	  test( sorry_dos.get() );
#endif
	  key=0; break;
      case HASH_cmAbout:
	  {
	  Collection *coll = makeCopyright();
	  sprintf( buf1024,"Version %s, %s", yui_version, __DATE__ );
	  char *s = new char[ 27 ];
	  memset( s, '-', 26 ); s[26]=0;
	  coll->atInsert( 0, s );
	  s=new char[ strlen(buf1024)+1 ];
	  strcpy( s, buf1024 );
	  coll->atInsert( 0, s );
	  Edit *e = new Edit( coll, "Copyright", scrRect(), new Lang(" Copyright, more information"," Информация о программе") );
	  e->readOnlyMode = 3;
	  insertExecWindow( e );
	  delete coll;
	  }
	  key=0; break;
      case HASH_cmQuitError_1:
	  returnStatus = 1;
	  sendMessage( this, new BackMessage( HASH_cmQuit ) );
	  key=0; break;
      case HASH_cmCtagPrompt:
	  getCtagWord();
	  key=0; break;
      case HASH_cmCtagGo:
	  {
	  if ( count < 1 )
	      break;
	  Window *w = ((winExecutor*)items[current])->win;
	  void *ptr=0;
	  if ( w->handleKey( HASH_cmIsEdit, ptr ) )
	      break;
	  char *ctag = ((Edit*)w)->currentWord();
	  if ( !ctag[0] )
	      break;
	  strcpy( getStringBuf, ctag );
	  }
      case HASH_cmCtagGoInternal:
	  findCtag( getStringBuf );
	  if ( started )
	      redraw();
	  key=0; break;
    }
   return key;
}

void Appl::openEdit( char *name, long key )
{
   Edit *e = 0;
   void *ptr = 0;
   int i;
   for( i=0; i<count; i++ ) {
       Window *w = (Window*)((winExecutor*)items[i])->wins.at(0);
       if ( w->handleKey( HASH_cmIsEdit, ptr ) || w->handleKey( HASH_cmNameCompare, (void*&)name ) )
	  continue;
       e = (Edit*)w;
       break;
   }
   switch( key ) {
      case HASH_cmOpenFile:
	  if ( e ) {
	      nselect( i );
	      sendMessage( e->Executor, new KeyMessage( HASH_cmLink ) );
	      return;
	  }
      case HASH_cmOpenFileNew:
	  e = new Edit( name, scrRect() );
	  break;
      case HASH_cmOpenFileLink:
	  e = new Edit( e, scrRect() );
	  break;
   }

   if ( !e->valid() )
      nselect( insert( e ) );
   else
      delete e;
}

void Appl::systemCommand( Collection *commands, int type )
{
   static char bufCommands[512];

   if ( !commands || commands->getCount() < 1 )
      return;
#ifndef DJGPP
   if ( type==1 )		// exec in window
     {
       bufCommands[0] = 0;
       for( int i=0, j=0; i<commands->getCount(); i++ )
	{
	  char *com = (char*)commands->at(i);
	  for( ; *com==' ' || *com=='\t'; com++ );
	  if ( *com == '#' )
	     continue;
	  if ( j > 0 )
	     strcat( bufCommands, ";" );
	  j += strlen( com ) + 1;
	  if ( j > 512 )
	     break;
	  strcat( bufCommands, com );
	}
       commands->freeAll();
       if ( strlen( bufCommands ) > 0 )
	{
	  Rect tr = scrRect();
	  if ( !borderVisible ) tr.a.y--;
	  YTerm *yt=new YTerm( bufCommands, 0, tr,
			tr.b.x-tr.a.x-2, tr.b.y-tr.a.y-2,
			aOpt.term_limit );
	  yt->flagFinishMessage = 1;
	  nselect( insert( yt ) );
	}
       return;
     }
#endif
   Screen::Clear( FG_WHITE | BG_BLACK );
   Screen::move( Screen::Lines-2, 0 );
   Screen::sync();
   Screen::Close();

   int i, j, k, end;
   for( i=0, end=commands->getCount(); i<end; i++ )
    {
      char *com=(char*)commands->at(i);
      puts( "" );
      puts( com );
      for( j=0; com[j]==' ' || com[j]=='\t'; j++ );
      if ( com[j]=='#' )
       {
	 for( j++; com[j]==' ' || com[j]=='\t'; j++ );
	 if ( !memcmp( com+j, "reload", 6 ) && (com[j+6]==' ' || com[j+6]=='\t') )
	  {
	    for( j+=7; com[j]==' ' || com[j]=='\t'; j++ );
	    for( k=0; com[j] && com[j]!=' ' && com[j]!='\t' && com[j]!='\r' && com[j]!='\n'; buf1024[k++]=com[j++] );
	    if ( !k )
	       continue;
	    buf1024[k]=0;
	    static IdentMessage imsg( HASH_cmNameCompare );
	    imsg.ptr = buf1024;
	    Task *executor = 0;
	    if ( sendAll(imsg) == 1 && (executor=findTask(imsg.getReceiver())) )
	      {
		 long ind = indexOf( executor );
		 if ( ind >= 0 && ind < count )
		  {
		    nselect( ind );
		    ((Edit*)((winExecutor*)executor)->win)->reload( 0 );
		    continue;
		  }
	      }
	    Edit *e=new Edit( buf1024, scrRect() );
	    if ( e )
	      {
		if ( !e->valid() )
		    nselect( insert( e ) );
		else
		    delete e;
	      }
	  }
       }
      else
       {
	  system( com+j );
	  fflush( stdout );
       }
    }
   commands->freeAll();
   printf( "Press any key to return to YUI ...\7" );
   fflush( stdout );

#if defined(bsdi) || defined(__FreeBSD__)
   struct termios ti, TI;
   ioctl( 0, TIOCGETA, &TI );
#else
   struct termio ti, TI;
   ioctl( 0, TCGETA, &TI );
#endif
   ti = TI;
   ti.c_iflag = ti.c_lflag = ti.c_oflag = 0;
#if !defined(bsdi) && !defined(__FreeBSD__)
   ti.c_line = 0;
#endif
   ti.c_cc[VMIN]=1;
   ti.c_cc[VTIME]=0;

#if defined(bsdi) || defined(__FreeBSD__)
   ioctl( 0, TIOCSETA, &ti );
#else
   ioctl( 0, TCSETA, &ti );
#endif
   int ch;
   read( 0, &ch, 1 );
#if defined(bsdi) || defined(__FreeBSD__)
   ioctl( 0, TIOCSETA, &TI );
#else
   ioctl( 0, TCSETA, &TI );
#endif
   Screen::Open();

   Screen::Clear( FG_WHITE | BG_BLACK );
   redraw();
}

void Appl::userMenu()
{
   char *name = "yui.mnu";
   strcpy( buf1024, name );
   if ( access( buf1024, F_OK ) )
    {
#ifndef DJGPP
      char *home = getenv("HOME");
#else
      char *home = StartPath();
#endif
      strcpy( buf1024, home );
      strcat( buf1024, FILESEPSTR );
      strcat( buf1024, name );
      if ( access( buf1024, F_OK ) )
	{ test( lang( "File 'yui.mnu' not found.","Файл 'yui.mnu' не найден." ) ); return; }
    }
   static int curList=0;
   insertExecWindow( new UserMenu( buf1024, &sysCommandCollection, &curList ) );
}

void Appl::find( int offset )
{
   Window *w = winStack.getCount() > 0 ? (Window*)((winExecutor*)winStack.at(0))->wins.at(0) : 0;
   int flag = 0;
   if ( !w && count > 0 )
     {
       flag = 1;
       w = getWin(current);
     }
   if ( !w )
      return;

   void *ptr = 0;
   Edit *e = w->handleKey( HASH_cmIsEdit, ptr ) ? 0 : (Edit*)w;
   if ( !e )
      return;

   int found = 0, cur = current;
   for( int i=cur, end=count; i<end; i++ )
     {
       if ( flag )
	  w = getWin(i);
       if ( !w->handleKey( HASH_cmIsEdit, ptr ) )
	 {
	   e = (Edit*)w;
	   if ( flag && i != cur )
	     { nselect( i ); redraw(); }
	   found = e->find( offset, 1, i==cur ? 0 : 1 );
	   if ( !flag || found>0 || found<0 || !(Edit::dFindRepl.options & FIND_ALL_WIN) )
	      break;
	 }
       if ( i+1 == count )
	 { i=-1; end=cur; }
     }

   if ( !found )
     {
       if ( flag )
	  nselect( cur );
       sprintf( Edit::Buf1, lang("String '%s' not found.","Строка '%s' не найдена."), Edit::dFindRepl.strFind );
       test( Edit::Buf1 );
     }
   else if ( e )
    {
      if ( found > 0 )
	 e->find_Info.found = 1;
      e->draw();
      e->drawStatus();
      Window::MoveCursor();
      Screen::sync();
    }
}

void Appl::replace()
{
   Window *w = winStack.getCount() > 0 ? (Window*)((winExecutor*)winStack.at(0))->wins.at(0) : 0;
   int flag = 0;
   if ( !w && count > 0 )
     {
       flag = 1;
       w = getWin(current);
     }
   if ( !w )
      return;

   void *ptr = 0;
   Edit *e = w->handleKey( HASH_cmIsEdit, ptr ) ? 0 : (Edit*)w;
   if ( !e )
      return;

   int cur = current;
   unsigned countRepl=0;
   for( int i=cur, end=count; i<end; i++ )
     {
       if ( flag )
	  w = getWin(i);
       if ( !w->handleKey( HASH_cmIsEdit, ptr ) )
	 {
	   e = (Edit*)w;
	   if ( flag && i != cur )
	     { nselect( i ); redraw(); }
	   if ( !e->replace( (i==cur ? 0 : 1), &countRepl ) || !flag ||
		!(Edit::dFindRepl.options & FIND_ALL_WIN) )
	     break;
	 }
       if ( i+1 == count )
	 { i=-1; end=cur; }
     }

   if ( flag )
      nselect( cur );
   sprintf( Edit::Buf1, lang("Made %d changes.","Выполнено %d замен."), countRepl );
   test( Edit::Buf1 );
   e->drawStatus();
   e->draw();
   Screen::sync();
}

int saveHistory( Collection *hist, FILE *f )
{
   int end = min( Appl::aOpt.hist_limit, hist->getCount() );
   if ( !fwrite( &end, sizeof(int), 1, f ) )
      return 0;
   for( int i=0; i < end; i++ )
     if ( fputs( (char*)hist->at(i), f ) < 0 || fputs("\n",f) < 0 )
	return 0;
   return 1;
}

int readHistory( Collection *hist, FILE *f )
{
   buf1024[255] = 0;
   int num;
   if ( fread( &num, sizeof(int), 1, f ) != 1 )
     return 0;
   for( int i=0; i<num && fgets( buf1024, 255, f ); i++ )
     {
       int len = strlen( buf1024 );
       if ( buf1024[len-1] == '\n' )
	  buf1024[--len] = 0;
       char *ch = new char[len+1];
       memcpy( ch, buf1024, len+1 );
       hist->insert( ch );
     }
   return 1;
}

int Appl::saveStatus()
{
   static IdentMessage imsg( HASH_cmIsEdit );

   if ( returnStatus || !STATUS_FILE(aOpt.options) )
      return 0;
   if ( sendAll(imsg) != 1 && historySystemCommand.getCount() < 1 &&
	historySystemWindow.getCount() < 1 &&
	filebox::fileHistory.getCount()<1 &&
	Edit::histFind.getCount()<1 &&
	Edit::histRepl.getCount()<1
#ifdef _USE_WWW_
	 && W3Win::historyUrl.getCount()<1
#endif
      )
     { unlink( name_status ); return 0; }
   int desc = open( name_status, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );
   FILE *f = fdopen( desc, "wb" );
   if ( !f )
      return 0;
   statusYui bufstat;
   void *ptr = 0;
   int i, j, end;
   Window *w=0; Edit *e=0;
   Rect sr( scrRect() );

   if ( !fwrite( &count, sizeof(int), 1, f ) )
      goto fileError;

   if ( !fwrite( yui_version, 5, 1, f ) )
      goto fileError;

   if ( !fwrite( &sr, sizeof(Rect), 1, f ) )
      goto fileError;

   for( i=0, j=0; i<count; i++ )
     {
       w = ((winExecutor*)items[i])->win;
       if ( w->handleKey( HASH_cmIsEdit, ptr ) )
	  continue;
       j++;
       e = (Edit*)w;
       strcpy( bufstat.name, e->winName() );
       bufstat.delta	=	e->delta;
       bufstat.cursor	=	e->cursor;
       bufstat.rect	=	e->rect;
       bufstat._flags	=	e->_flags;
       bufstat.active	=	(current == i);
       bufstat.readOnly	=	e->readOnlyMode;
       bufstat.trans_flag =	e->trans_flag;
       if ( !fwrite( &bufstat, sizeof( statusYui ), 1, f ) )
	   goto fileError;
     }

   if ( !saveHistory( &historySystemCommand, f ) )
      goto fileError;
   if ( !saveHistory( &historySystemWindow, f ) )
      goto fileError;
   if ( !saveHistory( &filebox::fileHistory, f ) )
      goto fileError;
   if ( !saveHistory( &Edit::histFind, f ) )
      goto fileError;
   if ( !saveHistory( &Edit::histRepl, f ) )
      goto fileError;

#ifdef _USE_WWW_
   if ( !saveHistory( &W3Win::historyUrl, f ) )
      goto fileError;
#endif

   fseek( f, SEEK_SET, 0 );
   if ( !fwrite( &j, sizeof(int), 1, f ) )
      goto fileError;

   fsync( desc );
   fclose( f );
   return 1;

fileError:
   fclose( f );
   unlink( name_status );
   return 0;
}

int Appl::restStatus()
{
  int i, binary=0, startCount=-1;
  Window *w;
  struct StartInfoStruct *sinfo = 0;
  long command;
  for( i=0; i < startInfo->getCount(); i++, startCount++ ) {
      w = 0;
      command = 0;
      binary = 0;
      sinfo = (StartInfoStruct*)startInfo->at( i );
      switch( sinfo->type ) {
	 case T_WEB_MAN:
	     sprintf( buf1024, "man:%s", sinfo->name );
	     goto create_web;
	 case T_WEB:
	     strcpy( buf1024, sinfo->name );
create_web:
#if defined(_USE_WWW_)
	     w = new W3Win( buf1024, scrRect() );
#elif !defined(DJGPP)
	     test( sorry.get() );
#else
	     test( sorry_dos.get() );
#endif
	     break;
	 case T_EDIT_MAN:
	     command = HASH_cmShowMan;
	     goto create_edit;
	 case T_EDIT_RO:
	     command = HASH_cmMode;
	     goto create_edit;
	 case T_EDIT_BIN:
	     binary = 1;
	     goto create_edit;
	 case T_EDIT:
create_edit:
	     w = new Edit( sinfo->name, scrRect(), (Lang*)0, binary );
	     switch( command ) {
		case HASH_cmShowMan:
		    ((Edit*)w)->showManMode( 0 );
		    break;
		case HASH_cmMode:
		    ((Edit*)w)->changeMode( 0 );
		    break;
	     }
	     break;
      }
      if ( !w )
	  continue;
      if ( w->valid() ) {
	  delete w; continue;
      }
      insert( w );
  }

  for( i=startInfo->getCount()-1; i >= 0; i -- )
     ::free( ((StartInfoStruct*)startInfo->at(i))->name );
  delete startInfo;

  if ( startCount > 0 )
     nselect( startCount-1 );

  FILE *f = fopen( name_status, "rb" );
  if ( f ) {
	  static statusYui bufstat;
	  int cur=-1, num=0;
	  char yui_ver[6];
	  Rect sr, srect( scrRect() );

	  if ( fread( &num, sizeof(int), 1, f ) != 1 )
	    { fclose(f); return !(started=1); }

	  if ( fread( yui_ver, 5, 1, f ) != 1 )
	    { fclose(f); return !(started=1); }
	  yui_ver[5] = 0;
#if 0
	  if ( strcmp( yui_ver, yui_version ) )
	    { fclose(f); return !(started=1); }
#endif
	  if ( fread( &sr, sizeof(Rect), 1, f ) != 1 )
	    { fclose(f); return !(started=1); }

	  for( i=0; i<num && fread( &bufstat, sizeof( statusYui ), 1, f )==1; i++ ) {
	     if ( startCount >= 0 )
		continue;
	     if ( bufstat.rect == sr ) {
		bufstat.rect = scrRect();
	     } else if ( bufstat.rect.a.x == sr.a.x && bufstat.rect.b.x == sr.b.x ) {
		bufstat.rect.a.x = srect.a.x;
		bufstat.rect.b.x = srect.b.x;
	     } else if ( bufstat.rect.a.y == sr.a.y && bufstat.rect.b.y == sr.b.y ) {
		bufstat.rect.a.y = srect.a.y;
		bufstat.rect.b.y = srect.b.y;
	     }
	     Edit *e = new Edit( bufstat.name, bufstat.rect );
	     if ( e->valid() )
		{ delete e; continue; }
	     e->delta	=	bufstat.delta;
	     e->cursor	=	bufstat.cursor;
	     e->_flags	=	bufstat._flags;
	     e->_flags &= ~0x00f;	/* clear block marker */
	     e->setChange(0);
	     if ( !e->readOnlyMode )
		e->readOnlyMode = bufstat.readOnly;
	     e->trans_flag = bufstat.trans_flag;
	     int c = nselect( insert( e ) );
	     if ( bufstat.active )
		cur = c;
	  }
	  readHistory( &historySystemCommand, f );
	  readHistory( &historySystemWindow, f );
	  readHistory( &filebox::fileHistory, f );
	  readHistory( &Edit::histFind, f );
	  readHistory( &Edit::histRepl, f );

#ifdef _USE_WWW_
	  readHistory( &W3Win::historyUrl, f );
#endif

	  fclose(f);
	  if ( cur >= 0 && startCount < 0 )
	      nselect( cur );
  }

  findCtag( startCtag );

  return !(started=1);
}

void Appl::findCtag( char *ctag )
{
  FILE *f = NULL;
  if ( !ctag || !(f=fopen( "tags", "r" )) ) {
      if ( ctag )
	  test( "Cannot open 'tags' file" );
      return;
  }

  char *s=NULL, *sf=NULL;
  int line=0;
  Edit *e = NULL;

  while( fgets( buf1024, 1023, f ) > 0 ) {
      buf1024[1023] = 0;
      if ( buf1024[0] == 0x1b || !(s=strtok( buf1024, "\t" )) || strcmp( ctag, s ) )
	  continue;
      if ( !(s=strtok( NULL, "\t" )) )
	  continue;

      if ( !(sf=strtok( NULL, "/" )) ) {
	  break;
      }

      if ( *sf != '^' ) {
	  if ( !(sf = strtok( sf, ";" )) )
	      break;
	  line = strtol( sf, NULL, 10 );
      }

      void *ptr = NULL;

      char *buf = (char*)calloc( MAXPATHLEN, 1 );
      sprintf( buf, "%s/%s", CurWorkPath(), s );

      for( int i=0; i<count; i++ ) {
	  Window *w = (Window*)((winExecutor*)items[i])->wins.at(0);
	  if ( w->handleKey( HASH_cmIsEdit, ptr ) || w->handleKey( HASH_cmNameCompare, (void*&)buf ) )
	      continue;
	  e = (Edit*)w;
	  break;
      }
      if ( e == NULL ) {
	  e = new Edit( buf, scrRect() );
	  insert( e );
      }

      ::free( buf );

      if ( e ) {
	  dataFindRepl df;
	  memcpy( &df, &Edit::dFindRepl, sizeof( struct dataFindRepl ) );
	  strcpy( Edit::dFindRepl.strFind, sf );
	  s = Edit::dFindRepl.strFind;
	  for( ; *sf; sf++ ) {
	      switch( *sf ) {
		  case '\\':
		  case '+':
		  case '.':
		  case '*':
		  case '?':
		  case '[':
		  case '(':
		  case ')':
		  case '|':
		      *s++='\\';
		      break;
	      }
	      *s++ = *sf;
	  }
	  *s = 0;
	  Edit::dFindRepl.options = FIND_REGEXPR;
	  Edit::dFindRepl.direction = 0;
	  Edit::dFindRepl.where = 0;
	  if ( line > 0 ) {
	      e->gotoLine( line, 0 );
	      e->moveHome( 0 );
	      select( e );
	  } else if ( e->find( 0, 0, 1, 0 ) ) {
	      e->find_Info.found = 0;
	      select( e );
	  }
	  memcpy( &Edit::dFindRepl, &df, sizeof( struct dataFindRepl ) );
	  break;
      }
  }

  if ( e == NULL ) {
      sprintf( buf1024, lang( "C-tag '%s' not found", "C-тзг '%s' не найден"), ctag ? ctag : "(null)" );
      test( buf1024 );
  }
  fclose( f );
}

static void set_opt_bool( const char *buf2, int shift, int *val_int )
{
	if ( !strcasecmp( buf2, "on" ) )
	    *val_int |= 1 << shift;		// set
	else if ( !strcasecmp( buf2, "off" ) )
	    *val_int &= ~(1 << shift);		// reset
}

static void set_opt_num( const char *buf2, int *val_int )
{
	int i = strtol( buf2, (char **)NULL, 10 );
	if ( i > 0 )
	    *val_int = i;
}

static void set_opt_str( const char *buf2, char **val_ptr )
{
	if ( *val_ptr )
	    ::free( *val_ptr );
	*val_ptr = strdup( buf2 );
}

#define OPT_BOOL	0
#define OPT_NUM		1
#define OPT_STR		2

#define KEY_APPL	0
#define KEY_EDIT	1
#define KEY_DIALOG	2
#define KEY_TERM	3
#define KEY_WEB		4

struct OPT_info {
	char *name;
	unsigned inited:1;
	unsigned type:2;	/* OPT_BOOL, OPT_NUM, OPT_STR */
	int n;
	int *val_int;
	char **val_ptr;
};

struct COM_info {
	char *name;
	unsigned inited:1;
	unsigned type:3;	/* KEY_APPL, KEY_EDIT, ... */
	long command;
};

struct KEY_info {
	char *name;
	long key;
};

struct COLOR_info {
	char *name;
	unsigned inited:1;
	unsigned char *attr;
	char *fg;
	char *bg;
};

static int len_opt_info=0;
static int len_key_info=0;
static int len_com_info=0;
static int len_color_info=0;

static struct OPT_info opt_info[] = {
	{ "auto_wrap",		0,	OPT_BOOL,	3,	&Edit::eOpt.options,	0 },
	{ "autosave",		0,	OPT_BOOL,	2,	&Edit::eOpt.options,	0 },
	{ "autosave_time",	0,	OPT_NUM,	-1,	&Edit::eOpt.timeSave,	0 },
	{ "background_symbol",	0,	OPT_NUM,	-1,	&Appl::aOpt.bg_symbol,	0 },
	{ "backup_file",	0,	OPT_BOOL,	0,	&Edit::eOpt.options,	0 },
	{ "expand_string",	0,	OPT_BOOL,	5,	&Edit::eOpt.options,	0 },
	{ "gdb_autoload",	0,	OPT_BOOL,	4,	&Appl::aOpt.options,	0 },
	{ "highlight_syntax",	0,	OPT_BOOL,	8,	&Edit::eOpt.options,	0 },
	{ "home_url",		0,	OPT_STR,	-1,	0, &Appl::aOpt.url },
	{ "inputline_fill_char",0,	OPT_NUM,	-1,	&Appl::aOpt.fill_char,	0 },
	{ "inputline_history",	0,	OPT_NUM,	-1,	&Appl::aOpt.hist_limit,	0 },
	{ "language",		0,	OPT_STR,	-1,	0, &Appl::aOpt.lang },
	{ "line_length",	0,	OPT_NUM,	-1,	&Edit::eOpt.lineLength,	0 },
	{ "menu_bar",		0,	OPT_BOOL,	2,	&Appl::aOpt.options,	0 },
	{ "monitor",		0,	OPT_STR,	-1,	0, &Appl::aOpt.monitor },
	{ "optimal_fill",	0,	OPT_BOOL,	7,	&Edit::eOpt.options,	0 },
	{ "paragraph_offset",	0,	OPT_NUM,	-1,	&Edit::eOpt.parOffset,	0 },
	{ "status_bar",		0,	OPT_BOOL,	1,	&Appl::aOpt.options,	0 },
	{ "status_file",	0,	OPT_BOOL,	0,	&Appl::aOpt.options,	0 },
	{ "strip_spaces",	0,	OPT_BOOL,	6,	&Edit::eOpt.options,	0 },
	{ "tabulation",		0,	OPT_NUM,	-1,	&Edit::eOpt.tab,	0 },
	{ "terminal_history",	0,	OPT_NUM,	-1,	&Appl::aOpt.term_limit,	0 },
	{ "terminal_info",	0,	OPT_STR,	-1,	0, &Appl::aOpt.term_base },
	{ "terminal_shell",	0,	OPT_STR,	-1,	0, &Appl::aOpt.shell },
	{ "trans_debug",	0,	OPT_STR,	-1,	0, &Appl::aOpt.trans_debug },
	{ "trans_edit",		0,	OPT_STR,	-1,	0, &Appl::aOpt.trans_edit },
	{ "trans_term",		0,	OPT_STR,	-1,	0, &Appl::aOpt.trans_term },
	{ "trans_web",		0,	OPT_STR,	-1,	0, &Appl::aOpt.trans_web },
	{ "undo_optimize",	0,	OPT_BOOL,	1,	&Edit::eOpt.options,	0 },
	{ "undo_steps",		0,	OPT_NUM,	-1,	&Edit::eOpt.stepUndo,	0 },
	{ "window_border",	0,	OPT_BOOL,	3,	&Appl::aOpt.options,	0 },
	{ "wrap_in_word",	0,	OPT_BOOL,	4,	&Edit::eOpt.options,	0 },
	{ 0, 0, 0, 0, 0, 0 }
};

static struct COM_info com_info[] = {
	{ "c_cancel",		0,	KEY_APPL,	HASH_cmCancel },
	{ "c_ctag_prompt",	0,	KEY_APPL,	HASH_cmCtagPrompt },
	{ "c_help",		0,	KEY_APPL,	HASH_cmHelpContext },
	{ "c_help_index",	0,	KEY_APPL,	HASH_cmHelpIndex },
	{ "c_help_prev",	0,	KEY_APPL,	HASH_cmHelpPrev },
	{ "c_man_page",		0,	KEY_APPL,	HASH_cmManPrompt },
	{ "c_menu",		0,	KEY_APPL,	HASH_cmMenu },
	{ "c_open_calc",	0,	KEY_APPL,	HASH_cmOpenCalc },
	{ "c_open_debug",	0,	KEY_APPL,	HASH_cmOpenDebug },
	{ "c_open_file",	0,	KEY_APPL,	HASH_cmOpen },
	{ "c_open_new",		0,	KEY_APPL,	HASH_cmNew },
	{ "c_open_term",	0,	KEY_APPL,	HASH_cmOpenTerm },
	{ "c_open_web",		0,	KEY_APPL,	HASH_cmOpenWeb },
	{ "c_quit",		0,	KEY_APPL,	HASH_cmQuit },
	{ "c_quit_error",	0,	KEY_APPL,	HASH_cmQuitError_1 },
	{ "c_refresh",		0,	KEY_APPL,	HASH_cmRefresh },
	{ "c_sys_command",	0,	KEY_APPL,	HASH_cmSystemCommand },
	{ "c_sys_window",	0,	KEY_APPL,	HASH_cmSystemWindow },
	{ "c_user_menu",	0,	KEY_APPL,	HASH_cmUserMenu },
	{ "c_win_close",	0,	KEY_APPL,	HASH_cmCloseWin },
	{ "c_win_choose",	0,	KEY_APPL,	HASH_cmChoose },
	{ "c_win_next",		0,	KEY_APPL,	HASH_cmNext },
	{ "c_win_next2",	0,	KEY_APPL,	HASH_cmNextWin },
	{ "c_win_position",	0,	KEY_APPL,	HASH_cmPosition },
	{ "c_win_prev",		0,	KEY_APPL,	HASH_cmPrev },
	{ "c_win_prev2",	0,	KEY_APPL,	HASH_cmPrevWin },
	{ "c_win_select1",	0,	KEY_APPL,	HASH_cmSelect1 },
	{ "c_win_select2",	0,	KEY_APPL,	HASH_cmSelect2 },
	{ "c_win_select3",	0,	KEY_APPL,	HASH_cmSelect3 },
	{ "c_win_select4",	0,	KEY_APPL,	HASH_cmSelect4 },
	{ "c_win_select5",	0,	KEY_APPL,	HASH_cmSelect5 },
	{ "c_win_select6",	0,	KEY_APPL,	HASH_cmSelect6 },
	{ "c_win_select7",	0,	KEY_APPL,	HASH_cmSelect7 },
	{ "c_win_select8",	0,	KEY_APPL,	HASH_cmSelect8 },
	{ "c_win_select9",	0,	KEY_APPL,	HASH_cmSelect9 },
	{ "c_win_size",		0,	KEY_APPL,	HASH_cmSize },
	{ "c_win_zoom",		0,	KEY_APPL,	HASH_cmZoom },

	{ "d_block_col",	0,	KEY_DIALOG,	HASH_cmBlockColumn },
	{ "d_block_row",	0,	KEY_DIALOG,	HASH_cmBlockLine },
	{ "d_block_unmark",	0,	KEY_DIALOG,	HASH_cmUnmarkBlock },
	{ "d_bottom",		0,	KEY_DIALOG,	HASH_cmFuncPgDn },
	{ "d_clip_copy_to",	0,	KEY_DIALOG,	HASH_cmCopyToClip },
	{ "d_cursor_mode",	0,	KEY_DIALOG,	HASH_cmMode },
	{ "d_down",		0,	KEY_DIALOG,	HASH_cmDown },
	{ "d_end",		0,	KEY_DIALOG,	HASH_cmEnd },
	{ "d_find",		0,	KEY_DIALOG,	HASH_cmFind },
	{ "d_find_next",	0,	KEY_DIALOG,	HASH_cmFindDown },
	{ "d_first",		0,	KEY_DIALOG,	HASH_cmFirst },
	{ "d_home",		0,	KEY_DIALOG,	HASH_cmHome },
	{ "d_last",		0,	KEY_DIALOG,	HASH_cmLast },
	{ "d_left",		0,	KEY_DIALOG,	HASH_cmLeft },
	{ "d_next",		0,	KEY_DIALOG,	HASH_cmNext },
	{ "d_next2",		0,	KEY_DIALOG,	HASH_cmNext2 },
	{ "d_pgdn",		0,	KEY_DIALOG,	HASH_cmPgDn },
	{ "d_pgup",		0,	KEY_DIALOG,	HASH_cmPgUp },
	{ "d_prev",		0,	KEY_DIALOG,	HASH_cmPrev },
	{ "d_prev2",		0,	KEY_DIALOG,	HASH_cmPrev2 },
	{ "d_right",		0,	KEY_DIALOG,	HASH_cmRight },
	{ "d_scroll_down",	0,	KEY_DIALOG,	HASH_cmUp2 },
	{ "d_scroll_up",	0,	KEY_DIALOG,	HASH_cmDown2 },
	{ "d_top",		0,	KEY_DIALOG,	HASH_cmFuncPgUp },
	{ "d_up",		0,	KEY_DIALOG,	HASH_cmUp },

	{ "e_backspace",	0,	KEY_EDIT,	HASH_cmBackspace },
	{ "e_block_col",	0,	KEY_EDIT,	HASH_cmBlockColumn },
	{ "e_block_copy",	0,	KEY_EDIT,	HASH_cmCopy },
	{ "e_block_del",	0,	KEY_EDIT,	HASH_cmDeleteBlock },
	{ "e_block_move",	0,	KEY_EDIT,	HASH_cmMove },
	{ "e_block_reverse",	0,	KEY_EDIT,	HASH_cmBlockReverse },
	{ "e_block_row",	0,	KEY_EDIT,	HASH_cmBlockLine },
	{ "e_block_unmark",	0,	KEY_EDIT,	HASH_cmUnmarkBlock },
	{ "e_breakpoint",	0,	KEY_EDIT,	HASH_cmBreakpoint },
	{ "e_clip_add",		0,	KEY_EDIT,	HASH_cmAddToClip },
	{ "e_clip_copy_from",	0,	KEY_EDIT,	HASH_cmCopyFromClip },
	{ "e_clip_copy_to",	0,	KEY_EDIT,	HASH_cmCopyToClip },
	{ "e_clip_edit",	0,	KEY_EDIT,	HASH_cmClipboard },
	{ "e_clip_move",	0,	KEY_EDIT,	HASH_cmMoveToClip },
	{ "e_ctag_go",		0,	KEY_EDIT,	HASH_cmCtagGo },
	{ "e_del_char",		0,	KEY_EDIT,	HASH_cmDelete },
	{ "e_del_line",		0,	KEY_EDIT,	HASH_cmDeleteLine },
	{ "e_down",		0,	KEY_EDIT,	HASH_cmDown },
	{ "e_end",		0,	KEY_EDIT,	HASH_cmEnd },
	{ "e_enter",		0,	KEY_EDIT,	HASH_cmEnter },
	{ "e_find",		0,	KEY_EDIT,	HASH_cmFind },
	{ "e_find_next",	0,	KEY_EDIT,	HASH_cmFindContinue },
	{ "e_find_replace",	0,	KEY_EDIT,	HASH_cmReplace },
	{ "e_first",		0,	KEY_EDIT,	HASH_cmFuncPgUp },
	{ "e_format_dumb",	0,	KEY_EDIT,	HASH_cmDumbFormat },
	{ "e_format_smart",	0,	KEY_EDIT,	HASH_cmSmartFormat },
	{ "e_goto",		0,	KEY_EDIT,	HASH_cmGoto },
	{ "e_home",		0,	KEY_EDIT,	HASH_cmHome },
	{ "e_input_code",	0,	KEY_EDIT,	HASH_cmInputCode },
	{ "e_ins_mode",		0,	KEY_EDIT,	HASH_cmIns },
	{ "e_last",		0,	KEY_EDIT,	HASH_cmFuncPgDn },
	{ "e_left",		0,	KEY_EDIT,	HASH_cmLeft },
	{ "e_man",		0,	KEY_EDIT,	HASH_cmManual },
	{ "e_man_mode",		0,	KEY_EDIT,	HASH_cmShowMan },
	{ "e_mark_line",	0,	KEY_EDIT,	HASH_cmMarkGotoLine },
	{ "e_math_down",	0,	KEY_EDIT,	HASH_cmMathDown },
	{ "e_math_up",		0,	KEY_EDIT,	HASH_cmMathUp },
	{ "e_pgdn",		0,	KEY_EDIT,	HASH_cmPgDn },
	{ "e_pgup",		0,	KEY_EDIT,	HASH_cmPgUp },
	{ "e_prevtab",		0,	KEY_EDIT,	HASH_cmBackTab },
	{ "e_readonly_mode",	0,	KEY_EDIT,	HASH_cmMode },
	{ "e_redo",		0,	KEY_EDIT,	HASH_cmRedo },
	{ "e_reload",		0,	KEY_EDIT,	HASH_cmReload },
	{ "e_right",		0,	KEY_EDIT,	HASH_cmRight },
	{ "e_save",		0,	KEY_EDIT,	HASH_cmSave },
	{ "e_save_as",		0,	KEY_EDIT,	HASH_cmSaveAs },
	{ "e_scroll_down",	0,	KEY_EDIT,	HASH_cmUp2 },
	{ "e_scroll_up",	0,	KEY_EDIT,	HASH_cmDown2 },
	{ "e_space_mode",	0,	KEY_EDIT,	HASH_cmSpaceMode },
	{ "e_truncate",		0,	KEY_EDIT,	HASH_cmTruncate },
	{ "e_undo",		0,	KEY_EDIT,	HASH_cmUndo },
	{ "e_up",		0,	KEY_EDIT,	HASH_cmUp },
	{ "e_word_left",	0,	KEY_EDIT,	HASH_cmLeft2 },
	{ "e_word_right",	0,	KEY_EDIT,	HASH_cmRight2 },

	{ "func1",		0,	KEY_APPL,	HASH_cmFUNC1 },
	{ "func2",		0,	KEY_APPL,	HASH_cmFUNC2 },

	{ "national",		0,	KEY_APPL,	HASH_cmNATIONAL },

	{ "t_block_col",	0,	KEY_TERM,	HASH_cmBlockColumn },
	{ "t_block_row",	0,	KEY_TERM,	HASH_cmBlockLine },
	{ "t_block_unmark",	0,	KEY_TERM,	HASH_cmUnmarkBlock },
	{ "t_clip_copy_from",	0,	KEY_TERM,	HASH_cmCopyFromClip },
	{ "t_clip_copy_to",	0,	KEY_TERM,	HASH_cmCopyToClip },
	{ "t_cursor_mode",	0,	KEY_TERM,	HASH_cmMode },
	{ "t_key_mode",		0,	KEY_TERM,	HASH_cmModeKey },

	{ "w_cursor_mode",	0,	KEY_WEB,	HASH_cmMode },
	{ "w_download",		0,	KEY_WEB,	HASH_cmDownLoad },
	{ "w_download2",	0,	KEY_WEB,	HASH_cmDownLoadNoAsk },
	{ "w_frame_next",	0,	KEY_WEB,	HASH_cmPrev2 },
	{ "w_frame_prev",	0,	KEY_WEB,	HASH_cmNext2 },
	{ "w_go",		0,	KEY_WEB,	HASH_cmGo },
	{ "w_history",		0,	KEY_WEB,	HASH_cmHistory },
	{ "w_info",		0,	KEY_WEB,	HASH_cmInfo },
	{ "w_main_page",	0,	KEY_WEB,	HASH_cmMainPage },
	{ "w_prev",		0,	KEY_WEB,	HASH_cmPreviousPage },
	{ "w_reload",		0,	KEY_WEB,	HASH_cmReload },
	{ "w_reload2",		0,	KEY_WEB,	HASH_cmReloadNoAsk },
	{ "w_search",		0,	KEY_WEB,	HASH_cmSearch },
	{ "w_source",		0,	KEY_WEB,	HASH_cmSource },

	{ 0, 0, 0, 0 }
};

static struct KEY_info key_info[] = {
	{ "backspace",	kbBS },
	{ "del",	kbDel },
	{ "down",	kbDown },
	{ "end",	kbEnd },
	{ "enter",	kbEnter },
	{ "esc",	kbEsc },
	{ "f1",		kbF1 },
	{ "f10",	kbF10 },
	{ "f11",	kbF11 },
	{ "f12",	kbF12 },
	{ "f2",		kbF2 },
	{ "f3",		kbF3 },
	{ "f4",		kbF4 },
	{ "f5",		kbF5 },
	{ "f6",		kbF6 },
	{ "f7",		kbF7 },
	{ "f8",		kbF8 },
	{ "f9",		kbF9 },
	{ "home",	kbHome },
	{ "ins",	kbIns },
	{ "left",	kbLeft },
	{ "pgdn",	kbPgDn },
	{ "pgup",	kbPgUp },
	{ "return",	kbEnter },
	{ "right",	kbRight },
	{ "space",	' ' },
	{ "tab",	kbTab },
	{ "up",		kbUp },
	{ 0, 0 }
};

extern unsigned char colorDialog[12];
extern unsigned char colorHyper[2];
extern unsigned char colorInput[2];
extern unsigned char colorGetbox[4];
extern unsigned char colorProgram[1];
extern unsigned char colorEdit[17];
extern unsigned char colorMenu[4];
extern unsigned char colorList[11];
extern unsigned char colorWeb[16];
extern unsigned char colorWebBox[2];
extern unsigned char colorWebButton[2];
extern unsigned char colorWebHyper[2];
extern unsigned char colorWebInput[2];
extern unsigned char colorWebList[11];
extern unsigned char colorTerm[2];

static struct COLOR_info color_info[] = {
	{ "d_attr1",		0,	colorDialog+5,	0,	0 },
	{ "d_attr2",		0,	colorDialog+6,	0,	0 },
	{ "d_attr3",		0,	colorDialog+7,	0,	0 },
	{ "d_attr4",		0,	colorDialog+8,	0,	0 },
	{ "d_attr5",		0,	colorDialog+9,	0,	0 },
	{ "d_attr6",		0,	colorDialog+10,	0,	0 },
	{ "d_attr7",		0,	colorDialog+11,	0,	0 },
	{ "d_block",		0,	colorDialog+3,	0,	0 },
	{ "d_box_active_cur",	0,	colorGetbox+2,	0,	0 },
	{ "d_box_active_item",	0,	colorGetbox,	0,	0 },
	{ "d_box_normal_cur",	0,	colorGetbox+3,	0,	0 },
	{ "d_box_normal_item",	0,	colorGetbox+1,	0,	0 },
	{ "d_button_active",	0,	colorHyper,	0,	0 },
	{ "d_button_normal",	0,	colorHyper+1,	0,	0 },
	{ "d_find",		0,	colorDialog+4,	0,	0 },
	{ "d_frame_active",	0,	colorDialog,	0,	0 },
	{ "d_frame_normal",	0,	colorDialog+1,	0,	0 },
	{ "d_input_active",	0,	colorInput,	0,	0 },
	{ "d_input_normal",	0,	colorInput+1,	0,	0 },
	{ "d_text",		0,	colorDialog+2,	0,	0 },

	{ "e_background",	0,  	colorProgram,	0,	0 },
	{ "e_block",		0,	colorEdit+3,	0,	0 },
	{ "e_breakpoint",	0,	colorEdit+8,	0,	0 },
	{ "e_code",		0,	colorEdit+6,	0,	0 },
	{ "e_comment", 		0,	colorEdit+10,	0,	0 },
	{ "e_debug_line",	0,	colorEdit+7,	0,	0 },
	{ "e_end_line",		0,	colorEdit+5,	0,	0 },
	{ "e_find",		0,	colorEdit+4,	0,	0 },
	{ "e_frame_active",	0,	colorEdit,	0,	0 },
	{ "e_frame_normal",	0,	colorEdit+1,	0,	0 },
	{ "e_highlight1",	0,	colorEdit+15,	0,	0 },
	{ "e_highlight2",	0,	colorEdit+16,	0,	0 },
	{ "e_ht_par_name",	0,	colorEdit+13,	0,	0 },
	{ "e_ht_tag",		0,	colorEdit+12,	0,	0 },
	{ "e_man",		0,	colorEdit+9,	0,	0 },
	{ "e_par_val",		0,	colorEdit+14,	0,	0 },
	{ "e_preproc",		0,	colorEdit+11,	0,	0 },
	{ "e_text",		0,	colorEdit+2,	0,	0 },

	{ "l_active_item",	0,	colorList+2,	0,	0 },
	{ "l_active_item_cur",	0,	colorList+6,	0,	0 },
	{ "l_active_item_sc",	0,	colorList+8,	0,	0 },
	{ "l_active_item_sel",	0,	colorList+4,	0,	0 },
	{ "l_frame_active",	0,	colorList,	0,	0 },
	{ "l_frame_normal",	0,	colorList+1,	0,	0 },
	{ "l_info",		0,	colorList+10,	0,	0 },
	{ "l_normal_item",	0,	colorList+3,	0,	0 },
	{ "l_normal_item_cur",	0,	colorList+7,	0,	0 },
	{ "l_normal_item_sc",	0,	colorList+9,	0,	0 },
	{ "l_normal_item_sel",	0,	colorList+5,	0,	0 },

	{ "m_active",		0,	colorMenu,	0,	0 },
	{ "m_hot_active",	0,	colorMenu+2,	0,	0 },
	{ "m_hot_normal",	0,	colorMenu+3,	0,	0 },
	{ "m_item",		0,	colorMenu+1,	0,	0 },

	{ "s_light",		0,	colorStatus+1,	0,	0 },
	{ "s_text",		0,	colorStatus,	0,	0 },

	{ "t_active",		0,	colorTerm,	0,	0 },
	{ "t_text",		0,	colorTerm+1,	0,	0 },

	{ "w_attr1",		0,	colorWeb+5,	0,	0 },
	{ "w_attr2",		0,	colorWeb+6,	0,	0 },
	{ "w_attr3",		0,	colorWeb+7,	0,	0 },
	{ "w_attr4",		0,	colorWeb+8,	0,	0 },
	{ "w_attr5",		0,	colorWeb+9,	0,	0 },
	{ "w_attr6",		0,	colorWeb+10,	0,	0 },
	{ "w_attr7",		0,	colorWeb+11,	0,	0 },
	{ "w_attr8",		0,	colorWeb+12,	0,	0 },
	{ "w_attr9",		0,	colorWeb+13,	0,	0 },
	{ "w_attr10",		0,	colorWeb+14,	0,	0 },
	{ "w_attr11",		0,	colorWeb+15,	0,	0 },
	{ "w_block",		0,	colorWeb+3,	0,	0 },
	{ "w_find",		0,	colorWeb+4,	0,	0 },
	{ "w_frame_active",	0,	colorWeb,	0,	0 },
	{ "w_frame_normal",	0,	colorWeb+1,	0,	0 },
	{ "w_text",		0,	colorWeb+2,	0,	0 },

	{ "w_list_active_item",		0,	colorWebList+2,	0,	0 },
	{ "w_list_active_item_cur",	0,	colorWebList+6,	0,	0 },
	{ "w_list_active_item_sc",	0,	colorWebList+8,	0,	0 },
	{ "w_list_active_item_sel",	0,	colorWebList+4,	0,	0 },
	{ "w_list_info",		0,	colorWebList+10,0,	0 },
	{ "w_list_normal_item",		0,	colorWebList+3,	0,	0 },
	{ "w_list_normal_item_cur",	0,	colorWebList+7,	0,	0 },
	{ "w_list_normal_item_sc",	0,	colorWebList+9,	0,	0 },
	{ "w_list_normal_item_sel",	0,	colorWebList+5,	0,	0 },

	{ "w_button_active",	0,	colorWebButton,		0,	0 },
	{ "w_button_normal",	0,	colorWebButton+1,	0,	0 },
	{ "w_getbox_active",	0,	colorWebBox,		0,	0 },
	{ "w_getbox_normal",	0,	colorWebBox+1,		0,	0 },
	{ "w_input_active",	0,	colorWebInput,		0,	0 },
	{ "w_input_normal",	0,	colorWebInput+1,	0,	0 },
	{ "w_ref_active",	0,	colorWebHyper,		0,	0 },
	{ "w_ref_normal",	0,	colorWebHyper+1,	0,	0 },

	{ 0, 0, 0, 0 }
};

struct COLOR_name {
	char *name;
	unsigned bg_flag:1;
	unsigned char fg;
	unsigned char bg;
};

static struct COLOR_name color_name[] = {
	{ "black",	1,	FG_BLACK,	BG_BLACK },
	{ "blue",	1,	FG_BLUE,	BG_BLUE	},
	{ "cyan",	1,	FG_CYAN,	BG_CYAN },
	{ "green",	1,	FG_GREEN,	BG_GREEN },
	{ "h_black",	0,	FG_HI_BLACK,	0 },
	{ "h_blue",	0,	FG_HI_BLUE,	0 },
	{ "h_cyan",	0,	FG_HI_CYAN,	0 },
	{ "h_green",	0,	FG_HI_GREEN,	0 },
	{ "h_magenta",	0,	FG_HI_MAGENTA,	0 },
	{ "h_red",	0,	FG_HI_RED,	0 },
	{ "h_white",	0,	FG_HI_WHITE,	0 },
	{ "h_yellow",	0,	FG_HI_YELLOW,	0 },
	{ "magenta",	1,	FG_MAGENTA,	BG_MAGENTA },
	{ "red",	1,	FG_RED,		BG_RED },
	{ "white",	1,	FG_WHITE,	BG_WHITE },
	{ "yellow",	1,	FG_YELLOW,	BG_YELLOW }
};

static int cmp_opt( const void *p1, const void *p2 )
{
	return strcasecmp( (const char*)p1, ((const struct OPT_info*)p2)->name );
}

static int cmp_com( const void *p1, const void *p2 )
{
	return strcasecmp( (const char*)p1, ((const struct COM_info*)p2)->name );
}

static int cmp_key( const void *p1, const void *p2 )
{
	return strcasecmp( (const char*)p1, ((const struct KEY_info*)p2)->name );
}

static int cmp_color( const void *p1, const void *p2 )
{
	return strcasecmp( (const char*)p1, ((const struct COLOR_info*)p2)->name );
}

static int cmp_color_name( const void *p1, const void *p2 )
{
	return strcasecmp( (const char*)p1, ((const struct COLOR_name*)p2)->name );
}

long key_by_name( const char *key_name )
{
	int func1=0, func2=0, func3=0;
	const char *name = key_name;
	if ( !memcmp( name, "func1_", 6 ) ) {
	    func1 = 1;
	    name += 6;
	} else if ( !memcmp( name, "func2_", 6 ) ) {
	    func2 = 1;
	    name += 6;
	} else if ( !memcmp( name, "func3_", 6 ) ) {
	    func3 = 1;
	    name += 6;
	}

	KEY_info *ki = (KEY_info*)bsearch( name, key_info, len_key_info,
					sizeof(struct KEY_info), cmp_key );

	long ret = ki ? ki->key : 0;

	if ( !ret ) {
	    int ctrl = 0;
	    if ( name[0] == '^' ) {
		ctrl = 1;
		name++;
	    }
	    if ( *(name+1) )
		return 0;
	    ret = ctrl ? toupper( *name ) - 64 : *name;
	}

	if ( func1 )
	    ret |= 0x200;
	else if ( func2 )
	    ret |= 0x400;
	else if ( func3 )
	    ret |= 0x600;

	return ret;
}

void Appl::setTransFlag( char *str, int *flag )
{
	if ( !strcasecmp( str, "866-koi8" ) )
	    *flag = TRANS_alt2koi8;
	else if ( !strcasecmp( str, "1251-koi8" ) )
	    *flag = TRANS_win2koi8;
	else if ( !strcasecmp( str, "main-koi8" ) )
	    *flag = TRANS_main2koi8;
	else if ( !strcasecmp( str, "koi8-866" ) )
	    *flag = TRANS_koi82alt;
	else if ( !strcasecmp( str, "koi8-1251" ) )
	    *flag = TRANS_koi82win;
	else if ( !strcasecmp( str, "koi8-main" ) )
	    *flag = TRANS_koi82main;
	else
	    *flag = TRANS_none;
}

void Appl::readOptions()
{
	int commonFile = 0;
	FILE *f = fopen( name_options, "r" );
	if ( f == NULL ) {
	    f = fopen( name_options_common, "r" );
	    commonFile = 1;
	}
	if ( f == NULL ) {
#ifdef _USE_WWW_
	    if ( !setW3Cache( aOpt.w3cache ) )
		aOpt.w3cache[0] = 0;
#endif
	} else {
	    for( int i=0; i < len_opt_info; i++ )
		opt_info[i].inited = 0;

	    char *s = NULL;
	    OPT_info *cinfo = NULL;
next:
	    while( fgets( buf1024, 1023, f ) > 0 ) {
		buf1024[1023] = 0;
		s=strtok( buf1024, " \t\r\n" );
		for( int first=1; s; s=strtok( 0, " \t\r\n" ) ) {
		    if ( *s == '#' )
			break;
		    if ( first ) {
			cinfo = (OPT_info*)bsearch( s, opt_info, len_opt_info,
					sizeof(struct OPT_info), cmp_opt );
			if ( !cinfo || cinfo->inited )
			    break;
			first = 0;
			continue;
		    }

		    switch( cinfo->type ) {
			case OPT_BOOL:
			    set_opt_bool( s, cinfo->n, cinfo->val_int );
			    break;
			case OPT_NUM:
			    set_opt_num( s, cinfo->val_int );
			    break;
			case OPT_STR:
			    set_opt_str( s, cinfo->val_ptr );
			    break;
		    }
		    cinfo->inited = 1;
		}
	    }

	    if ( !commonFile ) {
		fclose( f );
		if ( (f = fopen( name_options_common, "r" )) ) {
		    commonFile = 1;
		    goto next;
		}
	    }
	}

	Debug::flagInit = GDB_AUTOLOAD(aOpt.options) ? 1 : 0;
	if ( Edit::eOpt.tab == 2 || Edit::eOpt.tab == 4 || Edit::eOpt.tab == 8 )
	    Edit::stepTab = Edit::eOpt.tab - 1;
	backgroundSymbol = aOpt.bg_symbol;
	inputLineFillChar = aOpt.fill_char;

	language = 0;
	if ( !strcasecmp( aOpt.lang, "russian" ) )
	    language = 1;

	menuMgr->Visible = MENU_BAR(aOpt.options) ? 1 : 0;
	menuMgr->init();
	statusLine->visible = STATUS_BAR(aOpt.options) ? 1 : 0;
	borderVisible = WINDOW_BORDER(aOpt.options) ? 1 : 0;

#ifdef _USE_WWW_
	if ( aOpt.w3cache && !setW3Cache( aOpt.w3cache ) ) {
	    ::free( aOpt.w3cache );
	    aOpt.w3cache = NULL;
	}
#endif

	if ( f )
	    fclose( f );

	Screen::initScreen( strcmp( aOpt.term_base, "terminfo" ) ?
				 BASE_TERMCAP : BASE_TERMINFO );

	colorsType = 1;
	if ( !strcasecmp( aOpt.monitor, "bw" ) || !Term::hasColors() )
	    colorsType = 0;
	else if ( !strcasecmp( aOpt.monitor, "laptop" ) )
	    colorsType = 2;
	setColors( colorsType );

	setTransFlag( aOpt.trans_edit, &trans_flag_edit );
	setTransFlag( aOpt.trans_web, &trans_flag_web );
	setTransFlag( aOpt.trans_term, &trans_flag_term );
	setTransFlag( aOpt.trans_debug, &trans_flag_debug );
}

void Appl::readColormap()
{
	int commonFile = 0;
	FILE *f = fopen( name_colormap, "r" );
	if ( f == NULL ) {
	    if ( (f = fopen( name_colormap_common, "r" )) == NULL )
		return;
	    commonFile = 1;
	}

	char *s=NULL;
	COLOR_info *cinfo = NULL;
	COLOR_name *cname = NULL;

	for( int i=0; i < len_color_info; i++ )
	    color_info[i].inited = 0;
next:
	while( fgets( buf1024, 1023, f ) > 0 ) {
	    buf1024[1023] = 0;
	    s=strtok( buf1024, " \t\r\n" );
	    for( int first=1; s; s=strtok( 0, " \t\r\n" ) ) {
		if ( *s == '#' )
		    break;
		if ( !cinfo && ((cinfo = (COLOR_info*)bsearch( s, color_info, len_color_info,
						sizeof(struct COLOR_info), cmp_color ))) ) {
		    if ( !cinfo->inited )
			continue;
		}
		if ( !cinfo || cinfo->inited )
		    break;
		if ( !(cname = (COLOR_name*)bsearch( s, color_name, 16,
					sizeof(struct COLOR_name), cmp_color_name )) ) {
		    cinfo = NULL;
		    break;
		}
		if ( !cinfo->fg ) {
		    cinfo->fg = strdup( s );
		    continue;
		}
		if ( cname->bg_flag ) {
		    cinfo->bg = strdup( s );
		    cinfo->inited = 1;
		    cinfo = NULL;
		    break;
		}
	    }
	}

	if ( !commonFile ) {
	    fclose( f );
	    if ( (f = fopen( name_colormap_common, "r" )) ) {
		commonFile = 1;
		cinfo=NULL; cname=NULL;
		goto next;
	    }
	}

	for( int i=0; i < len_color_info; i++ ) {
	    if ( color_info[i].fg && color_info[i].bg ) {
		unsigned char fg=0, bg=0;
		int fg_flag=0, bg_flag=0;
		if ( (cname = (COLOR_name*)bsearch( color_info[i].fg, color_name, 16,
				sizeof(struct COLOR_name), cmp_color_name )) ) {
		    fg = cname->fg;
		    fg_flag = 1;
		}
		if ( (cname = (COLOR_name*)bsearch( color_info[i].bg, color_name, 16,
				sizeof(struct COLOR_name), cmp_color_name )) ) {
		    bg = cname->bg;
		    bg_flag = 1;
		}
		if ( fg_flag && bg_flag )
		    *(color_info[i].attr) = fg | bg;
	    }
	    if ( color_info[i].fg ) {
		::free( color_info[i].fg );
		color_info[i].fg = NULL;
	    }
	    if ( color_info[i].bg ) {
		::free( color_info[i].bg );
		color_info[i].bg = NULL;
	    }
	}

	fclose( f );
}

void Appl::readColortrans()
{
	int commonFile = 0;
	FILE *f = fopen( name_colortrans, "r" );
	if ( f == NULL ) {
	    if ( (f = fopen( name_colortrans_common, "r" )) == NULL )
		return;
	    commonFile = 1;
	}

	char *fg=NULL, *bg=NULL, *s=NULL;
	int found_term=0, c_fg=0, c_bg=0;
next:
	while( fgets( buf1024, 1023, f ) > 0 ) {
	    if ( fg && bg )
		break;
	    buf1024[1023] = 0;
	    s=strtok( buf1024, " \t\r\n" );
	    for( int first=1; s; s=strtok( 0, " \t\r\n" ) ) {
		if ( *s == '#' || fg && bg )
		    break;
		if ( first && !found_term ) {
		    if ( !strcasecmp( s, "terminal" ) ) {
			first = 0;
			continue;
		    }
		    break;
		}
		if ( !found_term ) {
		    if ( !strcmp( s, Term::tname ) ) {
			found_term = 1;
			break;
		    }
		    continue;
		}
		if ( !c_fg && !strcasecmp( s, "colormap_fg" ) ) {
		    c_fg = 1;
		    continue;
		}
		if ( !c_bg && !strcasecmp( s, "colormap_bg" ) ) {
		    c_bg = 1;
		    continue;
		}
		if ( c_fg > 0 && fg == NULL )
		    fg = strdup( s );

		if ( c_bg > 0 && bg == NULL )
		    bg = strdup( s );
	    }
	}

	if ( !commonFile && (!fg || !bg) ) {
	    fclose( f );
	    if ( (f = fopen( name_colortrans_common, "r" )) ) {
		commonFile = 1;
		found_term=0; c_fg=0; c_bg=0;
		goto next;
	    }
	}

	Term::initcolor( fg, bg );

	if ( fg )
	    ::free( fg );
	if ( bg )
	    ::free( bg );

	fclose( f );
}

void Appl::readKeymap()
{
	int commonFile = 0;
	FILE *f = fopen( name_keymap, "r" );
	if ( f == NULL ) {
	    f = fopen( name_keymap_common, "r" );
	    commonFile = 1;
	}
	if ( f ) {

	    char *s=NULL, *func_name=NULL;
	    long *keymap_appl	= (long*)calloc( 256, sizeof(long) );
	    long *kmap_appl	= keymap_appl;
	    long *keymap_edit	= (long*)calloc( 256, sizeof(long) );
	    long *kmap_edit	= keymap_edit;
	    long *keymap_dialog	= (long*)calloc( 256, sizeof(long) );
	    long *kmap_dialog	= keymap_dialog;
	    long *keymap_term	= (long*)calloc( 256, sizeof(long) );
	    long *kmap_term	= keymap_term;
	    long *keymap_web	= (long*)calloc( 256, sizeof(long) );
	    long *kmap_web	= keymap_web;

	    for( int i=0; i < len_com_info; i++ )
		com_info[i].inited = 0;
next:
	    while( fgets( buf1024, 1023, f ) > 0 ) {
		COM_info *cinfo = NULL;
		buf1024[1023] = 0;
		s=strtok( buf1024, " \t\r\n" );
		for( int first=1; s; s=strtok( 0, " \t\r\n" ) ) {
		    if ( *s == '#' )
			break;
		    if ( first ) {
			if ( (cinfo = (COM_info*)bsearch( s, com_info, len_com_info,
					sizeof(struct COM_info), cmp_com )) ) {
			    if ( cinfo->inited )
				break;
			    first=0;
			    continue;
			}
			break;
		    }

		    long key = key_by_name( s );

		    switch( cinfo->type ) {
			case KEY_APPL:
			    switch( cinfo->command ) {
				case HASH_cmFUNC1:
				    inputKeyTask->set_func1( key );
				    break;
				case HASH_cmFUNC2:
				    inputKeyTask->set_func2( key );
				    break;
				case HASH_cmNATIONAL:
				    inputKeyTask->set_cyr( key );
				    break;
				default:
				    *kmap_appl++ = key;
				    *kmap_appl++ = cinfo->command;
				    break;
			    }
			    break;
			case KEY_EDIT:
			    *kmap_edit++ = key;
			    *kmap_edit++ = cinfo->command;
			    break;
			case KEY_DIALOG:
			    *kmap_dialog++ = key;
			    *kmap_dialog++ = cinfo->command;
			    break;
			case KEY_TERM:
			    *kmap_term++ = key;
			    *kmap_term++ = cinfo->command;
			    break;
			case KEY_WEB:
			    *kmap_web++ = key;
			    *kmap_web++ = cinfo->command;
			    break;
		    }
		}
		if ( cinfo )
		    cinfo->inited = 1;
	    }

	    if ( !commonFile ) {
		fclose( f );
		if ( (f = fopen( name_keymap_common, "r" )) ) {
		    commonFile = 1;
		    goto next;
		}
	    }

	    if ( kmap_appl > keymap_appl ) {
		keyHolder.del( &ApplKeyMap );
		ProgramKeyMap.init( keymap_appl );
	    }
	    if ( kmap_edit > keymap_edit ) {
		EditKeyMap.init( keymap_edit );
	    }
	    if ( kmap_dialog > keymap_dialog ) {
		DialogKeyMap.init( keymap_dialog );
	    }
	    if ( kmap_term > keymap_term ) {
		YTermKeyMap.init( keymap_term );
	    }
#ifdef _USE_WWW_
	    if ( kmap_web > keymap_web ) {
		W3WinKeyMap.init( keymap_web );
	    }
#endif

	    if ( func_name )
		::free( func_name );
	    if ( keymap_appl )
		::free( keymap_appl );
	    if ( keymap_edit )
		::free( keymap_edit );
	    if ( keymap_dialog )
		::free( keymap_dialog );
	    if ( keymap_term )
		::free( keymap_term );
	    if ( keymap_web )
		::free( keymap_web );

	    if ( f )
		fclose( f );
	}

	menuMgr->init();
}

void Appl::readMacro()
{
	int commonFile = 0;
	FILE *f = fopen( name_macro, "r" );
	if ( f == NULL ) {
	    f = fopen( name_macro_common, "r" );
	    commonFile = 1;
	}
	if ( f ) {
next:
	    while( fgets( buf1024, 1023, f ) > 0 ) {


	    }

	    if ( !commonFile ) {
		fclose( f );
		if ( (f = fopen( name_keymap_common, "r" )) ) {
		    commonFile = 1;
		    goto next;
		}
	    }
	    if ( f )
		fclose( f );
	}
}

int Appl::restOptions()
{
	readKeymap();
	readOptions();
	readColormap();
	readColortrans();

	Rect r( 0, 0, 0, Screen::Columns-1 );
	menuMgr->setRect( r );
	menuMgr->reallocScrMap();
	menuMgr->sepchar = Screen::_VL | GraphAttr;
	menuMgr->init();

	r = Rect( Screen::Lines-1, 0, Screen::Lines-1, Screen::Columns-1 );
	statusLine->setRect( r );
	statusLine->reallocScrMap();
	statusLine->init();

	return 1;
}

#ifndef DJGPP

void Appl::getManWord()
{
   static Collection history( 10, 5 );
   char *s = 0;
   if ( history.getCount() > 0 )
      s = (char*)history.at(0);
   getString( HASH_cmOpenMan, 0, 30, &history, s, getStringBuf, "man" );
}

void Appl::getCtagWord()
{
   static Collection history( 10, 5 );
   char *s = 0;
   if ( history.getCount() > 0 )
      s = (char*)history.at(0);
   getString( HASH_cmCtagGoInternal, 0, 30, &history, s, getStringBuf, "C-tag" );
}

void Appl::manualPage( char *str )
{
   char *word = str;
   if ( !word ) {
       if ( count < 1 )
	  return;
       Window *w = ((winExecutor*)items[current])->win;
       void *ptr=0;
       if ( w->handleKey( HASH_cmIsEdit, ptr ) )
	  return;
       Edit *e = (Edit*)w;
       word = e->currentWord();
       if ( !word[0] )
	  return;
   }
#ifndef _USE_WWW_
   strcpy( buf1024, "man " );
   strcat( buf1024, word );
   Rect tr = scrRect();
   if ( !borderVisible ) tr.a.y--;
   insertExecWindow( new YTerm( buf1024, 0, tr, tr.b.x-tr.a.x-2, tr.b.y-tr.a.y-2 ) );
#else
   strcpy( buf1024, "man:" );
   if ( strlen( word ) > 0 )
     {
       strcat( buf1024, word );
       strcat( buf1024, "()" );
     }
   insertExecWindow( new W3Win( buf1024, scrRect() ) );
#endif
}

void Appl::gdbInitFile()
{
   char *gdbfile = ".gdbinit";
   char *name = access( gdbfile, F_OK ) ? (char*)"" : gdbfile;
   Edit *e = new Edit( name, Rect( 3, 1, 11, 56 ) );
   if ( e->valid() )
      { delete e; return; }
   if ( !name[0] )
      e->title->put( ".gdbinit", 0 );

   Dialog *d = new Dialog( Screen::center(14,60), new Lang("GDB init file","Файл инициализации GDB"), 0, new Lang("GNU debug initialization","Инициализация отладчика ~gdb~") );

   getBox *gb = new checkBox( Point(1,5), Debug::flagInit, &Debug::flagInit );
   gb->add( lang("Auto loading commands from '.gdbinit' file","Автозагрузка команд из файла '.gdbinit'") );
   d->insert( gb );
   d->insert( e );

   e->init();

   insertExecWindow( d );
}

void Appl::openDebug()
{
   static IdentMessage imsg( HASH_cmIsDebug );
   Task *executor = 0;
   if ( sendAll(imsg) == 1 && (executor=findTask(imsg.getReceiver())) ) {
	long ind = indexOf( executor );
	if ( ind >= 0 && ind < count ) {
	   nselect( ind );
	   return;
	}
   }
   Rect dr = scrRect();
   dr.a.y = dr.b.y - 9;
   Debug *db = new Debug( dr );
   if ( !db )
      return;
   if ( db->valid() )
     { delete db; return; }
   nselect( Insert( db ) );
}

#endif

void Appl::openShare()
{
   if ( count < 1 )
      return;
   Window *w = ((winExecutor*)items[current])->win;
   void *ptr=0;
   if ( w->handleKey( HASH_cmIsEdit, ptr ) )
      return;

   Edit *e = new Edit( (Edit*)w, scrRect() );
   if ( !e )
      return;
   if ( !e->valid() )
       nselect( insert( e ) );
   else
       delete e;
}

void Appl::runHelp( long type )
{
#ifdef _USE_WWW_
   char *ref=0, *iref = "Index";
   int flag = 0;
   if ( type == HASH_cmHelpContext )
     {
       Window *w = 0;
       if ( winStack.getCount() > 0 )
	  w = (Window*)((winExecutor*)winStack.at(0))->wins.at(0);
       if ( !w && count > 0 )
	  w = getWin(current);
       if ( w )
	  ref = (char*)w->getHelpContext();
       flag = 1;
       if ( !ref )
	  ref = iref;
     }

   if ( !flag )
     {
       switch( type )
	{
	  case HASH_cmHelpPrev:
	      ref = 0;
	      flag = 1; break;
	  case HASH_cmHelpIndex:
	      ref = "Index";
	      flag = 1; break;
	  case HASH_cmHelpOnHelp:
	      ref = "Help";
	      flag = 1; break;
	}
    }
   if ( !flag )
      return;

   W3Win *w3 = 0;
   if ( ref ) {
       sprintf( buf1024, "file:%s%cyhelp.html#%s", yui_path, FILESEP, ref );
       w3 = new W3Win( buf1024, scrRect() );
   } else {
       w3 = new W3Win( 0, scrRect() );
   }

   w3->setHelpContext( "Help" );
   insertExecWindow( w3 );
#else
   test( sorry.get() );
#endif
}

//-------------------------------- menu -------------------------------
void Appl::makeMenu()
{
   static Menu *Smenu = 0;
   if ( Smenu )
     { menu = Smenu; return; }

   menuColl *global = new menuColl;

   menuColl *coll = new menuColl;
   coll->insert( new menuItem( new Lang("~File",	"~Файл"),	new Lang("Open exist file",	"Открыть существующий файл"), 0, HASH_cmOpen ) );
   coll->insert( new menuItem( new Lang("~New file",	"~Новый файл"),	new Lang("Open empty window",	"Открыть пустое окно"),	 0, HASH_cmNew ) );
   coll->insert( new menuItem( new Lang("~Terminal",	"~Терминал"),	new Lang("Open terminal window","Открыть окно терминала"), 0, HASH_cmOpenTerm ) );
   coll->insert( new menuItem( new Lang("~Debug",	"~Отладчик"),	new Lang("Open debug window",	"Открыть окно отладчика"), 0, HASH_cmOpenDebug ) );
   coll->insert( new menuItem( new Lang("~User menu",	"~Меню"),	new Lang("Open user menu",	"Меню пользователя"),	 0, HASH_cmUserMenu ) );
   coll->insert( new menuItem( new Lang("~Web client",	"~Web клиент"),	new Lang("Open Web client",	"Открыть окно Web"),	 0, HASH_cmOpenWeb ) );
   coll->insert( new menuItem( new Lang("~System command","~Команда"),	new Lang("System command",	"Системная команда"),	 0, HASH_cmSystemCommand ) );
   coll->insert( new menuItem( new Lang("System ~window","~Терминал с командой"),new Lang("System command in the window","Открыть окно терминала с командой"), 0, HASH_cmSystemWindow ) );
   coll->insert( new menuItem( new Lang("~Calculator",	"К~алькулятор"),new Lang("Calculator",		"Калькулятор"),		 0, HASH_cmOpenCalc ) );
   coll->insert( new menuItem( "C-tag", "C-tag", 0, HASH_cmCtagPrompt ) );
   Menu *sub = new Menu( coll, Point(1,0) );
   global->insert( new menuItem( new Lang("~Open","Открыть"), new Lang("Open window ...","Открыть окно ..."), 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Next","~Следующее"),	new Lang("Next window",		"Следующее окно"),	0, HASH_cmNextWin ) );
   coll->insert( new menuItem( new Lang("~Prev","~Предыдущее"),	new Lang("Previous window",	"Предыдущее окно"),	0, HASH_cmPrevWin ) );
   coll->insert( new menuItem( new Lang("~First","П~ервое"),	new Lang("First window",	"Первое окно"),		0, HASH_cmFirst ) );
   coll->insert( new menuItem( new Lang("~Last","П~оследнее"),	new Lang("Last window",		"Последнее окно"),	0, HASH_cmLast ) );
   coll->insert( new menuItem( new Lang("~Size","~Размер"),	new Lang("Window size",		"Изменить размер окна"),0, HASH_cmSize ) );
   coll->insert( new menuItem( new Lang("~Move","Перемес~тить"),new Lang("Window position",	"Переместить окно"),	0, HASH_cmPosition ) );
   coll->insert( new menuItem( new Lang("~Zoom","~Масштаб"),	new Lang("Window zoom",		"Масштабирование"),	0, HASH_cmZoom ) );
   coll->insert( new menuItem( new Lang("~Close","~Закрыть"),	new Lang("Close window",	"Закрыть окно"),	0, HASH_cmCloseWin ) );
   coll->insert( new menuItem( new Lang("Lis~t","Сп~исок"),	new Lang("Windows list",	"Список окон"),	0, HASH_cmChoose ) );
   coll->insert( new menuItem( new Lang("Ma~x size","Ма~кс. размер"),	new Lang("Set size for current window to maximum","Установить размер окна максимальным"),	0, HASH_cmMaxSize ) );
   sub = new Menu( coll, Point(1,6), Point(1,9) );
   global->insert( new menuItem( new Lang("W~indow","Окно"), new Lang("Window control","Управление окнами"), 0, sub ) );

#if 0
   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Common",	"~Общая"),	0,	0, HASH_cmKeymap ) );
   coll->insert( new menuItem( new Lang("~Edit",	"~Редактор"),	0,	0, HASH_cmKeymapEdit ) );
   coll->insert( new menuItem( new Lang("~Dialog",	"~Диалог"),	0,	0, HASH_cmKeymapDialog ) );
   coll->insert( new menuItem( new Lang("~Terminal",	"~Терминал"),	0,	0, HASH_cmKeymapTerm ) );
   sub = new Menu( coll, Point(4,27), Point(4,30) );

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Global",	"~Общие"),	0,	0, HASH_cmGlobalOptions ) );
   coll->insert( new menuItem( new Lang("~Debug",	"О~тладчик"),	0,	0, HASH_cmGdbInitFile ) );
   coll->insert( new menuItem( new Lang("~Keyboard",	"~Клавиатура"),	0,	0, sub ) );
   coll->insert( new menuItem( new Lang("~Save",	"~Сохранить"),	0,	0, HASH_cmSaveOptions ) );
   sub = new Menu( coll, Point(1,14), Point(1,15) );
   global->insert( new menuItem( new Lang("O~ptions",	"Настройки"),	new Lang("Options","Настройка программы"), 0, sub ) );
#else

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Options",	"~Настройки"),	0,	0, HASH_cmModifyOptions ) );
   coll->insert( new menuItem( new Lang("~Keymap",	"~Клавиши"),	0,	0, HASH_cmModifyKeymap ) );
   coll->insert( new menuItem( new Lang("~Colormap",	"~Цвета"),	0,	0, HASH_cmModifyColormap ) );
   coll->insert( new menuItem( new Lang("Color~trans",	"Цвет-~транс."),0,	0, HASH_cmModifyColortrans ) );
   coll->insert( new menuItem( new Lang("~Macro",	"~Макро"),	0,	0, HASH_cmModifyMacro ) );
   sub = new Menu( coll, Point(1,14), Point(1,15) );
   global->insert( new menuItem( new Lang("~Modify",	"Изменить"),	new Lang("Options","Настройка"), 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Options",	"~Настройки"),	0,	0, HASH_cmReloadOptions ) );
   coll->insert( new menuItem( new Lang("~Keymap",	"~Клавиши"),	0,	0, HASH_cmReloadKeymap ) );
   coll->insert( new menuItem( new Lang("~Colormap",	"~Цвет"),	0,	0, HASH_cmReloadColormap ) );
   coll->insert( new menuItem( new Lang("Color~trans",	"Цвет-~транс."),0,	0, HASH_cmReloadColortrans ) );
   coll->insert( new menuItem( new Lang("~Macro",	"~Макро"),	0,	0, HASH_cmReloadMacro ) );
   sub = new Menu( coll, Point(1,22), Point(1,25) );
   global->insert( new menuItem( new Lang("~Reload",	"Перечитать"),	new Lang("Options","Настройка"), 0, sub ) );

#endif

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Context",	"~Контекст"),	new Lang("Help context",	"Помощь по контексту"),	0, HASH_cmHelpContext ) );
   coll->insert( new menuItem( new Lang("~Previous",	"~Предыдущая"),	new Lang("Previous help",	"Предыдущая помощь"),	0, HASH_cmHelpPrev ) );
   coll->insert( new menuItem( new Lang("~On help",	"~О помощи"),	new Lang("Help on help",	"О системе помощи"),	0, HASH_cmHelpOnHelp ) );
   coll->insert( new menuItem( new Lang("~Index",	"~Содержание"),	new Lang("Help index",		"Оглавление системы помощи"),	0, HASH_cmHelpIndex ) );
   coll->insert( new menuItem( new Lang("~About",	"О п~рограмме"),new Lang("Copyright",		"Информация об авторах"),	0, HASH_cmAbout ) );
   coll->insert( new menuItem( new Lang("~man"), 			new Lang("Manual page window",	"Окно ~man~"),	0, HASH_cmOpenMan ) );
   sub = new Menu( coll, Point(1,30), Point(1,37) );
   global->insert( new menuItem( new Lang("~Help","Помощь"), new Lang("Help system","Система помощи"), 0, sub ) );

   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Normal",	"~Нормальный"),	new Lang(" Normal ~Quit~",	" Нормальный ~выход~"),	0, HASH_cmQuit ) );
   coll->insert( new menuItem( new Lang("With ~error",	"С ~ошибкой"),	new Lang(" ~Quit~ without status saving and return error code ~1~",	" ~Выход~ без сохранения статуса с возвратом кода ошибки ~1~"),	0, HASH_cmQuitError_1 ) );
   sub = new Menu( coll, Point(1,36), Point(1,45) );
   global->insert( new menuItem( new Lang("~Quit","Выход"),	new Lang("End of work","Конец работы"), 0, sub ) );

   Smenu = menu = new Menu( global, Point(0,0), new Lang("Sys","Общ"), 0, 1, 0 );
   menu->fill( &keyHolder );
   sharedMenu.insert( menu );
   menuMgr->select( menu );
}

static KeyMessage km( HASH_cmResizeScreen );
static struct sigaction act, oact;

void resize_windows( int sig )
{
   appl->messToQueue( &km );
}

void parse_highlight( const char *home )
{
  char buf[1024], buf2[1024], fname[MAXPATHLEN];
  struct TagInfo tinfo;
  struct stat st;
  const char *name="highlight", *fmt="%s/%s";
  tinfo.type = 0;
  if ( home )
     sprintf( fname, fmt, home, name );
  else
     sprintf( fname, fmt, yui_path, name );
  if ( stat( fname, &st ) )
   {
     if ( !home )
	return;
     sprintf( fname, fmt, yui_path, name );
   }
  FILE *f = fopen( fname, "rt" );
  if ( !f )
     return;

  SortedCollection *ext = 0;
  TagProcessor *tproc = 0;
  CommentTagProcessor *ctproc = 0;
  int punctuate = 1;
  tinfo.name = buf2;
  for( int cycle=1; cycle; ) {
     if ( fgets( buf1024, 1023, f ) <= 0 )
      {
	cycle=0;
	if ( ext )
	 {
	   if ( !tproc )
	      tproc = new TagProcessor( TAG_PROCESSOR_LEN );
	   goto ins_tag;
	 }
	break;
      }
     if ( sscanf( buf1024, "%[^# \t]%s", buf, buf2 ) != 2 )
	continue;

     tinfo.type = -1;
     if ( !strcasecmp( buf, "extension" ) ) {
	if ( tproc ) {
ins_tag:
	   if ( tproc )
	       tproc->pack();
	   if ( ctproc )
	       ctproc->pack();
	   HighLight *hl = 0;
	   if ( (tproc && !tproc->empty()) || (ctproc && !ctproc->empty()) || punctuate )
	       hl = (HighLight*)calloc( 1, sizeof(struct HighLight) );

	   if ( tproc && tproc->empty() ) {
	       delete tproc;
	       tproc = 0;
	   }

	   if ( ctproc && ctproc->empty() ) {
	       delete ctproc;
	       ctproc = 0;
	   }

	   if ( hl ) {
	      hl->ext = ext;
	      ext = 0;
	      hl->tproc = tproc;
	      tproc = 0;
	      hl->ctproc = ctproc;
	      ctproc = 0;
	      hl->punctuate = punctuate;
	      Edit::highlight.insert( hl );
	   }
	   if ( ext ) {
	       delete ext;
	   }
	   if ( tproc ) {
	       delete tproc;
	   }
	   if ( ctproc ) {
	       delete ctproc;
	   }

	   ext=0; tproc=0; ctproc=0;
	   punctuate = 1;
	   if ( !cycle )
	      continue;
	}

	if ( !ext )
	   ext = new SortedCollection(10,10);
	char *ch = new char[ strlen(buf2)+1 ];
	strcpy( ch, buf2 );
	ext->insert( ch );
	continue;
     }

     if ( !strcasecmp( buf, "tag" ) ) {
	if ( !tproc )
	   tproc = new TagProcessor( TAG_PROCESSOR_LEN );
	tproc->insertTag( &tinfo );
	continue;
     }

     if ( !strcasecmp( buf, "punctuate" ) ) {
	if ( !tproc )
	   tproc = new TagProcessor( TAG_PROCESSOR_LEN );
	if ( !strcasecmp( buf2, "no" ) )
	   punctuate = 0;
	continue;
     }

     if ( !strcasecmp( buf, "comment_eol" ) ) {
	if ( !ctproc )
	   ctproc = new CommentTagProcessor;
	tinfo.type = TAG_COMMENT_EOL;
	ctproc->insertTag( &tinfo );
	continue;
     }

     if ( !strcasecmp( buf, "comment_start" ) ) {
	if ( !ctproc )
	   ctproc = new CommentTagProcessor;
	tinfo.type = TAG_COMMENT_START;
	ctproc->insertTag( &tinfo );
	continue;
     }

     if ( !strcasecmp( buf, "comment_end" ) ) {
	if ( !ctproc )
	   ctproc = new CommentTagProcessor;
	tinfo.type = TAG_COMMENT_END;
	ctproc->insertTag( &tinfo );
	continue;
     }
  }
  fclose( f );
}

void parse_highlight_html( const char *home )
{
  char buf[1024], buf2[1024], fname[MAXPATHLEN];
  struct TagInfo tinfo;
  struct stat st;
  const char *name="highlight_html", *fmt="%s/%s";
  tinfo.type = 0;
  if ( home )
     sprintf( fname, fmt, home, name );
  else
     sprintf( fname, fmt, yui_path, name );
  if ( stat( fname, &st ) ) {
     if ( !home )
	return;
     sprintf( fname, fmt, yui_path, name );
  }
  FILE *f = fopen( fname, "rt" );
  if ( !f )
     return;

  unsigned param=0, hend=0, ret=0;
  tinfo.name = buf2;
  int ext_flag = 1;
  while( fgets( buf, 1023, f ) > 0 )
   {
     buf[1023] = 0;
     if ( sscanf( buf, "%[^# \t]%u%u", buf2, &param, &hend ) != 3 )
      {
	if ( sscanf( buf, "%[^# \t]%s", buf1024, buf2 ) != 2 )
	   continue;
	if ( !strcmp( buf1024, "extension" ) )
	 {
	   char *ch = new char[ strlen(buf2)+1 ];
	   strcpy( ch, buf2 );
	   Edit::HTMLextension.insert( ch );
	 }
	continue;
      }
     ext_flag = 0;
     tinfo.param = param;
     tinfo.hend = hend;
     Edit::HTproc.insertTag( &tinfo );
   }
  fclose( f );
  Edit::HTproc.pack();
}

StartInfoStruct *editName( const char *name )
{
  StartInfoStruct *sinfo = (StartInfoStruct*)calloc( 1, sizeof( StartInfoStruct ) );
  sinfo->type = T_EDIT;
  if ( strchr( name, '/' ) )
     strcpy( buf1024, name );
  else
     sprintf( buf1024, "%s/%s", filebox::filePath, name );
  sinfo->name = strdup( buf1024 );
  return sinfo;
}

extern char *optarg;
extern int optind, opterr, optopt;

int main( int argc, char *argv[] )
{
  act.sa_handler = resize_windows;
  sigemptyset( &act.sa_mask );
  act.sa_flags |= SA_RESTART;
  sigaction( SIGWINCH, &act, &oact );

  char *env=getenv( "YUI_PATH" );
#ifndef DJGPP
  yui_path = strdup( env ? env : "/usr/local/lib/yui" );
#else
  yui_path = strdup( env ? env : StartPath() );
#endif
  sprintf( buf1024, "%s/options", yui_path );
  name_options_common = strdup( buf1024 );

  sprintf( buf1024, "%s/colormap", yui_path );
  name_colormap_common = strdup( buf1024 );

  sprintf( buf1024, "%s/colortrans", yui_path );
  name_colortrans_common = strdup( buf1024 );

  sprintf( buf1024, "%s/keymap", yui_path );
  name_keymap_common = strdup( buf1024 );

  sprintf( buf1024, "%s/macro", yui_path );
  name_macro_common = strdup( buf1024 );

#ifndef DJGPP
  env = getenv("HOME");
  char *home = (char*)(env ? calloc( MAXPATHLEN, 1 ) : NULL);
  struct stat st;
  if ( home ) {
      sprintf( home, "%s/.yui", env );
      if ( stat( home, &st ) )
	  mkdir( home, S_IRUSR | S_IWUSR | S_IXUSR );
      strcat( home, "/" );
  }

  parse_highlight( home );
  parse_highlight_html( home );

  if ( home ) {
      char *ch = home + strlen( home );
      strcpy( ch, "status" );
      if ( stat( home, &st ) )
	  mkdir( home, S_IRUSR | S_IWUSR | S_IXUSR );
      strcat( home, "/" );
      getcwd( buf1024, 1023 );
      buf1024[1023] = 0;
      int i=strlen(home), j=0;
      for( ; buf1024[j]; i++, j++ ) {
	  if ( j > 0 && buf1024[j] == '/' )
	      buf1024[j] = ',';
	  home[i] = buf1024[j];
      }
      if ( j == 1 )
	  home[i++] = ',';
      home[i] = 0;
      name_status = strdup( home );

      strcpy( ch, "options" );
      name_options = strdup( home );

      strcpy( ch, "colormap" );
      name_colormap = strdup( home );

      strcpy( ch, "colortrans" );
      name_colortrans = strdup( home );

      strcpy( ch, "keymap" );
      name_keymap = strdup( home );

      strcpy( ch, "macro" );
      name_macro = strdup( home );
  }

  ::free( home );
#endif

#ifdef _USE_WWW_
   W3_Lib_Init();
#endif

   StartPath( argv[0] );
   strcpy( filebox::filePath, CurWorkPath() );
   strcpy( filebox::fileMask, "*" );
   opterr = 0;
   StartInfoStruct *sinfo = 0;
   int c=0, count=1;
   startInfo = new Collection( 10, 10 );
   for( ; (c = getopt( argc, argv, "m:M:w:r:b:t:" )) != EOF; count++ ) {
      for( ; count < optind-2; count++ )
	  startInfo->insert( editName( argv[ count ] ) );

      sinfo = (StartInfoStruct*)calloc( 1, sizeof( StartInfoStruct ) );
      sinfo->type = T_EDIT;
      switch( c ) {
	case 'm':
	    count++;
	    sinfo->type = T_WEB_MAN;
	    break;
	case 'M':
	    count++;
	    sinfo->type = T_EDIT_MAN;
	    break;
	case 'w':
	    count++;
	    sinfo->type = T_WEB;
	    sinfo->name = strdup( optarg );
	    break;
	case 'r':
	    count++;
	    sinfo->type = T_EDIT_RO;
	    break;
	case 'b':
	    count++;
	    sinfo->type = T_EDIT_BIN;
	    break;
	case ':':	// argument missing
	case '?':	// unknown option
	    sinfo->name = strdup( argv[ optind-1 ] );
	    break;
	case 't':
	    count++;
	    startCtag = strdup( optarg );
	    ::free( sinfo );
	    sinfo = NULL;
	    continue;
      }
      if ( !sinfo->name ) {
	  if ( strchr( optarg, '/' ) ) {
	      sinfo->name = strdup( optarg );
	  } else {
	      sprintf( buf1024, "%s/%s", CurWorkPath(), optarg );
	      sinfo->name = strdup( buf1024 );
	  }
      }
      startInfo->insert( sinfo );
   }

   for( ; count < argc; count++ )
       startInfo->insert( editName( argv[ count ] ) );

   for( ; opt_info[len_opt_info].name; len_opt_info++ );
   for( ; key_info[len_key_info].name; len_key_info++ );
   for( ; com_info[len_com_info].name; len_com_info++ );
   for( ; color_info[len_color_info].name; len_color_info++ );

   inputKeyTask = new InputKeyTask( NULL );
   Appl *a = new Appl;
   TaskHolder k( a );
   inputKeyTask->set_reciever( k.task() );
   TaskHolder key( inputKeyTask );

   a->makeMenu();

#if 0
#ifdef _USE_MMAP_
   signal( SIGSEGV, reload_by_SIGSEGV );
#endif
#endif

   Task::run();

#ifdef _USE_WWW_
   W3_Lib_End();
#endif

	if ( yui_path )
	    ::free( yui_path );
	if ( name_status )
	    ::free ( name_status );
	if ( name_options )
	    ::free ( name_options );
	if ( name_options_common )
	    ::free ( name_options_common );
	if ( name_colormap )
	    ::free ( name_colormap );
	if ( name_colormap_common )
	    ::free ( name_colormap_common );
	if ( name_keymap )
	    ::free ( name_keymap );
	if ( name_keymap_common )
	    ::free ( name_keymap_common );
	if ( name_macro )
	    ::free ( name_macro );
	if ( name_macro_common )
	    ::free ( name_macro_common );
	if ( startCtag )
	    ::free( startCtag );

   delete a;
   sharedMenu.freeAll();
   Term::destroy();

   while( Edit::highlight.getCount() > 0 ) {
       HighLight *hl = (HighLight*)Edit::highlight.at( 0 );
       if ( hl->ext )
	   delete hl->ext;
       if ( hl->tproc )
	   delete hl->tproc;
       if ( hl->ctproc )
	   delete hl->ctproc;
       ::free( hl );
       Edit::highlight.atRemove(0);
   }

   //return returnStatus;
   exit( returnStatus );
}

