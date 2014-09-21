/*
	$Id: yui1.cc,v 3.2.2.1 2007/07/24 09:58:11 shelton Exp $
*/
#include <sys/types.h>
#ifdef _USE_MMAP_
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <sys/file.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <menu.h>
#include <status.h>
#include <filebox.h>
#include <program.h>
#include <tag.h>

#include "yui.h"
#include "hashcode.h"

#define EDIT_STACK_LEN	16384

static int tag_len=0;
static int ctag_len=0;
static int htag_len=0;
static int tag_type=-1;
static int htag_type=-1;
static int tag_state=TAG_NONE;
static int htag_state=TAG_NONE;
static int flagRevBlock=0;

char Edit::Buf1[maxLineLength];
char Edit::Buf2[maxLineLength];
HTMLTagProcessor Edit::HTproc;
SortedCollection Edit::HTMLextension(5,5);
Collection Edit::highlight(5,5);
int Edit::showSpace=0;
int Edit::stepTab=8;

int trans_flag_edit = TRANS_none;
int trans_flag_web = TRANS_none;
int trans_flag_term = TRANS_none;
int trans_flag_debug = TRANS_none;

/*
#ifdef _USE_MMAP_
Edit *currentEdit=0;
#endif
*/

EditOptions::EditOptions() : options(322), tab(2), timeSave(300),
		stepUndo(100), lineLength(72), parOffset(8)
{
  Edit::stepTab = (1 << (tab+1)) - 1;
}

EditOptions Edit::eOpt;

#ifndef DJGPP
Edit *Edit::debugEdit=0;
#endif

Menu *EditMenu=0;
Menu *EditMenuGetObj=0;

long editKeyMap[] = {
	kbHome,		HASH_cmHome,
	kbEnd,		HASH_cmEnd,
	kbPgUp,		HASH_cmPgUp,
	kbPgDn,		HASH_cmPgDn,
	kbLeft,		HASH_cmLeft,
	kbCtrlX,	HASH_cmLeft2,
	kbRight,	HASH_cmRight,
	kbCtrlB,	HASH_cmRight2,
	kbUp,		HASH_cmUp,
	kbCtrlF,	HASH_cmUp2,
	kbDown,		HASH_cmDown,
	kbCtrlV,	HASH_cmDown2,
	kbF2,		HASH_cmSave,
	FUNC1(kbF2),	HASH_cmSaveAs,
	kbF4,		HASH_cmGoto,
	FUNC1(kbF4),	HASH_cmMarkGotoLine,
	kbF5,		HASH_cmManual,
	kbF6,		HASH_cmFind,
	FUNC1(kbF6),	HASH_cmReplace,
	kbCtrlL,	HASH_cmFindContinue,
	kbF7,		HASH_cmBlockLine,
	kbF8,		HASH_cmBlockColumn,
	kbCtrlP,	HASH_cmUnmarkBlock,
	kbCtrlU,	HASH_cmUndo,
	kbCtrlR,	HASH_cmRedo,
	kbCtrlG,	HASH_cmMathDown,
	kbCtrlT,	HASH_cmMathUp,
	kbF9,		HASH_cmCopy,
	kbF10,		HASH_cmMove,
	FUNC1('+'),	HASH_cmCopyToClip,
	FUNC2('+'),	HASH_cmAddToClip,
	FUNC2(kbCtrlE),	HASH_cmClipboard,
	FUNC1(kbDel),	HASH_cmMoveToClip,
	FUNC1(kbIns),	HASH_cmCopyFromClip,
	FUNC1(kbPgUp),	HASH_cmFuncPgUp,
	FUNC1(kbPgDn),	HASH_cmFuncPgDn,
	kbIns,		HASH_cmIns,
	kbEnter,	HASH_cmEnter,
	kbCtrlY,	HASH_cmDeleteLine,
	kbCtrlD,	HASH_cmTruncate,
	FUNC1(kbCtrlY),	HASH_cmDeleteBlock,
	FUNC1(kbCtrlB),	HASH_cmBreakpoint,
	kbDel,		HASH_cmDelete,
	kbBS,		HASH_cmBackspace,
	FUNC2(kbCtrlV),	HASH_cmSpaceMode,
	FUNC1(kbCtrlL),	HASH_cmInputCode,
	FUNC2(kbCtrlL),	HASH_cmReload,
	FUNC2(kbCtrlR),	HASH_cmMode,
//	FUNC2(kbCtrlO),	HASH_cmShowMan,
	kbCtrlO,	HASH_cmSmartFormat,
	FUNC2(kbCtrlO),	HASH_cmDumbFormat,
	kbCtrlZ,	HASH_cmBackTab,
	0
};

#if 0
CommandDescr editCommandDescr[] = {
	{ HASH_cmSave,		new Lang("Save text","Сохранить текст") },
	{ HASH_cmSaveAs,	new Lang("Save text under new name","Сохранить под новым именем") },
	{ HASH_cmGoto,		new Lang("Go to line","Переход на строку") },
	{ HASH_cmMarkGotoLine,	new Lang("Mark line for Go To service","Запоминание номера строки") },
	{ HASH_cmBlockLine,	new Lang("Lines block marker","Маркер строчного блока") },
	{ HASH_cmBlockColumn,	new Lang("Columns block marker","Маркер колоночного блока") },
	{ HASH_cmUnmarkBlock,	new Lang("Unmark block","Снять маркер блока") },
	{ HASH_cmMathDown,	new Lang("Match symbol forward from cursor","Совпадающий символ ниже курсора") },
	{ HASH_cmMathUp,	new Lang("Match symbol backward from cursor","Совпадающий символ выше курсора") },
	{ HASH_cmCopy,		new Lang("Block copying","Копирование блока") },
	{ HASH_cmMove,		new Lang("Block moving","Перемещение блока") },
	{ HASH_cmCopyToClip,	new Lang("Block copying to clipboard","Копирование блока в буфер обмена") },
	{ HASH_cmMoveToClip,	new Lang("Block moving to clipboard","Перемещение блока в буфер обмена") },
	{ HASH_cmCopyFromClip,	new Lang("Block copying from clipboard","Копирование блока из буфера обмена") },
	{ HASH_cmDeleteLine,	new Lang("Line deleting","Удаление строки") },
	{ HASH_cmDeleteBlock,	new Lang("Block deleting","Удаление блока") },
	{ HASH_cmTruncate,	new Lang("Truncate current string","Усечение текущей строки") },
	{ HASH_cmSmartFormat,	new Lang("Smart formatting","Умное форматирование") },
	{ HASH_cmDumbFormat,	new Lang("Dumb formatting","Тупое форматирование") },
	{ HASH_cmBreakpoint,	new Lang("Set/unset breakpoint","Поставить/снять точку останова") },
	{ HASH_cmSpaceMode,	new Lang("Show spaces and tab-symbols","Видимость пробелов и табуляций") },
	{ HASH_cmInputCode,	new Lang("Input symbol by code","Ввод символа по коду") },
	{ HASH_cmReload,	new Lang("Reload file","Перечитать файл") },
	{ HASH_cmMode,		new Lang("Read only mode","Режим 'только чтение'") },
	{ HASH_cmUndo,		new Lang("Undo service","Откат") },
	{ HASH_cmRedo,		new Lang("Redo service","Обратный откат") },
	{ HASH_cmManual,	new Lang("Manual pages","Страницы 'man'") },
	{ HASH_cmShowMan,	new Lang("Browse mode set to 'manual pages'","Режим просмотра 'страница man'") },
	{ HASH_cmFind,		new Lang("Search","Поиск") },
	{ HASH_cmFindContinue,	new Lang("Continue search","Продолжение поиска") },
	{ HASH_cmReplace,	new Lang("Search and replace","Поиск с заменой") },
	{ HASH_cmDown2,		new Lang("Scroll up without cursor moving","Скроллинг вверх без перемещения курсора") },
	{ HASH_cmUp2,		new Lang("Scroll down without cursor moving","Скроллинг вниз без перемещения курсора") },
	{ HASH_cmLeft2,		new Lang("Next word","На следующее слово") },
	{ HASH_cmRight2,	new Lang("Previous word","На предыдущее слово") },
	{ 0, 0 }
};
#endif

Keymap EditKeyMap( editKeyMap, new Lang("Text editor commands","Команды в окне 'редактор'") );

unsigned char monoEdit[17] = {
	FG_HI_WHITE | BG_BLACK,		// active
	FG_WHITE | BG_BLACK,		// norm
	FG_WHITE | BG_BLACK,		// text
	FG_BLACK | BG_WHITE,		// block
	FG_HI_WHITE | BG_BLACK,		// find
	FG_BLACK | BG_WHITE,		// endLine
	FG_BLACK | BG_WHITE,		// code
	FG_HI_WHITE | BG_BLACK,		// debugLine
	FG_BLACK | BG_WHITE,		// breakpoint
	FG_HI_WHITE | BG_BLACK,		// man
	FG_WHITE | BG_BLACK,		// comment
	FG_WHITE | BG_BLACK,		// preproc
	FG_WHITE | BG_BLACK,		// ht_tag
	FG_WHITE | BG_BLACK,		// ht_par_name
	FG_WHITE | BG_BLACK,		// ht_par_val
	FG_HI_WHITE | BG_BLACK,		// highlight1
	FG_HI_WHITE | BG_BLACK		// highlight2
};

unsigned char colorEdit[17] = {
	FG_HI_YELLOW | BG_BLUE,		// active
	FG_HI_CYAN | BG_BLUE,		// norm
	FG_HI_CYAN | BG_BLUE,		// text
	FG_BLACK | BG_CYAN,		// block
	FG_BLACK | BG_WHITE,		// find
	FG_BLACK | BG_GREEN,		// endLine
	FG_WHITE | BG_BLUE,		// code
	FG_HI_WHITE | BG_GREEN,		// debugLine
	FG_HI_WHITE | BG_RED,		// breakpoint
	FG_HI_YELLOW | BG_BLUE,		// man
	FG_WHITE | BG_BLUE,		// comment
	FG_HI_GREEN | BG_BLUE,		// preproc
	FG_HI_YELLOW | BG_BLUE,		// ht_tag
	FG_HI_MAGENTA | BG_BLUE,	// ht_par_name
	FG_HI_GREEN | BG_BLUE,		// ht_par_val
	FG_HI_WHITE | BG_BLUE,		// highlight1
	FG_HI_YELLOW | BG_BLUE		// highlight2
};

unsigned char laptopEdit[17] = {
	FG_WHITE | BG_BLACK,		// active
	FG_WHITE | BG_BLACK,		// norm
	FG_WHITE | BG_BLACK,		// text
	FG_BLACK | BG_WHITE,		// block
	FG_HI_WHITE | BG_BLACK,		// find
	FG_BLACK | BG_WHITE,		// endLine
	FG_BLACK | BG_WHITE,		// code
	FG_HI_WHITE | BG_BLACK,		// debugLine
	FG_BLACK | BG_WHITE,		// breakpoint
	FG_HI_WHITE | BG_BLACK,		// man
	FG_BLACK | BG_WHITE,		// code
	FG_BLACK | BG_WHITE,		// code
	FG_BLACK | BG_WHITE,		// ht_tag
	FG_BLACK | BG_WHITE,		// ht_par_name
	FG_BLACK | BG_WHITE,		// ht_par_val
	FG_BLACK | BG_WHITE,		// highlight1
	FG_BLACK | BG_WHITE		// highlight2
};

void Edit::setColors( int type )
{
   switch( type )
    {
      case MONO:
	  clr = monoEdit;
	  break;
      case COLOR:
	  clr = colorEdit;
	  break;
      case LAPTOP:
	  clr = laptopEdit;
    }
}

Lang Yes( "  Yes  ", "  Да  " );
Lang No( "  No  ", "  Нет  " );
Lang Cancel( " Cancel ", " Отмена " );
Lang Warning( "Warning", "Внимание" );
Lang openError( "Can't open file\n%s", "Ошибка открытия файла\n%s" );
Lang existWarning( "File\n%s%\nalready exist. Overwrite?", "Файл\n%s\nуже существует. Переписать?" );
Lang writeError( "Write error, stop.", "Ошибка записи, стоп." );
Lang reloadWarning( "There is a change in the text.\nReload?","В тексте есть изменения.\nПеречитать?");

BpointArray::BpointArray()
{
}

Edit::Edit( const char *tit, Rect r, Lang *Status, int flag_binary ) :
	    Window( r, new Lang(tit), 0, Status ? Status : new Lang("~F3~-open  ~F2~-save  ~^J,F3~-new","~F3~-открыть  ~F2~-сохранить  ~^J,F3~-новый") ),
	    atext(0), cmap(NULL), getstrbuf(NULL),
#ifndef	DJGPP
	    debugLine(-1),
#endif
	    uInfo(0), Undo(0), externCollection(0), saveTimer(0), tproc(0),
	    ctproc(0), htproc(0),
	    punctuate(0), comment(0), hash_is_comment(0), readOnlyMode(RDONLY_0), shared(0)
{
   trans_flag = trans_flag_edit;
   stk_len = EDIT_STACK_LEN;
   keyHolder.add( &EditKeyMap/*, editCommandDescr*/ );
   freeAll();
   setLimit(1);
   changed = new short(0);
   saved = new short(0);
   fInfo = new FileStat;
   fInfo->s.st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
   fInfo->s.st_uid = getuid();
   fInfo->s.st_gid = getgid();
   CLEAR_BINARY; CLEAR_MAN; CLEAR_BLOCK; CLEAR_FREEZE;

   char *name = title->get();
   if ( !name || !(*name) ) {
       atext = new infoArray( 100, 100 );
   } else if ( !readFile( name, flag_binary ) ) {
       strError = lang( "Read error.", "Ошибка чтения файла." );
   }

   unmarkBlock( NO_UNDO );
   SET_INS;
}

Edit::Edit( Collection *ed, const char *tit, Rect r, Lang *Status ) :
	    Window( r, new Lang(tit), 0, Status ? Status : new Lang("~F3~-open  ~F2~-save  ~^J,F3~-new","~F3~-открыть  ~F2~-сохранить  ~^J,F3~-новый") ),
	    atext(0), cmap(NULL), getstrbuf(NULL),
#ifndef	DJGPP
	    debugLine(-1),
#endif
	    uInfo(0), Undo(0), externCollection(ed), saveTimer(0), tproc(0),
	    ctproc(0), htproc(0),
	    punctuate(0), comment(0), hash_is_comment(0), readOnlyMode(RDONLY_0), shared(0)
{
   trans_flag = trans_flag_edit;
   stk_len = EDIT_STACK_LEN;
   keyHolder.add( &EditKeyMap/*, editCommandDescr*/ );
   freeAll();
   setLimit(1);
   changed = new short(0);
   saved = new short(0);
   fInfo = new FileStat;
   fInfo->s.st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
   fInfo->s.st_uid = getuid();
   fInfo->s.st_gid = getgid();
   CLEAR_BINARY; CLEAR_MAN; CLEAR_BLOCK; CLEAR_FREEZE;
   readCollection();
   unmarkBlock( NO_UNDO );
   SET_INS;
}

