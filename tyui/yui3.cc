/*
	$Id: yui3.cc,v 3.2.2.1 2007/07/24 09:58:11 shelton Exp $
*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <reg_expr.h>
#include <dialog.h>
#include <status.h>
#include <i_lines.h>
#include <program.h>
#include "yui.h"
#include "hashcode.h"

long Edit::boxFind()
{
   int opt = dFindRepl.options;
   int dir = dFindRepl.direction;
   int whr = dFindRepl.where;
   static Rect r( Screen::center( 12, 52 ) );
   Dialog *d = new Dialog( r, new Lang("Find","Поиск"), 0, new Lang("~Enter~-start","~Enter~-старт") );
   d->setHelpContext( "Find" );

   static char word[256];
   char *wr = currentWord();
   if ( wr[0] )
       strncpy( word, wr, 255 );
   else if ( strlen( dFindRepl.strFind ) > 0 )
       strncpy( word, dFindRepl.strFind, 255 );
   else if ( histFind.getCount() > 0 )
       strcpy( word, (char*)histFind.at(0) );
   word[255] = 0;

   inputLine *il = new inputLine( Point(1,1), word, word, 47, 255, 0 , &histFind );
   if ( !il )
      return 0;
   d->insert( il );

   d->put( Point(3,3), lang("Options","Опции") );
   checkBox *cb1 = new checkBox( Point(4,1), opt, &opt );
   if ( !cb1 )
      return 0;
   cb1->add( lang("Case-sensitive",	"Чувство к регистру") );
   cb1->add( lang("From end",		"От конца") );
   cb1->add( lang("Word only",		"Только слово") );
   cb1->add( lang("Special symbols",	"Специальные символы") );
   cb1->add( lang("Regular expression", "Регулярное выражение") );
   cb1->add( lang("All windows",	"Во всех окнах") );
   d->insert( cb1 );

   d->put( Point(3,31), lang("Direction","Направление") );
   radioBox *rb1 = new radioBox( Point(4,29), dir, &dir );
   if ( !rb1 )
      return 0;
   rb1->add( lang("Forward",	"Вперед") );
   rb1->add( lang("Backward",	"Назад") );
   rb1->add( lang("From begin","От начала") );
   d->insert( rb1 );

   d->put( Point(7,31), lang("Where","Где") );
   radioBox *rb2 = new radioBox( Point(8,29), whr, &whr );
   if ( !rb2 )
      return 0;
   rb2->add( lang("Global",	"Во всем тексте") );
   rb2->add( lang("In block",		"В блоке") );
   d->insert( rb2 );

   long ret = execUp( d );	// no switch to another window

   if ( ret == HASH_cmOK ) {
	dFindRepl.options = opt;
	dFindRepl.direction = dir;
	dFindRepl.where = whr;
	strcpy( dFindRepl.strFind, word );
   }

   return ret;
}

long Edit::boxReplace()
{
   int opt = dFindRepl.options;
   int dir = dFindRepl.direction;
   int whr = dFindRepl.where;
   static Rect r( Screen::center( 15, 52 ) );
   Dialog *d = new Dialog( r, new Lang("Find & Replace","Поиск с заменой"), 0, new Lang("~Enter~-start","~Enter~-старт") );
   d->setHelpContext( "Replace" );

   static char wordF[256];
   static char wordR[256];
   char *wr = currentWord();
   if ( wr[0] )
       strncpy( wordF, wr, 255 );
   else if ( strlen( dFindRepl.strFind ) > 0 )
       strncpy( wordF, dFindRepl.strFind, 255 );
   else if ( histFind.getCount() > 0 )
       strcpy( wordF, (char*)histFind.at(0) );
   wordF[255] = 0;
   d->put( Point(1,3), lang(" Find:","Поиск:") );
   inputLine *il1 = new inputLine( Point(1,10), wordF, wordF, 38, 255, 0 , &histFind );
   if ( !il1 )
      return 0;
   d->insert( il1 );

   if ( strlen( dFindRepl.strRepl ) > 0 )
      strncpy( wordR, dFindRepl.strRepl, 255 );
   else if ( histRepl.getCount() > 0 )
       strcpy( wordR, (char*)histRepl.at(0) );
   wordR[255] = 0;
   d->put( Point(3,1), lang("Replace:"," Замена:") );
   inputLine *il2 = new inputLine( Point(3,10), wordR, wordR, 38, 255, 0 , &histRepl );
   if ( !il2 )
      return 0;
   d->insert( il2 );

   checkBox *cb1 = new checkBox( Point(5,1), opt, &opt );
   if ( !cb1 )
      return 0;
   cb1->add( lang("Case-sensitive",	"Чувство к регистру") );
   cb1->add( lang("From end",		"От конца") );
   cb1->add( lang("Word only",		"Только слово") );
   cb1->add( lang("Special symbols",	"Специальные символы") );
   cb1->add( lang("Regular expression","Регулярное выражение") );
   cb1->add( lang("All windows",	"Во всех окнах") );
   cb1->add( lang("Prompt on replace",	"Приглашение на замену") );
   cb1->add( lang("Change all",		"Заменить все") );
   d->insert( cb1 );

   d->put( Point(5,31), lang("Direction","Направление") );
   radioBox *rb1 = new radioBox( Point(6,29), dir, &dir );
   if ( !rb1 )
      return 0;
   rb1->add( lang("Forward",	"Вперед") );
   rb1->add( lang("Backward",	"Назад") );
   rb1->add( lang("From begin","От начала") );
   d->insert( rb1 );

   d->put( Point(9,31), lang("Where","Где") );
   radioBox *rb2 = new radioBox( Point(10,29), whr, &whr );
   if ( !rb2 )
      return 0;
   rb2->add( lang("Global",	"Во всем тексте") );
   rb2->add( lang("In block",	"В блоке") );
   d->insert( rb2 );

   long ret = execUp( d );

   if ( ret == HASH_cmOK )
     {
	dFindRepl.options = opt;
	dFindRepl.direction = dir;
	dFindRepl.where = whr;
	strcpy( dFindRepl.strFind, wordF );
	strcpy( dFindRepl.strRepl, wordR );
     }
   return ret;
}

Collection Edit::histGoto( 10, 10 );

void Edit::boxGoto()
{
   char *line = getString( lang("Line:","Номер"), 6, &histGoto, 0,0,0,0,
      inputLineFillChar, lang(" Enter line number"," Введите номер строки для перехода"), U_INT );
   if ( line && !gotoLine( atol(line) ) )
      appl->test( lang("Bad line number","Неправильный номер строки") );
}

int Edit::gotoLine( long line, int flagUndo )
{
   if ( line < 1 || line > BOTT )
      return 0;
   line--;
   if ( flagUndo )
       saveUndo( HASH_cmUndoGoto );
   if ( line < delta.y || line >= delta.y + size.y - 2 ) {
       cursor.y = size.y/2;
       delta.y = max( 0, line - cursor.y );
       if ( !delta.y )
	   cursor.y = line;
   } else {
       cursor.y = line - delta.y;
   }

   return 1;
}

void Edit::markGotoLine()
{
   static char line[10];
   sprintf( line, "%ld", LINE + 1 );
   char *s = new char[ strlen(line) + 1 ];
   strcpy( s, line );
   histGoto.atInsert( 0, s );
}

/*-------------------------- FIND & REPLACE --------------------------*/

