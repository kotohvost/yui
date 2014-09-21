#ifdef DMALLOC
#include <dmalloc.h>
#endif

extern "C"
{
#include <HTTP.h>
}

#include <fcntl.h>
#include <math.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <program.h>
#include <yterm.h>
#include <filebox.h>
#include <status.h>

#include "w3man.h"
#include "w3private.h"
#include "hashcode.h"

#ifdef SINIX
extern "C" int strcasecmp( const char *s1, const char *s2 );
extern "C" int strncasecmp( const char *s1, const char *s2, size_t n );
#endif

PUBLIC char *HTAppName;
PUBLIC char *HTAppVersion;
PUBLIC HTList *active_reqs = 0;
extern char * HTHostName;

W3HistoryCollection W3Win::history;

int setW3Cache( char *path )
{
   static char dirname[MAXPATHLEN];
   dirname[0] = 0;
   if ( *path == '~' )
     {
       char *home = getenv( "HOME" );
       if ( home )
	  strcpy( dirname, home );
       if ( *(path+1) != FILESEP )
	   strcat( dirname, FILESEPSTR );
       if ( *path )
	  strcat( dirname, path+1 );
     }
   else strcpy( dirname, path );

   int len = strlen(dirname)-1;
   if ( dirname[ len ] == FILESEP )
      dirname[len] = 0;

   if ( !access( dirname, F_OK | R_OK | W_OK | X_OK ) ||
	!mkdir( dirname, S_IRUSR | S_IWUSR | S_IXUSR ) )
    {
       strcat( dirname, FILESEPSTR );
       HTCacheDir = dirname;
       return 1;
    }
   HTCacheDir = NULL;
   return 0;
}

HistoryBucket::HistoryBucket( HTAnchor *Anchor, HTParentAnchor *pan,
	char *Tag, char *Method, char *Data ) :
	anchor(Anchor), parent_anchor(pan), tag(0), method(0), data(0)
{
   if ( Tag )
      tag = strdup( Tag );
   if ( Method )
      method = strdup( Method );
   if ( Data )
      data = strdup( Data );
}

HistoryBucket::~HistoryBucket()
{
   if ( tag )
      ::free( tag );
   if ( method )
      ::free( method );
   if ( data )
      ::free( data );
}

int W3HistoryCollection::topInsert( HistoryBucket *buck )
{
   for( int i=0; i < count; i++ )
    {
      HistoryBucket *b = (HistoryBucket*)items[i];
      if ( buck->anchor == b->anchor && (!buck->tag && !b->tag || buck->tag && b->tag && !strcmp( buck->tag, b->tag ) ) )
	{
	  atRemove( i );
	  atInsert( 0, b );
	  delete buck;
	  return 0;
	}
    }
   atInsert( 0, buck );
   return 1;
}

HistoryBucket *W3HistoryCollection::search( const char *addr )
{
   for( int i=0; i < count; i++ )
    {
      HistoryBucket *b = (HistoryBucket*)items[i];
      char *anchor_addr = HTAnchor_address( b->anchor );
      if ( !strcmp( addr, anchor_addr ) )
	 return b;
    }
   return 0;
}

HistoryBucket *W3HistoryCollection::search( HTAnchor *anchor, char *tag )
{
   for( int i=0; i < count; i++ )
    {
      HistoryBucket *b = (HistoryBucket*)items[i];
      if ( anchor == b->anchor && (!tag && !b->tag || tag && b->tag && !strcmp(tag,b->tag)) )
	 return b;
    }
   return 0;
}

void W3HistoryCollection::updateCoor( ccIndex ind, PointL delta, Point cursor, long current )
{
   if ( ind < 0 || ind >= count )
      return;
   HistoryBucket *buck = (HistoryBucket*)items[ind];
   buck->delta = delta;
   buck->cursor = cursor;
   buck->current = current;
}

static char statusBuf[60] = "";

extern "C" int write_net( int desc, const char *buf, size_t count )
{
  while ( 1 )
  {
  Message *msg = 0;
  long key = 0;
  if ( (msg=Task::peekMessage()) )
   {
   if(  msg->type() == HASH_Key &&
       (key=((KeyMessage*)msg)->data) == 'i' || key=='I' || key==HASH_cmInterrupt )
   {
     appl->test( lang("Request interrupted.","Запрос прерван.") );
     Task::respond(msg);
     return HT_INTERRUPTED;
   }
    Task::respond(msg);
   }
  if ( !Task::waitWrite( desc, 1000 ) )
    continue;
  int ret=write( desc, (void*)(buf), count );

  W3Win *wp=0;
  for( HTList *list=active_reqs->next; list; list = list->next )
   {
     HTRequest *rq = (HTRequest*)(list->object);
     if ( buf == rq->isoc->input_buffer )
       {
	 wp = (W3Win*)(((winExecutor*)rq->context)->win);
	 break;
       }
   }

  char ch='|';

  if ( wp )
   {
     wp->b_read += ret;
     if ( wp == appl->topWindow )
      {
	switch( ch )
	 {
	   case '|': ch='/'; break;
	   case '/': ch='-'; break;
	   case '-': ch='\\'; break;
	   case '\\': ch='|';
	 }
	sprintf( statusBuf, " (%c) Writing: %d bytes", ch, wp->b_write );
	appl->statusLine->draw( statusBuf, 1 );
	Screen::hideCursor();
	Screen::sync( Screen::Lines-1, 1 );
      }
   }
  return ret;
  }
}