Edit::Edit( Edit *e, Rect r ) :
	Window( r, new Lang((char*)e->winName()),
	new Lang( e->ident->get(0), e->ident->get(1) ),
	new Lang( e->status->get(0), e->status->get(1) ) ),
	atext(e->atext), cmap(e->cmap), getstrbuf(NULL), fInfo(e->fInfo),
#ifndef	DJGPP
	    debugLine(-1),
#endif
	    uInfo(e->uInfo), Undo(e->Undo),
	    externCollection(e->externCollection), saveTimer(0), tproc(e->tproc),
	    ctproc(e->ctproc), htproc(e->htproc),
	    punctuate(e->punctuate), comment(0), hash_is_comment(e->hash_is_comment),
	    readOnlyMode(e->readOnlyMode), shared(e->shared),
	    changed(e->changed), saved(e->saved)
{
   trans_flag = trans_flag_edit;
   stk_len = EDIT_STACK_LEN;
   keyHolder.add( &EditKeyMap/*, editCommandDescr*/ );
   freeAll();
   setLimit(1);
   winMenu = e->winMenu;
   if ( !shared )
     {
       shared = new Collection( 5, 2 );
       shared->insert( e );
       e->shared = shared;
       e->setWinIdent();
     }
   shared->insert( this );
   unmarkBlock( NO_UNDO );
   SET_INS;
   CLEAR_BINARY; CLEAR_MAN; CLEAR_BLOCK; CLEAR_FREEZE;
   if ( *changed ) SET_CHANGE; else CLEAR_CHANGE;
}

int Edit::close()
{
   if ( shared || !FLAG_CHANGE )
      return 1;
   int ret = 0;
   sprintf( Buf1, lang("%s\nnot saved. Save?","'%s'\nне сохранен. Сохранить?"), title->get() );
   modal *m = new modal( ALIGN_CENTER, (char*)Buf1, Warning.get(), HASH_cmOK, Yes.get(), HASH_cmNo, No.get(), HASH_cmCancel, Cancel.get() );
   long r = execUp( m );
   switch( r )
    {
      case HASH_cmOK:
	  ret = externCollection ? saveCollection() : saveText();
	  break;
      case HASH_cmNo:
	  ret = 1;
    }
   return ret;
}

Edit::~Edit()
{
   if ( winMenu )
       winMenu = 0;
   if ( saveTimer ) {
       saveTimer->close();
       Task::delref( saveTimer );
   }

   if ( shared ) {
       shared->remove( this );
       if ( shared->getCount() < 2 ) {
	   Edit *e = (Edit*)shared->at(0);
	   e->shared = 0;
	   e->setWinIdent();
	   shared->atRemove( 0 );
	   delete shared;
	   shared = 0;
       }
       return;
   }

   if ( shouldDelete ) {
       clearText();
       if ( atext ) {
	   delete atext;
	   atext = NULL;
      }
   }

   if ( changed )
      delete changed;

   if ( saved )
      delete saved;

#ifdef _USE_MMAP_
   if ( cmap )
      munmap( cmap, fInfo->s.st_size );
#endif

   if ( getstrbuf )
       ::free( getstrbuf );

   if ( Undo ) {
      for( int i=0; i < uInfo->max; i++ )
	Undo[i].freeData();
      ::free( Undo );
   }
   if ( uInfo )
      delete uInfo;
   if ( fInfo )
      delete fInfo;
}

int Edit::init( void *data )
{
   Window::init();

   if ( !uInfo )
      uInfo = new undoInfo( eOpt.stepUndo );
   if ( !Undo && uInfo->max > 0 )
     {
       Undo = (undoItem*)malloc( uInfo->max * sizeof(struct undoItem) );
       for( int i=0; i< uInfo->max; i++ )
	  Undo[i].init();
       uInfo->cur = uInfo->count = uInfo->fullCount = 0;
     }
   if ( !getHelpContext() )
      setHelpContext( "Edit" );

   setWinIdent();
   if ( !saveTimer )
     {
	saveTimer = new Timer( Executor, HASH_cmSave, eOpt.timeSave );
	Task::addref( saveTimer );
	saveTimer->start();
     }
   if ( saveTimer )
     {
       if ( AUTOSAVE(eOpt.options) )
	  saveTimer->open();
       else
	  saveTimer->close();
     }
   setWinIdent();
   setColors( colorsType );
   return 1;
}

void Edit::moveCursor()
{
   if ( find_Info.found )
      find_Info.found = 0;
   Point cur = getCursor();
   if ( cur.y == scr.lines - 1 && appl->statusLine->visible )
      Screen::hideCursor();
   else
      Screen::move( cur );
}

Point Edit::getCursor( int *hide )
{
   Point p = rect.a + cursor;
   if ( box )
      p += Point(1,1);
   return p;
}

void Edit::setChange( int flag )
{
   *changed = flag ? 1 : 0;
}

void Edit::info()
{
   if ( !box )
      return;
   long c = BOTT;
   int attr = (active && this==appl->topWindow) ? clr[0] : clr[1];
   int pr = c>0 ? min( 100, (delta.y+size.y)*100/c ) : 100;
   sprintf( Buf1, "<%li><%li><%i%%>", LINE+1, COLUMN+1, pr );

   Point pa = rect.a, pb = rect.b;
   char **saveMap = Screen::currentMap;
   if ( !isGetObj ) {
      Screen::currentMap = scr.map;
   } else {
      pa += owner->corrPoint;
      pb += owner->corrPoint;
   }
   Screen::put( pa.y, pa.x+2, strlen(Buf1) < (size_t)size.x ? (u_char*)Buf1 : (u_char*)"*", attr );
   int X, Y;
   if ( pb.y >= scr.lines || appl->statusLine->visible && pb.y == appl->statusLine->rect.b.y ) {
       Y = pa.y;
       X = pb.x;
   } else {
       Y = pb.y;
       X = pa.x;
   }
   if ( readOnlyMode != RDONLY_0 ) {
       if ( FLAG_MAN )
	 Screen::put( Y, X+(X==pa.x ? 2 : -9), (u_char*)"<M>", attr );
       else
	 Screen::put( pa.y, pb.x-9, (u_char*)lang("<R>","<Ч>"), attr );
   } else {
       if ( FLAG_BINARY )
	  Screen::put( Y, X+(X==pa.x ? 2 : -10), (u_char*)(X==pa.x ? "<binary>" : "B"), attr );
       Screen::put( pa.y, pb.x-9, FLAG_INS ? (u_char*)lang("<I>","<В>") : (u_char*)lang("<O>","<З>"), attr );
       if ( FLAG_CHANGE )
	  Screen::put( Y, X+(X==pa.x ? 2 : -6), (u_char*)"*", attr );
   }

   char *tr=NULL;
   switch( trans_flag ) {
       case TRANS_none:
	   tr = NULL; break;
       case TRANS_alt2koi8:
	   tr = "<ak>"; break;
       case TRANS_koi82alt:
	   tr = "<ka>"; break;
       case TRANS_win2koi8:
	   tr = "<wk>"; break;
       case TRANS_koi82win:
	   tr = "<kw>"; break;
       case TRANS_main2koi8:
	   tr = "<mk>"; break;
       case TRANS_koi82main:
	   tr = "<km>"; break;
   }
   if ( tr )
       Screen::put( pa.y, pb.x-13, (unsigned char*)tr, attr );

   if ( shared )
       Screen::put( Y, pb.x-(X==pa.x ? 7 : 1), (u_char*)(X==pa.x ? lang("<link>","<линк>"):"L"), attr );

   if ( !isGetObj )
       Screen::currentMap = saveMap;

   MoveCursor();
}

void Edit::setWinIdent()
{
   if ( *changed )
      SET_CHANGE;
   else
      CLEAR_CHANGE;
   static char bufIdent[3];
   bufIdent[0] = FLAG_CHANGE ? '*' : (readOnlyMode ? 'R':' ');
   bufIdent[1] = shared ? 'L' : ' ';
   bufIdent[2] = 0;
   ident->put( bufIdent, language );
}