dataFindRepl Edit::dFindRepl;
Collection Edit::histFind( 10, 10 );
Collection Edit::histRepl( 10, 10 );

static re_registers regs;

char *Edit::currentWord()
{
   Buf1[0] = 0;
   long line = LINE;
   if ( line >= BOTT )
      return Buf1;
   strInfo *si = STRINFO( line );
   if ( !si )
      return Buf1;
   int len;
   char *s = getStr( si, len );
   int rPos = realPos( si, COLUMN );
   if ( rPos >= len || strchr( wordDelim, s[rPos] ) )
      return Buf1;
   int i, j, k;
   for( i=rPos; i >= 0 && !strchr( wordDelim, s[i] ); i-- );
   i++;
   for( j=0, k=len - i; j < k && !strchr( wordDelim, s[i] ); i++, j++ )
	Buf1[j] = s[i];
   Buf1[j]=0;
   return Buf1;
}

int Edit::find( int offset, int flagStatus, int flagStart, int flagUndo )
{
   long line, end = BOTT-1, Line = LINE;
   int pr = 1, rPos = SHRT_MAX, sPos = COLUMN + (offset ? 1 : 0);

   if ( !dFindRepl.direction ) {		// forward
       line = flagStart ? 0 : (Line > end ? max( 0, end ) : Line);
   } else if ( dFindRepl.direction == 1 ) {	// backward
       line = flagStart || Line>end ? max( 0, end ) : Line;
       end=0;
       pr = -1;
   } else {
       line = 0;			// from text begin
   }

   if ( dFindRepl.where ) {		// find in block
       if ( !FLAG_BLOCKLINE && !FLAG_BLOCKCOL ) {
	   test( lang("No marked block","Нет отмеченного блока") );
	   return -1;
       }
       blockInfo *b = validBlock();
       if ( !dFindRepl.direction ) {	// forward
	   if ( line > b->bott )
	       return 0;
	   if ( line < b->top )
	       line = b->top;
	   end = b->bott;
       } else if ( dFindRepl.direction == 1 ) {	// backward
	   if ( line < b->top )
	       return 0;
	   if ( line > b->bott )
	       line = b->bott;
	   end = b->top;
       } else {				// from begin
	   line = b->top;
	   end = b->bott;
       }
   }

   if ( dFindRepl.options & FIND_SPECIAL ) {
       int i=0, slash=0;
       for( char *s=dFindRepl.strFind; *s; s++ ) {
	   switch( *s ) {
	       case '\\':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\\'; continue;
		   }
		   slash = 1;
		   continue;
	       case 'a':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\a'; continue;
		   }
		   break;
	       case 'b':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\b'; continue;
		   }
		   break;
	       case 'e':
		   if ( slash ) {
		       slash=0; Buf2[i++]=0x1b; continue;
		   }
		   break;
	       case 'f':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\f'; continue;
		   }
		   break;
	       case 'n':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\n'; continue;
		   }
		   break;
	       case 'r':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\r'; continue;
		   }
		   break;
	       case 't':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\t'; continue;
		   }
		   break;
	       case 'v':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\v'; continue;
		   }
		   break;
	   }
	   Buf2[i++] = *s;
       }
       Buf2[i]=0;
   } else {
       strcpy( Buf2, dFindRepl.strFind );
   }

   if ( dFindRepl.options & FIND_REGEXPR ) {
       strcpy( Buf1, Buf2 );
   } else {
       int i=0;
       for( char *s=Buf2; *s; s++ ) {
	   switch( *s ) {
	       case '\\':
	       case '+':
	       case '.':
	       case '*':
	       case '?':
	       case '[':
	       case '(':
	       case ')':
	       case '|':
	       case '^':
	       case '$':
		   Buf1[i++]='\\';
		   break;
	   }
	   Buf1[i++] = *s;
       }
       Buf1[i]=0;
   }

   static Regexpr ex;
   char *ch = ex.compile( Buf1, !(dFindRepl.options & FIND_CASE_SENSITIVE) );
   if ( ch ) {
       test( ch );
       return -1;
   }

   if ( flagStatus ) {
       strcpy( Buf2, lang(" ~Find :~ "," ~Поиск :~ ") );
       strcat( Buf2, dFindRepl.strFind );
       appl->statusLine->draw( Buf2, 1 );
       Screen::sync();
   }

   int fromEnd = 0;
   if ( find_Info.found )
       fromEnd = dFindRepl.options & FIND_FROM_END;
   int fullWord = dFindRepl.options & FIND_WORD_ONLY;
   find_Info.found = 0;
   for( ; !find_Info.found && (!dFindRepl.direction && line<=end ||
				dFindRepl.direction==1 && line>=end); line+=pr )
    {
       strInfo *si = STRINFO( line );
       if ( !si )
	  return 0;
       if ( rPos )
	{
	  rPos = realPos( si, sPos );
	  if ( fromEnd )
	     rPos += find_Info.end - find_Info.start;
	}
       int len;
       char *s = getStr( si, len, 1 );

       if ( rPos >= len )
	  { rPos=0; continue; }

       while( 1 ) {
	  int ret = ex.search( s, len, rPos, len - rPos, &regs );
	  if ( ret < 0 ) {
	      rPos = 0;
	      break;
	  }
	  //------------------------------------
	  if ( flagUndo )
	      saveUndo( HASH_cmUndoFind );
	  //------------------------------------
	  find_Info.line  = line;
	  find_Info.start = regs.start[0];
	  find_Info.end   = regs.end[0]-1;
	  if ( fullWord && ( find_Info.start > 0 && !strchr( wordDelim, s[find_Info.start - 1] ) ||
	       find_Info.end < len && !strchr( wordDelim, s[find_Info.end + 1] ) ) )
	    { rPos++; continue; }

	  find_Info.found = 1;
	  sPos = max( 0, scrPos( si, find_Info.start ) );
#if 0
	  if ( len > 0 && find_Info.start==len && find_Info.start == find_Info.end ) {
	      // наверное это '\n'
	      sPos++;
	  }
#endif
	  int d = sPos - delta.x;
	  if ( d<0 || d >= size.x-2 )
	     delta.x = max( 0, sPos - size.x + (box?2:0) + 1 );
	  cursor.x = sPos - delta.x;

	  if ( line < delta.y || line >= delta.y + size.y - (box?2:0) ) {
	      cursor.y = size.y/2;
	      delta.y = max( 0, line - cursor.y );
	      if ( !delta.y )
		  cursor.y = line;
	  } else {
	      cursor.y = line - delta.y;
	  }
	  rPos = 0;
	  correctBlock();
	  break;
      }
    }
   return find_Info.found;
}

