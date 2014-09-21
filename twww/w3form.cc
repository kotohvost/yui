#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <keycodes.h>
#include "w3private.h"
#include "w3table.h"

#ifdef SINIX
extern "C" int strcasecmp( const char *s1, const char *s2 );
extern "C" int strncasecmp( const char *s1, const char *s2, size_t n );
#endif

// [ HText interface

extern "C" void HText_beginForm(HText *text, const char *action, const char *method)
{
  if (!text->win)
    return;
  text->form = new Form( text->win, action, method);
  HText_appendLineBreak(text);
}

extern "C" void HText_endForm(HText *text)
{
  if (!text->win || !text->form)
    return;
  text->form = 0;
  HText_appendLineBreak(text);
}

extern "C" void HText_beginInput(HText *text, const char *name,
		const char *type,
		const char *value, const char *size,
		const char *maxlength, BOOL checked )
{
  if (!text->win || !text->form )
    return;

  text->flush(0);

  const char *s=0;
  int spaces=0;

  if ( !type || !strcasecmp(type,"text") || !strcasecmp(type,"int") )
   { // input line
type_text:
     size_t Size=(value && *value ? strlen(value) : 8), Len=0;
     if (size)
	Size=atoi(size);
     if (maxlength)
	Len=atoi(maxlength);
     if (!Len)
	Len=Size;
     if ( !text->cell && (text->curx+Size > (unsigned)text->rightMargin()) )
	text->new_line();
     FormInputLine *o = new FormInputLine( text->form, text->cury,
					text->curx, name, value, Size, Len );
     if ( text->cell )
      {
	o->init();
	text->cell->append( new InputLineElement( o ) );
      }
     else
      {
	text->win->insert( (inputLine*)o );
	s=o->getLabel();
	if ( Size > strlen(s) )
	   spaces = Size - strlen(s);
      }
     text->form->insert((FormElement*)o);
   }
  else if ( !strcasecmp(type,"password") )
   {
     size_t Size= (value && *value ? strlen(value) : 8), Len=0;
     if (size)
	Size=atoi(size);
     if (maxlength)
	Len=atoi(maxlength);
     if (!Len)
	Len=Size;
     if ( !text->cell && (text->curx+Size > (unsigned)text->rightMargin()) )
	text->new_line();
     FormInputLine *o = new FormInputLine( text->form, text->cury,
					text->curx, name, value, Size, Len, 1 );
     if ( text->cell )
      {
	o->init();
	text->cell->append( new InputLineElement( o ) );
      }
     else
      {
	text->win->insert( (inputLine*)o );
	s=o->getLabel();
	if ( Size > strlen(s) )
	   spaces = Size - strlen(s);
      }
     text->form->insert((FormElement*)o);
   }
  else if ( !strcasecmp( type, "checkbox" ) )
   {
     if ( !text->cell && (text->curx+3 > (int)text->rightMargin()) )
	text->new_line();
     FormCheck *o = new FormCheck( text->form, text->cury, text->curx,
			  name, value, checked );
     if ( text->cell )
      {
	o->init();
	text->cell->append( new CheckElement( o ) );
      }
     else
      {
	text->win->insert( (hyper*)o );
	s=o->getLabel();
      }
     text->form->insert((FormElement*)o);
   }
  else if ( !strcasecmp( type, "radio" ) )
   {
     FormRadio *prev=0;
     for( int i=text->form->getCount()-1; i>=0; i--)
      {
	FormElement *el = (FormElement*)text->form->at(i);
	if ( el->elementType( HASH_FormRadio ) &&
		!strcasecmp( name, el->getName()) )
	 {
	   prev = (FormRadio*)el->getObj();
	   break;
	 }
      }
     if ( !text->cell && (text->curx+3 > (int)text->rightMargin()) )
	text->new_line();
     FormRadio *el = new FormRadio( text->form, text->cury, text->curx,
			  name, value, prev, checked );
     if ( text->cell )
      {
	el->init();
	text->cell->append( new RadioElement( el ) );
      }
     else
      {
	text->win->insert( (hyper*)el );
	s=el->getLabel();
      }
     text->form->insert( (FormElement*)el );
   }
  else if ( !strcasecmp( type, "scribble" ) )
   {
   }
  else if ( !strcasecmp( type, "image" ) )
   {
   }
  else if ( !strcasecmp( type, "hidden" ) )
     text->form->insert( new FormElement( text->form, name, value ) );
  else if ( !strcasecmp( type, "reset" ) )
   {
     int Size = value && *value ? strlen( value ) : 6 /*"RESET"*/;
     if ( !text->cell && (text->curx+Size > (int)text->rightMargin()) )
	text->new_line();
     FormResetButton *o = new FormResetButton( text->form, text->cury, text->curx,
				name, value );
     if ( text->cell )
      {
	o->init();
	text->cell->append( new FormResetElement( o ) );
      }
     else
      {
	text->win->insert( (hyper*)o );
	s=o->getLabel();
      }
     text->form->insert((FormElement*)o);
   }
  else if ( !strcasecmp( type, "submit" ) )
   {
     int Size = value && *value ? strlen( value ) : 6 /*"SUBMIT"*/;
     if ( !text->cell && (text->curx+Size > (int)text->rightMargin()) )
	text->new_line();
     FormSubmitButton *o = new FormSubmitButton( text->form,  text->cury,
			text->curx, name, value );
     if ( text->cell )
      {
	o->init();
	text->cell->append( new FormSubmitElement( o ) );
      }
     else
      {
	text->win->insert( (hyper*)o );
	s=o->getLabel();
      }
     text->form->insert((FormElement*)o);
   }
  else goto type_text;

  if ( !text->cell )
   {
     if (s)
	text->PUTS( s );
     for( int i=spaces; i>0; i-- )
	text->PUTC(' ');
     text->PUTC(' ');
   }
}