short *Edit::scrLine( long line, int &len, int &isReload, int full )
{
   static short screenStr[512];
   unsigned short attrNorm = clr[2] << 8;
   unsigned short attrCode = clr[6] << 8;
   unsigned short attrMan = clr[9] << 8;
   unsigned short attrComment = clr[10] << 8;
   unsigned short attrHash = clr[11] << 8;
   unsigned short attrHtag = clr[12] << 8;
   unsigned short attrHpar_name = clr[13] << 8;
   unsigned short attrHpar_val = clr[14] << 8;
   unsigned short attrHL1 = clr[15] << 8;
   unsigned short attrHL2 = clr[16] << 8;
   unsigned short attrCur = attrNorm;
   int colorSyntax = HIGHLIGHT(eOpt.options) && !FLAG_BINARY && !FLAG_MAN;
   if ( !colorSyntax )
       attrHL1=attrHL2=attrComment=attrHash=attrHtag=attrHpar_name=attrHpar_val=attrCur=attrNorm;

   strInfo *si = STRINFO( line );
   if ( !si )
       return 0;

   unsigned char *s = (u_char*)(si->map ? cmap + (long)si->str : si->str);
   int pos=0, realPos=0, slash=0, hash=0, mul=0, flagMan=0;
   comment = 0;

   tag_len=ctag_len=htag_len=0;
   tag_state = htag_state = TAG_NONE;
   if ( tproc )
       tproc->reset();
   if ( ctproc ) {
       ctproc->reset_prev();
       ctproc->reset();
   }
   if ( htproc )
      htproc->reset();
   for( ; pos<delta.x; pos++, realPos++ ) {
      switch( s[realPos] ) {
	 case '#':
	    if ( hash_is_comment )
		goto dfl;
	    slash=mul=0;
	    if ( !hash )
		hash = 1;
	    break;
	 case '\b':
	    slash=mul=0;
	    if ( !hash )
	       hash = -1;
	    if ( !FLAG_BINARY && FLAG_MAN )
	       { flagMan=1; pos-=2; continue; }
	    break;
	 default:
dfl:
	    slash=mul=0;
	    switch( s[realPos] ) {
	       case '\t':
		   pos += stepTab - (pos & stepTab);
	       case ' ':
		   break;
	       default:
		   if ( !hash )
		      hash = -1;
	    }
	    if ( colorSyntax ) {
		if ( ctproc ) {
		    ctproc->checkTag( s[realPos], tag_type, ctag_len );
		    switch ( tag_type ) {
			case TAG_COMMENT_EOL:
			    comment = 1; break;
			case TAG_COMMENT_START:
			    comment = 2; break;
			case TAG_COMMENT_END:
			    comment = 0; break;
		    }
		}
		if ( hash <= 0 && tproc && tag_type == -1 )
		    tag_state = tproc->checkTag( s[realPos], tag_type, tag_len );
		if ( htproc )
		    htag_state = htproc->checkTag( s[realPos], htag_type, htag_len );
	    }
	    if ( flagMan ) {
	       attrCur = attrMan;
	       flagMan = 0;
	    } else if ( hash > 0 ) {
	       attrCur = attrHash;
	    } else switch( htag_state ) {
		 case TAG_START:
		 case TAG_NAME:
		 case TAG_PARAM_WAIT:
		 case TAG_EQU_WAIT:
		 case TAG_VAL_WAIT:
		     attrCur = attrHtag;
		     break;
		 case TAG_PARAM_NAME:
		     attrCur = attrHpar_name;
		     break;
		 case TAG_VAL_VALUE:
		     attrCur = attrHpar_val;
		     break;
		 default:
		     attrCur = comment ? attrComment : attrNorm;
	    }
      }
   }

   if ( si->len <= realPos )
      return 0;

   int flagFullString = full ? 1 : 0;

#ifndef DJGPP
   if ( line == debugLine && debugEdit == this ) {
      flagFullString = 1;
      attrHL1=attrHL2=attrCode=attrComment=attrHash=attrHtag=attrHpar_name=attrHpar_val=attrCur=attrNorm=clr[7] << 8;
   } else {
      BPinfo bp( line, -1 );
      ccIndex ind;
      if ( Bpoint.search( bp, ind ) ) {
	 flagFullString = 1;
	 attrHL1=attrHL2=attrCode=attrComment=attrHash=attrHtag=attrHpar_name=attrHpar_val=attrCur=attrNorm=clr[8] << 8;
      }
   }
#endif

   unsigned short space = showSpace ? attrCode | '.' : attrNorm | ' ';
   unsigned short spaceComment = space & 0xff;
   unsigned short spaceHash = spaceComment;
   unsigned short tab1 = attrNorm | ' ';
   unsigned short tab2 = showSpace ? attrCode | '_' : attrNorm | ' ';

   spaceComment |= attrComment;
   spaceHash |= attrHash;

   s+=realPos;
   len = 0;
   int punct = 0;
   if ( pos > delta.x ) {
       for( ; len < pos-delta.x; screenStr[len++] = tab1 );
       screenStr[len-1] = tab2;
   }

   for( int size_x=size.x-(box?2:0); len < size_x; pos++, realPos++, s++ ) {
      if ( realPos >= si->len ) {
	  if ( !flagFullString )
	     break;
	  screenStr[len++] = hash>0? spaceHash : (comment ? spaceComment : space);
	  continue;
      }
      switch( *s ) {
/*
#ifdef _USE_MMAP_
	  case '\n':
	      if ( !FLAG_BINARY )
	       {
		 reload( 0 ); isReload=1;
		 Task::sendMessage( Executor, new KeyMessage( HASH_cmRedraw ) );
		 return 0;
	       }
#endif
*/
	  case '\b':
	      slash=mul=0;
	      if ( !hash )
		 hash = -1;
	      if ( !FLAG_BINARY && FLAG_MAN )
		{ flagMan=1; len--; continue; }
	      goto m0;
	  case '#':
	      if ( hash_is_comment )
		  goto dfl2;
	      slash=mul=0;
	      if ( !hash )
		  hash = 1;
	      goto m0;
	  default:
dfl2:
	      slash=mul=0;
m0:
	      if ( comment == 1 ) {
		  attrCur = attrComment;
		  goto m1;
	      }
	      attrCur = attrNorm;
	      punct = 0;
	      if ( colorSyntax ) {
		 if ( ctproc ) {
		     ctproc->checkTag( *s, tag_type, ctag_len );
		     switch ( tag_type ) {
			 case TAG_COMMENT_EOL:
			 comment = 1;
			 goto fill;
		     case TAG_COMMENT_START:
			 comment = 2;
			 goto fill;
		     case TAG_COMMENT_END:
			 hash = 0;
			 attrCur = attrComment;
			 if ( !comment ) {
			     ctag_len = len + 1;
			 } else {
			     comment = 0;
			 }
fill:
			 for( int p=len-ctag_len+1; ctag_len > 1; ctag_len--, p++ ) {
			     screenStr[p] &= 0xff;
			     screenStr[p] |= attrComment;
			 }
			 ctag_len = 0;
		     }
		 }
		 if ( !comment && hash <= 0 && tproc && tag_type == -1 )
		     tag_state = tproc->checkTag( *s, tag_type, tag_len );
		 if ( htproc )
		     htag_state = htproc->checkTag( *s, htag_type, htag_len );
	      }
	      if ( flagMan ) {
		 flagMan = 0;
		 attrCur = attrMan;
		 goto m1;
	      }
	      if ( hash > 0 ) {
		 attrCur = comment > 0 ? attrComment : attrHash;
		 goto m1;
	      }
	      if ( comment > 0 ) {
		 attrCur = attrComment;
		 goto m1;
	      }
	      if ( !colorSyntax )
		 goto m1;

	      switch( tag_state ) {
		 case TAG_FOUND:
		     for( int p=len-tag_len; tag_len > 0; tag_len--, p++ ) {
			 screenStr[p] &= 0xff;
			 screenStr[p] |= attrHL1;
		     }
		     if ( punctuate && ispunct( *s ) && !flagFullString && *s != '_' ) {
			 switch( *s ) {
			   case '(':
			   case ')':
			   case '[':
			   case ']':
			   case '{':
			   case '}':
			       attrCur = attrHL2; break;
			   default:
			       attrCur = attrHL1;
			}
			punct = 1;
		     }
		     break;
		 case TAG_START:
		 case TAG_NAME:
		     break;
		 default:
		     if ( htag_len > 0 )
			tag_len = 0;
		     if ( punctuate && ctag_len <= 0 && ispunct( *s ) && *s != '_' && !flagFullString )
		      {
			switch( *s )
			 {
			   case '(':
			   case ')':
			   case '[':
			   case ']':
			   case '{':
			   case '}':
			       attrCur = attrHL2; break;
			   default:
			       attrCur = attrHL1;
			 }
			punct = 1;
		      }
	      }

	      switch( htag_state ) {
		 case TAG_FOUND:
		     attrCur = attrHtag;
		     tag_len = htag_len = 0;
		     break;
		 case TAG_START:
		     attrCur = comment ? attrComment : attrNorm;
		     for( int p=len-htag_len; htag_len > 0; htag_len--, p++ )
		      {
			screenStr[p] &= 0xff;
			screenStr[p] |= attrCur;
		      }
		     attrCur = attrHtag;
		     htag_len = 1;
		     break;
		 case TAG_NAME:
		 case TAG_PARAM_WAIT:
		 case TAG_EQU_WAIT:
		 case TAG_VAL_WAIT:
		     attrCur = attrHtag;
		     htag_len++;
		     break;
		 case TAG_PARAM_NAME:
		     attrCur = attrHpar_name;
		     htag_len++;
		     break;
		 case TAG_VAL_VALUE:
		     attrCur = attrHpar_val;
		     htag_len++;
		     break;
		 default:
		     if ( tag_len > 0 ) {
			htag_len = 0;
		     } else {
			unsigned short attr = comment ? attrComment : attrNorm;
			int t_len=htag_len, p=len-htag_len;;
			if ( punctuate ) {
			    screenStr[p] &= 0xff;
			    screenStr[p] |= attrHL1;
			    t_len--;
			}
			for( p=len-t_len; t_len > 0; t_len--, p++ ) {
			    screenStr[p] &= 0xff;
			    screenStr[p] |= attr;
			}
			htag_len = 0;
		     }
	      }
m1:
	      switch( *s ) {
		 case ' ':
		     screenStr[len++] = comment ? spaceComment : space;
		     break;
		 case '\t':
		     if ( !FLAG_BINARY )
		      {
			int spaces = stepTab - (pos & stepTab) + 1;
			for( int k=0; k < spaces; k++ )
			   screenStr[len++] = tab1;
			if ( htag_len > 0 && spaces > 0 )
			   htag_len += spaces-1;
			screenStr[len-1] = tab2;
			pos += spaces - 1;
			break;
		      }
		 default:
		     if ( !hash )
			hash = -1;
		     screenStr[len++] = *s < ' ' ? (attrCode | *s+64) : (attrCur | translate_out( (int)*s ));
		     attrCur = attrNorm;
	      }
      }
   }

   unsigned char *ss = s;
   if ( htproc ) {
       switch( htag_state ) {
	   case TAG_START:
	   case TAG_NAME:
	   case TAG_PARAM_WAIT:
	   case TAG_PARAM_NAME:
	   case TAG_EQU_WAIT:
	   case TAG_VAL_WAIT:
	   case TAG_VAL_VALUE:
	       for( pos=realPos; pos < si->len; pos++, ss++ )
		   if ( (htag_state = htproc->checkTag( *ss, htag_type, htag_len )) == TAG_FOUND )
		       break;
       }
       if ( htag_state != TAG_FOUND && htag_len > 0 ) {
	   int t_len=htag_len, p=len-htag_len;
	   if ( punctuate ) {
	       screenStr[p] &= 0xff;
	       screenStr[p] |= attrHL1;
	       t_len--;
	   }
	   attrCur = comment ? attrComment : attrNorm;
	   for( p=len-(punct?1:0)-t_len; t_len > 0; t_len--, p++ ) {
	       screenStr[p] &= 0xff;
	       screenStr[p] |= attrCur;
	   }
       }
   }

   if ( tproc ) {
       if ( hash <= 0 ) {
	   ss = s;
	   switch( tag_state ) {
	       case TAG_START:
	       case TAG_NAME:
		   for( pos=realPos; pos < si->len; pos++, ss++ )
		       if ( (tag_state = tproc->checkTag( *ss, tag_type, tag_len )) == TAG_FOUND )
			 break;
	   }
       }
       if ( tag_state != TAG_FOUND )
	   tag_state = tproc->checkTag( (int)' ', tag_type, tag_len );

       if ( tag_state == TAG_FOUND && !comment && tag_len > 0 ) {
	   for( int p=len-tag_len; tag_len > 0; tag_len--, p++ ) {
	       screenStr[p] &= 0xff;
	       screenStr[p] |= attrHL1;
	   }
       }
   }

   return screenStr;
}

void Edit::drawLine( long line, int sync, int fromDraw )
{
   if ( !isVisual() || line < 0 || line >= BOTT )
      return;
   char **saveMap = Screen::currentMap;
   if ( !fromDraw )
     {
	if ( FLAG_COL || FLAG_LINE )
	   { draw(); Screen::sync(); return; }
	if ( !isGetObj )
	   Screen::currentMap = scr.bord;
     }
   int y = rect.a.y + cursor.y + (box?1:0),
       x = rect.a.x + (box?1:0);
   if ( owner )
     {
       y += owner->corrPoint.y;
       x += owner->corrPoint.x;
     }
   Screen::Clear( y, x, 1, size.x-(box?2:0), clr[2] );
   int len, isReload=0;
   short *str = scrLine( line, len, isReload );
   if ( isReload )
     {
       Screen::currentMap = saveMap;
       return;
     }
   if ( str )
      Screen::putLimited( y, x, (u_short*)str, len );
   if ( !fromDraw )
     {
	if ( BLOCK_EXIST && line >= block.top && line <= block.bott )
	  {
	    blockInfo *b = validBlock();
	    Screen::attrSet( y, b->left - delta.x + x, 1, b->right - b->left + 1, COLOR_BLOCK );
	  }
	info();
	if ( sync )
	   Screen::sync();
     }
   Screen::currentMap = saveMap;
}

int Edit::draw( int Redraw, int sy, int sx )
{
   if ( !appl->started || !Window::draw( Redraw, sy, sx ) )
      return 0;

   char **saveMap = Screen::currentMap;
/*
   Rect oldRegion = Screen::getClipRegion();
   Rect r = rect;
   r.a.y += sy; r.b.y += sy;
   r.a.x += sx; r.b.x += sx;
   if ( box )
      r.grow( -1, -1 );
   Screen::setClipRegion( r );
*/
   comment = 0;
   if ( !isGetObj )
      Screen::currentMap = scr.bord;
   scroll( clr[2], isGetObj ? 1 : Redraw );
   if ( topDraw == scr.lines - 1 )
      topDraw--;
   Point pp( 0, 0 );
   if ( owner )
      pp = owner->corrPoint;
   int y = pp.y + rect.a.y + topDraw - delta.y + (box?1:0),
       x = pp.x + rect.a.x + (box?1:0);
   Screen::Clear( y, x, bottDraw-topDraw+1, size.x-(box?2:0), clr[2] );
   ccIndex ind, i, end;
   static BPinfo bp( -1, -1 );
   for( i=topDraw, end=min( BOTT-1, bottDraw ); i<=end; i++, y++ )
    {
      int len, isReload=0;
      short *str = scrLine( i, len, isReload );
      if ( isReload )
	{
	  if ( !isGetObj )
	     Screen::currentMap = saveMap;
//	  Screen::setClipRegion( oldRegion );
	  return 0;
	}
      if ( str )
	 Screen::putLimited( y, x, (u_short*)str, len );
    }

   if ( i == BOTT && /*box &&*/ BOTT - delta.y + (box?2:0) != size.y )
     {
       char *end=lang("<* END of text *>","<* конец текста *>");
       if ( (size_t)delta.x < strlen(end) )
	  Screen::put( y, x, (u_char*)end+delta.x );
       Screen::attrSet( y, x, 1, size.x-(box?2:0), clr[5] );
     }

   if ( BLOCK_EXIST ) {
       blockInfo *b = validBlock();
       y = pp.y + rect.a.y + (box ? 1 : 0) - delta.y;
       x = pp.x + rect.a.x + (box ? 1 : 0) - delta.x;
       Rect r( b->top + y, b->left + x, b->bott + y, b->right + x );
       Rect r2 = Rect( pp + rect.a, pp + rect.b );
       if ( box )
	   r2.grow(-1,-1);
       r.intersect( r2 );
       Screen::attrSet( r, COLOR_BLOCK );
   }
   if ( find_Info.found ) {
       strInfo *si = STRINFO( find_Info.line );
       if ( si ) {
	  int start = scrPos( si, find_Info.start );
	  int end = scrPos( si, find_Info.end );
	  Screen::attrSet( find_Info.line - delta.y + pp.y + rect.a.y + (box?1:0),
			start - delta.x + pp.x + rect.a.x + (box?1:0),
			1, end - start + 1,
			COLOR_FIND );
       }
   }

   info();

   if ( !isGetObj )
    {
      Screen::currentMap = saveMap;
      if ( appl->topWindow != this )
	 Screen::attrSet( Screen::shadowMap );
    }
   oldDelta = delta;
   return 1;
}