extern "C" int read_net( int desc, const char *buf, size_t count )
{
  while (1)
  {
  Message *msg = 0;
  long key = 0;

  if ( (msg=Task::peekMessage()) )
   {
    if ( msg->type() == HASH_Key &&
       (key=((KeyMessage*)msg)->data) == 'i' || key=='I' || key==HASH_cmInterrupt )
   {
     appl->test( lang("Request interrupted.","Запрос прерван.") );
     Task::respond(msg);
     return HT_INTERRUPTED;
   }
   Task::respond(msg);
   }

  if ( !Task::waitRead( desc, 1000 ) )
    continue;
  int ret=read( desc, (void*)(buf), count );

  W3Win *wp=0;
  for( HTList *list=active_reqs->next; list; list = list->next )
   {
     HTRequest *rq = (HTRequest*)(list->object);
     const char *test = rq->isoc->input_buffer;
     if ( !test )
	test = rq->net_info->isoc->input_buffer;
     if ( buf == test )
       {
	 wp = (W3Win*)(((winExecutor*)rq->context)->win);
	 break;
       }
   }

  char ch='|';
  if ( wp )
   {
     wp->b_read += ret;
     if ( wp == appl->topWindow )
      {
	switch( ch )
	 {
	   case '|': ch='/'; break;
	   case '/': ch='-'; break;
	   case '-': ch='\\'; break;
	   case '\\': ch='|';
	 }
	sprintf( statusBuf, " (%c) Reading: %d bytes", ch, wp->b_read );
	appl->statusLine->draw( statusBuf, 1 );
	Screen::hideCursor();
	Screen::sync( Screen::Lines-1, 1 );
      }
   }
  return ret;
  }
}

PRIVATE HTList *converters=0;
static char saveLocallyDir[MAXPATHLEN];

void W3_Lib_Init()
{
   HTAppName = "Yui";
   HTAppVersion	= "1.0";

   converters = HTList_new();
   active_reqs = HTList_new();

   HTFormatInit( converters );
   HTSetConversion( converters,"text/man",	"text/html",	ManToHTML,	1.0, 0.0, 0.0 );
   HTSetConversion( converters,"text/man-index","text/html",	ManIndexToHTML,	1.0, 0.0, 0.0 );
   HTSetConversion( converters,"text/apropos",	"text/html",	AproposToHTML,	1.0, 0.0, 0.0 );

   HTConversions = converters;
   CONST char * home =  (CONST char*)getenv("HOME");

   if ( home )
      strcpy( saveLocallyDir, home );
   else
      strcpy( saveLocallyDir, HTSaveLocallyDir );
   HTSaveLocallyDir = saveLocallyDir;
   setW3Cache( "/tmp/" );
}

void W3_Lib_End()
{
   HTFormatDelete( converters );
   HTList_delete( active_reqs );
   //HTCacheDeleteAll();
   if ( HTHostName )
    {
      ::free( HTHostName );
      HTHostName = NULL;
    }
}

static Lang w3status( " ~g~o ~/~info ~s~earch ~r~eload ~i~nterrupt ~d~ownload s~o~urce ~-~previous ~m~ain ~h~istory",
" ~g~-URL ~/~-инф ~s~-поиск ~r~-перечит. ~i~-прерв. ~d~-взять ~o~-исх. ~-~назад ~m~-начало ~h~-история" );

//static Lang loadInProgress("Document loading in progress","Происходит загрузка документа");
static Lang Yes( "  Yes  ", "  Да  " );
static Lang No( "  No  ", "  Нет  " );

static Lang Reload( "Reload document?", "Перечитать документ?" );
static Lang ReloadNet( "  Net  ", "  Сеть  " );
static Lang ReloadCache( " Cache ", "  Кэш  " );
static Lang Cancel( " Cancel ", " Отмена " );

unsigned char colorWebHyper[2] = {
	FG_HI_YELLOW | BG_BLUE,
	FG_HI_YELLOW | BG_CYAN };
unsigned char monoWebHyper[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };
unsigned char laptopWebHyper[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };

unsigned char colorWebInput[2] = {
	FG_HI_WHITE | BG_BLUE,
	FG_WHITE | BG_BLUE };
unsigned char monoWebInput[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };
unsigned char laptopWebInput[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };

unsigned char colorWebBox[2] = {
	FG_HI_WHITE | BG_BLUE,
	FG_WHITE | BG_BLUE };
unsigned char monoWebBox[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };
unsigned char laptopWebBox[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };

unsigned char colorWebButton[2] = {
	FG_HI_WHITE | BG_BLUE,
	FG_WHITE | BG_BLUE };
unsigned char monoWebButton[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };
unsigned char laptopWebButton[2] = {
	FG_HI_WHITE | BG_WHITE,
	FG_BLACK | BG_WHITE };

unsigned char monoWeb[16] =
{
	FG_HI_WHITE | BG_BLACK,		// рамка, окно верхнее
	FG_HI_WHITE | BG_BLACK,		// рамка, окно не верхнее
	FG_WHITE | BG_BLACK,		// обычный текст
	FG_BLACK | BG_WHITE,		// выделенное в блок
	FG_HI_WHITE | BG_BLACK,		// найденное при поиске
	FG_HI_WHITE | BG_BLACK,		// атрибут 1
	FG_HI_WHITE | BG_BLACK,		// атрибут 2
	FG_HI_WHITE | BG_BLACK,		// атрибут 3
	FG_HI_WHITE | BG_BLACK,		// атрибут 4
	FG_HI_WHITE | BG_BLACK,		// атрибут 5
	FG_HI_WHITE | BG_BLACK,		// атрибут 6
	FG_HI_BLACK | BG_BLACK,		// атрибут 7
	FG_WHITE | BG_BLACK,		// атрибут 8
	FG_WHITE | BG_BLACK,		// атрибут 9
	FG_WHITE | BG_BLACK,		// атрибут 10
	FG_WHITE | BG_BLACK,		// атрибут 11
};

unsigned char colorWeb[16] =
{
	FG_HI_WHITE | BG_CYAN,		// рамка, окно верхнее
	FG_WHITE | BG_CYAN,		// рамка, окно не верхнее
	FG_BLACK | BG_CYAN,		// обычный текст
	FG_BLACK | BG_GREEN,		// выделенное в блок
	FG_BLACK | BG_WHITE,		// найденное при поиске
	FG_HI_CYAN  | BG_CYAN,		// атрибут 1
	FG_HI_WHITE | BG_CYAN,		// атрибут 2
	FG_HI_WHITE | BG_CYAN,		// атрибут 3
	FG_HI_CYAN  | BG_CYAN,		// атрибут 4
	FG_HI_WHITE | BG_CYAN,		// атрибут 5
	FG_HI_WHITE | BG_CYAN,		// атрибут 6
	FG_HI_CYAN  | BG_CYAN,		// атрибут 7
	FG_HI_WHITE | BG_CYAN,		// атрибут 8
	FG_HI_WHITE | BG_CYAN,		// атрибут 9
	FG_HI_CYAN  | BG_CYAN,		// атрибут 10
	FG_HI_WHITE | BG_CYAN		// атрибут 11
};