extern "C" void HText_beginTextArea(HText *text, const char *name,
		const char *cols, const char *rows)
{
  if ( !text->win || !text->form )
    return;
  text->flush(0);

  if (text->area)
    free(text->area);
  text->arealen = 64;
  text->areapos = 0;

  if (name)
    text->areaname = strdup(name);
  else
    text->areaname = strdup("textarea");

  if (rows)
    text->arearows = atoi(rows);
  else
    text->arearows = 2;

  if (cols)
    text->areacols = atoi(cols);
  else
    text->areacols = 40;

  text->area = (char*) calloc( sizeof(char), text->arealen+1 );
}

extern "C" void HText_endTextArea(HText *text)
{
  if ( !text->area )
     return;
  text->flush(0);
  text->area[text->areapos]=0;

  FormEdit *ed = new FormEdit( text->form, text->cury, text->curx, text->areaname,
		text->arearows, text->areacols, text->area );
  if ( !text->cell )
     text->win->insert( (Edit*)ed );
  else
   {
     ed->init();
     text->cell->append( new EditElement( ed ) );
   }
  text->form->insert((FormElement*)ed);

  if ( text->area )
     free( text->area );
  free( text->areaname );
  text->area=0;

  if ( !text->cell )
    for( int i=0; i <= text->arearows; i++ )
      text->new_line();
}


extern "C" void HText_beginSelect(HText *text, const char *name,
		const char *size, BOOL multiple)
{
   if ( text->inSelect )
      HText_endSelect( text );
   if ( text->sellist )
      delete text->sellist;
   text->sellist = new Collection(5,5);
   if ( text->selval )
      delete text->selval;
   text->selval = new Collection(5,5);
   if ( text->selstate )
      delete text->selstate;

   text->selname = strdup( name && *name ? name : "SELECT" );
   text->selsize = size && *size ? atoi(size) : 0;
   text->selsel = -1;
   text->inSelect = 1;
   if ( multiple == YES )
    {
      text->selmul = 1;
      text->selstate = new Collection(5,5);
    }
   else
    {
      text->selmul = 0;
      text->selstate = 0;
    }

}

