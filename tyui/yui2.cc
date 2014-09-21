/*
	$Id: yui2.cc,v 3.2.2.1 2007/07/24 09:58:11 shelton Exp $
*/
#include <string.h>
#include <limits.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <status.h>
#include <program.h>
#include "yui.h"
#include "hashcode.h"

Collection blockBuf( 50, 50 );

void Edit::markBlock( int type, int flagUndo )
{
   //-----------------------------------------------
   if ( flagUndo == UNDO )
     {
       undoItem *u = saveUndo( HASH_cmUndoMarkBlock );
       if ( u )
	  u->sym1 = type;
     }
   //-----------------------------------------------
   Window::markBlock( type );
}

void Edit::unmarkBlock( int flagUndo )
{
   //-----------------------------------------------
   if ( flagUndo )
      saveUndo( HASH_cmUndoUnmarkBlock, 1 );
   //-----------------------------------------------
   Window::unmarkBlock();
}

void Edit::copyToBuf( Collection &buf, blockInfo *b )
{
   char *str; int len;
   for( long i=b->top; i<=b->bott; i++ ) {
       strInfo *si = STRINFO( i );
       str = getStr( si, len );
       int left = realPos( si, b->left ),
	   right= realPos( si, b->right );
       int len2 = right - left + 1;
       int l = min( len2, len - left );
       char *s = new char[len2+1];
       if ( l > 0 ) {
	   memcpy( s, str + left, l );
	   if ( right >= len )
	       memset( s+l, ' ', right-len+1 );
       } else {
	  memset( s, ' ', len2 );
       }
       s[len2]=0;
       buf.insert( s );
   }
}

int Edit::copyFromBuf( long line, int sPos, Collection &buf, short l, int flagUndo, int flagMove )
{
  unsigned char insLines = 0;
  blockInfo *b = flagMove ? validBlock() : 0;
  int len, cursorCorrect = (b && line==b->top && line==b->bott && sPos > b->right) ? l : 0;
  long i=0, bott=buf.getCount();
  strInfo *si = NULL;
  char *s=NULL, *ss=NULL;
  for( ; i < bott; i++, line++ ) {
      if ( line >= BOTT ) {
	  insertStr( line, "", 0 );
	  insLines++;
      }

      si = STRINFO( line );
      s = getStr( si, len );
      int j, k, spaces=0, rPos = realPos( si, sPos - cursorCorrect );
      for( j=0; j < len && j < rPos; j++ )
	  Buf1[j] = s[j];
      if ( rPos < len ) {
	  if ( s[rPos] == '\t' && (sPos & stepTab) )
	      spaces = sPos - scrPos( si, rPos-1 ) - 1;
	  for( int sp=0; sp < spaces; sp++, Buf1[j++]=' ' );
      } else {
	  for( ; j < rPos; Buf1[j++]=' ' );
      }

      ss = (char*)buf.at( i );
      for( k=0; ss[k]; Buf1[j++] = ss[k++] );
      for( ; rPos < len; Buf1[j++] = s[rPos++] );

      replaceStr( line, Buf1, j );
  }
  if ( i > 0 && flagUndo ) {
      undoItem *u = saveUndo( HASH_cmUndoCopyBlock );
      u->sym1 = insLines;
  }
  unmarkBlock( NO_UNDO );
  block.bott = line-1;
  block.top = block.bott - i + 1;
  block.left = sPos - cursorCorrect;
  block.right = block.left + l - 1;
  SET_BLOCKCOL;
  if ( delta.x > block.left )
      delta.x = block.left;
  cursor.x = block.left - delta.x;
  return insLines;
}

void Edit::copyBlock( int flagUndo )
{
   if ( !FLAG_LINE && !FLAG_BLOCKLINE && !FLAG_COL && !FLAG_BLOCKCOL )
       return;
   long line = LINE;
   appl->statusLine->draw( lang(" Block copying ..."," Копирование блока ..."), 1 );
   Screen::sync();
   blockInfo *b = validBlock();
   long lenBlock = b->bott - b->top;
   if ( FLAG_LINE || FLAG_BLOCKLINE ) {
       if ( line > b->top && line <= b->bott ) {
	   appl->drawStatus();
	   return;
       }
       if ( line >= BOTT )
	   addStrings( flagUndo );
      //-----------------------------------------------
      if ( flagUndo )
	  saveUndo( HASH_cmUndoCopyBlock );
      //-----------------------------------------------
      int pr = line <= b->top ? 1 : 0;
      for( long i=b->top, bott=b->bott; i<=bott; i+=pr+1, bott+=pr )
	  insertStr( line++, STRINFO( i ) );
      unmarkBlock( NO_UNDO );
      block.bott = line - 1;
      block.top = block.bott - lenBlock;
      SET_BLOCKLINE;
   } else if ( FLAG_COL || FLAG_BLOCKCOL ) {
      if ( line >= BOTT )
	  addStrings( flagUndo );
      blockBuf.freeAll();
      copyToBuf( blockBuf, b );
      copyFromBuf( LINE, COLUMN, blockBuf, b->right - b->left + 1, flagUndo );
      blockBuf.freeAll();
   }
   appl->drawStatus();
}