long Edit::handleKey( long key, void *&ptr )
{
   if ( !key )
      return key;
   long origKey = key;
   key = keyHolder.translate( key );
   int flagDraw = 1, flagMove = 0, flagRedraw=0, flagFind=1;
   BackMessage *bm=0;
   switch( key )
    {
      case HASH_cmIsEdit:
	  return 0;
      case HASH_cmNameCompare:
	  return ptr ? strcmp( title->get(), (char*)ptr ) : -1;
      case HASH_cmShortNameCompare:
	  {
	  char *n = (char*)strrchr( title->get(), FILESEP );
	  return n ? strcmp( n+1, (char*)ptr ) : strcmp( title->get(), (char*)ptr );
	  }
      case HASH_cmLink:
	  {
	  sprintf( Buf1, lang("Window exists:\n%s","Окно существует:\n%s"), title->get() );
	  modal *m = new modal( ALIGN_CENTER, (char*)Buf1, Warning.get(), HASH_cmCancel, Cancel.get(), HASH_cmOpenFileNew, lang("  New  "," Новое "), HASH_cmOpenFileLink, lang(" Link "," Линк ") );
	  long ret = execUp( m );
	  if ( ret != HASH_cmCancel && ret != HASH_cmEsc )
	     Task::sendMessage( appl, new BackMessage( ret ) );
	  }
	  key=0; break;
      case HASH_cmUp:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  if ( FLAG_FREEZE )
	     goto freeze_up;
	  if ( !moveUp() ) flagMove=1;
	  key=0; break;
      case HASH_cmUp2:
	  if ( owner && !owner->hardMode )
	     return origKey;
freeze_up:
	  if ( !moveUp( UNDO, 1 ) ) flagMove=1;
	  else if ( BLOCK_EXIST ) flagDraw=1;
	  key=0; break;
      case HASH_cmDown:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  if ( FLAG_FREEZE )
	     goto freeze_down;
	  if ( !moveDown() ) flagMove=1;
	  else if ( BLOCK_EXIST ) flagDraw=1;
	  key=0; break;
      case HASH_cmDown2:
	  if ( owner && !owner->hardMode )
	     return origKey;
freeze_down:
	  if ( !moveDown( UNDO, 1 ) ) flagMove=1;
	  key=0; break;
      case HASH_cmLeft:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  if ( !moveLeft() ) flagMove=1;
	  key=0; break;
      case HASH_cmLeft2:
	  if ( !moveLeft( UNDO, 1 ) ) flagMove=1;
	  key=0; break;
      case HASH_cmRight:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  if ( !moveRight() ) flagMove=1;
	  key=0; break;
      case HASH_cmRight2:
	  if ( !moveRight( UNDO, 1 ) ) flagMove=1;
	  key=0; break;
      case HASH_cmHome:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  if ( !moveHome() ) return 0;
	  key=0; break;
      case HASH_cmEnd:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  if ( !moveEnd() ) return 0;
	  key=0; break;
      case HASH_cmPgUp:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  movePgUp();
	  key=0; break;
      case HASH_cmPgDn:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  movePgDn();
	  key=0; break;
      case HASH_cmFuncPgUp:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  moveTop();
	  key=0; break;
      case HASH_cmFuncPgDn:
	  if ( owner && !owner->hardMode )
	     return origKey;
	  moveBott();
	  key=0; break;
      case HASH_cmIns:
	  if ( FLAG_INS ) CLEAR_INS; else SET_INS;
	  key=0; break;
      case HASH_cmEnter:
	  if ( !readOnlyMode ) {
	      enter();
	      if ( cursor.y==size.y-(box?2:0)-1 || LINE == BOTT-1 )
		 flagRedraw=1;
	  }
	  key=0; break;
      case HASH_cmDeleteLine:
	  if ( !readOnlyMode )
	     deleteLine( (!isGetObj || owner->hardMode) ? LINE :
			owner->curPos.y - rect.a.y + delta.y - (box?1:0) );
	  key=0; break;
      case HASH_cmDelete:
	  if ( !readOnlyMode )
	     flagDraw = deleteChar();
	  key=0; break;
      case HASH_cmBackspace:
	  if ( !readOnlyMode )
	     flagDraw = backspace();
	  key=0; break;
      case HASH_cmBlockReverse:
	  flagRevBlock = !flagRevBlock;
	  key=0; break;
      case HASH_cmBlockLine:
	  markBlock( flagRevBlock ? BLOCK_COL : BLOCK_LINE );
	  key=0; break;
      case HASH_cmBlockColumn:
	  markBlock( flagRevBlock ? BLOCK_LINE : BLOCK_COL );
	  key=0; break;
      case HASH_cmUnmarkBlock:
	  unmarkBlock( UNDO );
	  key=0; break;
      case HASH_cmCopy:
	  if ( !readOnlyMode )
	     copyBlock();
	  key=0; break;
      case HASH_cmMove:
	  if ( !readOnlyMode )
	     moveBlock( LINE, COLUMN );
	  key=0; break;
      case HASH_cmDeleteBlock:
	  if ( !readOnlyMode )
	    { deleteBlock(); flagRedraw=1; }
	  key=0; break;
      case HASH_cmCopyToClip:
	  copyToClip();
	  key=0; break;
      case HASH_cmMoveToClip:
	  if ( !readOnlyMode )
	     moveToClip();
	  key=0; break;
      case HASH_cmAddToClip:
	  copyToClip( 0 );
	  key=0; break;
      case HASH_cmCopyFromClip:
	  if ( !readOnlyMode )
	     copyFromClip();
	  key=0; break;
      case HASH_cmSave:
	  if ( !readOnlyMode )
	    if ( !externCollection )
	       saveText();
	    else
	       saveCollection();
	  key=0; break;
      case HASH_cmTruncate:
	  if ( !readOnlyMode )
	     truncateStr();
	  key=0; break;
      case HASH_cmSaveAs:
	  if ( saveText( "" ) )
	     title->put( Buf2, 0 );
	  key=0; break;
      case HASH_cmReload:
	  reload();
	  key=0; break;
      case HASH_cmReloadNoWarning:
	  reload(0);
	  key=0; break;
      case HASH_cmRedraw:
	  draw( 1 );
	  Screen::sync();
	  return 0;
      case HASH_cmSpaceMode:
	  showSpace = !showSpace;
	  key=0; break;
      case HASH_cmMathDown:
	  if ( !findMatch( 1 ) )
	     return 0;
	  key=0; break;
      case HASH_cmMathUp:
	  if ( !findMatch( -1 ) )
	     return 0;
	  key=0; break;
      case HASH_cmMarkGotoLine:
	  markGotoLine();
	  key=0; break;
      case HASH_cmGoto:
	  boxGoto();
	  key=0; break;
      case HASH_cmMode:
	  changeMode();
	  key=0; break;
      case HASH_cmBreakpoint:
#ifdef DJGPP
	  appl->test( sorry_dos.get() );
#else
	  breakpoint( LINE );
#endif
	  key=0; break;
      case HASH_cmClearBpoint:
#ifdef DJGPP
	  appl->test( sorry_dos.get() );
#else
	  clearBpoints();
#endif
	  key=0; break;
      case HASH_cmLoadBlock:
	  if ( !readOnlyMode )
	     readBlock();
	  key=0; break;
      case HASH_cmSaveBlock:
	  saveBlock();
	  key=0; break;
      case HASH_cmUndo:
	  undo_redo( UNDO );
	  key=0; break;
      case HASH_cmRedo:
	  undo_redo( REDO );
	  key=0; break;
      case HASH_cmClipboard:
	  if ( externCollection != &clipBoard )
	    {
	      Edit *e = new Edit( &clipBoard, "CLIPBOARD", scrRect() );
	      exec( e );
	      delete e;
	    }
	  key=0; break;
      case HASH_cmManual:
	  Task::sendMessage( appl, new BackMessage( key ) );
	  key=0; break;
      case HASH_cmShowMan:
	  showManMode();
	  key=0; break;
      case HASH_cmFind:
m1:
	  if ( boxFind() == HASH_cmOK ) {
	     if ( isGetObj ) {
		 flagFind = 0; goto m2;
	     }
	     Task::sendMessage( appl, new BackMessage( HASH_cmFindString ) );
	  }
	  key=0; break;
      case HASH_cmFindContinue:
	  if ( !dFindRepl.strFind[0] )
	     goto m1;
	  if ( isGetObj ) {
m2:
	      if ( !find( flagFind, 1, 0 ) ) {
		  sprintf( Buf1, lang("String '%s' not found.","Строка '%s' не найдена."), dFindRepl.strFind );
		  test( Buf1 );
	      }
	  } else {
	      Task::sendMessage( appl, new BackMessage( HASH_cmFindStringNext ) );
	  }
	  key=0; break;
      case HASH_cmReplace:
	  if ( readOnlyMode )
	    { key=0; break; }
	  if ( boxReplace() == HASH_cmOK )
	    {
	      if ( isGetObj )
		{
		  unsigned countRepl=0;
		  replace( 0, &countRepl );
		  sprintf( Edit::Buf1, lang("Made %d changes.","Выполнено %d замен."), countRepl );
		  test( Edit::Buf1 );
		}
	      else
		  Task::sendMessage( appl, new BackMessage( HASH_cmReplaceString ) );
	    }
	  key=0; break;
      case HASH_cmSmartFormat:
	  if ( !readOnlyMode )
	     formatParagraph( LINE, UNDO, 0, 1 );
	  key=0; break;
      case HASH_cmDumbFormat:
	  if ( !readOnlyMode )
	     formatParagraph( LINE, UNDO, 0, 0 );
	  key=0; break;
      case HASH_cmFormatText:
	  if ( !readOnlyMode )
	     formatText();
	  key=0; break;
      case HASH_cmFileInfo:
	  fileInfo();
	  key=0; break;
#ifdef DJGPP
      case HASH_cmPrint:
	  printText();
	  key=0; break;
#endif
      case HASH_cmInputCode:
	  {
	    char *code = getString( lang("Code:","Код:"), 5, 0,0,0,0,0,
	    inputLineFillChar, lang(" Enter code of symbol"," Введите код символа")/*, U_INT*/ );
#if 0
	    if ( !code || (key=atol(code)) < 0 || key > 255 ) {
		key=0;
		break;
	    }
#else
	    if ( !code ) {
		key=0; break;
	    }
	    switch( *code ) {
		case '0':
		    if ( *(code+1) == 'x' )
			goto _hex;
		    if ( (key = strtol( code, NULL, 8 )) >= 0 && key <= 255 )
			goto _default;
		    key = 0;
		    break;
		case '1'...'9':
		    if ( (key = strtol( code, NULL, 10 )) >= 0 && key <= 255 )
			goto _default;
		    key = 0;
		    break;
		case 'a'...'f':
		case 'A'...'F':
_hex:
		    if ( (key = strtol( code, NULL, 16 )) >= 0 && key <= 255 )
			goto _default;
		default:
		    key = 0;
		    break;
#endif
	    }
	  }
	  goto _default;
      case HASH_cmFreeze:
	  if ( FLAG_FREEZE ) CLEAR_FREEZE; else SET_FREEZE;
	  key=0; break;
      case HASH_cmCtagGo:
	  Task::sendMessage( appl, new BackMessage( HASH_cmCtagGo ) );
	  key=0; break;
      case HASH_cmTransAltKoi8:
	  trans_flag = TRANS_alt2koi8;
	  key=0; break;
      case HASH_cmTransKoi8Alt:
	  trans_flag = TRANS_koi82alt;
	  key=0; break;
      case HASH_cmTransWinKoi8:
	  trans_flag = TRANS_win2koi8;
	  key=0; break;
      case HASH_cmTransKoi8Win:
	  trans_flag = TRANS_koi82win;
	  key=0; break;
      case HASH_cmTransMainKoi8:
	  trans_flag = TRANS_main2koi8;
	  key=0; break;
      case HASH_cmTransKoi8Main:
	  trans_flag = TRANS_koi82main;
	  key=0; break;
      case HASH_cmTransNone:
	  trans_flag = TRANS_none;
	  key=0; break;
	case HASH_cmBackTab:
	  moveBackTab();
	  key=0; break;
_default:
      default:
	  if ( ISKEY(key) && key<=255 ) {
	      key = translate_in( key );
	      if ( !readOnlyMode && (flagDraw=insertChar( key, FLAG_INS )) )
		  flagRedraw=1;
	      key = 0;
	  }
    }

   if ( !key ) {
       correctBlock();
       if ( !isGetObj ) {
	   if ( flagMove ) {
	       info();
	       drawStatus();
	       Screen::sync();
	   } else if ( flagDraw ) {
	       if ( FLAG_COL )
		  flagRedraw = 1;
	       draw( flagRedraw ); drawStatus(); Screen::sync();
	   }
       } else if ( owner && !owner->hardMode ) {
	   owner->bindCursor = 1;
       }
   }
   return key;
}

int Edit::accept()
{
   return externCollection ? saveCollection() : saveText();
}

void Edit::correctShared()
{
   if ( !shared )
      return;
   for( int i=0; i < shared->getCount(); i++ )
    {
      Edit *e = (Edit*)shared->at(i);
      if ( e == this )
	 continue;
      e->cmap = cmap;
#ifdef _USE_MMAP_
//	e->validMap =	validMap;
#endif
      e->fInfo = fInfo;
      if ( e->delta.y >= BOTT )
	 e->delta.y = BOTT;
      e->unmarkBlock( NO_UNDO );
      e->setWinIdent();
    }
}

int Edit::moveHome( int flagUndo )
{
   if ( !cursor.x && !delta.x )
      return 0;
   long line = LINE;
   if ( flagUndo )
      saveUndo( HASH_cmUndoHome, 1 );
   if ( line < BOTT )
     {
       strInfo *si = STRINFO( line );
       if ( !si )
	  return 0;
       int i=0, len;
       char *s = getStr( si, len );
       for( ; i<len && (s[i]==' ' || s[i]=='\t'); i++ );
       if ( i >= len || i >= realPos( si, COLUMN ) )
	   delta.x = cursor.x = 0;
       else
	 {
	   int scrP = max( 0, scrPos( si, i ) );
	   int d = scrP - delta.x;
	   if ( d<0 || d >= size.x-(box?2:0) )
	      delta.x = max( 0, scrP - size.x + (box?2:0) + 1 );
	   cursor.x = scrP - delta.x;
	 }
     }
   else
       delta.x = cursor.x = 0;
   return 1;
}

int Edit::moveEnd( int flagUndo )
{
   long line = LINE;
   if ( line < BOTT )
     {
       if ( flagUndo )
	  saveUndo( HASH_cmUndoEnd, 1 );
       strInfo *si = STRINFO( line );
       if ( !si )
	  return 0;
       int len;
       char *s = getStr( si, len );
       int scrP = scrPos( si, len ) + (FLAG_COL ? 0:1);
       int d = scrP - delta.x;
       if ( d < 1 || d >= size.x - (box?2:0) )
	  delta.x = max( 0, scrP - size.x + (box?2:0) + 1);
       cursor.x = scrP - delta.x;
     }
   else
       moveHome( flagUndo );
   return 1;
}

int Edit::moveUp( int flagUndo, int scroll )
{
   if ( !delta.y && (!cursor.y || scroll) )
      return 0;
   if ( flagUndo )
      saveUndo( HASH_cmUndoUp, 1 );
   int ret=0;
   if ( cursor.y-1<0 || scroll )
     { delta.y--; ret=1; }
   else
     { cursor.y--; if ( FLAG_COL || FLAG_LINE ) ret = 1; }
   return ret;
}

int Edit::moveDown( int flagUndo, int scroll )
{
   int ret=0;
   if ( flagUndo )
      saveUndo( HASH_cmUndoDown, 1 );
   if ( cursor.y+(box?2:0)+1 >= size.y || scroll )
     {
       if ( delta.y>=BOTT ) return 0;
       delta.y++; ret=1;
     }
   else
     { cursor.y++; if ( FLAG_COL || FLAG_LINE ) ret = 1; }
   return ret;
}

void Edit::moveBackTab( int flagUndo )
{
	int pos = delta.x + cursor.x;
	int n = pos % (stepTab+1);
	if ( pos > 0 && n == 0 )
	    n = stepTab+1;
	for( ; n > 0; n-- )
	    moveLeft( flagUndo );
}

int Edit::moveLeft( int flagUndo, int word )
{
   if ( cursor.x <= 0 && delta.x <=0 )
      return 0;
   if ( flagUndo )
      saveUndo( HASH_cmUndoLeft, 1 );
   int ret=0;
   if ( !word ) {
       if ( cursor.x-1>=0 )
	  cursor.x--;
       else
	  { delta.x--; ret=1; }
   } else {
       long l = LINE;
       strInfo *si = l < BOTT ? STRINFO( l ) : 0;
       if ( !si )
	 { delta.x = cursor.x = 0; ret = 1; }
       else
	 {
	   int len, pos = realPos( si, COLUMN );
	   char *s = getStr( si, len );
	   if ( pos >= len )
	      pos = len - 1;
	   if ( !strchr( wordDelim, s[pos] ) && pos > 0 && strchr( wordDelim, s[pos-1] ) )
	      pos--;
	   if ( strchr( wordDelim, s[pos] ) )
	      for( ; pos >= 0; pos-- )
		if ( !strchr( wordDelim, s[pos] ) )
		   { break; }
	   for( ; pos >= 0; pos-- )
	     if ( strchr( wordDelim, s[pos] ) )
	       { pos++; break; }
	   int sPos = scrPos( si, pos );
	   if ( sPos < 0 ) sPos = 0;
	   if ( sPos < delta.x )
	     { delta.x = sPos; ret = 1; }
	   cursor.x = sPos - delta.x;
	 }
   }
   if ( FLAG_COL || FLAG_LINE )
      ret = 1;
   return ret;
}