extern "C" void HText_endSelect(HText *text)
{
   if ( !text->sellist || !text->form )
     { text->inSelect=0; return; }

   if ( text->option )
      text->endOption();
   char *value=0;
   int w, width=0;
   for( int i=0; i < text->sellist->getCount(); i++ )
    {
      w = strlen( (char*)text->sellist->at(i) );
      if ( w > width )
	width = w;
    }

   FormInputLine *il = 0;
   FormListBox *lb = 0;
   if ( text->selmul || text->selsize > 1 )
    {
      int lines = text->selsize > 0 ? text->selsize : 4;
      lb = new FormListBox( text->form, text->cury, text->curx, text->selname,
			width, lines, text->selmul, text->sellist, text->selval,
			text->selstate, text->selsel );
      if ( text->cell )
	 lb->init();
      else
	 text->win->insert( (listBox*)lb );
      text->form->insert( (FormElement*)lb );
      delete text->sellist;
    }
   else
    {
      int valIndex = 0;
      if ( text->selsel >= 0 && text->selsel < text->sellist->getCount() )
       {
	 value = (char*) text->sellist->at(text->selsel);
	 valIndex = text->selsel;
       }
      il = new FormInputLine( text->form, text->cury, text->curx, text->selname,
			value, width, text->sellist, text->selval, valIndex );
      if ( text->cell )
	 il->init();
      else
	 text->win->insert( (inputLine*)il );
      text->form->insert( (FormElement*)il );
    }

   if ( text->cell )
    {
      if ( il )
	 text->cell->append( new InputLineElement( il ) );
      else
	 text->cell->append( new ListBoxElement( lb ) );
    }

   text->sellist=0;
   text->selval=0;
   text->selstate=0;

   if ( !text->cell )
    {
      if ( il )
       {
	 text->curx += width + 1;
	 text->win->put( text->cury, text->curx, 0, -1, 0 );
       }
      else for( int i=0; i < text->selsize; i++ )
	 text->new_line();
    }
   text->inSelect=0;
}


extern "C" void HText_beginOption(HText *text, const char *value, int selected)
{
   text->flush(0);
   if ( !text->inSelect )
      return;
   if ( text->option )
      text->endOption();
   text->optlen = 20;
   text->option = (char*)calloc( 1, text->optlen+1 );
   text->optpos = 0;
   text->spaceCount=-1;
   text->option_state = selected ? 1 : 0;
   text->option_value = value ? strdup( value ) : 0;
   if ( selected && text->selsel < 0 && text->sellist )
      text->selsel = text->sellist->getCount();
}

extern "C" void HText_endOption(HText *text)
{
   text->endOption();
}

// HText interface ][ Form class


Form::Form(W3Win* Win, const char* Action, const char*Method): win(Win)
{
//  refcount=0;
  action = strdup( Action ? Action : "" );
  method = strdup( Method ? Method : "GET" );
}

Form::~Form()
{
  freeAll();
  ::free(action);
  ::free(method);
}

void Form::freeItem(void *item)
{
  FormElement *el = (FormElement*)item;
//  if ( el->mustDel )
     delete el;
}


void Form::reset()
{
  for( int i=count-1; i >= 0; i-- )
   {
     FormElement *el = (FormElement*)items[i];
     el->reset();
   }
  win->draw(1);
  Screen::sync();
}

/*
** main actions
*/
void Form::submit( FormElement *caller )
{
   if ( !win )
     return;

   if ( win->submit_data )
    {
      ::free( win->submit_data );
      win->submit_data=0;
    }
   int post_flag = !strcasecmp( method, "POST" );
   char *ref = 0;
   const char *name = 0;
   if ( !post_flag )
    {
      int le = strlen( action ) + 2;
      ref = (char*)calloc( sizeof(char), le );
      strcpy( ref, action );
      strtok( ref, "#" );     /* Clip out anchor. */
      strtok( ref, "?" );     /* Clip out old query. */
      strcat( ref, "?" );     /* add the question mark */
    }

   for( int i=0, and_flag=0; i < count; i++ )
    {
      FormElement *f = (FormElement*)items[i];
      if ( /*!post_flag &&*/ f->elementType( HASH_FormSubmitButton ) &&
		(f != caller) || !(name = f->getName()) )
	 continue;
      f->startValue();
      char *s=0, *s2=0, *enam=0, *eval=0;
      for( int j=0; 1; j++ )
       {
	 const char *value = f->getValue();
	 if ( !value )
	    break;
	 s = enam = HTEscape( name, URL_XALPHAS );
	 while( 1 )
	  {
	    if ( !(s2=strstr(s, "%20")) )
	       break;
	    *s2 = '+';
	    memmove( s2+1, s2+3, strlen(s2+3)+1 );
	    s = s2;
	  }
	 s = eval = HTEscape(value, URL_XALPHAS);
	 while( 1 )
	  {
	    if ( !(s2=strstr(s, "%20")) )
	       break;
	    *s2 = '+';
	    memmove( s2+1, s2+3, strlen(s2+3)+1 );
	    s = s2;
	  }
	 if ( and_flag )
	    StrAllocCat( ref, "&" );
	 and_flag = 1;

	 StrAllocCat( ref, enam );
	 StrAllocCat( ref, "=" );
	 StrAllocCat( ref, eval );

	 ::free(eval);
	 ::free(enam);
       }
    }
   if ( post_flag )
    {
      win->submit_data = ref;
      win->newStart( action, method, HTAnchor_parent(win->node_anchor) );
    }
   else
    {
      win->newStart( ref );
      ::free( ref );
    }
}


