/*
	$Id: message.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef MESSAGE_H
#define MESSAGE_H

#include "list.h"
#include "hashcode.h"

class Link;

class Message
{
    friend class Task;
    friend class MessageList;

    unsigned fFree:1,		// сообщение должно уничтожаться после обработки
	     fBlock:1;          // ждать ответа

    enum {
	    Empty,
	    Wait,		// ждет обработки
	    Proc,               // находится в обработке
	    Resp                // обработано
	 } state:8;

    unsigned long receiver;
    unsigned long sender;

    Message *next;
    Message *prev;
    Message *resp;
    unsigned long id;
    static unsigned long seqId;

 public:
    unsigned long getReceiver()		{ return receiver; }
    unsigned long getSender()		{ return sender; }
    void setReceiver( unsigned long r )	{ receiver=r; }
    void setSender( unsigned long s )	{ sender=s; }

    Message();
    virtual ~Message();

    virtual long type() { return HASH_Message; }
};

class MessageList: public TList
{
  virtual void *getNext(void *item) { return (void*) ((Message*)item)->next; }
  virtual void *getPrev(void *item) { return (void*) ((Message*)item)->prev; }
  virtual void setNext(void *item, void *next) { ((Message*)item)->next = (Message*)next; }
  virtual void setPrev(void *item, void *prev) { ((Message*)item)->prev = (Message*)prev; }
  virtual void freeItem(void *item);
public:
  MessageList() {;}
  ~MessageList(){ freeAll(); }
  Message *item()   { return (Message*) getCurrent(); }
};


#define MESSAGE( msgname )	case HASH_##msgname: return msgname( (msgname##Message *)msg );
#define SELECT_MESSAGE 		switch(msg->type()) {
#define END_SELECT		default: return Task::handleMessage(msg); }
/*
   Эти define'ы предназначены для облегчения жизни при
   обработке сообщения ( например, типа NameMessage )
    - предполагается, что переменная Message *msg указывает на сообщение
    - предполагается наличие в классе задачи метода
       int Name( NameMessage *msg)
    - используются так:
      SELECT_MESSAGE
	MESSAGE( FindThisTask )
	MESSAGE( cmKey )
	MESSAGE( cmKill )
      END_SELECT
*/

/*
 * Производные классы
 */

class Sema;

class SemaMessage: public Message
{
public:
   Sema *data;
   SemaMessage(Sema *Data) : data(Data) {;}
   virtual long type() { return HASH_Sema; }
};

struct OKMessage: public Message
{
   virtual long type() { return HASH_OK; }
};

struct CancelMessage: public Message
{
   virtual long type() { return HASH_Cancel; }
};

struct AbortMessage: public Message
{
   virtual long type() { return HASH_Abort; }
};

struct CloseMessage: public Message
{
   virtual long type() { return HASH_Close; }
};


class IntMessage: public Message
{
 public:
   long data;
   IntMessage(long Data): data(Data){;}
   virtual long type() { return HASH_Int; }
};

class DoubleMessage: public Message
{
 public:
   double data;
   DoubleMessage(double Data): data(Data){;}
   virtual long type() { return HASH_Int; }
};

class StringMessage: public Message
{
protected:
   char *data;
public:
   StringMessage(char *Data);
   char *item()		{ return data; }
   char *getItem()	{ char *ret=data; data=0; return ret; }
   ~StringMessage()	{ delete data; }
   virtual long type()	{ return HASH_String; }
};

class CharPtrMessage: public Message
{
public:
   const char * data;
   CharPtrMessage(const char *Data): data(Data){;}
   virtual long type() { return HASH_CharPtr; }
};

class PtrMessage: public Message
{
public:
   void *data;
   PtrMessage(void *Data): data(Data){;}
   /*~PtrMessage(){ if (data) delete data; }*/
   virtual long type() { return HASH_Ptr; }
};

class ArgvMessage: public Message
{
public:
   char ** argv;
   int argc;
   ArgvMessage(char **Argv, int Argc):argv(Argv),argc(Argc) {}
   ~ArgvMessage()	{ delete [] argv; }
   virtual long type()	{ return HASH_Argv; }
};

class KeyMessage: public Message
{
public:
   long data;
   void *ptr;
   KeyMessage( long Data, void *Ptr=0 ) : data(Data), ptr(Ptr) {}
   virtual long type() { return HASH_Key; }
};

enum MouseEventType
{
  LeftClick,
  RightClick,
  LeftDClick,
  RightDClick,
  LeftDown,
  RightDown,
  Drag,
  Up
};

struct MouseEvent
{
   short x;
   short y;
   MouseEventType type;
};

class MouseMessage: public Message
{
public:
   MouseEvent data;
   MouseMessage(MouseEvent Data) : data(Data) {}
   virtual long type() { return HASH_Mouse; }
};


#endif