int Edit::moveRight( int flagUndo, int word )
{
//   if ( delta.x+cursor.x+1 >= maxLineLength )
//       return 0;
   if ( flagUndo )
       saveUndo( HASH_cmUndoRight, 1 );
   int ret=0;
   if ( !word ) {
       if ( cursor.x + (box?2:0) + 1 < size.x )
	   cursor.x++;
       else {
	   delta.x++;
	   ret=1;
       }
   } else {
       long l = LINE;
       strInfo *si = l < BOTT ? STRINFO( l ) : 0;
       if ( si ) {
	   int len, pos = realPos( si, COLUMN );
	   char *s = getStr( si, len );
	   if ( pos < len ) {
	       if ( !strchr( wordDelim, s[pos] ) )
		  for( ; pos < len; pos++ )
		    if ( strchr( wordDelim, s[pos] ) )
		       break;
	       for( ; pos < len; pos++ )
		 if ( !strchr( wordDelim, s[pos] ) )
		     break;
	       int sPos = scrPos( si, pos );
	       int d = sPos - delta.x;
	       if ( d<1 || d >= size.x-(box?2:0) ) {
		   delta.x = max( 0, sPos - size.x + (box?2:0) + 1 );
		   ret = 1;
	       }
	       cursor.x = sPos - delta.x;
	   }
       }
   }
   if ( FLAG_COL || FLAG_LINE ) ret = 1;
   return ret;
}

void Edit::movePgUp( int flagUndo )
{
   if ( flagUndo )
      saveUndo( HASH_cmUndoPgUp, 1 );
   if ( !delta.y )
      delta.x = cursor.y = cursor.x = 0;
   else
      delta.y = max( delta.y-size.y+(box?2:0), 0 );
}

void Edit::movePgDn( int flagUndo )
{
   if ( flagUndo )
      saveUndo( HASH_cmUndoPgDn, 1 );
   delta.y = min( delta.y+size.y-(box?2:0), BOTT );
}

void Edit::moveTop( int flagUndo )
{
   if ( flagUndo )
      saveUndo( HASH_cmUndoTop, 1 );
   delta.y = cursor.y = 0;
}

void Edit::moveBott( int flagUndo )
{
   if ( flagUndo )
      saveUndo( HASH_cmUndoBott, 1 );
   delta.y = max( 0, BOTT - size.y + (box?2:0) + 1 );
   cursor.y = size.y - (box?2:0) - 1;
}

strInfo *Edit::STRINFO( long l )
{
  static strInfo bufInfo( 0, 0, 0 );
  long lines = BOTT;
  if ( l >= 0 && l < lines )
      return (strInfo*)atext->at(l);
  return 0;
}

char *Edit::getStr( strInfo *si, int &len, int flagNL )
{
   char *s = si->map ? cmap + (long)si->str : si->str;
   len = si->len;

   if ( !FLAG_MAN
#if 0
     && !flagNL
#endif
      )
      return s;

   getstrbuf = (char*)realloc( getstrbuf, si->len + (si->len>0?1:2) );
   char *b = getstrbuf;
   for( int i=0; i < si->len && len > 0; i++, s++ ) {
       if ( *s == '\b' ) {
	   len -= 2; b--; continue;
       }
       *b++ = *s;
   }
#if 0
   if ( flagNL ) {
       len++;
       *b = '\n';
   }
#endif

   return getstrbuf;
}

int Edit::realPos( strInfo *si, int scrPos )
{
   int pos=0, realPos=0, len;
   char *s = getStr( si, len );
   for( ; realPos<len; pos++ ) {
      if ( s[realPos] == '\t' && !FLAG_BINARY )
	  pos += stepTab - (pos & stepTab);
      if ( pos>=scrPos )
	  break;
      realPos++;
   }
   return realPos + (pos<scrPos ? scrPos-pos : 0);
}

int Edit::scrPos( strInfo *si, int realPos )
{
   int pos=0, scrPos=0, len;
   char *s = getStr( si, len );
   for( ; pos<len && pos<=realPos; pos++ ) {
      if ( s[pos] == '\t' && !FLAG_BINARY )
	  scrPos += stepTab - (scrPos & stepTab);
      scrPos++;
   }
   return scrPos-1;
}

void Edit::replaceStr( long line, char *str, int len )
{
   strInfo *si = STRINFO( line );
   if ( !si )
      return;

   if ( si->map )
       si->str = 0;
   si->str = (char*)realloc( si->str, len+(len>0?0:1) );
   memcpy( si->str, str, len );
   si->len = len;
   si->map = 0;

   setChange( 1 );
   setWinIdent();
}

void Edit::insertStr( long line, char *str, int len )
{
   static strInfo si( 0, 0, 0 );
   si.str = NULL;
   si.len = len;
   si.str = (char*)realloc( si.str, len+(len>0?0:1) );
   memcpy( si.str, str, len );
   atext->atInsert( line, si );

   setChange( 1 );
   setWinIdent();
}

void Edit::insertStr( long line, strInfo *si )
{
   char *s=0;
   static strInfo Si( 0, 0, 0 );
   Si.str = NULL;
   Si.len = si->len;
   Si.map = si->map;
   if ( si->map ) {
       Si.str = si->str;
   } else {
       Si.str = (char*)realloc( Si.str, Si.len + (Si.len>0?0:1) );
       memcpy( Si.str, si->str, si->len );
   }

   atext->atInsert( line, Si );
   setChange( 1 );
   setWinIdent();
}

int Edit::insertChar( int ch, int Ins, int flagUndo )
{
   long line = LINE;
   int sPos = COLUMN;
   if ( isGetObj && !owner->hardMode ) {
       line = owner->curPos.y - rect.a.y + delta.y - (box?1:0);
       cursor.y = line - delta.y;
       sPos = owner->curPos.x - rect.a.x + delta.x - (box?1:0);
       cursor.x = sPos - delta.x;
   }
   int newLen = 0, fullDraw = addStrings( flagUndo ) ? 1 : 0;
   strInfo *si = STRINFO( line );
   if ( !si )
      return 0;
   int rPos = realPos( si, sPos );
   int len, owrSym = 0;
   char *nstr=NULL, *str = getStr( si, len );
//   if ( sPos+1 >= maxLineLength )
//      return 0;
   int cursorOffset = 1 + (ch == '\t' ? stepTab-(sPos & stepTab) : 0);

   if ( rPos < len ) {
	int spaces=0, owr=0;
	if ( (sPos & stepTab) && str[rPos] == '\t' )
	   spaces = sPos - scrPos( si, rPos-1 ) - 1;
	if ( !Ins && (str[rPos]!='\t' || (str[rPos]=='\t' && (sPos & stepTab)==stepTab)))
	   { owr=1; owrSym=str[rPos]; }
	newLen = si->len + spaces + (owr ? 0 : 1);
	nstr = (char*)malloc( newLen + 1 );
	memcpy( nstr, str, rPos );
	memset( nstr + rPos, ' ', spaces );
	nstr[ rPos + spaces ] = ch;
	int offset = rPos + owr;
	memcpy( nstr + rPos + spaces + 1, str + offset, len - offset );
   } else {
	int tabs=0, spaces=0, sp = len > 0 ? scrPos( si, len ) + 1 : 0;
	if ( OPTIMAL_FILL(eOpt.options) ) {
	   int i = stepTab - (sp & stepTab) + 1;
	   if ( sp + i < sPos && sp & stepTab ) {
	       tabs++;
	       sp += i;
	   }
	   tabs += (sPos - sp) / (stepTab + 1);
	   spaces += (sPos - sp) % (stepTab + 1);
	} else {
	   spaces = sPos - sp;
	}
	newLen = si->len + tabs + spaces + 1;
	nstr = (char*)malloc( newLen + 1 );
	memcpy( nstr, str, len );
	memset( nstr + len, '\t', tabs );
	memset( nstr + len + tabs, ' ', spaces );
	nstr[ newLen - 1 ] = ch;
   }

   if ( nstr == NULL )
       return 0;

   nstr[ newLen ] = 0;
   char *s1=0, *s2=0, *src=nstr;
   if ( AUTOWRAP(eOpt.options) && sPos >= eOpt.lineLength ) {
       int curOffset = compress( src, Buf2, rPos, eOpt.lineLength, stepTab ) ;
       if ( curOffset ) {
	   s1 = src = Buf2;
	   cursorOffset = curOffset;
       }
       if ( sPos + curOffset >= eOpt.lineLength || strchr( wordDelim, ch ) ) {
	  int off = format( src, sPos+curOffset, s1, s2, 1 );
	  if ( s1 )
	     src = s1;
	  if ( s2 )
	     cursorOffset = off;
       }
   }

   //-----------------------------------------------------
   if ( flagUndo ) {
      if ( !s2 ) {
	 if ( !s1 || !cursorOffset ) {
	    undoItem *u = undoOptimize( HASH_cmUndoChar );
	    if ( u && u->sym1==ch && FLAG_INS == (u->_flags & 0x020) &&
					(FLAG_INS || u->sym2==owrSym) ) {
		u->cycl++;
	    } else {
		u = saveUndo( HASH_cmUndoChar );
		u->sym1 = ch;
		u->sym2 = owrSym;
	    }
	 } else {
	    char *s = new char[len+1];
	    memcpy( s, str, len ); s[len]=0;
	    undoItem *u = saveUndo( HASH_cmUndoTruncate );
	    Collection *coll = new Collection( 2, 0 );
	    coll->insert( s );
	    u->ptr = coll;
	    s = new char[strlen(s1)+1];
	    strcpy( s, s1 );
	    coll->insert(s);
	 }
      } else {
	  char *s = new char[len+1];
	  memcpy( s, str, len ); s[len] = 0;
	  undoItem *u = saveUndo( HASH_cmUndoEnter );
	  u->ptr = s;
      }
   }
   //-----------------------------------------------------
   if ( src == nstr ) {
       if ( !si->map && si->str )
	   ::free( si->str );
       si->str = nstr;
       si->len = newLen;
       si->map = 0;
       setChange( 1 );
       setWinIdent();
   } else {
       ::free( nstr );
       replaceStr( line, src, strlen( src ) );
   }

   int bindCursor = 1;
   if ( s2 ) {
       insertStr( ++line, s2, strlen(s2) );
       fullDraw = 1;
       moveDown( NO_UNDO );
       bindCursor = 0;
       fullDraw = 1;
       if ( cursorOffset >= size.x - (box?2:0) ) {
	   delta.x = cursorOffset - size.x - (box?2:0);
	   cursor.x = 0;
       } else {
	   delta.x = 0;
	   cursor.x = cursorOffset;
       }
   } else if ( s1 ) {
       if ( cursorOffset )
	  cursorOffset++;
       else {
	  strInfo si( s1, strlen(s1), 0 );
	  cursorOffset = scrPos( &si, si.len ) - sPos + 1;
       }
       unmarkBlock( NO_UNDO );
   } else if ( Ins && FLAG_BLOCKCOL && LINE == block.top && block.top == block.bott ) {
       int col = COLUMN;
       if ( col < block.left )
	 { block.left += cursorOffset; block.right += cursorOffset; }
       else if ( col <= block.right )
	   block.right += cursorOffset;
   }

   if ( bindCursor ) {
       if ( cursor.x + cursorOffset >= size.x - (box?2:0) ) {
	   delta.x += cursorOffset;
	   fullDraw = 1;
       } else {
	   cursor.x += cursorOffset;
       }
   }

   if ( fullDraw )
      return 1;
   if ( flagUndo )
      drawLine( line );
   return 0;
}

int Edit::addStrings( int flagUndo )
{
   long i, line = (!isGetObj || owner->hardMode) ? LINE :
			owner->curPos.y - rect.a.y + delta.y - (box?1:0);
   if ( line < BOTT )
      return 0;
   strInfo si( 0, 0, 0 );
   char *ch = NULL;
   for( i=0; line>=BOTT; i++ )  {
      ch = new char[1];
      si.str = ch;
      atext->insert( si );
   }
   //---------------------------------------------------
   if ( flagUndo )
    {
      undoItem *u = saveUndo( HASH_cmUndoAddStrings );
      u->longVal = i;
    }
   //---------------------------------------------------
   return 1;
}

int Edit::deleteChar( int flagUndo )
{
   long line = LINE;
   int sPos = COLUMN;
   if ( isGetObj && !owner->hardMode ) {
       line = owner->curPos.y - rect.a.y + delta.y - (box?1:0);
       cursor.y = line - delta.y;
       sPos = owner->curPos.x - rect.a.x + delta.x - (box?1:0);
       cursor.x = sPos - delta.x;
   }
   if ( line >= BOTT )
      return 0;
   strInfo *si = STRINFO( line );
   if ( !si )
      return 0;
   int spaces=0, offset=1;
   int rPos = realPos( si, sPos );
   if ( rPos >= si->len ) {
       glueStrings( line, rPos - si->len, flagUndo );
       return 1;
   }
   int len;
   char *s = getStr( si, len );
   if ( (sPos & stepTab) && s[rPos]=='\t' )
      spaces = sPos - scrPos( si, rPos-1 ) - 1;
   if ( s[rPos] == '\t' )
      offset = stepTab - (sPos & stepTab) + 1;
   //-----------------------------------------------
   if ( flagUndo ) {
      undoItem *u = undoOptimize( HASH_cmUndoDelete );
      if ( u && u->sym1 == s[rPos] ) {
	 u->cycl++;
      } else {
	 u = saveUndo( HASH_cmUndoDelete );
	 u->sym1 = s[rPos];
      }
   }
   //-----------------------------------------------

   int newLen = len + spaces - 1;
   char *nstr = NULL;
   if ( newLen > 0 ) {
       nstr = (char*)malloc( newLen );
       memcpy( nstr, s, rPos );
       memset( nstr + rPos, ' ', spaces );
       memcpy( nstr + rPos + spaces, s + rPos + 1, len - rPos - 1 );
   }
   if ( !si->map )
       ::free( si->str );
   si->str = nstr;
   si->len = newLen;
   si->map = 0;
   setChange( 1 );
   setWinIdent();

   if ( FLAG_BLOCKCOL && LINE == block.top && block.top == block.bott ) {
       blockInfo *b = validBlock();
       block.left = b->left;
       block.right = b->right;
       int col = COLUMN;
       if ( col < block.left )
	 { block.left -= offset; block.right -= offset; }
       else if ( col <= block.right )
	  block.right -= offset;
       if ( block.right < block.left )
	  Window::unmarkBlock();
   }

   drawLine( line );
   return 0;
}

