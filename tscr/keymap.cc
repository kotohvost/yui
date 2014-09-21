/*
	$Id: keymap.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#include <stdio.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "listbox.h"
#include "keybox.h"
#include "program.h"
#include "menu.h"
#include "hashcode.h"


Lang strstat( "Press ~Enter~ for key change",
	      "Нажмите ~Enter~ для изменения клавиши" );
Lang strkey( " Press key for this command",
	     " Нажмите клавишу для этой команды" );

char KeyBox::Buf[128];

int qsort_compare( const void *key1, const void *key2 )
{
   long k1=*(long*)key1;
   long k2=*(long*)key2;
   return k1<k2 ? -1 : ( k1>k2 ? 1:0 );
}

int qsort_compare2( const void *key1, const void *key2 )
{
   long k1=*((long*)key1 + 1);
   long k2=*((long*)key2 + 1);
   return k1<k2 ? 1 : ( k1>k2 ? -1:0 );
}

/*-------------------------- class Keymap ---------------------------*/

Keymap::Keymap( long *m, Lang *mIdent ) :
	mas(0), mas2(0), len(0), size(0), mapIdent(mIdent)
{
   if ( m )
      init( m );
}

Keymap::~Keymap()
{
   if ( mas ) delete mas;
   if ( mas2 ) delete mas2;
   if ( mapIdent ) delete mapIdent;
}

void Keymap::init( long *m )
{
   for( len=0; m[len]; len++ );
   if ( mas )
      delete mas;
   mas = new long[len];
   memcpy( mas, m, len * sizeof(long) );
   size = sizeof(long) * 2;
   qsort( mas, len/2, size, qsort_compare );
   if ( mas2 )
      delete mas2;
   mas2 = new long[len];
   memcpy( mas2, m, len * sizeof(long) );
   len /= 2;
   qsort( mas2, len, size, qsort_compare2 );
}

long Keymap::translate( long key )
{
   long k = key;
   long *cm = (long*)bsearch( &k, mas, len, size, qsort_compare );
   if ( !cm )
      return key;
   return *(cm+1);
}

long Keymap::getKey( long command )
{
   long com[2];
   com[1] = command;
   long *cm = (long*)bsearch( &com, mas2, len, size, qsort_compare2 );
   if ( !cm )
      return command;
   return *cm;
}

/*------------------------- class KeyHolder ---------------------------*/

KeyHolder::KeyHolder() : len(0)
{
   kmas = new Keymap *[1];
   kmas[0] = 0;
   descr = new CommandDescr *[1];
   descr[0] = 0;
   ident = new char *[1];
   ident[0] = 0;
}

KeyHolder::~KeyHolder()
{
   delete kmas;
   delete descr;
   delete ident;
}

int KeyHolder::add( Keymap *km, CommandDescr *cd )
{
   if ( !km )
      return 0;
   Keymap **newkmas = new Keymap *[len+1];
   CommandDescr **newdescr = new CommandDescr *[len+1];
   char **newident = new char *[len+1];
   for( int i=len++; i>0; i-- )
    {
      newkmas[i] = kmas[i-1];
      newdescr[i] = descr[i-1];
      newident[i] = ident[i-1];
    }
   newkmas[0] = km;
   newdescr[0] = cd;
   newident[0] = km->mapIdent ? km->mapIdent->get() : 0;
   delete kmas;
   delete descr;
   delete ident;
   kmas = newkmas;
   descr = newdescr;
   ident = newident;
   return 1;
}

int KeyHolder::del( Keymap *km )
{
   for( int i=0; i<len; i++ )
     if ( kmas[i] == km )
       {
	 for( ; i<len; i++ )
	   {
	     descr[i] = descr[i+1];
	     kmas[i] = kmas[i+1];
	     ident[i] = ident[i+1];
	   }
	 len--;
       }
   return 1;
}

Keymap *KeyHolder::item( int index )
{
  if ( index < 0 || index >= len )
     return 0;
  return kmas[index];
}

/*
long KeyHolder::changeKeymap( Collection *wins, Menu *menu )
{
   KeyBox *kb = new KeyBox( this, menu );
   long ret = kb->exec( wins );
   delete kb;
   return ret;
}
*/

void KeyHolder::changeDescr( Keymap *km, CommandDescr *cd )
{
   for( int i=0; i<len; i++ )
      if ( kmas[i] == km )
	 { descr[i] = cd; return; }
}

long KeyHolder::translate( long key )
{
   long ret;
   for( int i=0; i<len; i++ )
     if ( key != (ret=(kmas[i])->translate(key)) )
	return ret;
   return key;
}

