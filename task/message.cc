/*
	$Id: message.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/

#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "message.h"

unsigned long Message::seqId = 0;

Message::Message() : fFree(0), fBlock(0), state(Empty), receiver(0), sender(0),
		next(0), prev(0), resp(0)
{
  id=seqId++;
}

Message::~Message()
{
  if (resp)
    delete resp;
}

void MessageList::freeItem(void *item)
{
  Message *mp=(Message *)item;

  if (mp->fFree)
     delete mp;
}

StringMessage::StringMessage(char *Data)
{
  data=strdup(Data);
}