void Edit::moveBlock( long line, int column, int flagUndo )
{
   if ( !FLAG_LINE && !FLAG_BLOCKLINE && !FLAG_COL && !FLAG_BLOCKCOL )
      return;
   if ( !flagUndo )
     {
       appl->statusLine->draw( lang(" Block moving ..."," Перемещение блока ..."), 1 );
       Screen::sync();
     }
   blockInfo *b = validBlock();
   if ( FLAG_LINE || FLAG_BLOCKLINE )
    {
      if ( line >= b->top && line <= b->bott )
	 { appl->drawStatus(); return; }
      if ( line >= BOTT )
	 addStrings( flagUndo );
      long l, i, lenBlock = b->bott - b->top, pr = line < b->top ? 1 : 0;
      strInfo *si=NULL, Si(0,0,0);
      for( l=0, i=b->top; l<=lenBlock; l++, line+=pr ) {
	  si = STRINFO( i );
	  Si.str = si->str;
	  Si.len = si->len;
	  Si.map = si->map;
	  atext->atInsert( line, Si );
	  i+=pr;
	  atext->atRemove( i );
      }
      //---------------------------------------------
      if ( flagUndo )
	{
	  undoItem *u = saveUndo( HASH_cmUndoMoveBlock );
	  u->longVal = i;
	}
      //---------------------------------------------
      unmarkBlock( NO_UNDO );
      block.top = LINE - (pr ? 0 : lenBlock+1);
      block.bott = block.top + lenBlock;
      cursor.y = block.top - delta.y;
      if ( cursor.y<0 )
	 { delta.y += cursor.y; cursor.y=0; }
      SET_BLOCKLINE;
      setChange( 1 );
      setWinIdent();
    }
   else if ( FLAG_COL || FLAG_BLOCKCOL )
    {
      if ( line >= BOTT )
	 addStrings( flagUndo );
      //---------------------------------------------
      undoItem *u = flagUndo ? saveUndo( HASH_cmUndoMoveBlock ) : 0;
      if ( u )
	{
	  u->longVal = b->top;
	  u->column = b->left;
	}
      //---------------------------------------------
      blockBuf.freeAll();
      copyToBuf( blockBuf, b );
      deleteBlock( 0, NO_UNDO );
      int insLines = copyFromBuf( line, column, blockBuf, b->right - b->left + 1, NO_UNDO, 1 );
      if ( u )
	 u->sym1 = insLines;
      blockBuf.freeAll();
    }
   if ( !flagUndo )
      appl->drawStatus();
}

void Edit::deleteBlock( int flagCorrectCursor, int flagUndo, undoItem *ui )
{
   if ( !FLAG_LINE && !FLAG_BLOCKLINE && !FLAG_COL && !FLAG_BLOCKCOL )
      return;
   appl->statusLine->draw( lang(" Block deleting ..."," Удаление блока ..."), 1 );
   Screen::sync();
   blockInfo *b = validBlock();
   char *s=0, *ss=0;
   int len;
   //----------------------------------------------------
   Collection *coll = flagUndo || (ui && !ui->ptr) ? new Collection( 50, 50 ) : 0;
   undoItem *u = flagUndo ? saveUndo( HASH_cmUndoDelLineBlock ) : 0;
   //----------------------------------------------------
   if ( FLAG_LINE || FLAG_BLOCKLINE )
    {
      for( long i=b->top, bott=b->bott; i<=bott; bott-- )
       {
	 strInfo *si = STRINFO( i );
	 //-------------------------------------------------
	 if ( coll )
	  {
	    s = new char[si->len+1];
	    ss = getStr(si,len);
	    memcpy( s, ss, si->len ); s[si->len] = 0;
	    coll->insert( s );
	  }
	 //-------------------------------------------------
	 if ( !si->map )
	    ::free( si->str );
	 atext->atRemove( i );
       }
      setChange( 1 );
      setWinIdent();
    }
   else if ( FLAG_COL || FLAG_BLOCKCOL )
    {
      //-------------------------------------------------
      if ( u )
	 u->command = HASH_cmUndoDelColumnBlock;
      //-------------------------------------------------
      for( long i=b->top; i<=b->bott; i++ )
       {
	 strInfo *si = STRINFO( i );
	 //-------------------------------------------------
	 if ( coll )
	  {
	    s = new char[si->len+1];
	    ss = getStr(si,len);
	    memcpy( s, ss, si->len ); s[si->len] = 0;
	    coll->insert( s );
	  }
	 //-------------------------------------------------
	 s = getStr( si, len );
	 int left = realPos( si, b->left ),
	     right= realPos( si, b->right ),
	     j;
	 for( j=0; j < len && j < left; j++ )
	    Buf1[j] = s[j];
	 if ( s[j] == '\t' )
	    for( int k = b->left & stepTab; k>0; k-- )
	       Buf1[j++] = ' ';
	 for( int k = right + 1; k < len; k++ )
	    Buf1[j++] = s[k];
	 replaceStr( i, Buf1, j );
       }
      if ( flagCorrectCursor )
       {
	 if ( delta.x > b->left )
	    delta.x = b->left;
	 cursor.x = b->left - delta.x;
       }
     }
   if ( flagCorrectCursor )
     {
       if ( delta.y > b->top )
	  delta.y = b->top;
       cursor.y = b->top - delta.y;
     }
   if ( u )
      u->ptr = coll;
   else if ( ui && !ui->ptr )
      ui->ptr = coll;
   appl->drawStatus();
   unmarkBlock( NO_UNDO );
}

