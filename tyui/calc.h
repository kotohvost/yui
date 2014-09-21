/*
	$Id: calc.h,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#ifndef _CALC_H_
#define _CALC_H_

#include <dialog.h>

class inputLine;

#define BIN	2
#define OCT	8
#define	DEC	10
#define HEX	16

class Calculator : public Dialog
{
protected:
	inputLine *il;
	double x, y, m;
	char str[128];
	long prepare( long key );
	int base, flagEnter;
	long oldKey;
	char *baseStr;
	void setBase( int p=0 );
	double getInput( int flag=0 );
	void setStr( double d );
public:
	Calculator();
	long handleKey( long key, void *&ptr );
};

#endif