int Edit::backspace( int flagUndo )
{
   long line = LINE;
   int sPos = COLUMN;
   if ( isGetObj && !owner->hardMode ) {
       line = owner->curPos.y - rect.a.y + delta.y - (box?1:0);
       cursor.y = line - delta.y;
       sPos = owner->curPos.x - rect.a.x + delta.x - (box?1:0);
       cursor.x = sPos - delta.x;
   }
   if ( line >= BOTT ) {
       if ( cursor.x || delta.x ) moveLeft();
       else moveUp();
       return 1;
   }
   strInfo *si = STRINFO( line );
   if ( !si )
      return 0;
   int fullDraw = 0;
   int rPos = realPos( si, sPos );
   if ( rPos > si->len ) {
       moveLeft( flagUndo );
       return 1;
   }
   if ( rPos < 1 ) {
       if ( line < 1 )
	  return 0;
       if ( delta.x || cursor.x ) {
	   moveHome( flagUndo );
	   return 1;
       }
       si = STRINFO( line-1 );
       if ( !si )
	  return 0;
       sPos = scrPos( si, si->len );
       glueStrings( line-1, 0, flagUndo );
       int d = sPos - delta.x;
       if ( d<1 || d >= size.x-(box?2:0) )
	  delta.x = max( 0, sPos - size.x + (box?2:0) + 1 );
       cursor.x = sPos - delta.x + 1;
       moveUp( NO_UNDO );
       if ( flagUndo ) {
	  undoItem *u = &Undo[uInfo->cur];
	  u->sym1 = 1;
	  long *ptr = new long[4];
	  ptr[0] = delta.y; ptr[1] = delta.x;
	  ptr[2] = cursor.y; ptr[3] = cursor.x;
	  Collection *coll = (Collection*)u->ptr;
	  coll->insert( ptr );
       }
       return 1;
   }

   int prepare=1, cursorOffset=1, len;
   char *s = getStr( si, len );
   if ( s[rPos] == '\t' ) {
       cursorOffset = sPos - scrPos( si, rPos-1 ) - 1;
       if ( cursorOffset > 0 )
	   prepare = 0;
       else if ( rPos>0 && s[rPos-1] == '\t' )
	   goto	m1;
       else
	   cursorOffset = 1;
   } else if ( rPos>0 && s[rPos-1] == '\t' ) {
m1:
	   cursorOffset = rPos > 1 ? (sPos - scrPos( si, rPos-2 ) - 1) : stepTab + 1;
   }

   unsigned char undoSym = 0;
   if ( prepare ) {
       undoSym = s[rPos-1];
       char *nstr = NULL;
       if ( len > 1 ) {
	   nstr = (char*)malloc( len - 1 );
	   memcpy( nstr, s, rPos - 1 );
	   memcpy( nstr + rPos - 1, s + rPos, len - rPos );
       }
       if ( !si->map )
	   ::free( si->str );
       si->str = nstr;
       si->len = len - 1;
       si->map = 0;
       setChange( 1 );
       setWinIdent();
   }
   if ( flagUndo ) {
       undoItem *u = undoOptimize( HASH_cmUndoBackspace );
       if ( u && u->sym1 == undoSym ) {
	  u->cycl++;
       } else {
	  u = saveUndo( HASH_cmUndoBackspace );
	  u->sym1 = undoSym;
       }
   }

   if ( FLAG_BLOCKCOL && LINE == block.top && block.top == block.bott ) {
       int col = COLUMN;
       if ( col < block.left ) {
	   block.left -= cursorOffset;
	   block.right -= cursorOffset;
       } else if ( col <= block.right + 1 ) {
	   block.right -= cursorOffset;
       }
       blockInfo *b = validBlock();
       if ( b->right < b->left )
	  CLEAR_BLOCK;
   }

   if ( cursor.x - cursorOffset < delta.x ) {
       delta.x -= cursorOffset;
       fullDraw = 1;
   } else {
       cursor.x -= cursorOffset;
   }

   if ( fullDraw )
      return 1;
   if ( flagUndo )
      drawLine( line );
   return 0;
}

void Edit::enter( int flagUndo )
{
   long line = LINE;
   int sPos = COLUMN;
   if ( isGetObj && !owner->hardMode ) {
       line = owner->curPos.y - rect.a.y + delta.y - (box?1:0);
       cursor.y = line - delta.y;
       sPos = owner->curPos.x - rect.a.x + delta.x - (box?1:0);
       cursor.x = sPos - delta.x;
   }
   if ( line >= BOTT )
       addStrings( flagUndo );
   strInfo *si = STRINFO( line );
   if ( !si )
       return;
   int len;
   char *s = getStr( si, len );
   //--------------------------------------------------------
   if ( flagUndo ) {
       char *str = new char[len+1];
       memcpy( str, s, len ); str[len] = 0;
       undoItem *u = saveUndo( HASH_cmUndoEnter );
       u->ptr = str;
     }
   //--------------------------------------------------------
   int sp=0, rPos = realPos( si, sPos );
   for( ; sp < len && (s[sp]==' ' || s[sp]=='\t'); sp++ )
       Buf1[sp] = s[sp];
   int spCount = sp;
   int l = len;
   if ( sp > rPos )
       sp = 0;
   int paragraph = AUTOWRAP(eOpt.options);
   if ( rPos < l ) {
       int ostat = l - rPos;
       memcpy( Buf1+sp, s + rPos, ostat );
       sp += ostat;
   } else if ( paragraph ) {
       memset( Buf1, ' ', sp=eOpt.parOffset );
   } else {
       sp=0;
   }
   if ( (FLAG_BLOCKCOL || FLAG_BLOCKLINE) && line < block.top )
     { block.top++; block.bott++; }
   replaceStr( line, s, min( l, rPos ) );
   insertStr( line+1, Buf1, sp );
   if ( !paragraph && (rPos != spCount || spCount == l) )
      moveHome( NO_UNDO );
   moveDown( NO_UNDO );
   if ( paragraph )
      moveEnd( NO_UNDO );
}

void Edit::deleteLine( long line, int flagUndo )
{
   if ( line<0 || line >= BOTT )
      return;
   strInfo *si = STRINFO(line);
   if ( !si )
      return;
   //------------------------------------------------
   if ( flagUndo )
     {
       cursor.y = line - delta.y;
       char *s = new char[si->len+1];
       int len;
       char *undo_s = getStr( si, len );
       memcpy( s, undo_s, len );
       s[len] = 0;
       undoItem *u = saveUndo( HASH_cmUndoDeleteLine );
       u->ptr = s;
     }
   //------------------------------------------------
   if ( !si->map )
      ::free( si->str );
   atext->atRemove( line );

   if ( line == block.top && line == block.bott )
       unmarkBlock( NO_UNDO );
   else {
       if ( line <= block.bott ) block.bott--;
       if ( line < block.top ) block.top--;
   }
   setChange( 1 );
   setWinIdent();
}

int Edit::glueStrings( long topline, int spaces, int flagUndo )
{
   long bott = BOTT;
   strInfo *si1=STRINFO(topline), *si2=NULL;
   if ( !si1 )
      return 0;

   int len1, len2;
   char *s1=getStr(si1,len1), *s2=NULL;
   if ( topline < 0 || topline + 1 >= bott )
      return len1 + spaces;

   si2 = STRINFO( topline+1 );
   if ( !si2 )
      return 0;

   s2 = getStr( si2, len2 );
   int pos=len1, newLen=len1+spaces+len2;
   char *nstr = NULL;
   if ( newLen > 0 ) {
       nstr = (char*)malloc( newLen );
       s1 = getStr( si1, len1 );
       memcpy( nstr, s1, len1 );
       memset( nstr + pos, ' ', spaces );

       pos += spaces;
       s2 = getStr( si2, len2 );
       memcpy( nstr + pos, s2, len2 );
   }

   //------------------------------------------------------
   if ( flagUndo ) {
       Collection *coll = new Collection( 3, 2 );
       char *s = new char[len1+1];
       memcpy( s, s1, len1 ); s[len1] = 0;
       coll->insert( s );
       s = new char[len2+1];
       memcpy( s, s2, len2 ); s[len2] = 0;
       coll->insert( s );
       undoItem *u = saveUndo( HASH_cmUndoGlue );
       u->ptr = coll;
       u->longVal = topline;
       u->column = spaces;
   }
   //------------------------------------------------------

   if ( !si1->map )
       ::free( si1->str );
   si1->str = nstr;
   si1->len = newLen;
   si1->map = 0;
   setChange( 1 );
   setWinIdent();

   deleteLine( topline+1, NO_UNDO );
   return pos;
}

char *getRealFileName( char *name )
{
   static char path[MAXPATHLEN];
#ifdef DJGPP
   strcpy( path, name );
#else
   char *short_name = new char[MAXPATHLEN];
   strcpy( path, name );
   char *ch = strrchr( path, FILESEP );
   if ( ch )
    {
      strcpy( short_name, ch+1 );
      *ch = 0;
    }
   Collection *chain = new Collection(10,10);
   __readlink( path, (ch ? short_name : 0), chain );
   strcpy( path, chain->getCount() > 0 ? (char*)chain->at(0) : name );
#endif
   delete chain;
   delete short_name;
   return path;
}

void Edit::setTprocByExt( char *ext )
{
  long index=-1;
  tproc = 0;
  for( int i=highlight.getCount()-1; i >= 0; i-- )
   {
     HighLight *hl = (HighLight*)highlight.at( i );
     if ( hl->ext->search( ext, index ) && index>=0 )
      {
	tproc = hl->tproc;
	ctproc = hl->ctproc;
	punctuate = hl->punctuate;
	if ( ctproc && ctproc->checkTag( '#', tag_state, ctag_len ) == TAG_FOUND )
	    hash_is_comment = 1;
	break;
      }
   }
}

void Edit::setHTprocByExt( char *ext )
{
  long index=-1;
  htproc = 0;
  int ret = HTMLextension.search( ext, index );
  if ( ret && index>=0 )
     htproc = &HTproc;
  else
     htproc = 0;
}

int Edit::readFile( char *name, int flag_binary )
{
   if ( saveTimer )
      saveTimer->close();
   char *Name = getRealFileName( name );
   char *ext = strrchr( name, '.' );
   int fd = open( Name, O_RDWR );
   if ( fd < 0 ) {
       if ( (fd = open( Name, O_RDONLY )) >= 0 ) {
	   readOnlyMode = RDONLY_3;
       } else {
	   sprintf( Buf1, openError.get(), name );
	   appl->test( Buf1 );
	   if ( saveTimer && AUTOSAVE(eOpt.options) )
	      saveTimer->open();
	   return 0;
       }
   } else if ( readOnlyMode == RDONLY_3 ) {
       readOnlyMode = RDONLY_0;
   }

   fstat( fd, &fInfo->s );
   if ( S_ISDIR( fInfo->s.st_mode ) ) {
       sprintf( Buf1, lang( "File\n%s\nis directory","Файл\n%s\nявляется каталогом"), name );
       appl->test( Buf1 );
       if ( saveTimer && AUTOSAVE(eOpt.options) )
	  saveTimer->open();
       return 0;
   }

   if ( !atext )
       atext = new infoArray( 100, 100 );

   clearText( 1 );

#ifdef _USE_MMAP_
   if ( fInfo->s.st_size > 0 && !( S_IXUSR & fInfo->s.st_mode ||
				   S_IXGRP & fInfo->s.st_mode ||
				   S_IXOTH & fInfo->s.st_mode ) )
       cmap = (char*)mmap( 0, fInfo->s.st_size, PROT_READ, MAP_SHARED, fd, 0 );
#else
   cmap = 0;
#endif

   FILE *f = fdopen( fd, "rb" );
   strcpy( Buf1, lang(" Reading file "," Чтение файла ") );
   strcat( Buf1, name );
   appl->statusLine->draw( Buf1, 1 );
   Screen::sync();
   tproc = 0;
   ctproc = 0;
   htproc = 0;

#ifdef _USE_MMAP_
   char *pos= 0;
#endif
   long len=0, c=0;

   if ( flag_binary )
       goto read_binary;

   while( 1 ) {
      if ( (c=fgetc(f)) != EOF ) {
	  switch( c ) {
#ifdef DJGPP
	     case '\r':
		 continue;
#endif
	     case '\n':
		 break;
	     default:
		 if ( !cmap )
		     Buf1[len] = c;
		 len++;
		 if ( len >= maxLineLength && !cmap )
		     break;
		 continue;
	  }
      }
      if ( len > 0 || c == '\n' ) {
	  strInfo si( 0, len, 1 );
	  if ( cmap ) {
	      si.str = pos;
	      pos += len+1;
	  } else {
	      si.map = 0;
	      si.len = len;
	      si.str = (char*)realloc( si.str, len+(len>0?0:1) );
	      memcpy( si.str, Buf1, len );
	  }
	  atext->insert( si );
	  fInfo->linesNumber++;
      }
      len = 0;
      if ( c == EOF )
	  break;
   }

   if ( !cmap )
      setChange( 0 );

   fclose( f );
   ::close( fd );
   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();

   if ( ext && *(ext+1) )
    {
      setTprocByExt( ext+1 );
      setHTprocByExt( ext+1 );
    }

   goto	end_read;

read_binary:
   for( pos=0, len=0; len <= fInfo->s.st_size; ) {
       strInfo si( 0, 64, 1 );
       if ( cmap ) {
	     si.str = pos;
	     pos += 64;
       } else {
	     si.map = 0;
	     si.str = (char*)realloc( si.str, 64 );
	     fread( si.str, 64, 1, f );
       }
       len += 64;
       if ( len > fInfo->s.st_size )
	     si.len = 64 - len + fInfo->s.st_size;
       fInfo->linesNumber++;
       atext->insert( si );
   }

   fclose( f );
   ::close( fd );
   SET_BINARY;
   if ( readOnlyMode < RDONLY_3 && !FLAG_MAN )
      readOnlyMode = RDONLY_2;
   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();

end_read:
   appl->drawStatus();
   return 1;
}

int Edit::readCollection()
{
   if ( !atext )
       atext = new infoArray( 100, 100 );

   if ( !externCollection )
       return 0;

   if ( saveTimer )
       saveTimer->close();

   clearText();

   int pr = (externCollection == &clipBoard) ? 1 : 0;
   for( long i=0, j=externCollection->getCount() - pr; i < j; i++ ) {
      char *str = (char*)externCollection->at( i+pr );
      insertStr( i, str, strlen(str) );
   }
   setChange( 0 );
   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();
   return 1;
}