// Form ][ FormElement

FormElement::FormElement( Form *_form, const char *Name, const char *Value ):
	       form(_form), getval_flag(0)
{
  name = Name ? strdup( Name ) : 0;
  value = strdup( Value ? Value : "" );
}

FormElement::~FormElement()
{
  form->remove(this);
  if (name)
    ::free(name);
  if (value)
    ::free(value);
  if ( form->getCount() <= 0 )
    delete form;
}

char *FormElement::getValue()
{
  if ( !getval_flag )
     return 0;
  getval_flag = 0;
  return value;
}

void FormElement::reset()
{
  if (name)
   { ::free(name); name=0; }
  if (value)
   { ::free(value); value=0; }
}

const char* FormElement::getLabel()
{
  return name;
}

int FormElement::elementType( long t )
{
  return t == HASH_FormElement;
}

/*
FormHidden::FormHidden( Form *_form, const char *Name, const char *Value ):
	      FormElement(_form, Name, Value)
{
//  mustDel=1;
}
*/

// FormElement ][ FormInputLine

//extern unsigned char *FormInputColors;
extern unsigned char colorWebInput[2];
extern unsigned char colorWebBox[2];
extern unsigned char colorWebButton[2];
extern unsigned char colorWebList[9];

extern unsigned char monoWebInput[2];
extern unsigned char monoWebBox[2];
extern unsigned char monoWebButton[2];
extern unsigned char monoWebList[9];

extern unsigned char laptopWebInput[2];
extern unsigned char laptopWebBox[2];
extern unsigned char laptopWebButton[2];
extern unsigned char laptopWebList[9];

long inputKeyMap[] = {
	kbEnter,	HASH_cmNext,
	' ',		HASH_cmHistory,
	0
};

long listKeyMap[] = {
	kbEnter,	HASH_cmNext,
	' ',		HASH_cmMode,
	0
};

long formEditKeyMap[] = {
	kbTab,		HASH_cmNext,
	kbCtrlB,	HASH_cmMode,
	0
};

Keymap InputKeyMap( inputKeyMap );
Keymap ListKeyMap( listKeyMap );
Keymap FormEditKeyMap( formEditKeyMap );

FormInputLine::FormInputLine( Form *_form, int y, int x, const char *Name,
		 const char *Value, int Size, int MaxLen, int isPasswd ):
   FormElement(_form, Name, Value),
   inputLine( Point(y,x), Value, 0, Size, MaxLen, 0, 0, inputLineFillChar, isPasswd ),
   hist(0), vals(0)
{
  keyHolder.add( &InputKeyMap );
  setColors( colorsType );
}

void FormInputLine::setColors( int Type )
{
   switch(Type)
     {
       case MONO:
	  clr = monoWebInput; break;
       case COLOR:
	  clr = colorWebInput; break;
       case LAPTOP:
	  clr = laptopWebInput; break;
     }
}

FormInputLine::FormInputLine( Form *_form, int y, int x,
		 int Hight, int Size,
		 const char *Name, const char *Value,
		 int MaxLen, int isPasswd ):
   FormElement(_form, Name, Value),
   inputLine(Rect( y, x, y+Hight-1, x+Size-1), Value, 0, MaxLen, 0, 0,inputLineFillChar, isPasswd),
   hist(0), vals(0)
{
  keyHolder.add( &InputKeyMap );
  setColors( colorsType );
}

FormInputLine::FormInputLine( Form *_form, int y, int x, const char *Name,
		 const char *Value, int Size, Collection *Hist,
		 Collection *Vals, int ValIndex ):
   FormElement(_form, Name, Value),
   inputLine( Point(y,x), Value, 0, Size, /*Size*/0, 0, Hist ),
   hist(Hist), vals(Vals), valIndex(ValIndex)
{
  keyHolder.add( &InputKeyMap );
  setColors( colorsType );
}