void Edit::copyToClip( int freeBefore )
{
   if ( externCollection == &clipBoard || !FLAG_LINE && !FLAG_BLOCKLINE && !FLAG_COL && !FLAG_BLOCKCOL )
      return;
   appl->statusLine->draw( lang(" Block copying to Clipboard ..."," Копирование блока в буфер обмена ..."), 1 );
   Screen::sync();
   if ( freeBefore )
       clipBoard.freeAll();
   blockInfo *b = validBlock();
   if ( clipBoard.getCount() > 0 )
      clipBoard.atFree( 0 );
   if ( FLAG_LINE || FLAG_BLOCKLINE )
     {
       for( long i=b->top; i <= b->bott; i++ )
	{
	  strInfo *si = STRINFO( i );
	  int len;
	  char *str = getStr( si, len );
	  char *s = new char[len+1];
	  memcpy( s, str, len );
	  s[len]=0;
	  clipBoard.insert( s );
	}
       short *type = new short(0);
       clipBoard.atInsert( 0, type );
     }
   else if ( FLAG_COL || FLAG_BLOCKCOL )
     {
       copyToBuf( clipBoard, b );
       short *type = new short( b->right - b->left + 1 );
       clipBoard.atInsert( 0, type );
     }
   appl->drawStatus();
}

void Edit::moveToClip( int freeBefore )
{
   if ( externCollection == &clipBoard )
      return;
   copyToClip( freeBefore );
   deleteBlock();
}

void Edit::copyFromClip( Collection *clip )
{
   if ( !clip )
      clip = &clipBoard;
   if ( externCollection == &clipBoard || clip->getCount() <= 0 )
      return;
   long line = LINE, i, bott;
   if ( line >= BOTT )
      addStrings( UNDO );
   short *len = (short*)clip->at(0);
   clip->atRemove( 0 );
   appl->statusLine->draw( lang(" Block copying from Clipboard ..."," Копирование блока из буфера обмена"), 1 );
   Screen::sync();
   if ( !*len )				// line block
     {
       for( i=0, bott=clip->getCount(); i<bott; i++ )
	{
	  char *s = (char*)clip->at( i );
	  insertStr( line++, s, strlen( s ) );
	}
       saveUndo( HASH_cmUndoInsertBlock );
       unmarkBlock( NO_UNDO );
       block.bott = line-1;
       block.top = block.bott - i + 1;
       block.left = 0;
       block.right = SHRT_MAX;
       SET_BLOCKLINE;
     }
   else
       copyFromBuf( LINE, COLUMN, *clip, *len );
   clip->atInsert( 0, len );
   appl->drawStatus();
}

/*-------------------------------- UNDO-REDO -------------------------*/

void undoItem::freeData()
{
   sym1 = sym2 = 0;
   cycl = 1;
   if ( ptr ) {
       switch( command ) {
	   case HASH_cmUndoGlue:
	   case HASH_cmUndoDelLineBlock:
	   case HASH_cmUndoDelColumnBlock:
	   case HASH_cmUndoReplace:
	   case HASH_cmUndoTruncate:
	   case HASH_cmUndoFormat:
	       delete (Collection*)ptr;
	       break;
	   default:
	       delete ptr;
       }
      ptr=0;
   }
   command=0;
}