unsigned char laptopWeb[16] =
{
	FG_HI_WHITE | BG_BLACK,		// рамка, окно верхнее
	FG_HI_WHITE | BG_BLACK,		// рамка, окно не верхнее
	FG_WHITE | BG_BLACK,		// обычный текст
	FG_BLACK | BG_WHITE,		// выделенное в блок
	FG_HI_WHITE | BG_BLACK,		// найденное при поиске
	FG_BLACK | BG_WHITE,		// атрибут 1
	FG_HI_WHITE | BG_BLACK,		// атрибут 2
	FG_HI_WHITE | BG_BLACK,		// атрибут 3
	FG_HI_WHITE | BG_BLACK,		// атрибут 4
	FG_HI_WHITE | BG_BLACK,		// атрибут 5
	FG_HI_WHITE | BG_BLACK,		// атрибут 6
	FG_HI_WHITE | BG_BLACK,		// атрибут 7
	FG_WHITE | BG_BLACK,		// атрибут 8
	FG_WHITE | BG_BLACK,		// атрибут 9
	FG_WHITE | BG_BLACK,		// атрибут 10
	FG_WHITE | BG_BLACK,		// атрибут 11
};

unsigned char monoWebList[11] = {
	FG_HI_WHITE | BG_WHITE,    // active frame (WINDOW)
	FG_BLACK | BG_WHITE,        // normal frame (WINDOW)
	FG_WHITE | BG_BLACK,	    // item, object is active
	FG_WHITE | BG_BLACK,	    // item, not active
	FG_HI_WHITE | BG_BLACK,    // selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // selected item, not active
	FG_HI_WHITE | BG_BLACK,    // current item, object is active
	FG_HI_WHITE | BG_BLACK,    // current item, not actve
	FG_HI_WHITE | BG_BLACK,    // current selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // current selected item, not actve
	FG_HI_WHITE | BG_BLACK	    // info
};

unsigned char colorWebList[11] = {
	FG_HI_WHITE | BG_WHITE,    // active frame (WINDOW)
	FG_BLACK | BG_WHITE,        // normal frame (WINDOW)
	FG_HI_CYAN | BG_BLUE,      // item, object is active
	FG_CYAN | BG_BLUE,          // item, not active
	FG_HI_YELLOW | BG_BLUE,    // selected item, object is active
	FG_HI_WHITE | BG_BLUE,     // selected item, not active
	FG_BLACK | BG_GREEN,        // current item, object is active
	FG_BLACK | BG_GREEN,        // current item, not actve
	FG_HI_YELLOW | BG_GREEN,   // current selected item, object is active
	FG_HI_WHITE | BG_GREEN,    // current selected item, not actve
	FG_HI_WHITE | BG_BLUE      // info
};

unsigned char laptopWebList[11] = {
	FG_HI_WHITE | BG_WHITE,    // active frame (WINDOW)
	FG_BLACK | BG_WHITE,        // normal frame (WINDOW)
	FG_WHITE | BG_BLACK,        // item, object is active
	FG_WHITE | BG_BLACK,        // item, not active
	FG_HI_WHITE | BG_BLACK,    // selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // selected item, not active
	FG_HI_WHITE | BG_BLACK,    // current item, object is active
	FG_HI_WHITE | BG_BLACK,    // current item, not actve
	FG_HI_WHITE | BG_BLACK,    // current selected item, object is active
	FG_HI_WHITE | BG_BLACK,    // current selected item, not actve
	FG_HI_WHITE | BG_BLACK     // info
};

long w3winKeyMap[] = {
	'h',		HASH_cmHistory,
	'm',		HASH_cmMainPage,
	's',		HASH_cmSearch,
	'o',		HASH_cmSource,
	'-',		HASH_cmPreviousPage,
	'/',		HASH_cmInfo,
	'd',		HASH_cmDownLoad,
	'D',		HASH_cmDownLoadNoAsk,
	'r',		HASH_cmReload,
	'R',		HASH_cmReloadNoAsk,
	'g',		HASH_cmGo,
	kbCtrlA,	HASH_cmPrev2,
	kbCtrlX,	HASH_cmNext2,
	FUNC1(kbTab),	HASH_cmMode,
	0
};

Keymap W3WinKeyMap( w3winKeyMap );

long anchorKeyMap[] = {
	'/',		HASH_cmInfo,
	'=',		HASH_cmInfo,
	'd',		HASH_cmDownLoad,
	'D',		HASH_cmDownLoad,
	kbEnter,	HASH_cmOK,
	0
};

Keymap AnchorKeyMap( anchorKeyMap );

// ][ W3Win class

static HistoryBucket *startBuck=0;

void W3Win::setColors(int Type)
{
   Dialog::setColors( Type );
   switch(Type)
     {
       case MONO:
	  clr = monoWeb; break;
       case COLOR:
	  clr = colorWeb; break;
       case LAPTOP:
	  clr = laptopWeb; break;
     }
}

int W3Win::isType(long typ)
{
  return ( typ==HASH_W3Win ? 1 : Dialog::isType(typ) );
}

W3Win::W3Win( const char *StartRef, Rect r ):
       Dialog((r.a.x==-1 && r.a.y==-1 && r.b.x==-1 && r.b.y==-1) ? scrRect() : r,
       new Lang( " WWW browser ", " Смотрелка WWW "),
       new Lang( " WWW " ),
       new Lang( w3status.get(0), w3status.get(1) ), 0 ),
       mainRef(0),
       startRef(0),
       startMethod(0),
       updateStatus(0),
       first(1),
       cleared(0),
       base_href(0), base_target(0)
{
  keyHolder.add( &W3WinKeyMap );
  setColors( colorsType );
  stk_len = 65536*4;
  parent_anchor = 0;
  node_anchor = 0;
  submit_data = 0;
  text = 0;
  request = 0;
  b_read = b_write = 0;
  if ( StartRef )
   {
     startRef=strdup(StartRef);
     addSlash(&startRef);
   }
  else if ( history.getCount() > 0 )
     startBuck = (HistoryBucket*)history.at( 0 );
}