FormInputLine::~FormInputLine()
{
  if (hist)
    delete hist;
  if (vals)
   {
     char *ch;
     for( int i=vals->getCount()-1; i>=0; i-- )
       if ( (ch = (char*)vals->at(i)) )
	 ::free( ch );
     vals->removeAll();
     delete vals;
   }
}

long FormInputLine::handleKey( long key, void *&ptr )
{
  if ( keyHolder.translate( key ) == HASH_cmHistory && len <= 0 )
   {
     if ( !history )
	return 0;
     int ind = runHistory();
     if ( ind >= 0 )
	valIndex = ind;
     return 0;
   }
  return inputLine::handleKey( key, ptr );
}

char *FormInputLine::getValue()
{
  if ( !getval_flag )
     return 0;
  getval_flag = 0;
  char *ch = str;
  if ( vals && valIndex >=0 && valIndex < vals->getCount() && vals->at(valIndex) )
     ch = (char*)vals->at(valIndex);
  return ch;
}

void FormInputLine::reset()
{
  initString(value);
}

const char *FormInputLine::getLabel()
{
  return str;
}

//  FormInputLine ] FormListBox

FormListBox::FormListBox( Form *_form, int y, int x, const char *Name,
			int width, int lines, int Multiple,
			Collection *slist, Collection *sval,
			Collection *state, int cur ) :
		FormElement(_form, Name, 0),
		listBox( Rect( y, x, y+lines-1, x+width-1), width, 0, slist, state, 0, cur ),
		selval(sval), selstate(state), multiple(Multiple), val_index(0), val_buf(0)
{
  keyHolder.add( &ListKeyMap );
  setColors( colorsType );
}

FormListBox::~FormListBox()
{
  if ( selval )
     delete selval;
  if ( selstate )
     delete selstate;
  if ( val_buf )
     ::free( val_buf );
}

long FormListBox::handleKey( long key, void *&ptr )
{
  key = keyHolder.translate( key );
  switch( key )
   {
     case HASH_cmIns:
     case HASH_cmSetMask:
     case HASH_cmUnsetMask:
     case HASH_cmInverse:
	if ( !multiple )
	   return 0;
   }
  return listBox::handleKey( key, ptr );
}

char *FormListBox::getValue()
{
  if ( !getval_flag )
     return 0;
  if ( !val_buf )
     val_buf = (char*)malloc( 1024 );
  char *val = 0;
  val_buf[0] = 0;
  if ( !multiple )
   {
     getval_flag = 0;
     val = (char*)(selval ? selval->at(current) : 0);
     strcpy( val_buf, (char*)(val ? val : ((listItem*)items[current])->item) );
     return val_buf;
   }
  for( int cycle=1; cycle && val_index < count; val_index++ )
   {
     if ( ((listItem*)items[val_index])->state == 0 )
	continue;
     val = (char*)(selval ? selval->at(val_index) : 0);
     strcpy( val_buf, (char*)(val ? val : ((listItem*)items[val_index])->item) );
     cycle=0;
   }
  if ( val_index < count )
     return val_buf;

  getval_flag = 0;
  return 0;
}

void FormListBox::reset()
{
  manualPos = 0;
  delta.y = 0;
  current = -1;
  int i=0, *s=0, state=0;
  for( ; i < count; i++ )
     ((listItem*)items[i])->state = 0;
  countIns = 1;
  for( i=0; i < count; i++ )
   {
     state = (selstate && i < selstate->getCount()) ? *(int*)selstate->at(i) : 0;
     if ( !state )
	continue;
     if ( current < 0 )
	current = i;
     s = &((listItem*)items[i])->state;
     *s = countIns++;
   }
  if ( current < 0 && count > 0 )
     current = 0;
}

int FormListBox::elementType( long hash )
{
  return hash == HASH_FormList ? 1 : listBox::isType( hash );
}

void FormListBox::setColors(int Type)
{
  switch(Type)
   {
     case MONO:
	 clr = monoWebList; break;
     case COLOR:
	 clr = colorWebList; break;
     case LAPTOP:
	 clr = laptopWebList; break;
   }
}

void FormListBox::startValue()
{
  getval_flag = 1;
  val_index = 0;
}

//  FormListBox ][ FormEdit