void Edit::undo_redo( int type )
{
   long i;
   Collection *coll;
   char *str;
   for( int Exit=0; !Exit; )
    {
      if ( type==UNDO && uInfo->count<1 || type==REDO && uInfo->count>=uInfo->fullCount )
	 return;
      Exit = 1;
      if ( type == REDO )
	{
	  uInfo->cur = uInfo->cur < uInfo->max ? uInfo->cur + 1 : 0;
	  uInfo->count++;
	}
      undoItem *u = &Undo[uInfo->cur];

      switch( u->command )
       {
	 case HASH_cmUndoHome:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveHome( NO_UNDO );
	     break;
	 case HASH_cmUndoEnd:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveEnd( NO_UNDO );
	     break;
	 case HASH_cmUndoUp:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveUp( NO_UNDO );
	     break;
	 case HASH_cmUndoDown:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveDown( NO_UNDO );
	     break;
	 case HASH_cmUndoLeft:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveLeft( NO_UNDO );
	     break;
	 case HASH_cmUndoRight:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveRight( NO_UNDO );
	     break;
	 case HASH_cmUndoTop:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveTop( NO_UNDO );
	     break;
	 case HASH_cmUndoBott:
	     if ( type == REDO )
	       for( i=0; i < u->cycl; i++ )
		  moveBott( NO_UNDO );
	     break;
	 case HASH_cmUndoGoto:
	 case HASH_cmUndoFind:
	     if ( type == REDO )
		restoreCursor( u );
	     break;
	 case HASH_cmUndoChar:
	     for( i=0; i < u->cycl; i++ )
	       if ( type == UNDO )
		 {
		   if ( !u->sym2 || u->_flags & 0x020 )
		      backspace( NO_UNDO );
		   else
		    {
		      moveLeft( NO_UNDO );
		      insertChar( u->sym2, 0, NO_UNDO );
		      moveLeft( NO_UNDO );
		    }
		 }
	       else
		   insertChar( u->sym1, u->_flags & 0x020, NO_UNDO );
	     break;
	 case HASH_cmUndoBackspace:
	     for( i=0; i < u->cycl; i++ )
	       if ( type == UNDO )
		  insertChar( u->sym1, 1, NO_UNDO );
	       else
		  backspace( NO_UNDO );
	     break;
	 case HASH_cmUndoDelete:
	     for( i=0; i < u->cycl; i++ )
	       if ( type == UNDO )
		 {
		   insertChar( u->sym1, 1, NO_UNDO );
		   moveLeft( NO_UNDO );
		 }
	       else
		   deleteChar( NO_UNDO );
	     break;
	 case HASH_cmUndoDeleteLine:
	     if ( type == UNDO )
		insertStr( u->dY+u->cY, (char*)u->ptr, strlen((char*)u->ptr) );
	     else
		deleteLine( LINE, NO_UNDO );
	     break;
	 case HASH_cmUndoEnter:
	     if ( type == UNDO )
	       {
		 i = u->dY + u->cY;
		 atext->atRemove( i );
		 replaceStr( i, (char*)u->ptr, strlen((char*)u->ptr) );
	       }
	     else
		 enter( NO_UNDO );
	     break;
	 case HASH_cmUndoGlue:
	     coll = (Collection*)u->ptr;
	     if ( type == UNDO )
	       {
		 i = u->dY + u->cY - u->sym1;
		 str = (char*)coll->at( 1 );
		 replaceStr( i, str, strlen(str) );
		 str = (char*)coll->at( 0 );
		 insertStr( i, str, strlen(str) );
	       }
	     else
	       {
		 glueStrings( u->longVal, u->column, NO_UNDO );
		 if ( u->sym1 > 0 )
		  {
		    long *l = (long*)coll->at( 2 );
		    delta.y = l[0]; delta.x = l[1];
		    cursor.y = l[2]; cursor.x = l[3];
		  }
	       }
	     break;
	 case HASH_cmUndoReplace:
	 case HASH_cmUndoTruncate:
	     coll = (Collection*)u->ptr;
	     if ( type == UNDO )
		str = (char*)coll->at( 0 );
	     else
		str = (char*)coll->at( 1 );
	     replaceStr( u->dY + u->cY, str, strlen(str) );
	     if ( u->command == HASH_cmUndoReplace && u->sym1 )
	      {
		Exit = 0;
		if ( type == REDO )
		  {
		    restoreCursor( u );
		    uInfo->cur = uInfo->cur < uInfo->max ? uInfo->cur + 1 : 0;
		  }
	      }
	     break;
	 case HASH_cmUndoAddStrings:
	     if ( type == UNDO ) {
		for( i=u->longVal; i>0; i-- )
		    atext->atRemove( BOTT-1 );
	     } else {
		addStrings( NO_UNDO );
	     }
	     break;
	 case HASH_cmUndoDelLineBlock:
	     if ( type == UNDO )
	       {
		 coll = (Collection*)u->ptr;
		 for( i=coll->getCount()-1; i>=0; i-- )
		  {
		    str = (char*)coll->at( i );
		    insertStr( u->bi.top, str, strlen(str) );
		  }
	       }
	     else
		deleteBlock( 1, NO_UNDO );
	     break;
	 case HASH_cmUndoDelColumnBlock:
	     if ( type == UNDO )
	       {
		 coll = (Collection*)u->ptr;
		 for( i=0; i<coll->getCount(); i++ )
		  {
		    str = (char*)coll->at( i );
		    replaceStr( u->bi.top + i, str, strlen(str) );
		  }
	       }
	     else
		deleteBlock( 1, NO_UNDO );
	     break;
	 case HASH_cmUndoCopyBlock:
	     if ( type == UNDO ) {
		 deleteBlock( 0, NO_UNDO );
		 for( i=u->sym1; i>0; i-- )
		     atext->atRemove( BOTT-1 );
	     } else {
		copyBlock( NO_UNDO );
	     }
	     break;
	 case HASH_cmUndoInsertBlock:
	     if ( type == UNDO ) {
		 u->bi = block;
		 u->_flags = _flags;
		 deleteBlock( 0, NO_UNDO, u );
		 for( i=u->sym1; i>0; i-- )
		     atext->atRemove( BOTT-1 );
	     } else {
		 coll = (Collection*)u->ptr;
		 for( i=coll->getCount()-1; i>=0; i-- ) {
		    str = (char*)coll->at( i );
		    insertStr( u->bi.top, str, strlen(str) );
		 }
		 restoreCursor( u );
	     }
	     break;
	 case HASH_cmUndoMoveBlock:
	     if ( type == UNDO ) {
		 moveBlock( u->longVal, u->column, NO_UNDO );
		 for( i=u->sym1; i>0; i-- )
		     atext->atRemove( BOTT-1 );
	     } else {
		 moveBlock( LINE, COLUMN, NO_UNDO );
	     }
	     break;
	 case HASH_cmUndoMarkBlock:
	     if ( type == REDO )
		 markBlock( u->sym1, NO_UNDO );
	     break;
	 case HASH_cmUndoUnmarkBlock:
	     u->cycl = 1;
	     if ( type == REDO )
		 unmarkBlock( NO_UNDO );
	     break;
	 case HASH_cmUndoMatch:
	     if ( type == REDO )
		 findMatch( u->longVal, NO_UNDO );
	     break;
	 case HASH_cmUndoFormat:
	     if ( type == REDO ) {
		 formatParagraph( LINE, NO_UNDO, 1, 1 );
	     } else {
		for( i=u->longVal; i > 0; i-- )
		    atext->atRemove( u->dY + u->cY );
		coll = (Collection*)u->ptr;
		for( i=coll->getCount()-1; i >= 0; i-- ) {
		   str = (char*)coll->at(i);
		   insertStr( u->dY + u->cY, str, strlen(str) );
		}
	     }
	     break;
       }
      if ( type == UNDO ) {
	 restoreCursor( u );
	 uInfo->cur = uInfo->cur > 0 ? uInfo->cur - 1 : uInfo->max - 1;
	 uInfo->count--;
      }
      if ( uInfo->count > 0 ) {
	  if ( !Exit && Undo[uInfo->cur].command != u->command )
	     Exit = 1;
	  else if ( Undo[uInfo->cur].command == HASH_cmUndoAddStrings )
	     Exit = 0;
      }
    }
   draw();
   Screen::sync();
}