int Edit::replace( int flagStart, unsigned *replaced )
{
   find_Info.replaced = 0;
   int offset=0;
   Message *msg = 0;
   KeyMessage *km = 0;
   static char bufStatus[256];

   if ( dFindRepl.options & REPL_PROMPT ) {
       strcpy( bufStatus, lang(" ~Esc~-exit  ~Enter~-replace  ~Any key~-skip;   ~Replaced:~ ",
			       " ~Esc~-конец  ~Enter~-заменить  ~Любая клавиша~-пропустить;   ~Заменено:~ " ) );
   } else {
       strcpy( bufStatus, lang(" ~Replaced:~ "," ~Заменено:~ ") );
   }

   if ( dFindRepl.options & FIND_SPECIAL ) {
       int i=0, slash=0;
       for( char *s=dFindRepl.strRepl; *s; s++ ) {
	   switch( *s ) {
	       case '\\':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\\'; continue;
		   }
		   slash = 1;
		   continue;
	       case 'a':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\a'; continue;
		   }
		   break;
	       case 'b':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\b'; continue;
		   }
		   break;
	       case 'e':
		   if ( slash ) {
		       slash=0; Buf2[i++]=0x1b; continue;
		   }
		   break;
	       case 'f':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\f'; continue;
		   }
		   break;
	       case 'n':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\n'; continue;
		   }
		   break;
	       case 'r':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\r'; continue;
		   }
		   break;
	       case 't':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\t'; continue;
		   }
		   break;
	       case 'v':
		   if ( slash ) {
		       slash=0; Buf2[i++]='\v'; continue;
		   }
		   break;
	   }
	   if ( slash ) {
	       Buf2[i++] = '\\';
	   }
	   Buf2[i++] = *s;
       }
       Buf2[i]=0;
   } else {
       strcpy( Buf2, dFindRepl.strFind );
   }

   char *sRepl = strdup( Buf2 );
   if ( !sRepl ) {
       return 0;
   }

   int returnStatus=1, lenStatus = strlen( bufStatus );
   while( find( offset, 0, flagStart, NO_UNDO ) ) {
       flagStart = 0;
       offset = 1;
       int flag = 1;
       sprintf( bufStatus+lenStatus, "%d", find_Info.replaced );
       appl->statusLine->draw( bufStatus, 1 );
       Screen::sync();
       if ( dFindRepl.options & REPL_PROMPT )
	 {
	    if ( owner )
	       owner->draw();
	    else
	       draw();
	    Screen::sync();
	    while( (msg = Task::getMessage()) )
	      {
		if ( msg->type() == HASH_Key )
		   break;
		Task::respond( msg );
	      }
	    km = (KeyMessage*)msg;
	    Task::respond(msg);

	    if ( km->data == kbEsc )
	       { returnStatus=0; break; }
	    if ( km->data != kbEnter )
	       flag=0;
	 }
       if ( !flag )
	  continue;

       strInfo *si = STRINFO( find_Info.line );
       //---------------------------------------------------
       int len;
       char *s = getStr( si, len );
       char *str = new char[len+1];
       memcpy( str, s, len ); str[len]=0;
       undoItem *u = saveUndo( HASH_cmUndoReplace );
       Collection *coll = new Collection( 2, 0 );
       coll->insert( str );
       u->ptr = coll;
       u->sym1 = dFindRepl.options & REPL_PROMPT ? 0 : 1;
       //---------------------------------------------------
       str = s;
       char *strRepl = sRepl;
       if ( dFindRepl.options & FIND_REGEXPR ) {
	   int i=0, j=0;
	   for( ; sRepl[i]; i++ ) {
	       if ( sRepl[i] != '\\' ) {
		   Buf1[j++] = sRepl[i];
	       } else if ( sRepl[++i] && isdigit( sRepl[i] ) ) {
		   int n = sRepl[i] - '0';
		   if ( regs.end[n] >= 0 ) {
		       for( int k = regs.start[n]; k<regs.end[n]; k++ )
			   Buf1[j++] = str[k];
		   } else {
		       Buf1[j++] = '\\';
		       Buf1[j++] = sRepl[i];
		   }
	       } else {
		 Buf1[j++] = sRepl[i];
	       }
	   }
	   Buf1[j]=0;
	   strRepl = Buf1;
       }

       int l = strlen( strRepl );
       int newLen = len - (find_Info.end - find_Info.start + 1) + l;
       char *_buf = new char[newLen+1];
       s = _buf;
       memcpy( s, str, find_Info.start );
       s += find_Info.start;
       memcpy( s, strRepl, l );
       s += l;
       memcpy( s, str + find_Info.end + 1, len - find_Info.end - 1 );
       //------------------------------------------------------
       coll->insert( _buf );
       //------------------------------------------------------
       replaceStr( find_Info.line, _buf, newLen );
       find_Info.replaced++;

       if ( dFindRepl.options & FIND_FROM_END )
	  cursor.x += l - (l > 0 ? 1 : 0);

       if ( !( dFindRepl.options & REPL_CHANGE_ALL ) )
	  break;
   }

   if ( replaced )
       *replaced += find_Info.replaced;

   if ( sRepl )
       ::free( sRepl );

   return returnStatus;
}

