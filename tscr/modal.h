/*
	$Id: modal.h,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/
#ifndef _MODAL_H
#define _MODAL_H

#include "dialog.h"

extern long modalKeyMap[];
extern Keymap ModalKeyMap;

#define ALIGN_CENTER	-1

class modal : public Dialog
{
protected:
   int align;
   void fill( Collection *Text, char *Title, int cm1, char *s1,
          int cm2, char *s2, int cm3, char *s3,
          int cm4, char *s4, int cm5, char *s5 );
public:
   virtual int 	isType(long typ);
   /* Align == -1 --> центрирование, иначе величина отступа слева */
   modal( int Align, Collection *Text, char *Title=0, int cm1=0, char *s1=0,
          int cm2=0, char *s2=0, int cm3=0, char *s3=0,
          int cm4=0, char *s4=0, int cm5=0, char *s5=0 );
   modal( int Align, char *str, char *Title=0, int cm1=0, char *s1=0,
          int cm2=0, char *s2=0, int cm3=0, char *s3=0,
          int cm4=0, char *s4=0, int cm5=0, char *s5=0 );
};

#endif