undoItem *Edit::undoOptimize( long command )
{
   return ( uInfo->count > 0 && command == Undo[uInfo->cur].command &&
	UNDO_OPTIMIZE(eOpt.options) ) ? &Undo[uInfo->cur] : 0;
}

undoItem *Edit::saveUndo( long command, int canOptimize )
{
   undoItem *u=0;
   if ( canOptimize && (u=undoOptimize( command )) )
     {
       u->cycl++;
       return 0;
     }
   incUndo();
   u = &Undo[uInfo->cur];
   u->command = command;
   u->dY = delta.y;
   u->_flags = _flags;
   u->dX = delta.x;
   u->cY = cursor.y;
   u->cX = cursor.x;
   u->bi = block;
   return u;
}

void Edit::restoreCursor( undoItem *u )
{
   delta.y	= u->dY;
   int i = FLAG_CHANGE;
   _flags	= u->_flags;
   switch( u->command )
    {
      case HASH_cmUndoHome:
      case HASH_cmUndoEnd:
      case HASH_cmUndoUp:
      case HASH_cmUndoDown:
      case HASH_cmUndoLeft:
      case HASH_cmUndoRight:
      case HASH_cmUndoTop:
      case HASH_cmUndoBott:
      case HASH_cmUndoGoto:
      case HASH_cmUndoFind:
	  if ( i == FLAG_CHANGE )
	     break;
	  if ( FLAG_CHANGE )
	     CLEAR_CHANGE;
	  else
	     SET_CHANGE;
	  break;
      default:
	  if ( *saved )
	     SET_CHANGE;
    }
   delta.x	= u->dX;
   cursor.y	= u->cY;
   cursor.x	= u->cX;
   block	= u->bi;
   setChange( FLAG_CHANGE );
   setWinIdent();
}