W3Win::~W3Win()
{
  history.updateCoor( 0, delta, cursor, current );
  if ( mainRef )
    ::free( mainRef );
  if ( startRef )
    ::free( startRef );
  if ( startMethod )
    ::free( startMethod );
  if ( base_href )
    ::free( base_href );
  if ( base_target )
    ::free( base_target );
  if ( submit_data )
    ::free( submit_data );
  winMenu=0;
}

long W3Win::runWin(Window *win)
{
  if ( !win )
    return -1;
  return exec( win );
}

extern "C" int w3system( const char *cmd, HTRequest *request )
{
  W3Win *win = (W3Win*)HTAnchor_document( request->anchor );
  if ( !win )
     return system( cmd );
  YTerm *yt = new YTerm( cmd );
  long ret = win->runWin( yt );
  delete yt;
  return ret;
}

void W3Win::newStart( const char* href, const char *Method, HTParentAnchor *pan )
{
  b_read = b_write = 0;
  if ( !href )
    return;

  if ( startRef )
    ::free( startRef );
  startRef = strdup( href );

  if ( startMethod )
    ::free( startMethod );
  startMethod = Method ? strdup( Method ) : 0;

  parent_anchor = pan;
  first=1;
}

/*
* В связи с отсутствием переопределяемого хандлера эта функция может быть
* вызвана только когда request равен 0
*/
int W3Win::close()
{
/*
  if ( request )
   {
     HTRequest_delete(request);
     // ??? Активное ожидание - надо заменить на семафор?
//     while(request)
//        sleep(100);
   }
*/
  if ( history.getCount() > 0 && node_anchor == ((HistoryBucket*)history.at(0))->anchor )
     history.updateCoor( 0, delta, cursor, current );
  return 1;
}

/*
* Зачем вообще было переопределять?

int W3Win::init( void *data )
{
  int ret=Dialog::init(data);
  return ret;
}
*/

int W3Win::testLoaded()
{
  if (!node_anchor)
    {
      appl->test( lang("Document not loaded.\nUse 'g' to start.","Документ не загружен.\nИспользуйте 'g' для старта.") );
      return 0;
    }
/*
  if( request )
    {
      appl->test( loadInProgress.get() );
      return 0;
    }
*/
  return 1;
}


void W3Win::makeMenu( Menu *m )
{
   static Menu *menu=0;

   if (menu)
      { winMenu=menu; return; }

   menuColl *global = new menuColl;

   menuColl *coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Go to URL",	"Новый ~URL"),	new Lang(" Manual choice of URL"," Новый URL" )    , 0, HASH_cmGo ) );
   coll->insert( new menuItem( new Lang("~Information",	"Ин~формация"),	new Lang(" Information about current document and selected reference"," Информация о текущем документе и выбранной ссылке" ) , 0, HASH_cmInfo ) );
   coll->insert( new menuItem( new Lang("~Search",	"~Поиск"),	new Lang(" Enter search string for current document"," Поиск в текущем документе" ), 0, HASH_cmSearch ) );
   coll->insert( new menuItem( new Lang("~Interrupt",	"П~рервать"),	new Lang(" Interrupt current request", " Прервать текущий запрос" ), 0, HASH_cmInterrupt ) );
   coll->insert( new menuItem( new Lang("~Download",	"Приня~ть"),	new Lang(" Download current document ~or~ selected reference to local file"," Принять текущий документ или выбранную ссылку в локальный файл" ), 0, HASH_cmDownLoad ) );
   coll->insert( new menuItem( new Lang("S~ource view",	"~Исходник"),	new Lang(" View document source"," Просмотр исходного текста документа" ), 0, HASH_cmSource ) );
   coll->insert( new menuItem( new Lang("~Previous page","~Назад"),	new Lang(" Go to ~previous~ hypertext page"," Переход на предыдущую страницу гипертекста" ), 0, HASH_cmPreviousPage ) );
   coll->insert( new menuItem( new Lang("~Main page",	"~Главная"),	new Lang(" Go to ~main (start)~ hypertext page"," Переход на начальную страницу" ), 0, HASH_cmMainPage ) );
   coll->insert( new menuItem( new Lang("~Reload",	"П~еречитать"),	new Lang(" Reload current document"," Перечитать документ" ), 0, HASH_cmReload ) );
   coll->insert( new menuItem( new Lang("~History",	"~История"),    new Lang(" Select page from history list"," Выбрать страницу из истории"), 0, HASH_cmHistory ) );
   Menu *sub = new Menu( coll, Point( 1, 0 ) );
   global->insert( new menuItem( new Lang("~Navigation","Навигация"),	new Lang(" ~W~orld ~W~ide ~W~eb navigation"," Навигация по Всемирной Паутине"), 0, sub ) );

   coll	= new menuColl;
   coll->insert( new menuItem( new Lang("~1) original"),	0, 0, HASH_cmTransNone ) );
   coll->insert( new menuItem( new Lang("~2) 866  -> koi8"),	0, 0, HASH_cmTransAltKoi8 ) );
   coll->insert( new menuItem( new Lang("~3) 1251 -> koi8"),	0, 0, HASH_cmTransWinKoi8 ) );
   coll->insert( new menuItem( new Lang("~4) main -> koi8"),	0, 0, HASH_cmTransMainKoi8 ) );
   coll->insert( new menuItem( new Lang("~5) koi8 -> 866"),	0, 0, HASH_cmTransKoi8Alt ) );
   coll->insert( new menuItem( new Lang("~6) koi8 -> 1251"),	0, 0, HASH_cmTransKoi8Win ) );
   coll->insert( new menuItem( new Lang("~7) koi8 -> main"),	0, 0, HASH_cmTransKoi8Main ) );
   sub = new Menu( coll, Point(1,12), Point(1,11) );
   global->insert( new menuItem( new Lang("~View","Кодировка"),	0, 0, sub ) );

   menu = winMenu = new Menu( global, Point(0,0), new Lang("WWW"), 0, 1, 0 );
   winMenu->fill(&keyHolder);
   sharedMenu.insert( winMenu );
}

