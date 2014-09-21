/*
	$Id: list.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef LIST_H
#define LIST_H

#ifdef DJGPP

#ifndef u_char
#define u_char unsigned char
#endif

#ifndef u_short
#define u_short unsigned short
#endif

#ifndef u_long
#define u_long unsigned long
#endif

#endif

class TList
{
protected:
  void *Head;
  void *Current;

  // определяются типом объекта в списке
  virtual void *getNext(void *item) = 0;
  virtual void *getPrev(void *item) = 0;
  virtual void setNext(void *item, void *next) = 0;
  virtual void setPrev(void *item, void *prev) = 0;
  virtual void freeItem(void *item) = 0;

public:
  TList():Head(0),Current(0) {;}
  virtual ~TList() {;}

  // в производном классе надо определять что-нибудь вроде:
  // Type *item() { return (Type*) getCurrent(); }
  // для типизированного доступа к элементу
  void *getCurrent() { return Current; }
  int empty() {return Head==0;}

  int  first();
  int  last();
  int  next();
  void  Next(); // ring next
  int  prev();
  void  Prev(); // ring prev
  int  seek(void *item);   // устанавливает текущий

  // Следующие две функции не проверяет принадлежность к списку
  // и должны использоваться только тогда, когда есть полная
  // уверенность, что item принадлежит списку
  void setCurrent(void *item)	{ Current=item; }
  void removeIt(void *item);
  void freeIt(void *item);

  void insert(void *item);       // вставляет после текущего
  void insertBefore(void *item); // вставляет перед текущим
  void append(void *item);       // вставляет в конец
  void prepend(void *item);      // вставляет в начало

  int  remove(void *item);
  void remove();           // удаляет текущий; текущим становится следующий
  void removeAll();
  int  free_item(void *item);
  void free_cur();             // освобождает текущий
  void freeAll();

  void put(void *item);     // замещает текущий
  void putFree(void *item); // замещает текущий и освобождает его
};

#endif