void Edit::incUndo()
{
   uInfo->cur = uInfo->cur + 1 < uInfo->max ? uInfo->cur + 1 : 0;
   if ( uInfo->count + 1 > uInfo->max )
      uInfo->start = uInfo->start + 1 < uInfo->max ? uInfo->start + 1 : 0;
   else
      uInfo->count++;
   Undo[uInfo->cur].freeData();
   uInfo->fullCount = uInfo->count;
}

void Edit::truncateStr()
{
   long line = LINE;
   if ( line >= BOTT )
      return;
   strInfo *si = STRINFO( line );
   int len, sPos = COLUMN;
   int rPos = realPos( si, sPos );
   char *s = getStr( si, len );
   if ( rPos < 0 || rPos >= len )
      return;
   //---------------------------------------------------
   char *str = new char[len+1];
   memcpy( str, s, len ); str[len]=0;
   undoItem *u = saveUndo( HASH_cmUndoTruncate );
   Collection *coll = new Collection( 2, 0 );
   coll->insert( str );
   u->ptr = coll;
   //---------------------------------------------------
   memcpy( Buf1, s, rPos );
   int spaces = s[rPos] == '\t' ? sPos - scrPos( si, rPos-1 ) - 1 : 0;
   if ( spaces )
      memset( Buf1+rPos, ' ', spaces );
   replaceStr( line, Buf1, rPos+spaces );
   //---------------------------------------------------
   str = new char[rPos+spaces+1];
   memcpy( str, Buf1, rPos+spaces );
   str[rPos+spaces] = 0;
   coll->insert(str);
   //---------------------------------------------------
}

//----------------------- форматирование ---------------------------------

int Edit::compress( char *src, char *dst, int pos, int len, int tab )
{
   char *s1=dst, *s2=src, *end=0;
   int rp1=0, rp2=0, vp1=0, vp2=0, off=0, t=0;
   for( end=src+strlen(src); end>src && (*(end-1)==' ' || *(end-1)=='\t'); end-- );
   for( ; s2+rp2 < end && (s2[rp2]==' ' || s2[rp2]=='\t'); rp2++, vp2++ )
    {
      if ( (s1[rp2] = s2[rp2]) != '\t' )
	 continue;
      vp2 += tab - (vp2 & tab);
    }
   for( rp1=rp2, vp1=vp2; s2+rp2 < end; rp2++, vp2++ )
    {
      if ( vp1 >= len || rp2 >= pos )
       {
	 end = s2 + rp2;
	 break;
       }
      switch( s2[rp2] )
       {
	 case ' ':
	     if ( s1[rp1-1] == ' ' )
		{ off++; break; }
	     goto m1;
	 case '\t':
	     t = tab - (vp2 & tab);
	     off += t + 1;
	     vp2 += t;
	     if ( s1[rp1-1] == ' ' )
		break;
	     off--;
m1:
	     s1[rp1++] = ' '; vp1++;
	     break;
	 default:
	     s1[rp1++] = s2[rp2]; vp1++;
       }
    }
   strcpy( s1 + rp1, end );
   return -off;
}

char *Edit::uncompress( char *s, int len )
{
   static char buf[maxLineLength];
   int i=0, j, k, words=0, l=strlen( s );
   for( ; l>0 && s[l-1]==' ' || s[l-1]=='\t'; l-- );
   s[l]=0;
   if ( !l || l >= len )
      return 0;
   for( ; (s[i]==' ' || s[i]=='\t') && i <= l; i++ )
      buf[i] = s[i];
   for( j=i; j <= l; j++ )
      if ( s[j]==' ' )
	 words++;
   if ( !words )
      return 0;

   int Space = (len-l)/words+1, Sp = (len-l)%words;
   for( j=0, k=i; j<Sp; j++ )
    {
	while ( s[i] != ' ' && i<=l )
		buf[k++]=s[i++];
	i++;
	for( int m=0; m<Space+1; m++ )
		buf[k++]=' ';
    }
   for( ; i<=l; i++ )
    {
	while( s[i] != ' ' && i<=l )
	   buf[k++]=s[i++];
	if ( i>l )
	   break;
	for( int m=0; m<Space; m++ )
	   buf[k++]=' ';
    }
   return buf;
}

