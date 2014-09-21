/*
	$Id: usermenu.h,v 3.2.2.1 2007/07/24 09:58:10 shelton Exp $
*/
#ifndef _USERMENU_H_
#define _USERMENU_H_

#include <listbox.h>

class UserMenu : public listBox
{
protected:
	char *fileName;
	Collection *commands;
        Collection allCommands;
        int *curCommand;
	void fill();
public:
	UserMenu( char *fName, Collection *Commands, int *current );
	long handleKey( long key, void *&ptr );
};

#endif