FormEdit::FormEdit( Form *_form, int y, int x, const char *name,
		int rows, int cols, char *textarea ) :
		FormElement(_form, name, 0),
		Edit( (Collection*)0, "textarea",  Rect( y, x, y+rows-1, x+cols-1 ) ),
		val_buf(0)
{
  box = 0;
  keyHolder.add( &FormEditKeyMap );
  setColors( colorsType );
  reset_buf = new Collection( 10, 10 );
  externCollection = new Collection( 10, 10 );
  if ( textarea && *textarea )
   {
     while( 1 )
      {
	if ( !textarea )
	   break;
	char *ch = strchr( textarea, '\n' );
	int len = ch ? ch - textarea : strlen(textarea);
	char *s = new char[ len + 1 ];
	memcpy( s, textarea, len );
	s[len] = 0;
	reset_buf->insert( s );
	textarea = ch ? ch+1 : 0;
      }
   }
  reset();
}

FormEdit::~FormEdit()
{
  if ( reset_buf )
   {
     reset_buf->freeAll();
     delete reset_buf;
     reset_buf = 0;
   }
  if ( externCollection )
   {
     externCollection->freeAll();
     delete externCollection;
     externCollection = 0;
   }
}

long FormEdit::handleKey( long key, void *&ptr )
{
  key = keyHolder.translate( key );
  switch( key )
   {
     case HASH_cmMode:
	 return key;
     case HASH_cmNext:
	 if ( owner->hardMode )
	    owner->hardMode = !owner->hardMode;
	 return key;
   }
  return Edit::handleKey( key, ptr );
}

char *FormEdit::getValue()
{
  if ( !getval_flag )
     return 0;
  if ( val_buf )
     ::free( val_buf );
  val_buf = strdup( "" );
  int i=0, len=0;
  for( ; i < externCollection->getCount(); i++ )
   {
     char *s = (char*)externCollection->at( i );
     len += strlen( s ) + (i>0?1:0) + 2;
     val_buf = (char*)realloc( val_buf, len );
     strcat( val_buf, s );
     strcat( val_buf, "\r\n" );
   }
  getval_flag = 0;
  return val_buf;
}

void FormEdit::reset()
{
  if ( !reset_buf || !externCollection )
     return;
  externCollection->freeAll();
  for( int i=reset_buf->getCount()-1; i >= 0; i-- )
   {
     char *s = (char*)reset_buf->at(i);
     char *ch = new char[ strlen( s ) + 1 ];
     strcpy( ch, s );
     externCollection->atInsert( 0, ch );
   }
  readCollection();
}

int FormEdit::elementType( long hash )
{
  return hash == HASH_FormEdit ? 1 : Edit::isType( hash );
}

void FormEdit::setColors( int Type )
{
  Edit::setColors( Type );
}

void FormEdit::startValue()
{
  getval_flag = 1;
  saveCollection();
}

// FormEdit ][ FormCheck/FormRadio

const char CheckStr[]	= "[ ]";
const char SCheckStr[]	= "[X]";
const char RadioStr[]	= "( )";
const char SRadioStr[]	= "(*)";

long checkKeyMap[] = {
	kbEnter,	HASH_cmSelect,
	' ',		HASH_cmSelect,
	0
};

Keymap CheckKeyMap( checkKeyMap );

FormCheck::FormCheck( Form *_form, int y, int x, const char *Name,
		 const char *Value, int Checked ):
   FormElement(_form, Name, Value),
   hyper(Point(y,x), Checked?SCheckStr:CheckStr),
   _checked(Checked), checked(Checked)
{
  keyHolder.add( &CheckKeyMap );
  if ( !value || !value[0] )
   {
     if ( value )
	::free( value );
     value = strdup( "on" );
   }
  setColors( colorsType );
}

void FormCheck::setColors( int Type )
{
   switch(Type)
     {
       case MONO:
	  clr = monoWebBox; break;
       case COLOR:
	  clr = colorWebBox; break;
       case LAPTOP:
	  clr = laptopWebBox; break;
     }
}

long FormCheck::handleKey( long key, void *&ptr )
{
  switch( keyHolder.translate( key ) )
   {
     case HASH_cmSelect:
       checked=!checked;
       setLabel();
       hyper::draw();
       Screen::sync();
       return 0;
   }

  return hyper::handleKey( key, ptr );
}

char *FormCheck::getValue()
{
  if ( !getval_flag || !checked )
     return 0;
  getval_flag = 0;
  return value;
}