int Edit::format( char *str, int vpos, char *&s1, char *&s2, int smartFlag )
{
   static char buf1[maxLineLength];
   static char buf2[maxLineLength];
   s1 = s2 = 0;
   int rp1=0, rp2=0, vp1=0, vp2=0, l=eOpt.lineLength;
   for( ; vp1 < l && (str[rp1] == ' ' || str[rp1] == '\t'); rp1++, vp1++ )
    {
      buf1[rp1] = buf2[rp1] = str[rp1];
      if ( str[rp1] == '\t' )
	 vp1 += stepTab - (vp1 & stepTab);
    }
   if ( vp1 >= l )
      return 0;
   if ( vp1 >= eOpt.parOffset && smartFlag )
      { vp2 = rp2 = 0; }
   else
      { rp2=rp1; vp2=vp1; }

   char *sk1=",.:;?!)>]}\\%+-*/",
	*glas="аеиоуыэюяАЕИОУЫЭЮЯeyuioaEYUIOA",
	*sogl="бвгджзклмнпрстфхцчшщБВГДЖЗКЛМНПРСТФХЦЧШЩqwrtpsdfghjklzxcvbnmQWRTPSDFGHJKLZXCVBNM";

   for( ; str[rp1] && (vp1<l || !strchr(wordDelim,str[rp1])); rp1++, vp1++ )
    {
      if ( vpos == vp1 )
	 return 0;
      buf1[rp1] = str[rp1];
      if ( str[rp1] == '\t' )
	 vp1 += stepTab - (vp1 & stepTab);
    }
   if ( strchr( sk1, str[rp1] ) )
     { buf1[rp1] = str[rp1]; rp1++; vp1++; }

   //----------------------------------------------------------
   int __c=0, _c=0, c=0, c_=0, c__=0, Rp1=rp1, Vp1=vp1;
   rp1 -= vp1-l+1;
   vp1 = l-1;
   if ( strchr( wordDelim, str[rp1+1] ) && !strchr( sk1, str[rp1+1] ) )
      { buf1[++rp1]=0; vp1++; goto wrap; }
   for( ; vp1 > vp2 && !strchr( wordDelim, (c=buf1[rp1]) ); rp1--, vp1-- )
    {
      if ( !WRAP_IN_WORD(eOpt.options) || strchr(wordDelim,(__c=buf1[rp1-2]))
	  || strchr(wordDelim,(_c=buf1[rp1-1])) || strchr(wordDelim,(c_=buf1[rp1+1]))
	  || strchr( "ьъЬЪ", c ) )
	 continue;
      if ( strchr( glas, c ) )
	{
	  if ( strchr( glas, _c ) )
	    { buf1[rp1]='-'; buf1[rp1+1]=0; goto wrap; }
	}
      else
	{
	  c__=buf1[rp1+2];
	  if ( strchr(glas,_c) && strchr(glas,c_) || strchr(glas,__c) && (strchr(glas,c_) || strchr(glas,c__)) )
	    { buf1[rp1]='-'; buf1[rp1+1]=0; goto wrap; }
	}
    }
   if ( vp2 && vp1 <= vp2 || !vp2 && vp1 <= eOpt.parOffset )
      { rp1=Rp1; vp1=Vp1; }
   else
    {
      if ( buf1[rp1] == ' ' )
	{ rp1++; vp1++; }
      else if ( strchr( sk1, str[rp1] ) )
	{ buf1[rp1] = str[rp1]; rp1++; vp1++; }
    }
   //----------------------------------------------------------
   buf1[rp1] = 0;

wrap:
   int r=vpos-vp1+vp2+1;
   for( ; r > vp2 && (str[rp1]==' ' || str[rp1]=='\t'); rp1++, r-- );
   strcpy( buf2+rp2, str+rp1 );

   if ( EXPAND_STRING(eOpt.options) )
    {
      char *s = uncompress( buf1, eOpt.lineLength );
      if ( s )
	 strcpy( buf1, s );
    }
   s1 = buf1;
   s2 = buf2;

   return r;
}