int Edit::saveText( char *Name )
{
   if ( !Name && strlen( title->get() ) > 0 && !FLAG_CHANGE )
      return 1;

   if ( saveTimer )
      saveTimer->close();

   char *name = Name ? Name : title->get();
   if ( strlen(name) <= 0 ) {
       name = (HASH_cmOK == execUp( new filebox( filebox::filePath, filebox::fileMask, Buf2 ) ) ) ? Buf2 : 0;
       if ( !name ) {
	   if ( saveTimer && AUTOSAVE(eOpt.options) )
	       saveTimer->open();
	  return 0;
       }
       if ( !access( name, F_OK ) ) {
	   sprintf( Buf1, existWarning.get(), name );
	   modal *m = new modal( ALIGN_CENTER, (char*)Buf1, Warning.get(), HASH_cmOK, Yes.get(), HASH_cmNo, No.get() );
	   if ( execUp( m ) != HASH_cmOK ) {
	       if ( saveTimer && AUTOSAVE(eOpt.options) )
		   saveTimer->open();
	       return 0;
	   }
       }
   }

   char *realName = getRealFileName( name );

   if ( strcmp( title->get(), name ) ) {
       int bd = open( realName, O_RDWR | O_CREAT | O_TRUNC, fInfo->s.st_mode );
       if ( bd < 0 ) {
	   sprintf( Buf1, openError.get(), realName );
	   appl->test( Buf1 );
	   if ( saveTimer && AUTOSAVE(eOpt.options) )
	       saveTimer->open();
	   return 0;
       }
      ::close( bd );
   }

   char *tmp = (char *) calloc(MAXPATHLEN, 1);
   // This is subject to replace tiis hardcoded path to setup variable
   sprintf(tmp, "/var/tmp/yuitmp.XXXXXX");
   int fd = mkstemp(tmp);

   FILE *f = (fd >= 0 ? fdopen( fd, "w+" ) : 0);
   if ( !f ) {
      appl->test( lang("Can't open temporary file","Ошибка открытия временного файла") );
      if ( saveTimer && AUTOSAVE(eOpt.options) )
	 saveTimer->open();
      return 0;
   }

   strcpy( Buf1, lang(" Saving file "," Сохранение файла ") );
   strcat( Buf1, name );
   appl->statusLine->draw( Buf1, 1 );
   Screen::sync();
   if ( strlen(name) > 0 && BAK_FILE(eOpt.options) ) {
       FILE *forig = fopen( name, "r" );
       if ( forig <= 0 ) {
	   modal *m = new modal( ALIGN_CENTER,
	   lang( "Can't open original file,\ncreation of backup file skipped",
		 "Ошибка открытия исходного файла,\nсоздание '.bak'-файла пропущено" ), Warning.get() );
	   execUp( m );
       } else {
	   sprintf( Buf1, "%s.bak", name );
	   char *nameBack = strdup( Buf1 );
	   fd = open( nameBack, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );
	   FILE *fbak = (fd >= 0 ? fdopen( fd, "w+" ) : 0);
	   if ( fbak <= 0 ) {
	       sprintf( Buf1, openError.get(), nameBack );
	       modal *m = new modal( ALIGN_CENTER, (char*)Buf1, Warning.get() );
	       execUp( m );
	   } else {
	       for( int c; (c=fgetc(forig)) != EOF; )
		   fputc( c, fbak );
	       fclose( fbak );
	   }
	   fclose( forig );
	   ::free( nameBack );
       }
   }

   strInfo *si;
   int len;
   long i, bott;
   fInfo->linesNumber = 0;
#ifdef _USE_MMAP_
   char *pos = 0;
#endif
   for( i=0, bott=BOTT; i<bott; i++ ) {
       si = STRINFO(i);
       char *s = getStr( si, len );
       if ( !FLAG_BINARY && STRIP_SPACES(eOpt.options) ) {
	   for( int cycl=1; cycl && len > 0; ) {
	       switch( s[len-1] ) {
		   case ' ':
		   case '\t':
		       len--;
		       si->len--;
		       continue;
		   default:
		       cycl = 0;
	       }
	   }
       }
       int tabs=0, first=-1, flag_realloc=0;
       if ( OPTIMAL_FILL(eOpt.options) ) {
	  int j=0, sp=0;
	  for( ; j < len; j++ ) {
	     switch( s[j] ) {
	       case ' ':
		  if ( sp < 0 )
		     goto write_char;
		  if ( sp < stepTab )
		     { sp++; continue; }
		  fputc( '\t', f );
		  tabs++;
		  flag_realloc = 1;
		  if ( sp > 0 )
		     si->len -= sp;
		  sp = 0;
		  continue;
	       case '\t':
		  if ( sp > 0 ) {
		      si->len -= sp;
		      flag_realloc = 1;
		  }
		  sp = 0;
		  if ( first < 0 )
		      tabs++;
		  goto write_char;
	       default:
		  if ( first < 0 )
		      first = j - sp;
		  for( ; sp > 0; sp-- )
		      fputc( ' ', f );
		  sp = -1;
write_char:
		  fputc( s[j], f );
	    }
	 }
      } else if ( len > 0 && fwrite( s, len, 1, f ) < 1 ) {
	 test( writeError.get() );
	 break;
      }

      if ( !FLAG_BINARY && fputc( '\n', f ) == EOF ) {
	  test( writeError.get() );
	  break;
      }
      fInfo->linesNumber++;

#ifdef _USE_MMAP_
      if ( !cmap ) {
	  if ( flag_realloc ) {
	      char *ch = (char*)malloc( si->len );
	      memset( ch, '\t', tabs );
	      memcpy( ch + tabs, si->str + first, len - first );
	      ::free( si->str );
	      si->str = ch;
	  }
	  continue;
      }

      if ( !si->map ) {
	  ::free( si->str );
	  si->map=1;
      }
      si->str = pos;
      pos += si->len + (FLAG_BINARY ? 0 : 1);
#endif

   }

   unlink( realName );
   fd = open( realName, O_RDWR | O_CREAT | O_TRUNC, fInfo->s.st_mode );
   FILE *forig = (fd >= 0 ? fdopen( fd, "w+" ) : 0);
   if ( forig <= 0 ) {
      fclose( f );
      sprintf( Buf1, lang("Can't open file\n%s\n\nCheck permissions, your file saved in\n%s",
			"Ошибка открытия файла\n%s\n\nПроверьте права доступа, файл сохранен в\n%s"), name, tmp );
      modal *m = new modal( ALIGN_CENTER, (char*)Buf1, Warning.get() );
      execUp( m );
      return 0;
   }

   fseek( f, SEEK_SET, 0 );
   for( int c; (c=fgetc(f)) != EOF; )
      fputc( c, forig );
   fclose( f );
   fclose( forig );
   unlink( tmp );
   chown( name, fInfo->s.st_uid, fInfo->s.st_gid );

   fd = open( realName, O_RDWR );
#ifdef _USE_MMAP_
   if ( fd > 0 ) {
       if ( cmap ) {
	   munmap( cmap, fInfo->s.st_size );
	   cmap=0;
       }
       fsync( fd );
       fstat( fd, &fInfo->s );
       if ( fInfo->s.st_size > 0 && !( S_IXUSR & fInfo->s.st_mode ||
				       S_IXGRP & fInfo->s.st_mode ||
				       S_IXOTH & fInfo->s.st_mode ) )
	   cmap = (char*)mmap( 0, fInfo->s.st_size, PROT_READ, MAP_SHARED, fd, 0 );
       ::close( fd );
   }
#else
   cmap = 0;
#endif

   if ( strlen( title->get() ) < 1 )
      title->put( name, 0 );

   *saved = 1;
   setChange( 0 );
   setWinIdent();
   correctShared();

   char *ext = strrchr( name, '.' );
   if ( ext && *(ext+1) ) {
      setTprocByExt( ext+1 );
      setHTprocByExt( ext+1 );
   }

   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();

   Task::sendMessage( appl, new BackMessage( HASH_cmRedraw ) );
   return 1;
}

int Edit::saveCollection()
{
   if ( !FLAG_CHANGE )
      return 1;
   if ( !externCollection )
      return 0;
   if ( saveTimer )
      saveTimer->close();
   externCollection->freeAll();
   for( long i=0, j=BOTT; i<j; i++ ) {
      strInfo *si = STRINFO( i );
      int len;
      char *str = getStr( si, len );
      char *s = new char[len+1];
      memcpy( s, str, len );
      s[len]=0;
      externCollection->insert( s );
   }
   if ( externCollection == &clipBoard )
      externCollection->atInsert( 0, new short(0) );
   setChange( 0 );
   setWinIdent();
   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();
   return 1;
}

int Edit::readBlock()
{
   if ( saveTimer )
      saveTimer->close();
   char *name = (HASH_cmOK == execUp( new filebox( filebox::filePath, filebox::fileMask, Buf2 ) ) ) ? Buf2 : 0;
   if ( !name )
    {
      if ( saveTimer && AUTOSAVE(eOpt.options) )
  saveTimer->open();
      return 0;
    }
   FILE *f = fopen( name, "r" );
   if ( !f )
     {
       sprintf( Buf1, openError.get(), name );
       test( Buf1 );
       if ( saveTimer && AUTOSAVE(eOpt.options) )
	  saveTimer->open();
       return 0;
     }
   addStrings( UNDO );
   appl->statusLine->draw( lang(" Block reading ..."," Чтение блока ..."), 1 );
   Screen::sync();
   long startLine = LINE, line;
   for( line = startLine; fgets( Buf1, maxLineLength-2, f ); line++ )
     {
       int len = strlen(Buf1);
       if ( len > 0 && Buf1[len-1] == '\n' )
	  Buf1[--len]=0;
       insertStr( line, Buf1, len );
     }
   fclose(f);

   saveUndo( HASH_cmUndoInsertBlock );
   unmarkBlock( NO_UNDO );
   block.bott = line-1;
   block.top = startLine;
   block.left = 0;
   block.right = SHRT_MAX;
   SET_BLOCKLINE;
//   appl->drawStatus();
   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();

   Task::sendMessage( appl, new BackMessage( HASH_cmRedraw ) );
   return 1;
}

int Edit::saveBlock()
{
   if ( !BLOCK_EXIST )
     {
       test( lang("No marked block","Нет отмеченного блока") );
       return 0;
     }
   if ( saveTimer )
      saveTimer->close();

   char *name = (HASH_cmOK == execUp( new filebox( filebox::filePath, filebox::fileMask, Buf2 ) ) ) ? Buf2 : 0;
   if ( !name )
     {
       if ( saveTimer && AUTOSAVE(eOpt.options) )
	  saveTimer->open();
       return 0;
     }
   static struct stat s;
   int existFile = 0;
   if ( !access( name, F_OK ) )
     {
       sprintf( Buf1, existWarning.get(), name );
       modal *m = new modal( ALIGN_CENTER, (char*)Buf1, Warning.get(), HASH_cmOK, Yes.get(), HASH_cmNo, No.get() );
       long ret = execUp( m );
       if ( HASH_cmOK != ret )
	{
	  if ( saveTimer && AUTOSAVE(eOpt.options) )
	     saveTimer->open();
	  return 0;
	}
       existFile = 1;
     }

   if ( existFile )
     {
       name = getRealFileName( name );
       stat( name, &s );
       unlink( name );
     }

   FILE *f = fopen( name, "w+" );
   if ( !f )
     {
       sprintf( Buf1, openError.get(), name );
       test( Buf1 );
       if ( saveTimer && AUTOSAVE(eOpt.options) )
	  saveTimer->open();
       return 0;
     }
   blockInfo *b = validBlock();
   appl->statusLine->draw( lang(" Block saving ..."," Сохранение блока ..."), 1 );
   Screen::sync();
   if ( FLAG_BLOCKLINE || FLAG_LINE )
     {
	strInfo *si; int len; char *s;
	for( long i=b->top; i <= b->bott; i++ )
	 {
	   si = STRINFO(i);
	   s = getStr( si, len );
	   if ( len > 0 && fwrite( s, len, 1, f ) < 1 )
	     { test( writeError.get() ); break; }
	   if ( !FLAG_BINARY && fputc( '\n', f ) == EOF )
	     { test( writeError.get() ); break; }
	 }
     }
   else
     {
	copyToBuf( blockBuf, b );
	for( long i=0, end=blockBuf.getCount(); i<end; i++ )
	  {
	    fputs( (char*)blockBuf.at(i), f );
	    fputs( "\n", f );
	  }
	blockBuf.freeAll();
     }
   fclose( f );
   if ( existFile )
      chmod( name, s.st_mode );

//   appl->drawStatus();
   if ( saveTimer && AUTOSAVE(eOpt.options) )
      saveTimer->open();

   Task::sendMessage( appl, new BackMessage( HASH_cmRedraw ) );
   return 1;
}

void Edit::reload( int flag )
{
   if ( flag && FLAG_CHANGE &&
	HASH_cmOK != test( reloadWarning.get(), Warning.get(), HASH_cmOK,
	Yes.get(), HASH_cmNo, No.get() ) )
      return;
   if ( !externCollection )
     {
       clearText();
       readFile( title->get() );
     }
   else
     readCollection();
   uInfo->cur = uInfo->start;
   uInfo->count = uInfo->fullCount = 0;
   if ( delta.y >= BOTT )
      delta.y = BOTT;
   unmarkBlock( NO_UNDO );
   setChange( 0 );
   correctShared();
   setWinIdent();
}

void Edit::clearText( int flag )
{
  long i, bott;
  strInfo *si=0;
  for( i=0, bott=BOTT; i < bott; i++ )
      if ( (si = (strInfo*)atext->at(i)) && !si->map )
	  ::free( si->str );
  if ( atext )
      atext->removeAll();

  if ( !flag )
      return;

#ifdef _USE_MMAP_
   if ( cmap )
     { munmap( cmap, fInfo->s.st_size ); cmap=0; }
#endif

   fInfo->linesNumber = 0;
}