Collection W3Win::historyUrl(10,10);

long W3Win::handleKey( long key, void *&ptr )
{
  if ( updateStatus ) {
      W3Win *w = (W3Win*)(owner ? owner : this);
      w->setStatus( w3status.get() );
      updateStatus=0;
  }

  key = keyHolder.translate( Dialog::handleKey( key, ptr ) );
  long ret=0, Key=key;

  switch( key )
   {
     case 0:
       if ( first && (startRef||startBuck) )
	{
	  if ( startRef )
	   {
	     newRef( startRef, 0, startMethod );
	     startRef = 0;
	     if ( startMethod )
		::free( startMethod );
	     startMethod = 0;
	   }
	  else
	   {
	     PointL _delta( startBuck->delta );
	     Point _cursor( startBuck->cursor );
	     long _current = startBuck->current;
	     newRef( 0, startBuck->anchor, 0, &_delta, &_cursor, &_current );
	     startBuck=0;
	   }
	  first = 0;
	}
       break;
     case HASH_cmMessage:
       key=0; break;
     case HASH_cmHistory:
       {
	 listBox *lb = new listBox( Screen::center(Screen::Lines-10,Screen::Columns-6), Screen::Columns-6, 0 );
	 for( int i=0; i < history.getCount(); i++ )
	  {
	    HistoryBucket *buck = (HistoryBucket*)history.at(i);
	    char *url = HTAnchor_address( buck->anchor );
	    if (buck->tag && !strchr(url, '#') )
	     {
	       int le=strlen(url)+strlen(buck->tag)+3;
	       char *s=(char*)malloc(le);
	       strcpy(s, url);
	       strcat(s, "#");
	       strcat(s, buck->tag);
	       ::free(url);
	       url=s;
	     }
	    lb->add( url );
	    ::free(url);
	  }

	 lb->setTitle( lang(" History "," История ") );
	 if ( runWin(lb) == HASH_cmOK && history.getCount() > 1 )
	  {
	    int curr = lb->getCurrent();
	    HistoryBucket *buck = (HistoryBucket*)history.at( curr );
	    if ( buck->anchor != node_anchor )
	     {
	       Dialog *w = owner ? owner : this;
	       ((W3Win*)w)->newRef( buck->tag, buck->anchor, 0, &buck->delta, &buck->cursor, &buck->current );
	     }
	    else
	     {
	       if ( buck->tag )
		  newAnchor( (char*)buck->tag, 1, &buck->delta, &buck->cursor );
	       else
		{
		  history.updateCoor( 0, delta, cursor, current );
		  setCursor( buck->delta, buck->cursor );
		  history.atRemove( curr );
		  history.atInsert( 0, buck );
		}
	     }
	  }
	 delete lb;
       }
       key=0; break;
     case HASH_cmMainPage:
       if ( isGetObj )
	  break;
       if ( !testLoaded() )
	 { key=0; break; }
       if ( mainRef )
	  newRef( strdup(mainRef) );
       key=0; break;
     case HASH_cmSearch:
       if ( isGetObj )
	  break;
       if ( !testLoaded() )
	 { key=0; break; }
       if ( !HTAnchor_isIndex( HTAnchor_parent(node_anchor) ) )
	  { test( lang( "Document is not searchable.\nUse 'F6' to text search.",
			"Операция поиска неприменима.\nИспользуйте 'F6' для текстового поиска.") ); return 0; }
       {
	 static Collection hist;
	 char *keywords = getString( lang("Keyword:","Ключевое слово:"), 40, &hist );
	 if ( keywords && node_anchor )
	  {
	    char *base_url = HTAnchor_address( (HTAnchor *)HTAnchor_parent(node_anchor) );
	    if (*keywords) {
		char *plus;
		strtok(base_url, "#");
		strtok(base_url, "?");
		StrAllocCat(base_url, "?");
		StrAllocCat(base_url, keywords);
		plus = (char*)strchr(base_url, '?');
		while (*plus) {
		if (*plus == ' ') *plus = '+';
		plus++;
		}
	    }
	    newRef( base_url );
	  }
       }
       key=0; break;
     case HASH_cmSource:
       {
	 if ( !testLoaded() )
	   { key=0; break; }

	 HTList *conv = HTList_new();
	 HTSetConversion( conv, "*/*", "www/present", HTPlainPresent,
		       1.0, 0.0, 0.0 );
	 HTList *saveConv = HTConversions;
	 HTConversions = conv;
	 newRef( 0, (HTAnchor*)node_anchor );
	 HTConversions = saveConv;
       }
       key=0; break;
     case HASH_cmPreviousPage:
       {
	 W3HistoryCollection *hist = owner ? &frameHistory : &history;
	 if ( !testLoaded() || hist->getCount() < 2 )
	   { key=0; break; }

	 int r_ind=1;
	 HistoryBucket *buck = (HistoryBucket*)hist->at( r_ind );
	 if ( buck->anchor != node_anchor )
	    newRef( 0, buck->anchor, 0, &buck->delta, &buck->cursor, &buck->current );
	 else
	  {
	    hist->updateCoor( 0, delta, cursor, current );
	    select( buck->current );
	    setCursor( buck->delta, buck->cursor );
	    r_ind = 0;
	  }
	 buck = (HistoryBucket*)hist->at( r_ind );
	 hist->atRemove( r_ind );
	 hist->insert( buck );
       }
       key=0; break;
     case HASH_cmInfo:
       {
	 if ( !testLoaded() )
	   { key=0; break; }

	 Collection coll;
	 coll.insert( strdup( lang("document reference:","ссылка документа:") ) );
	 coll.insert( HTAnchor_address( (HTAnchor*)node_anchor) );
	 if ( base_href )
	  {
	     coll.insert( strdup(lang("base reference","базовая ссылка")) );
	     coll.insert( strdup(base_href) );
	  }
	 test( &coll, lang("URL info","Информация URL") );
       }
       key=0; break;
     case HASH_cmDownLoad:
       if ( !testLoaded() )
	 { key=0; break; }
       {
       Collection coll;
       coll.insert( strdup( lang("Download current document?","Принять текущий документ?") ) );
       coll.insert( HTAnchor_address( (HTAnchor*)node_anchor) );

       if ( test( &coll, 0, HASH_cmOK, Yes.get(), HASH_cmNo, No.get() ) == HASH_cmOK )
	 {
	   HTList *conv = HTList_new();
	   HTSetConversion( conv, "*/*", "www/present", HTSaveLocally,
		       1.0, 0.0, 0.0 );
	   HTList *saveConv = HTConversions;
	   HTConversions = conv;
	   newRef( 0, (HTAnchor*)node_anchor );
	   HTConversions = saveConv;
	   HTConversions = conv;
	 }
       }
       key=0; break;
     case HASH_cmDownLoadNoAsk:
       if ( !testLoaded() )
	 { key=0; break; }
       {
       HTList *conv = HTList_new();
       HTSetConversion( conv, "*/*", "www/present", HTSaveLocally,
		     1.0, 0.0, 0.0 );
       HTList *saveConv = HTConversions;
       HTConversions = conv;
       newRef( 0, (HTAnchor*)node_anchor );
       HTConversions = saveConv;
       }
       key=0; break;
     case HASH_cmReloadNoAsk:
       newRef( 0, (HTAnchor*)node_anchor );
       key=0; break;
     case HASH_cmReload:
       if ( !testLoaded() )
	 { key=0; break; }
       {
	 HTList *cacheItems = HTAnchor_parent(node_anchor)->cacheItems;
	 if ( cacheItems )
	    ret = test( Reload.get(), 0,
			HASH_cmReloadNet, ReloadNet.get(),
			HASH_cmReloadCache, ReloadCache.get(),
			HASH_cmCancel, Cancel.get() );
	 else
	    ret = test( Reload.get(), 0,
			HASH_cmYes, Yes.get(),
			HASH_cmNo, No.get() );
	 if ( ret == HASH_cmCancel || ret == HASH_cmNo )
	   { key=0; break; }
	 if ( ret == HASH_cmReloadNet )
	    HTCacheClear( cacheItems );
	 W3Win *w = this;
#if 0
	 while( w->owner )
	   w = (W3Win*)w->owner;
#endif
	 w->newRef( 0, w->node_anchor );
       }
       key=0; break;
     case HASH_cmGo:
       if ( isGetObj )
	  break;
       {
       char *url = getString( "Location:", Screen::Columns - 30, &historyUrl );
       if ( !url || !url[0] )
	 { key=0; break; }
       url=strdup(url);
       if ( !memcmp(url, "file:",5) )
	{
	  if ( strlen(url) > 5 )
	   {
	     if (url[5]!='/')      // add current path
	       {
		 int le=strlen(url);
		 char *buf = new char[MAXPATHLEN];
		   getcwd( buf, MAXPATHLEN );
		   int le1=strlen(buf);
		 char *nurl=(char*)malloc(le+le1+2);
		 memcpy(nurl, "file:", 5);
		 memcpy(nurl+5, buf, le1);
		 nurl[le1+5]='/';
		 memcpy(nurl+le1+6, url+5, le-5);
		 nurl[le+le1+1]=0;
		 ::free(url);
		 url=nurl;
		 delete buf;
	       }
	   }
	  else
	   {
	     filebox *fb=new filebox( filebox::filePath, "*.html" );
	     ret=runWin(fb);
	     if ( ret == HASH_cmOK )
	       {
		 char *new_path=fb->getName();
		 int le=strlen(new_path);
		 char *new_url=new char[le+6];
		 memcpy( new_url, "file:", 5);
		 memcpy( new_url+5, new_path, le );
		 new_url[le+5]=0;
		 url=new_url;
		 delete fb;
	       }
	     else
	       {
		 delete fb;
		 key=0;
		 break;
	       }
	   }
	}
       addSlash(&url);
       newRef( url );
       }
       key=0; break;
   }

  if ( key == HASH_cmOK )
     key=0;

  int flagRef = 0;
  if ( startRef )
   {
     newRef( startRef, 0, startMethod );
     startRef = 0;
     flagRef = 1;
   }

  if ( Key && !key )
   {
     correctCursor();
     curPos = delta + PointL( cursor.y, cursor.x );
     setObjmap(1);
   }

  if ( (flagRef || !key) && draw() )
     Screen::sync();

  return key;
}