long Edit::formatParagraph( long startLine, int flagUndo, int skip, int smartFlag )
{
   static unsigned char strGraph[] = {
	130, 136, 131, 134, 138, 135, 132, 137, 133,
	164, 183, 167, 176, 189, 180, 170, 186, 173,
	162, 182, 166, 175, 188, 178, 169, 185, 172,
	165, 184, 168, 177, 190, 181, 171, 187, 174,
	129, 128, 161, 160, 0 };
   long line=startLine;
   if ( startLine < 0 || startLine >= BOTT )
      return -1;
   strInfo *si=0;
   char *str=0, *s=new char[2 * maxLineLength], *s1=0, *s2=0;
   int off1=maxLineLength, off2=0, i=0, l=0, len;

   Collection *coll = flagUndo==UNDO ? new Collection(10,10) : 0;

   for( ; line < BOTT; line++ )
    {
      si = STRINFO( line );
      str = getStr( si, len );
      for( ; str[len-1]==' ' || str[len-1]=='\t'; len-- );
      if ( len < 1 )
       {
	 if ( line == startLine && skip )
	  {
	    if ( coll )
	      {
		char *ch = new char[1];
		*ch = 0;
		coll->insert( ch );
	      }
	    continue;
	  }
	 break;
       }
      for( i=0, off2=0; i<len && (str[i] == ' ' || str[i] == '\t'); i++, off2++ )
	if ( str[i] == '\t' )
	   off2 += stepTab - (off2 & stepTab);
      if ( off2 > off1 )
	 break;
      off1 = off2;

      if ( l > 1 )
	{
	  if ( s[l-1]=='-' && !strchr( wordDelim, s[l-2] ) )
	     l--;
	  else
	     s[l++]=' ';
	  memcpy( s+l, str+i, len-i );
	  l -= i;
	}
      else
	  memcpy( s+l, str, len );
      l += len;
      s[l] = 0;

      if ( strpbrk( (char*)strGraph, s ) )
	 { l=0; break; }

      //-------------------------------------------------
      if ( coll )
	{
	  char *ch = new char[si->len+1];
	  memcpy( ch, str, si->len ); ch[si->len] = 0;
	  coll->insert( ch );
	}
      //-------------------------------------------------

      if ( !si->map )
	 ::free( si->str );
      atext->atRemove( line );
m1:
      compress( s, Buf1, maxLineLength, eOpt.lineLength, stepTab );
      strcpy( s, Buf1 );
      l = strlen( s );
      if ( l <= eOpt.lineLength )
	{ line--; continue; }
      format( s, maxLineLength, s1, s2, smartFlag );
      l = strlen( s1 );
      insertStr( line, s1, l );
      l = strlen( s2 );
      memcpy( s, s2, l+1 );

      if ( l > eOpt.lineLength )
	{ line++; goto m1; }
    }
   if ( line > startLine || l > 0 )
      insertStr( line++, s, l );
   delete s;
   if ( coll && line > startLine )
     {
       undoItem *u = saveUndo( HASH_cmUndoFormat );
       u->ptr = coll;
       u->longVal = line - startLine;
     }
   else
       delete coll;

   if ( line >= delta.y + size.y - 2 )
     {
       cursor.y=0;
       delta.y=line;
     }
   cursor.y = line - delta.y;

   return line;
}

void Edit::formatText()
{
  delta.y = cursor.y = 0;
  for( long l, line = 0; line < BOTT; )
     line = formatParagraph( line, UNDO, 1, 1 );
}

void Edit::showManMode( int warning )
{
   if ( FLAG_BINARY )
      return;
   if ( FLAG_MAN )
      CLEAR_MAN;
   else
     {
       if ( FLAG_CHANGE )
	 {
	   if ( warning )
	     test( lang("There is a change in the text.\nSave or undo, please.",
			"В тексте есть изменения.\nСохраните или сделайте откат.") );
	 }
       else
	 { SET_MAN; if ( readOnlyMode == RDONLY_0 ) readOnlyMode=RDONLY_1; }
     }
}

void Edit::changeMode( int warning )
{
   if ( readOnlyMode > 2 )
      return;
   if ( FLAG_CHANGE )
     {
       if ( warning )
	  test( lang("There is a change in the text.\nSave or undo, please.",
		     "В тексте есть изменения.\nСохраните или сделайте откат.") );
     }
   else if ( !FLAG_MAN )
      readOnlyMode = (readOnlyMode == RDONLY_0) ?
		(FLAG_BINARY ? RDONLY_2 : RDONLY_1) : RDONLY_0;
   setWinIdent();
}

blockInfo *Edit::validBlock()
{
   blockInfo *b = Window::validBlock();
   if ( b->bott >= BOTT )
      b->bott = BOTT-1;
   return b;
}