void FormCheck::reset()
{
  checked=_checked;
  setLabel();
}

void FormCheck::setLabel()
{
  hyper::setLabel( checked ? SCheckStr : CheckStr );
}

Point FormCheck::getCursor( int *hide )
{
 return hyper::getCursor(hide)+Point(0,1);
}

int FormCheck::elementType(long hash)
{
  return hash==HASH_FormCheck ? 1 : hyper::isType(hash);
}

FormRadio::FormRadio( Form *_form, int y, int x, const char *Name,
		 const char *Value, FormRadio *Prev, int Checked ):
   FormElement(_form, Name, Value),
   hyper(Point(y,x), Checked?SRadioStr:RadioStr),
   _checked(Checked), checked(Checked),
   prev(Prev), next(0)
{
  keyHolder.add( &CheckKeyMap );
  if ( Prev )
     prev->next=this;
  if ( !value || !value[0] )
   {
     if ( value )
	::free( value );
     value = strdup( "on" );
   }
  setColors( colorsType );
}

void FormRadio::setColors( int Type )
{
   switch(Type)
     {
       case MONO:
	  clr = monoWebBox; break;
       case COLOR:
	  clr = colorWebBox; break;
       case LAPTOP:
	  clr = laptopWebBox; break;
     }
}

long FormRadio::handleKey( long key, void *&ptr )
{
  switch( keyHolder.translate( key ) )
   {
     case HASH_cmSelect:
       {
	 FormRadio *p=this;
	 for( ; p->prev; p=p->prev );
	 for( ; p; p=p->next)
	  {
	   if (p!=this)
	     p->checked=0;
	   else
	     p->checked=1;
	   p->setLabel();
	  }

	 hyper::owner->draw(1);
	 Screen::sync();
       }
       return 0;
   }
  return hyper::handleKey( key, ptr );
}

char *FormRadio::getValue()
{
  if ( !getval_flag || !checked )
     return 0;
  getval_flag = 0;
  return value;
}

void FormRadio::reset()
{
  checked=_checked;
  setLabel();
}

void FormRadio::setLabel()
{
  hyper::setLabel( checked ? SRadioStr : RadioStr );
}

Point FormRadio::getCursor( int *hide )
{
 return hyper::getCursor(hide)+Point(0,1);
}

int FormRadio::elementType(long hash)
{
  return hash==HASH_FormRadio ? 1 : hyper::isType(hash);
}

// FormCheck/FormRadio ][ Buttons

FormResetButton::FormResetButton( Form *_form, int y, int x,
		const char *Name, const char *Value ):
		FormElement(_form, Name, Value),
		hyper(Point(y,x), Value?Value:"RESET")
{
  setColors( colorsType );
}

void FormResetButton::setColors( int Type )
{
   switch(Type)
     {
       case MONO:
	  clr = monoWebButton; break;
       case COLOR:
	  clr = colorWebButton; break;
       case LAPTOP:
	  clr = laptopWebButton; break;
     }
}

int FormResetButton::elementType(long hash)
{
  return hash==HASH_FormResetButton ? 1 : hyper::isType(hash);
}

long FormResetButton::handleKey( long key, void *&ptr )
{
  switch( keyHolder.translate(key) )
   {
     case HASH_cmOK:
	 form->reset();
	 return 0;
   }
  return hyper::handleKey(key, ptr);
}

FormSubmitButton::FormSubmitButton( Form *_form, int y, int x,
		const char *Name, const char *Value ):
		FormElement(_form, Name, Value),
		hyper(Point(y,x), Value?Value:"SUBMIT")
{
  setColors( colorsType );
}

void FormSubmitButton::setColors( int Type )
{
   switch(Type)
     {
       case MONO:
	  clr = monoWebButton; break;
       case COLOR:
	  clr = colorWebButton; break;
       case LAPTOP:
	  clr = laptopWebButton; break;
     }
}

int FormSubmitButton::elementType(long hash)
{
  return hash==HASH_FormSubmitButton ? 1 : hyper::isType(hash);
}

long FormSubmitButton::handleKey( long key, void *&ptr )
{
  switch( keyHolder.translate(key) )
   {
     case HASH_cmOK:
	 form->submit( this );
	 return 0;
   }
  return hyper::handleKey(key, ptr);
}

// Buttons ]