void Edit::makeMenu( Menu *m )
{
   if ( !isGetObj && EditMenu )
     { winMenu = EditMenu; return; }
   if ( isGetObj && EditMenuGetObj )
     { winMenu = EditMenuGetObj; return; }

   menuColl *global = new menuColl;

   menuColl *coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Save",	"~Сохранить"),		new Lang("Save file","Сохранить файл"),	0, HASH_cmSave ) );

   if (	!isGetObj	)
   coll->insert( new menuItem( new Lang("S~ave as",	"Сохранить ~как"),	new Lang("Save file under new name","Сохранить под новым именем"), 0, HASH_cmSaveAs	)	);

   coll->insert( new menuItem( new Lang("~Reload",	"~Перечитать"),		new	Lang("Reload file","Перечитать файл с диска"),	0, HASH_cmReload ) );

   if (	!isGetObj	)
   coll->insert( new menuItem( new Lang("~Link",	"~Линк"),		new	Lang("Link window","Разделить окно"),	0, HASH_cmOpenShare )	);

   coll->insert( new menuItem( new Lang("~Information",	"~Информация"),		new	Lang("Information about text","Информация о тексте"),	0, HASH_cmFileInfo ) );
   coll->insert( new menuItem( new Lang("~Read only",	"~Чтение"),		new	Lang("Set/unset ~read only~ mode","Включить/выключить режим ~только чтение~"), 0, HASH_cmMode	)	);
#ifdef DJGPP
   coll->insert( new menuItem( new Lang("~Print",	"П~ечать"),		new	Lang("Print text","Печать текста"),	0, HASH_cmPrint	) );
#endif
   Menu	*sub = new Menu( coll, Point(1,0)	);
   global->insert( new menuItem( new Lang("~File","Файл"), 0, 0, sub ) );

   coll	=	new menuColl;
   coll->insert( new menuItem( new Lang("L~ine block marker",	"~Строчный маркер"),		new Lang("Set line block marker",	"Поставить/снять маркер строчного блока"), 0, HASH_cmBlockLine ) );
   coll->insert( new menuItem( new Lang("C~olumn block marker",	"~Колоночный маркер"),		new Lang("Set column block marker",	"Поставить/снять маркер колоночного блока"), 0, HASH_cmBlockColumn ) );
   coll->insert( new menuItem( new Lang("~Unmark",		"С~нять маркер блока"),		new Lang("Unmark block",		"Снять маркер блока"), 0, HASH_cmUnmarkBlock	)	);
   coll->insert( new menuItem( new Lang("~Copy",		"К~опировать"),			new Lang("Copy marked block",		"Копировать блок"), 0, HASH_cmCopy )	);
   coll->insert( new menuItem( new Lang("~Move",		"~Перенести"),			new Lang("Move marked block",		"Перенести блок"), 0, HASH_cmMove ) );
   coll->insert( new menuItem( new Lang("~Delete",		"~Удалить"),			new Lang("Delete marked block",		"Удалить блок"),	0, HASH_cmDeleteBlock )	);
   coll->insert( new menuItem( new Lang("Co~py to clipboard",	"Копировать в ~буфер"),		new Lang("Copy marked block to buffer",	"Копировать блок в буфер обмена"),	0, HASH_cmCopyToClip ) );
   coll->insert( new menuItem( new Lang("Mo~ve to clipboard",	"П~еренести в буфер"),		new Lang("Move marked block to buffer",	"Перенести блок в буфер обмена"), 0,	HASH_cmMoveToClip	)	);
   coll->insert( new menuItem( new Lang("~Add to clipboard",	"~Добавить к буферу"),		new Lang("Add marked block to buffer",	"Добавить блок к буферу обмена"), 0, HASH_cmAddToClip	)	);
   coll->insert( new menuItem( new Lang("~Edit clipboard",	"~Редактирование буфера"),	new Lang("Editing to buffer",		"Редактирование буфера обмена"),	0, HASH_cmClipboard	)	);
   coll->insert( new menuItem( new Lang("Copy ~from clipboard",	"Копировать ~из буфера"),	new Lang("Copy from buffer",		"Копирование из буфера обмена"), 0, HASH_cmCopyFromClip ) );
//   if	(	!isGetObj	)
//  {
   coll->insert( new menuItem( new Lang("~Load block",		"~Загрузить блок"),		new	Lang("Load file to block",	"Загрузить блок с диска"),	0, HASH_cmLoadBlock ) );
   coll->insert( new menuItem( new Lang("~Save block",		"Со~хранить блок"),		new	Lang("Write block to file",	"Записать блок на диск"), 0,	HASH_cmSaveBlock ) );
//   }
   sub = new Menu( coll, Point(1,6) );
   global->insert( new menuItem( new Lang("Bl~ock","Блок"), 0, 0, sub	)	);

   coll	=	new menuColl;
   coll->insert( new menuItem( new Lang("~Find",		"~Поиск"),		0, 0, HASH_cmFind )	);
   coll->insert( new menuItem( new Lang("~Replace",		"~Замена"),		0, 0, HASH_cmReplace ) );
   coll->insert( new menuItem( new Lang("~Go to line",		"П~ереход на строку"),	0, 0, HASH_cmGoto ) );
   coll->insert( new menuItem( new Lang("Go to ~C-tag",		"Переход на ~C-тзг"),	0, 0, HASH_cmCtagGo ) );
   coll->insert( new menuItem( new Lang("Mark ~line",		"З~апомнить строку"),	new Lang("Mark line for ~goto~ service","Запомнить строку для перехода"),	0, HASH_cmMarkGotoLine ) );
   coll->insert( new menuItem( new Lang("~Match forward",	"~Совпадение вниз"),	0, 0, HASH_cmMathDown ) );
   coll->insert( new menuItem( new Lang("Match ~backward",	"С~овпадение вверх"),	0, 0,	HASH_cmMathUp )	);
   coll->insert( new menuItem( new Lang("~Show spaces",		"~Видимость пробелов"),	new Lang("Show spaces and tab symbols","Видимость пробелов и символов табуляции"), 0,	HASH_cmSpaceMode ) );
   coll->insert( new menuItem( new Lang("~Format (smart)",	"~Умный формат"),	new Lang("Smart formatting of paragraph","Умное форматирование от текущей строки до конца абзаца"), 0, HASH_cmSmartFormat	)	);
   coll->insert( new menuItem( new Lang("F~ormat (dumb)",	"Тупой ~формат"),	new Lang("Dumb formatting of paragraph","Тупое форматирование от текущей строки до конца абзаца"), 0,	HASH_cmDumbFormat	)	);
   coll->insert( new menuItem( new Lang("~Text format",		"Фо~рматирование текста"),new Lang("Text formatting","Форматирование всего текста"), 0,	HASH_cmFormatText	)	);
   coll->insert( new menuItem( new Lang("~Insert code",		"Вставка ~кода"),	new Lang("Insert code","Вставка символа по коду"), 0, HASH_cmInputCode ) );
   coll->insert( new menuItem( new Lang("~Undo",		"О~ткат"),		0, 0, HASH_cmUndo ) );
   coll->insert( new menuItem( new Lang("R~edo",		"О~братный откат"),	0, 0, HASH_cmRedo ) );
   if (	!isGetObj )
   {
   coll->insert( new menuItem( new Lang("M~an",			"Стра~ница man"),	new Lang("Get man page for current word","Страница ~man~ по текущему слову"), 0, HASH_cmManual ) );
   coll->insert( new menuItem( new Lang("man ~browse mode",	"Ре~жим 'страница man'"),new Lang("Browse mode set to 'manual pages'","Режим просмотра текста 'страница ~man~'"), 0, HASH_cmShowMan )	);
   }
   coll->insert( new menuItem( new Lang("~Hard/soft cursor",	"~Жесткий/мягкий курсор"),	new	Lang("Hard/soft cursor switch","~Жесткий/мягкий режим курсора"), 0,	HASH_cmFreeze	)	);
   sub = new Menu( coll, Point(1,13), Point(1,12) );
   global->insert( new menuItem( new Lang("Serv~ice","Сервис"),	0, 0,	sub )	);

   coll	= new menuColl;
   coll->insert( new menuItem( new Lang("~1) original"),	0, 0, HASH_cmTransNone ) );
   coll->insert( new menuItem( new Lang("~2) 866  -> koi8"),	0, 0, HASH_cmTransAltKoi8 ) );
   coll->insert( new menuItem( new Lang("~3) 1251 -> koi8"),	0, 0, HASH_cmTransWinKoi8 ) );
   coll->insert( new menuItem( new Lang("~4) main -> koi8"),	0, 0, HASH_cmTransMainKoi8 ) );
   coll->insert( new menuItem( new Lang("~5) koi8 -> 866"),	0, 0, HASH_cmTransKoi8Alt ) );
   coll->insert( new menuItem( new Lang("~6) koi8 -> 1251"),	0, 0, HASH_cmTransKoi8Win ) );
   coll->insert( new menuItem( new Lang("~7) koi8 -> main"),	0, 0, HASH_cmTransKoi8Main ) );
   sub = new Menu( coll, Point(1,22), Point(1,20) );
   global->insert( new menuItem( new Lang("Vie~w","Кодировка"),	0, 0,	sub )	);

   if (	!isGetObj )
   {
   coll = new menuColl;
   coll->insert( new menuItem( new Lang("~Open",	"~Открыть"),		new Lang("Open debug window",			"Открыть окно отладчика"), 0, HASH_cmOpenDebug )	);
   coll->insert( new menuItem( new Lang("~Breakpoint",	"~Точка останова"),	new Lang("Set breakpoint in current line",	"Поставить/снять точку останова"), 0, HASH_cmBreakpoint ) );
   coll->insert( new menuItem( new Lang("Clear in ~window","~Убрать в окне"),	new Lang("Clear breakpoints in current window",	"Убрать точки останова в окне"), 0, HASH_cmClearBpoint ) );
   coll->insert( new menuItem( new Lang("Clear in ~all","У~брать во всех"),	new Lang("Clear breakpoints in all windows",	"Убрать точки останова во всех окнах"), 0, HASH_cmClearBpointAll ) );
   coll->insert( new menuItem( new Lang("~Close",	"~Закрыть"),		new Lang("Close debug window",			"Закрыть окно отладчика"), 0, HASH_cmCloseDebug ) );
   sub = new Menu( coll, Point(1,28), Point(1,31) );
   global->insert( new menuItem( new Lang("Deb~ug","Отладчик"),	0, 0,	sub )	);
   }

   winMenu = new Menu( global, Point(0,0), new Lang("Edit","Ред"), 0, 1, 0 );
   if ( !isGetObj )
      EditMenu = winMenu;
   else
      EditMenuGetObj = winMenu;
   winMenu->fill( &keyHolder );
   sharedMenu.insert( winMenu );
}

void infoInsert( Collection *coll, char *str )
{
   int len = strlen( str );
   char *ch = new char[ len + 1 ];
   memcpy( ch, str, len + 1 );
   if ( len > 1 && ch[len-1] == '\n' )
     ch[len-1] = 0;
   coll->insert( ch );
}

void Edit::fileInfo()
{
   Collection *infoColl = new Collection(10,5);

   char *name = title->get();
   char *realName = getRealFileName( name );
   if ( !strcmp( name, realName ) )
    {
      sprintf( Buf1, lang("Name: %s", "Имя: %s"), name );
      infoInsert( infoColl, Buf1 );
    }
   else
    {
      sprintf( Buf1, lang("Name(link): %s", "Имя(сим.линк): %s"), name );
      infoInsert( infoColl, Buf1 );
      sprintf( Buf1, lang("Name(real): %s", "Имя(реальное): %s"), realName );
      infoInsert( infoColl, Buf1 );
    }
   static char uidname[32];
   static char gidname[32];
   if ( !externCollection && strlen(name) > 0 )
     {
       struct passwd *pw = getpwuid( fInfo->s.st_uid );
       if ( pw )
	   strcpy( uidname, pw->pw_name );
       else
	   sprintf( uidname, "%ld", (long)fInfo->s.st_uid );
       struct group *gr  = getgrgid( fInfo->s.st_gid );
       if ( gr )
	   strcpy( gidname, gr->gr_name );
       else
	   sprintf( gidname, "%ld", (long)fInfo->s.st_gid );

       sprintf( Buf1, lang("Owner ....................... %s.%s",	"Владелец ...................... %s.%s"),	uidname, gidname );
       infoInsert( infoColl, Buf1 );

       strcpy( Buf1, lang( "Protection .................. ",		"Права доступа ................. ") );
       int len = strlen( Buf1 );

       int uidf = getuid() == fInfo->s.st_uid;
       int gidf = getgid() == fInfo->s.st_gid;
       if ( uidf ) gidf=0;
       if ( gidf ) uidf=0;
       int othf = !(uidf || gidf);

       Buf1[len++] = fInfo->s.st_mode & S_IRUSR ? (uidf ? 'R' : 'r') : '-';
       Buf1[len++] = fInfo->s.st_mode & S_IWUSR ? (uidf ? 'W' : 'w') : '-';
       Buf1[len++] = fInfo->s.st_mode & S_IXUSR ? (uidf ? 'X' : 'x') : '-';

       Buf1[len++] = fInfo->s.st_mode & S_IRGRP ? (gidf ? 'R' : 'r') : '-';
       Buf1[len++] = fInfo->s.st_mode & S_IWGRP ? (gidf ? 'W' : 'w') : '-';
       Buf1[len++] = fInfo->s.st_mode & S_IXGRP ? (gidf ? 'X' : 'x') : '-';

       Buf1[len++] = fInfo->s.st_mode & S_IROTH ? (othf ? 'R' : 'r') : '-';
       Buf1[len++] = fInfo->s.st_mode & S_IWOTH ? (othf ? 'W' : 'w') : '-';
       Buf1[len++] = fInfo->s.st_mode & S_IXOTH ? (othf ? 'X' : 'x') : '-';
       Buf1[len] = 0;
       infoInsert( infoColl, Buf1 );

       sprintf( Buf1, lang("File size ................... %ld",	"Размер файла .................. %ld"), fInfo->s.st_size );
       infoInsert( infoColl, Buf1 );

       sprintf( Buf1, lang("Number of lines in file ..... %ld",	"Количество строк в файле ...... %ld"), fInfo->linesNumber );
       infoInsert( infoColl, Buf1 );
     }
   long lines = BOTT;
   sprintf( Buf1, lang("Number of lines in window ... %ld",	"Количество строк в окне ....... %ld"), lines );
   infoInsert( infoColl, Buf1 );

   if ( !externCollection && strlen(name) > 0 )
     {
       sprintf( Buf1, lang("Time of last access ......... %s",		"Время последнего доступа ...... %s"), ctime( &fInfo->s.st_atime ) + 4 );
       infoInsert( infoColl, Buf1 );

       sprintf(	Buf1, lang("Time of last modification ... %s",		"Время последней модификации ... %s"), ctime( &fInfo->s.st_mtime ) + 4 );
       infoInsert( infoColl, Buf1 );
     }

   modal *m = new modal( 2/*отступ*/, infoColl,	lang("Information","Информация") );
   exec( m );
   delete m;

   delete infoColl;
}

#ifdef DJGPP

void Edit::printText()
{
   int prnDesc=-1;
   long ret;
m1:
   if ( (prnDesc = open( "prn", O_WRONLY )) < 0 )
    {
      ret = test( lang("Printer not ready. Retry?","Принтер не готов. Повторить?"), Warning.get(), HASH_cmOK, Yes.get(), HASH_cmNo, No.get() );
      if ( ret == HASH_cmOK )
	 goto m1;
      return;
    }
   int byteCount=0, len;
   for( long line=0, end=BOTT; line < end; line++ )
    {
      strInfo *si = STRINFO( line );
      char *s = getStr( si, len );
      while( 1 )
       {
	 byteCount = write( prnDesc, s, len );
	 if ( byteCount == len )
	    break;
	 ret = test( lang("Print error. Retry?","Ошибка печати. Повторить?"), Warning.get(), HASH_cmOK, Yes.get(), HASH_cmNo, No.get() );
	 if ( ret == HASH_cmOK )
	   {
	     len -= byteCount;
	     continue;
	   }
	 goto end;
       }
      write( prnDesc, "\r\n", 2 );
    }
end:
   ::close( prnDesc );
}

#endif