/*
void W3Win::showProgress ( W3ProgressMessage *msg )
{
    switch (msg->op) {
      case HT_PROG_DNS:
	setStatus( lang(" Looking up host %s"," Поиск машины %s"),  msg->input );
	break;
      case HT_PROG_CONNECT:
	setStatus( lang(" Connecting with host"," Соединение с машиной") );
	break;
      case HT_PROG_ACCEPT:
	setStatus( lang(" Waiting for connection ..."," Ожидание соединения ...") );
	break;
      case HT_PROG_READ:
	{
	    long cl = HTAnchor_length(HTRequest_anchor(msg->request));
	    long b_read = HTRequest_bytesRead(msg->request);
	    char buf[10];
	    HTNumToStr((unsigned long) b_read, buf, 10);
	    if (cl > 0)
	      {
		char buf1[10];
		double pro = (double) b_read/cl*100;
		HTNumToStr((unsigned long) cl, buf1, 10);
		setStatus( lang(" Read %ld of %ld (%d%%)"," Считано %ld из %ld (%d%%)"), b_read, cl, (int) pro );
	      }
	    else
		setStatus( lang(" Read %ld"," Считано %ld"), b_read);
	}
	break;
      case HT_PROG_WRITE:
	setStatus( lang(" Writing ...", " Запись ...") );
	break;
      case HT_PROG_DONE:
	setStatus( lang(" Finished"," Закончено") );
	break;
      case HT_PROG_WAIT:
	setStatus( lang(" Waiting for free socket ..."," Ожидание освобождения socket'а") );
	break;
      default:
	setStatus( lang(" ~Unknown progress state~"," ~Неизвестное состояние процесса~") );
	break;
    }
  Screen::sync();
}
*/

