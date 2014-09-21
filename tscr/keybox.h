/*
	$Id: keybox.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _KEYBOX_H_
#define _KEYBOX_H_

#include "dialog.h"

class Menu;
class KeyHolder;

class KeyBox : public Dialog
{
protected:
   static char Buf[128];
   KeyHolder *kh;
   Menu *menu;
   int **identMas;
   int lenmas;
   int *cur;
   long **mas;
   listBox **lb;
   void newKey( long key );
   void fill();
public:
   KeyBox( KeyHolder *Kh, Menu *m );
   ~KeyBox();
   int flagNewKey;
   long handleKey( long key, void *&ptr );
};


#endif