int Edit::findMatch( int direction, int flagUndo )
{
   long line = LINE, bott = BOTT;
   if ( line >= bott )
      return 0;
   strInfo *si = STRINFO( line );
   if ( !si )
      return 0;
   int len;
   char *s = getStr( si, len );
   int j, d, check=1, rPos = realPos( si, COLUMN );
   if ( rPos >= len )
      return 0;
   const char *s1 = "{([<", *s2="})]>";
   char end, sym = s[rPos];
   char *f=(char*)strchr( s1, sym );
   if ( f )
     { d=1; end = s2[f-s1]; }
   else if ( (f=(char*)strchr( s2, sym )) )
     { d=-1; end=s1[f-s2]; }
   else
     { d=direction; end = sym; }

   rPos += d;
   if ( rPos < 0 || rPos >= len )
     {
       line += d;
       if ( d > 0 )
	  rPos = 0;
       else if ( line < 0 )
	  goto notfound;
       else
	  rPos = (STRINFO(line))->len-1;
     }
   for( ; line >= 0 && line < bott; line+=d )
     {
       si = STRINFO( line );
       if ( !si )
	  goto notfound;
       s = getStr( si, len );
       j = rPos<0 ? (d>0 ? 0 : len-1) : rPos;
       for( ; j >= 0 && j < len; j+=d )
	 {
	    if ( s[j]==sym && s[j]!=end )
		check++;
	    else if ( s[j]==end )
		check--;
	    if ( check )
	       continue;
	    //--------------------------------------------------
	    if ( flagUndo )
	     {
	       undoItem *u = saveUndo( HASH_cmUndoMatch );
	       u->longVal = direction;
	     }
	    //--------------------------------------------------
	    if ( line < delta.y )
		delta.y = max( 0, line-1 );
	    else if ( line >= delta.y + size.y - (box?2:0) )
		delta.y = line - size.y + (box?2:0) + 1;
	    cursor.y = line - delta.y;
	    int sPos = scrPos( si, j );
	    if ( sPos < delta.x )
		delta.x = sPos;
	    else if ( sPos >= delta.x + size.x - (box?2:0) )
		delta.x = sPos - size.x + (box?2:0) + 1;
	    cursor.x = sPos - delta.x;
	    return 1;
	 }
       rPos = -1;
     }
notfound:
   strcpy( Buf1, lang("Symbol '","Символ '") );
   Buf1[8] = end;
   strcpy( Buf1+9, lang("' not found","' не найден") );
   test( Buf1 );
   return 0;
}