PUBLIC HTLink * HTAnchor_findLink (HTAnchor * src, HTAnchor * dest)
{
    if (src && dest) {
	if (src->mainLink.dest == dest)
	    return &(src->mainLink);
	if (src->links) {
	    HTList *cur = src->links;
	    HTLink *pres;
	    while ((pres = (HTLink *) HTList_nextObject(cur)) != NULL) {
		if (pres->dest == dest)
		    return pres;
	    }
	}
    }
    return NULL;
}

int W3Win::newRef( char *addr, HTAnchor *anchor, const char* method, PointL *_delta, Point *_cursor, long *_cur )
{
  PointL saveDelta=delta;
  Point  saveCursor=cursor;
  long   saveCurrent=current;
  cleared=0;
  b_read = b_write = 0;
  current = -1;

  HTParentAnchor *panchor=0;
  HTAnchor *save_node_anchor = node_anchor;
  BOOL res=NO, freeSubmitData=YES;
  char *tag=0;
  HistoryBucket *b = 0;
  W3HistoryCollection *hist = owner ? &frameHistory : &history;
  W3Win *w = (W3Win*)(owner ? owner : this);

  request=HTRequest_new();
  request->context = Executor;

  if ( anchor )
   {
     if ( addr ) { tag=addr; addr=0; }
     b = hist->search( anchor, tag );
     if ( b )
      {
_anchor:
	method = b->method;
	parent_anchor = b->parent_anchor;
	if ( !submit_data )
	 {
	   submit_data = b->data;
	   freeSubmitData = NO;
	 }
      }
   }
  else
   {
      if ( this == appl->topWindow )
	{
	  appl->statusLine->draw( lang(" Searching ..."," Поиск ..."), 1 );
	  Screen::hideCursor();
	  Screen::sync( Screen::Lines-1, 1 );
	}
      if ( !(anchor = HTAnchor_findAddress( addr )) )
	{
	  char *buf = new char[512];
#if defined(SOLARIS_SPARC) || defined(SINIX)
	  sprintf( buf, lang("Can't find\n%s","Невозможно найти\n%s"), addr );
#else
	  snprintf( buf, 512, lang("Can't find\n%s","Невозможно найти\n%s"), addr );
#endif
	  buf[512]=0;
	  appl->test( buf );
	  delete buf;
	  goto end_process;
	}
      if ( this == appl->topWindow )
	{
	  appl->statusLine->draw( lang( " Connecting ..."," Соединение ..." ), 1 );
	  Screen::hideCursor();
	  Screen::sync( Screen::Lines-1, 1 );
	}
      if ( (b=hist->search(HTAnchor_address(anchor))) )
	{
	  anchor = b->anchor;
	  goto _anchor;
	}
   }
  if ( method )
   {
      HTMethod Method = HTMethod_enum( (char*)method );
      request->method = Method;
   }

  if ( !(panchor=HTAnchor_parent(anchor)) )
     goto end_process;
  if ( HTAnchor_document( panchor ) )
   {
     appl->test( lang("Anchor busy","Ссылка занята") );
     goto end_process;
   }
  node_anchor = anchor;
  request->parentAnchor = parent_anchor;
  HTAnchor_setDocument( panchor, (HyperDoc*)this );

  HTList_addObject( active_reqs, request );
  res=HTLoadAnchor( anchor, request );
  HTList_removeObject( active_reqs, request );

  if ( res == NO )
   {
      node_anchor = save_node_anchor;
      current = saveCurrent;
   }
  else
   {
      if ( addr && (tag=(char*)strrchr( addr, '#' ) ) )
	 tag++;
      if ( tag )
	 newAnchor( tag, 0 );
      w->setStatus( lang(" Document loaded"," Документ принят") );
   }

end_process:
  if ( cleared && node_anchor != save_node_anchor )
    {
//      if ( !strstr( node_anchor->parent->address, "/cgi-bin/" ) )
//       {
	 hist->updateCoor( 0, saveDelta, saveCursor, saveCurrent );
	 hist->topInsert( new HistoryBucket( node_anchor, parent_anchor, tag, (char*)method, submit_data ) );
//       }
      const char *tit=HTAnchor_title( panchor );
      if (!tit)
	tit=lang( "untitled", "без имени" );
      if ( HTAnchor_isIndex( panchor ) )
	setTitle( lang(" %s (searchable) "," %s (с поиском) "), tit);
      else
	setTitle(" %s ", tit);
    }
  if ( !mainRef )
     mainRef = HTAnchor_address( (HTAnchor*)panchor );
  text=0;
  if ( submit_data )
    {
      if ( freeSubmitData == YES )
	 delete submit_data;
      submit_data = 0;
    }
  if ( parent_anchor )
     parent_anchor = 0;

  HTAnchor_setDocument(panchor, 0);
  HTRequest_delete(request);
  request = 0;

  oldCur = current;
  if (addr)
    ::free(addr);
  if ( _cur )
     select( *_cur );
  if ( _delta && _cursor )
     setCursor( *_delta, *_cursor );
  if ( topFset )
   {
     void *ptr=0;
     for( int i=0; i < count; i++ )
      {
	getobj *g = (getobj*)items[i];
	if ( g->isType( HASH_W3Win ) )
	   ((W3Win*)g)->handleKey( 0, ptr );
      }
   }
  if ( owner )
    owner->draw( 1 );
  appl->drawStatus();
  return res;
}

int W3Win::newAnchor( char *tag, int correctHist, PointL *_delta, Point *_cursor )
{
  for( int i=0; i < count; i++ )
   {
     getobj *obj = (getobj*)at(i);
     if ( !obj->isType( HASH_AnchorHyper ) )
	continue;
     char *htag = ((AnchorHyper*)obj)->anchor->tag;
     if ( !htag || strcmp( htag, tag ) )
	continue;
     if ( correctHist )
       {
	 history.updateCoor( 0, delta, cursor, current );
	 history.topInsert( new HistoryBucket( node_anchor, 0, tag ) );
       }
     select( i );
     if ( _delta && _cursor )
	setCursor( *_delta, *_cursor );
     bindCursor = 1;
     return 1;
   }
  return 0;
}