/*------------------------ class KeyBox ---------------------------*/

KeyBox::KeyBox( KeyHolder *Kh, Menu *m ) :
	Dialog( scrRect(), new Lang("Keyboard","Клавиатура") ),
	kh(Kh), menu(m), flagNewKey(0)
{
   setHelpContext( "KeyMap" );
   redrawAfterExec = 0;
   cur = new int[kh->len];
   mas = new long *[kh->len];
   identMas = new int *[kh->len];
   lb = new listBox *[kh->len];
   int i, kmasCount;
   for( i=0, kmasCount=0; i < kh->len; i++ )
    {
      if ( kh->descr[i] )
	 kmasCount++;
      lb[i] = 0;
    }
   int Y=1, lenY = (Screen::Lines-5) / kmasCount - kmasCount;
   for( i=0; i<kh->len; i++ )
    {
      mas[i] = new long[ 2 * kh->kmas[i]->len + 1 ];
      identMas[i] = new int[ kh->kmas[i]->len ];
      if ( !kh->descr[i] )
	 continue;
      memcpy( mas[i], kh->kmas[i]->mas2, kh->kmas[i]->len * kh->kmas[i]->size );
      mas[i][ 2 * kh->kmas[i]->len ]=0;
      put( Y-1, 2, kh->ident[i] );
      if ( kh->len > 1 && i+1==kh->len && Y+lenY >= rect.b.y - rect.a.y - 3 )
	 lenY++;
      lb[i] = new listBox( Rect( Y, 2, Y+lenY, 63 ), 64, &cur[i] );
      Y+=lenY+2;
      insert( lb[i] );
    }

   insert( new button( Point(5,68),  lang("   OK   ","    OK     "), HASH_cmOK ) );
   insert( new button( Point(10,68), lang("  Save  "," Сохранить "), HASH_cmSaveKeymap ) );
   insert( new button( Point(15,68), lang(" Cancel ","   Выход   "), HASH_cmCancel ) );

   status->put( strstat.get(), language );
   fill();
}

KeyBox::~KeyBox()
{
   for( int i=0; i<kh->len; i++ )
    {
      delete identMas[i];
      delete mas[i];
    }
   delete identMas;
   delete mas;
   delete cur;
   delete lb;
   if ( appl->keyBox == this )
      appl->keyBox = 0;
}

void KeyBox::fill()
{
   for( int i=0; i<kh->len; i++ )
     {
       if ( lb[i] )
	  lb[i]->freeAll();
       if ( !kh->descr[i] )
	  continue;
       for( int j=0; kh->descr[i][j].command; j++ )
	{
	  CommandDescr *cd = &kh->descr[i][j];
	  for( int k=0; k < kh->kmas[i]->len; k++ )
	   {
	     if ( mas[i][k*2+1] != cd->command )
		continue;
	     sprintf( Buf, " %s ............ ", _descriptKey( mas[i][k*2] ) );
	     Buf[14] = ' ';
	     char *ch = cd->descr->get();
	     if ( ch )
		strcpy( Buf+15, ch );
	     else
		Buf[15] = 0;
	     identMas[i][ lb[i]->add( Buf ) ] = k;
	   }
	}
     }
}

long KeyBox::handleKey( long key, void *&ptr )
{
   if ( flagNewKey )
     {
       newKey( key );
       return 0;
     }
   key = keyHolder.translate( key );
   switch( key )
     {
       case HASH_cmOK:
	   if ( ((getobj*)items[current])->type() == TYPE_LIST )
	     {
	       status->put( strkey.get(), language );
	       drawStatus();
	       Screen::sync();
	       flagNewKey=1; key=0;
	     }
	   break;
     }
   key = Dialog::handleKey( key, ptr );
   switch( key )
     {
       case HASH_cmOK:
       case HASH_cmSaveKeymap:
	   for( int i=0; i < kh->len; i++ )
	     if ( kh->descr[i] )
	       kh->kmas[i]->init( mas[i] );
	   if ( menu )
	      menu->fill( kh );
     }
   if ( key == HASH_cmSaveKeymap )
      Task::sendMessage( appl, new BackMessage( key ) );
   return key;
}

void KeyBox::newKey( long key )
{
   lb[current]->accept();
   mas[current][ 2 * identMas[current][ cur[current] ] ] = key;
   fill();
   flagNewKey=0;
   status->put( strstat.get(), language );
   draw();
   drawStatus();
   Screen::sync();
};