void W3Win::processSelect( getobj *Obj )
{
 W3Win *w = (W3Win*)(owner ? owner : this);
 if ( Obj->isType(HASH_AnchorHyper) )
  {
    AnchorHyper *h = (AnchorHyper*)Obj;
    HTAnchor *link_dest = HTAnchor_followMainLink((HTAnchor *)h->anchor);
    char *addr=HTAnchor_address(link_dest);
    if (addr)
     {
       w->setStatus( " %s", addr );
       ::free(addr);
     }
    else if (h->anchor->tag)
       w->setStatus( " #%s", h->anchor->tag );
    else
       w->setStatus( " (null)" );
    updateStatus=1;
  }
 else if ( Obj->isType(HASH_FormSubmitButton) )
  {
    FormSubmitButton * fobj = (FormSubmitButton *) Obj->getObj();
    w->setStatus( " ~%s~ %s", fobj->form->method, fobj->form->action );
  }
}

// W3Win class ][ AnchorHyper class

int AnchorHyper::isType(long typ)
{
  return ( typ==HASH_AnchorHyper ? 1 : hyper::isType(typ) );
}

AnchorHyper::AnchorHyper( int y, int x, const char *tit,
	HTChildAnchor *Anchor, HTParentAnchor *parent, char *anchor_target ):
     hyper(Point(y,x), tit), anchor(Anchor), _target(0)
{
//  HTAnchor *link_dest = HTAnchor_followMainLink(  (HTAnchor *)anchor);
//  HTParentAnchor *panchor=HTAnchor_parent(link_dest);
//  if ( panchor == parent /*|| HTAnchor_cacheHit( panchor )*/ )
//    setColor( &CachedColors );
//  else
//    hyper::setColor( &HyperColors  );
  keyHolder.add( &AnchorKeyMap );
  setColors( colorsType );
  if ( anchor_target )
     _target = strdup( anchor_target );
}

AnchorHyper::~AnchorHyper()
{
  if ( _target )
     ::free( _target );
}

void AnchorHyper::setColors(int Type)
{
  switch(Type)
   {
      case MONO:
	 clr = monoWebHyper; break;
      case COLOR:
	 clr = colorWebHyper; break;
      case LAPTOP:
	 clr = laptopWebHyper; break;
   }
}

/*
AnchorHyper::~AnchorHyper()
{
}
*/

long AnchorHyper::handleKey( long key, void *&data )
{
  key = keyHolder.translate( key );
  switch( key )
   {
     case HASH_cmInfo:
       {
	 W3Win * win=(W3Win *) owner;
	 Collection coll;
	 coll.insert( strdup(lang("document reference:","ссылка документа:")) );
	 coll.insert( HTAnchor_address( (HTAnchor*)(win->node_anchor)) );
	 if (win->base_href)
	  {
	     coll.insert( strdup(lang("base reference","базовая ссылка")) );
	     coll.insert( strdup( win->base_href ) );
	  }
	 coll.insert( strdup(lang("current reference:","текущая ссылка:")) );

	 HTAnchor *link_dest = HTAnchor_followMainLink(  (HTAnchor *)anchor);
	 char *addr=HTAnchor_address(link_dest);
	 if (!addr) addr=strdup("(null)");
	 coll.insert( addr );
	 char *ra=HTParse(HTAnchor_address( (HTAnchor*)win->node_anchor), addr, PARSE_ANCHOR);

	 if (ra[0])
	   coll.insert(ra);
	 else
	   free(ra);

	 owner->test(&coll, lang("URL info","Информация URL") );
	 return 0;
       }
     case HASH_cmOK:
       {
	 W3Win *win = (W3Win*)owner;
	 HTAnchor *link_dest = HTAnchor_followMainLink( (HTAnchor*)anchor );
	 char *addr = HTAnchor_address(link_dest);
	 if ( !addr )
	    return 0;
	 addSlash( &addr );
	 if ( _target )
	  {
	    if ( !strcmp( _target, "_blank" ) )		// в новое окно
	     {
	       W3Win *w = new W3Win( addr );
	       win = w;
	       appl->nselect( appl->insert( win ) );
	       return 0;
	     }
	    if ( !strcmp( _target, "_parent" ) && win->owner )	// на 1 уровень вверх
	     {
	       win = (W3Win*)win->owner;
	       win->newStart( addr );
	       return 0;
	     }
	    if ( !strcmp( _target, "_top" ) )	// на самый верх
	     {
	       while( win->owner )
		  win = (W3Win*)win->owner;
	       win->newStart( addr );
	       return 0;
	     }
	    if ( strcmp( _target, "_self" ) )		// поиск нужного под-окна
	     {
	       FrameSet *fs = win->topFset;
	       if ( !fs && win->owner )
		  fs = ((W3Win*)win->owner)->topFset;
	       if ( fs )
		{
		  W3Win *w = (W3Win*)fs->findTarget( _target );
		  if ( w )
		   {
		     win = w;
		     if ( win  != (W3Win*)owner )
			win->frameHistory.freeAll();
		   }
		}
	     }
	  }

	 char *ra = HTParse( HTAnchor_address( (HTAnchor*)win->node_anchor), addr, PARSE_ANCHOR );
	 if ( !ra[0] || (HTAnchor*)HTAnchor_parent(link_dest) != win->node_anchor )
	    win->newRef( addr );
	 if ( ra[0] )
	    win->newAnchor( ra, 1 );
	 ::free( ra );
       }
       return 0;
     case HASH_cmDownLoad:
       {
	 W3Win * win=(W3Win *) owner;
	 HTAnchor *link_dest = HTAnchor_followMainLink( (HTAnchor*)anchor );
	 char *addr = HTAnchor_address( link_dest );
	 if ( !addr )
	    return 0;
	 Collection coll;
	 coll.insert( strdup( lang("Download document?","Принять документ?") ) );
	 coll.insert( addr );

	 if ( owner->test( &coll, 0, HASH_cmOK, Yes.get(), HASH_cmNo, No.get() ) == HASH_cmOK )
	   {
	     HTList *conv = HTList_new();
	     HTSetConversion( conv, "*/*", "www/present", HTSaveLocally,
			   1.0, 0.0, 0.0 );
	     HTList *saveConv = HTConversions;
	     HTConversions = conv;
	     win->newRef( strdup(addr) );
	     HTConversions = saveConv;
	   }
       }
       return 0;
   }
  return key;
}

// AnchorHyper class ]

