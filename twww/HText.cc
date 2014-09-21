#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <term.h>
#include "w3private.h"
#include "w3table.h"

#ifdef SINIX
extern "C" int strcasecmp( const char *s1, const char *s2 );
extern "C" int strncasecmp( const char *s1, const char *s2, size_t n );
#endif

static char *tbl_border=0;

static HTStyle defaultStyle = {
	0,  "default", "DEF",
	HT_FONT, 1.0, 0, 0, 0,
	0, 0, 4, HT_LEFT, 1, 0, 0,
	YES, YES, 1, 1, 2, 0,
	0, 0, 0, 0 };

static int isLower( unsigned char ch )
{
  if (ch<128)
     return islower(ch);
  if (ch>=224)
    return 0;
  return 1;
}

// [ external HText interface
/*
  CREATE HYPERTEXT OBJECT
     There  are several methods depending on how much you want
   to  specify.  The  output stream is used with objects which
   need  to output the hypertext to a stream. The structure is
   for  objects  which need to refer to the structure which is
   kep by the creating stream.
*/

extern "C" HText* HText_new( HTParentAnchor *anchor )
{
  static char *BORDER = "-|+++++++++";
  tbl_border = Term::G1 ? Term::G1 : 0;
  if ( !tbl_border )
     tbl_border = Term::G2 ? Term::G2 : 0;
  if ( !tbl_border )
     tbl_border = BORDER;

  HText *self = (HText *)calloc( 1, sizeof(HText) );

  self->target = NULL;
  self->style = &defaultStyle;
  self->color = defaultStyle.color;
  self->subStyle = 0;
  self->strBefore = 0;
  self->strAfter = 0;
  self->buflen = 0;
  self->currAnchor = 0;
  self->anchor_target = 0;
  self->cury = 0;
  self->curx = 0;
  self->spaceCount = 0;
  self->waitDigit = 0;
  self->shift = 0;
  self->lateBreak = 0;
  self->newPara = 0;
  self->softPara = 0;
  self->inAnchor = 0;
  self->inSelect = 0;
  self->wasBreak = 0;
  self->form = 0;
  self->table = 0;
  self->row = 0;
  self->cell = 0;

  self->area = 0;
  self->areaname= 0;
  self->arearows= 0;
  self->areacols= 0;
  self->arealen= 0;

  self->sellist=0;
  self->selval=0;
  self->selstate=0;
  self->selname=0;
  self->selsize=0;
  self->selmul=0;
  self->selsel=0;

  self->option=0;
  self->optlen=0;
  self->optpos=0;
  self->option_value=0;
  self->option_state=0;

  self->win = (W3Win *)HTAnchor_document(anchor);

  if (self->win)
   {
     self->win->clear();
     self->win->cleared=1;
     self->win->text = self;
     self->win->node_anchor = (HTAnchor*)anchor;
     self->new_paragraph();
     if (self->win->base_href)
       free(self->win->base_href);
     self->win->base_href=0;
     if (self->win->base_target)
       free(self->win->base_target);
     self->win->base_target=0;
   }

  return self;
}

extern "C" HText* HText_new2( HTParentAnchor *anchor, HTStream *output_stream )
{
    HText * me = HText_new( anchor );
    if ( output_stream )
     {
	me->target = output_stream;
	me->targetClass = *output_stream->isa;
     }
    return me;
}

extern "C" void HText_free( HText * self )
{
    if (!self)
      return;
    if (self->win)
      self->win->text=0;
    if (self->form)
      delete self->form;
    if (self->table)
      delete self->table;
    if (self->row)
      delete self->row;
    if (self->cell)
      delete self->cell;
    free (self);
}

/*
Object Building methods
     These are used by a parser to build the text in an object
   HText_beginAppend  must  be called, then any combination of
   other append calls, then HText_endAppend. This allows opti-
   mised  handling  using buffers and caches which are flushed
   at the end.
*/
extern "C" void HText_beginAppend( HText * text )
{
}

extern "C" void HText_endAppend (HText * text)
{
}

/*
  SET THE STYLE FOR FUTURE TEXT
*/
extern "C" void HText_setStyle (HText * text, HTStyle * style)
{
    if (text->inAnchor || text->inSelect)
       return;
    int softPara=text->softPara;
    if ( !text->newPara)
    text->flush(0);
    text->softPara=softPara;
    if (text->cell)
      {
       text->cell->append(new StyleElement(style));
       return;
      }

    if (!style)
       return;

    int after = (int) text->style->spaceAfter;
    int before = (int) style->spaceBefore;
    int n = ((after>before) ? after : before);

//    if (SGML_TRACE)
//	fprintf(stdout, "HTML: Change to style %s\n", style->name);

    if (style->onlyColor)
     {
       if ( text->strAfter &&  text->subStyle && (text->subStyle->onlyColor & CHANGE_QUOTA) )
	  text->PUTS(text->strAfter);

       if( style->onlyColor & CHANGE_COLOR )
	  text->color = style->color;

       if (style->onlyColor & CHANGE_QUOTA )
	{
	  text->strBefore = style->strBefore;
	  text->strAfter = style->strAfter;

	  if (text->strBefore)
	    text->PUTS(text->strBefore);
	}
       text->subStyle = style;
     }
    else
     {
       if (text->strAfter && (!text->subStyle || (text->subStyle->onlyColor & CHANGE_QUOTA) ) )
	 text->PUTS(text->strAfter);

       if ( text->subStyle )
	 n = 0;
       else if ( text->style->strList && style->strList )
	 { text->shift = 0; }
       else if ( n )
	 text->shift = 0;

       int indentFlag=0;
       if ( text->newPara && !text->style->strAfter )
	 { n=0; indentFlag=1; }

       if (n>0)
	 { text->newPara=1; text->softPara=1; }

       if ( n > 1)
	 { text->_new_line(); n--; }

       text->style = style;
       text->subStyle = 0;
       text->color = style->color;
       text->strBefore = style->strBefore;
       text->strAfter = style->strAfter;

       if (n>0)
       {
	 while(n-->1)
	  text->_new_line();
	 text->new_line();
       }
       if (indentFlag)
	 text->newIndent();

       if (text->strBefore && (!text->subStyle || text->subStyle &&
	    (text->subStyle->onlyColor & CHANGE_QUOTA ) ) )
	 text->PUTS(text->strBefore);

       text->subStyle = 0;
     }
}

/*
  ADD ONE CHARACTER
*/
extern "C" void HText_appendCharacter (HText * text, char ch)
{
    if (ch==27)
      ch='[';    // ]
    if ( text->inAnchor || text->currAnchor || text->inSelect )
      {
	 if ( ch>=0 && ch<32 )
	    ch=' ';
	 // eat leading & multiple spaces
	 if( ch==' ' && (!text->buflen || text->buf[text->buflen-1]==' ') )
	    return;
	 text->buf[text->buflen++]=ch;
	 if ( (unsigned)text->buflen >= sizeof(text->buf)-1 )
	    text->buflen = sizeof(text->buf)-2;
	 text->wasBreak=0;
	 text->newPara=0;
	 return;
      }

    if ( text->lateBreak )
      text->new_line();

    switch(ch)
      {
	case '.':
	case ',':
	  if ( !text->cell )
	     text->flush(ch);
	  else if ( text->area )
	     text->charToArea( ch );
	  else
	     text->buf[text->buflen++]=ch;
	  if ( text->style->freeFormat || text->cell )
	     text->waitDigit=1;
	  return;
	case '\r':
	  return;
	case '\n':
	  if ( text->area )
	     break;
	  if ( text->style->freeFormat || text->cell )
	   {
	     if ( text->spaceCount >= 0 )
		text->spaceCount++;
	   }
	  else
	     text->New_line();
	  return;

	case '\t':
//	  if ( !text->table && text->style->tabs )
//	    break;
	  ch=' ';
	case ' ':
	  if ( text->area )
	     break;
	  if ( !text->option && (text->style->freeFormat || text->cell) )
	   {
	     if ( text->buflen )
	       text->flush(0);
	     if (text->spaceCount>=0)
	       text->spaceCount++;
	     return;
	   }
	  break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '<':
	case '>':
	case ':':
	case ';':
	case '-':
	case '+':
	case '=':
	case '|':
	case '\\':
	case '/':
	case '`':
	case '\'':
	case '\"':
	case '?':
	case '~':
	case '!':
	case '@':
	case '#':
	case '$':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '{':
	case '}':
	case '[':
	case ']':
	   if ( text->waitDigit )
	     { text->spaceCount=0; text->waitDigit=0; }

	default:
	  text->wasBreak=0;
	  text->newPara=0;
	  if ( isLower(ch) )
	    text->waitDigit=0;
	  if (text->spaceCount>0)
	    text->flush(0);
	  else if ( text->waitDigit && (text->style->freeFormat || text->cell) )
	    text->flush(' ');
	  if ( text->area )
	    text->charToArea( ch );
	  else
	    text->buf[text->buflen++]=ch;
	  if ( (unsigned)text->buflen >= sizeof(text->buf)-1 )
	    break;
	  return;
      }

    text->wasBreak=0;
    text->newPara=0;
    text->flush(ch);
}

/*
  ADD A ZERO-TERMINATED STRING
*/
extern "C" void HText_appendText (HText * text, CONST char * str)
{
  for( ; *str; str++)
    HText_appendCharacter(text, *str);
}

/*
  NEW PARAGRAPH
   and similar things
*/
extern "C" void HText_appendParagraph (HText * text)
{
   text->newPara = 0;
   if (text->cell)
     { text->flush(0); return; }
   text->new_paragraph( 1 );
}

extern "C" void HText_appendPara (HText * text )
{
   if (text->cell)
     { text->flush(0); return; }
   text->new_paragraph( 0 );
}

extern "C" void HText_appendLineBreak (HText * text)
{
  if ( text->inAnchor || text->inSelect || text->newPara || text->lateBreak )
     return;
  text->flush(0);
  if ( text->cell )
   {
     if ( text->cell->getCurrent() )
      {
	text->cell->append( new BreakElement );
	text->spaceCount=-1;
      }
     return;
   }
  if ( text->style->freeFormat )
    text->spaceCount=-1;
  else
    text->spaceCount=0;
  text->lateBreak=1;
  text->waitDigit=0;
}

extern "C" void HText_appendSoftPara (HText * text)
{
  if ( text->inAnchor || text->inSelect || text->newPara || text->cell )
     return;
  text->softPara=1;
}

extern "C" void HText_appendHorizontalRule (HText * text)
{
   if ( text->inAnchor || text->inSelect )
     return;
   if (text->cell)
    {
      text->flush(0);
      text->cell->append( new BreakElement );
      text->spaceCount=-1;
      return;
    }

  text->flush(0);
  text->new_line();
  text->softPara=0;

  int r=text->rightMargin();
  if ( text->win && text->win->getWidth()<r )
    r=text->win->getWidth();

  while( text->curx<r )
    text->PUTC('-');

  HText_appendLineBreak(text);
}

/*
 START/END SENSITIVE TEXT
     The anchor object is created and passed to HText_beginAn-
   chor.  The  senstive  text is added to the text object, and
   then  HText_endAnchor is called. Anchors may not be nested.
*/
extern "C" void HText_BeginAnchor( HText * text, HTChildAnchor * anc, BOOL haveHREF, const char *a_target )
{
  if ( text->inAnchor || text->currAnchor )
    HText_endAnchor(text);
  else
    text->flush(0);
  text->currAnchor=anc;
  if ( haveHREF )
    text->inAnchor=1;
  if ( text->anchor_target )
   {
    ::free( text->anchor_target );
    text->anchor_target = 0;
   }
  if ( a_target )
    text->anchor_target = strdup( a_target );
  else if ( text->win && text->win->base_target )
    text->anchor_target = strdup( text->win->base_target );
}

extern "C" void HText_endAnchor( HText * text )
{
  if ( text->cell && text->currAnchor )
   {
     char *label = NULL;
     if ( text->inAnchor )
      {
	text->buf[text->buflen] = 0;
	label = text->buf;
      }
     text->cell->append( new AnchorElement( text->currAnchor, label, text->anchor_target ) );
     text->currAnchor=0;
     text->inAnchor=0;
     text->buflen=0;
     text->spaceCount=0;
     return;
   }

  if ( text->win && text->currAnchor )
   {
     long y=0; short x=0;
     int r = (unsigned)text->rightMargin();
     AnchorHyper *a=0;
     if ( text->inAnchor )
      {
	Collection *bindList = new Collection( 5, 5 );
	while( 1 )
	 {
	   int pos=0, last=0;
	   for( int cx=text->curx; cx < r && pos < text->buflen; cx++, pos++ )
	      if ( ispunct( text->buf[pos] ) || isspace( text->buf[pos] ) )
		 last = pos + 1;
	   if ( pos < text->buflen && last > 0 )
	      pos = last;
	   char *subbuf = (char*)malloc( pos + 1 );
	   memcpy( subbuf, text->buf, pos );
	   subbuf[pos]=0;
	   text->buflen -= pos;
	   memmove( text->buf, text->buf + pos, text->buflen );
	   text->win->getPos(y,x);
	   a = new AnchorHyper( y, x, subbuf, text->currAnchor, HTAnchor_parent(text->win->node_anchor), text->anchor_target );
	   bindList->insert( a );
	   a->bindList = bindList;
	   if ( bindList->getCount() > 0 && ((getobj*)bindList->at(0))->bind_flag )
	      a->bind_flag = 1;
	   text->PUTS(subbuf);
	   text->win->insert(a);
	   ::free( subbuf );
	   if ( text->buflen <= 0 )
	      break;
	   text->new_line();
	 }
      }
     else
      {
	text->win->getPos(y,x);
	a = new AnchorHyper( y, x, "", text->currAnchor, HTAnchor_parent(text->win->node_anchor), text->anchor_target );
	text->win->insert(a);
      }
   }
  text->currAnchor=0;
  if ( text->anchor_target )
   {
     ::free( text->anchor_target );
     text->anchor_target = 0;
   }
  text->inAnchor=0;
  text->wasBreak = 0;
  text->newPara = 0;
  text->flush(0);
}


/*
  SET THE NEXT FREE IDENTIFIER

      The string must be of the form aaannnnn where aaa is the
   prefix  for  automatically generated ids, normally "z", and
   nnnn is the next unused number. If not present, defaults to
   z0. An editor should pick up both the a and n bits, and in-
   crement  n  when  generating ids. This ensures that old ids
   are  not  reused, even if the elements using them have been
   deleted from the document.

*/
/* not used by basic HTML.c
extern "C" void HText_setNextId (
	HText *         text,
	CONST char *    s)
{
}
*/

/*
  APPEND AN INLINE IMAGE

     The  image  is handled by the creation of an anchor whose
   destination  is  the image document to be included. The se-
   mantics is the intended inline display of the image.
     An  alternative  implementation could be, for example, to
   begin  an  anchor,  append the alternative text or "IMAGE",
   then  end  the anchor. This would simply generate some text
   linked to the image itself as a separate document.

*/

extern "C" void HText_appendImage (
	HText *         text,
	HTChildAnchor * anc,
	CONST char *    alternative_text,
	CONST char *    alignment,
	BOOL            isMap)
{
  HText_appendCharacter( text, ' ' );
  if ( text->inAnchor )
   {
     if ( text->currAnchor && text->currAnchor->tag )
	HText_appendText( text, text->currAnchor->tag );
     else
	HText_appendText( text, alternative_text ? alternative_text : "[IMG]" );
   }
  else
   {
     HText_BeginAnchor( text, anc, YES, text->anchor_target );
     HText_appendText( text, alternative_text ? alternative_text : "[IMG]" );
     HText_endAnchor( text );
   }
  HText_appendCharacter(text, ' ');
}

extern "C" void HText_base( HText *text, const char *base_href, const char *base_target )
{
  if ( !text->win )
     return;
  if ( base_href )
   {
     if ( text->win->base_href )
	free( text->win->base_href );
     text->win->base_href = strdup( base_href );
   }
  if ( base_target )
   {
     if ( text->win->base_target )
	free( text->win->base_target );
     text->win->base_target = strdup( base_target );
   }
}

extern "C" void HText_putListPrefix(HText *text, char *prefix)
{
  if ( text->cell )
   {
     text->flush(0);
     return;
   }

  if ( !prefix )
   {
     if ( !text->style->strList )
	return;
     prefix = (char*)text->style->strList;
   }

   if ( !text->wasBreak && !text->newPara )
    {
      text->spaceCount=0;
      text->flush(0);

      int n = (int)text->style->spacePara;
      if ( n < 1 )
	 n=1;
      while( n-- )
	{ text->PUTC('\n'); text->cury++; }
    }
   else if (text->win)
     text->win->put(text->cury, 0, "");

   text->curx=0;
   text->shift=strlen(prefix)+1;
   int beg=(int)text->style->leftIndent;

   for(int i=0; i<beg; i++)
      text->PUTC(' ');

   text->PUTS(prefix);
   text->PUTC(' ');
   if (text->style->freeFormat)
      text->spaceCount=-1;
   else
      text->spaceCount=0;
   text->newPara=0;
   text->softPara=0;
   text->lateBreak=0;
   text->wasBreak=0;
}

extern "C" void addSlash( char **str )
{
  int len;
  char *dot, *slash, *f;
  if (!str || !*str)
     return;
  if ( !strncmp(*str, "file:", 5) )
   {
     char *s=*str+5;
     char *s1=s+1;
     while( *s1 )
      if (*s=='/' && *s1=='/')
	strcpy(s, s1);
      else
	{ s++; s1++; }

     return;
   }
  if ( strncmp(*str, "http:", 5) )
    return;
  if ( strpbrk(*str, "?#") )
    return;
  if ( strstr(*str, "/cgi-bin/") )
    return;

  len=strlen(*str);
  dot=(char*)strrchr(*str, '.');
  slash=(char*)strrchr(*str, '/');
  f=(char*)strstr(*str, "//");

  if ( dot && !slash )
    return;
  if ( dot && slash && dot>slash && (!f || slash>(f+1) ) )
    return;
  if ( slash && (slash-*str) == (len-1) )
    return;

  *str=(char*)realloc(*str, len+2);
  (*str)[len]='/';
  (*str)[len+1]=0;
}

extern "C" void HText_beginTable(HText *text, BOOL border)
{
  if ( !text )
     return;
  text->flush(0);
  if ( !text->cell )		// таблица верхнего уровня
     text->table = new Table( border );
  else				// вложенная талица
   {
     Table *t = new Table( border, text->table, text->row, text->cell );
     TableElement *te = new TableElement( t );
     t->parent = te;
     text->cell->append( te );
     text->table = t;
     text->row = 0;
     text->cell = 0;
   }
}

extern "C" void HText_endTable( HText *text )
{
  if ( !text || !text->table )
     return;

  text->flush(0);
  if ( text->row )
   {
     if ( text->cell )
	text->row->append( text->cell );
     text->table->insert( text->row );
   }

  text->table->init();

  if ( text->table->parent )
   {
     text->row = text->table->prev_row;
     text->cell = text->table->prev_cell;
     text->table = text->table->prev_table;
   }
  else
   {
     text->table->setWidth( text->rightMargin() - (int)text->style->leftIndent );
     text->table->print( text );

     delete text->table;

     text->table = 0;
     text->row = 0;
     text->cell = 0;
   }
}

extern "C" void HText_setCaption(HText *text, const char *data)
{
  if ( !text || !text->table )
     return;
  text->table->caption_buf.clear();
  text->table->caption_buf.put( data );
}

extern "C" void HText_row(HText *text)
{
  if ( !text || !text->table )
     return;
  text->flush(0);
  if ( text->row )
   {
     if ( text->cell )
	text->row->append( text->cell );
     text->table->insert( text->row );
     text->cell=0;
   }
  text->row = new TableRow;
}

extern "C" void HText_cell(HText *text, int rowspan, int colspan, const char *align, BOOL isHeader)
{
  if ( !text || !text->table )
     return;
  text->flush(0);
  if ( !text->row )
     HText_row( text );
  if ( text->cell )
     text->row->append( text->cell );
  int al = isHeader ? 1 : 0;
  if (!align)
    ;
  else if (!strncasecmp(align, "left", 4))
    al=0;
  else if (!strncasecmp(align, "center", 6))
    al=1;
  else if (!strncasecmp(align, "right", 5))
    al=2;
  text->cell = new TableData( rowspan, colspan, al, isHeader );
}

struct ColumnInfo
{
  short minsize;
  short maxsize;
  short size;
//  TableData *current;
  ColumnInfo() : minsize(0), maxsize(0), size(0)/*, current(0)*/ {}
};


extern "C" void HText_beginFrame(HText *text, const char *src, const char *name, int noresize)
{
  if ( !text->win )
     return;
  W3Win *w = new W3Win( src );
  text->win->beginFrame( w, name, noresize );
}

extern "C" void HText_beginFrameset(HText *text, const char *cols, const char *rows)
{
  if ( text->win )
     text->win->beginFrameset( cols, rows );
}

extern "C" void HText_endFrameset(HText *text)
{
  if ( text->win )
     text->win->endFrameset();
}

int WordElement::width( int recommend_width )
{
  return strlen(str);
}

getobj *WordElement::insertTo( HText *text, int index, int recommend_width )
{
  text->PUTS(str);
  return 0;
}

int BreakElement::width( int recommend_width )
{
  return -1;
}

getobj *BreakElement::insertTo( HText *text, int index, int recommend_width )
{
  text->PUTS("<BR>");
  return 0;
}

AnchorElement::AnchorElement( HTChildAnchor *Anchor, const char *s, const char *a_target ) :
		anchor(Anchor), buf(0), text(0), offset(0), max_word_width(0),
		bindList(0)
{
  empty = 0;
  anchor_target = a_target ? strdup( a_target ) : 0;
  if ( !s )
     return;
  buf = (char*)malloc(1024);
  text = (char**)calloc( 1, sizeof(char*) );
  for( int i=0, w_count=0; 1; s++ )
   {
     switch( *s )
      {
	case 0:
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	   if ( i <= 0 )
	      break;
	   buf[i++] = 0;
	   text = (char**)realloc( text, (w_count + 2) * sizeof(char*) );
	   text[ w_count++ ] = strdup( buf );
	   text[ w_count ] = 0;
	   if ( i > max_word_width )
	      max_word_width = i;
	   i=0;
	   break;
	default:
	   buf[i++] = *s;
      }
     if ( *s == 0 || i > 1023 )
	break;
   }
  if ( buf )
   {
     ::free( buf );
     buf = 0;
   }
}

AnchorElement::~AnchorElement()
{
  if ( buf )
   {
     ::free( buf );
     buf = 0;
   }
  if ( text )
   {
     for( char **s=text; *s; s++ )
	::free(*s);
     ::free( text );
     text = 0;
   }
  if ( anchor_target )
   {
     ::free( anchor_target );
     anchor_target = 0;
   }
}

int AnchorElement::width( int recommend_width )
{
  if ( !text )
     return 0;
  if ( recommend_width < 0 )
     return max_word_width;

  int w=0;
  for( int i=offset, l=0; 1; i++ )
   {
     if ( !text[i] )
	break;
     l = strlen( text[i] );
     if ( w>0 && (w + 1 + l > recommend_width) )
	break;
     if ( w > 0 )
	w++;
     w += l;
   }
  return w;
}

getobj *AnchorElement::insertTo( HText *htext, int index, int recommend_width )
{
  if ( !htext->win )
     return 0;
  long y; short x;
  htext->win->getPos( y, x );
  if ( !buf )
     buf = (char*)malloc( 512 );
  buf[0] = 0;
  if ( text )
   {
     int w=0;
     for( int l=0; 1; l++ )
      {
	if ( !text[offset] )
	 {
	   empty = 1;
	   break;
	 }
	l = strlen( text[offset] );
	if ( w>0 && (w + 1 + l > recommend_width) )
	   break;
	if ( w > 0 )
	 {
	   w++;
	   strcat( buf, " " );
	 }
	w += l;
	strcat( buf, text[offset++] );
      }
   }
  AnchorHyper *a = new AnchorHyper( y, x, buf, anchor, HTAnchor_parent(htext->win->node_anchor), anchor_target );
  if ( !bindList )
     bindList = new Collection( 5, 5 );
  bindList->insert( a );
  a->bindList = bindList;
  if ( bindList->getCount() > 0 && ((getobj*)bindList->at(0))->bind_flag )
     a->bind_flag = 1;

  htext->PUTS( buf );
  htext->win->insert( a, index );
  if ( buf )
   {
     ::free( buf );
     buf = 0;
   }
  return (getobj*)a;
}

int CheckElement::width( int recommend_width )
{
  return 3;
}

getobj *CheckElement::insertTo( HText *htext, int index, int recommend_width )
{
  if ( !htext->win )
    return 0;
  long y; short x;
  htext->win->getPos(y, x);
  Point p(y,x);
  el->setRect( p );
  htext->win->insert( el, index );
  htext->PUTS( el->getLabel() );
  return (getobj*)el;
}

int RadioElement::width( int recommend_width )
{
  return strlen( el->getLabel() );
}

getobj *RadioElement::insertTo( HText *htext, int index, int recommend_width )
{
  if (!htext->win)
    return 0;
  long y; short x;
  htext->win->getPos(y, x);
  Point p(y,x);
  el->setRect( p );
  htext->win->insert( el, index );
  htext->PUTS( el->getLabel() );
  return (getobj*)el;
}

int InputLineElement::width( int recommend_width )
{
  return il->getWidth();
}

getobj *InputLineElement::insertTo( HText *text, int index, int recommend_width )
{
  if (!text->win)
    return 0;
  long y; short x;
  text->win->getPos(y, x);
  Point p(y,x);
  il->setRect( p );
  text->win->insert( il, index );
  for( int i = il->getWidth(); i>0; i-- )
     text->PUTC(' ');
  return (getobj*)il;
}

int StyleElement::width( int recommend_width )
{
  return 0;
}

getobj *StyleElement::insertTo( HText *text, int index, int recommend_width )
{
    if ( style->onlyColor )
     {
       if (style->onlyColor & CHANGE_COLOR)
	  text->color = style->color;
       text->subStyle = style;
     }
    else
     {
       text->style = style;
       text->subStyle = 0;
       text->color = style->color;
     }
  return 0;
}

int FormSubmitElement::width( int recommend_width )
{
  const char *label = bt->getLabel();
  return label ? strlen( label ) : 0;
}

getobj *FormSubmitElement::insertTo( HText *htext, int index, int recommend_width )
{
  if (!htext->win)
    return 0;
  long y;
  short x;
  htext->win->getPos(y, x);
  Point p(y,x);
  bt->setRect( p );
  htext->win->insert( bt, index );
  htext->PUTS( bt->getLabel() );
  return (getobj*)bt;
}

int FormResetElement::width( int recommend_width )
{
  const char *label = bt->getLabel();
  return label ? strlen( label ) : 0;
}

getobj *FormResetElement::insertTo( HText *htext, int index, int recommend_width )
{
  if (!htext->win)
    return 0;
  long y;
  short x;
  htext->win->getPos(y, x);
  Point p(y,x);
  bt->setRect( p );
  htext->win->insert( bt, index );
  htext->PUTS( bt->getLabel() );
  return (getobj*)bt;
}

TableElement::~TableElement()
{
  if ( tb )
     delete tb;
}

int TableElement::width( int recommend_width )
{
  return tb->width() + 2;
}

void TableElement::setWidth( int new_width )
{
  if ( !tb )
     return;
  tb->setWidth( new_width );
}

getobj *TableElement::insertTo( HText *htext, int index, int recommend_width )
{
  if ( htext->win && !tb->printRow( htext ) )
     empty = 1;
  return 0;
}

ListBoxElement::ListBoxElement( FormListBox *_el ) : el(_el)
{
  break_before=1;
  lines = el->rect.b.y - el->rect.a.y + 1;
  el_width = el->getWidth();
  empty = 0;
}

int ListBoxElement::width( int recommend_width )
{
  return el_width;
}

getobj *ListBoxElement::insertTo( HText *text, int index, int recommend_width )
{
  if ( !text->win )
     return 0;
  getobj *g = 0;
  if ( el )
   {
     long y; short x;
     text->win->getPos(y, x);
     Point p(y,x);
     el->setRect( p );
     text->win->insert( el, index );
     g = el;
     el = 0;
   }
  for( int i=el_width; i > 0; i-- )
     text->PUTC(' ');
  if ( --lines <= 0 )
     empty = 1;
  return g;
}

EditElement::EditElement( FormEdit *_el ) : el(_el)
{
  break_before=1;
  lines = el->rect.b.y - el->rect.a.y + 1;
  el_width = el->getWidth();
  empty = 0;
}

int EditElement::width( int recommend_width )
{
  return el_width;
}

getobj *EditElement::insertTo( HText *text, int index, int recommend_width )
{
  if ( !text->win )
     return 0;
  getobj *g = 0;
  if ( el )
   {
     long y; short x;
     text->win->getPos(y, x);
     Point p(y,x);
     el->setRect( p );
     text->win->insert( el, index );
     g = el;
     el = 0;
   }
  for( int i=el_width; i > 0; i-- )
     text->PUTC(' ');
  if ( --lines <= 0 )
     empty = 1;
  return g;
}

// external HText interface ][ internal HText interface

int _HText::rightMargin()
{
  int r_indent = (int)(style->rightIndent > 0 ? style->rightIndent : 2);
  int margin = (win ? win->getWidth() : 80) - r_indent;
  if ( margin <= style->leftIndent + 8 )
    margin = (int)style->leftIndent + 8;
  if ( margin <= style->indent1st + 8 )
    margin = (int)style->indent1st + 8;
  return margin;
}


void _HText::endOption()
{
  if ( option && optpos <= 0 && buflen > 0 )
    { PUTBUF(); buflen=0; }
  if ( !sellist || !option || !selval )
     return;
  for( int i=optpos-1; i>0 && option[i]==' '; option[i--]=0 );
  sellist->insert(option);
  option = 0;
  selval->insert(option_value);
  if ( selstate )
     selstate->insert( new int( option_state ? 1 : 0 ) );
  option_value=0;
}


void _HText::PUTC(char ch, int graph)
{
//  if (target)
//    targetClass.put_character(target, ch);
  if ( area )
   {
     charToArea( ch );
     return;
   }
  if ( option )
   {
     if (ch=='\n')
       ch=' ';
     if ( ch == ' ' && !optpos )
	return;
     int npos = optpos+1;
     if ( npos >= optlen )
      {
	optlen = npos *3/2;
	option=(char*)realloc(option, optlen);
      }
     option[optpos++]=ch;
     option[optpos]=0;
     return;
   }
  if ( win )
     win->put( ch, color, graph );
  if ( ch != '\n' )
     curx++;
}

void _HText::PUTS(const char *str)
{
  if (area || option)
   {
     while( *str)
       PUTC( *str++);
     return;
   }
//  if (target)
//    targetClass.put_string(target, str);
  if (win)
    win->put( str, -1, color );
  curx+=strlen(str);
}

void _HText::PUTBUF()
{
  if ( buflen < 0 )
    {
      puts( "Internal error." );
      exit(111);
    }
  buf[buflen]=0;
  if (area || option)
   {
     char *str=buf;
     while( *str)
      PUTC( *str++);
     return;
   }
  //if (target)
//    targetClass.put_string(target, buf);
  if (win)
    win->put( buf, -1, color );
  curx+=buflen;
}

void _HText::charToArea( char ch )
{
  if ( areapos+1 >= arealen )
   {
     arealen += 64;
     area = (char*)realloc( area, arealen );
   }
  area[areapos++] = ch;
}

// flush word buffer
void _HText::flush(char ch)
{
  waitDigit=0;

  if ( area )
   {
     charToArea( ch );
     return;
   }

  if ( cell || option )
   {
       if ( ch=='\t' )
	  ch=' ';
       if ( ch )
	 buf[buflen++] = ch;
       buf[buflen] = 0;
       if ( !option )
	{
	  if ( buflen )
	    cell->append( new WordElement( strdup(buf) ) );
	  spaceCount=0;
	  buflen=0;
	  newPara = 0;
	  wasBreak = 0;
	  softPara = 0;
	}
       return;
   }

  if (!buflen)
   {
       if (spaceCount>0 && ch!=' ')
	 PUTC(' ');
       spaceCount=0;
       if ( ch )
	 PUTC(ch);
       if ( lateBreak && !newPara )
	 new_line();
       newPara = 0;
       softPara=0;
       return;
   }

  int right=rightMargin();
  if ( buflen + curx > right )
     new_line();

  softPara=0;

  PUTBUF();

  if ( spaceCount>0 && ch!=' ' )
     PUTC( ' ' );

  if ( ch )
     PUTC( ch );

  buflen=0;
  if ( lateBreak )
    new_line();
  else
    spaceCount=0;
  newPara = 0;
  wasBreak = 0;
}

void HText::new_line()
{
   _new_line();
   lateBreak = 0;
   if (style->freeFormat)
     spaceCount = -1;
   else
     spaceCount = 0;
   newPara = 0;
   int i=(softPara ? (int)style->indent1st:(int)style->leftIndent)+shift;
   for( ; i; i--)
     PUTC(' ');
   wasBreak=1;
}

void HText::_new_line()
{
   waitDigit = 0;
   PUTC('\n');
   cury++; curx=0;
}

void HText::New_line()
{
   if ( newPara )
      return;
   spaceCount=0;
   lateBreak=1;
   flush(0);
}

void HText::new_paragraph( int isFirst )
{
   if ( newPara || softPara )
     return;
   if (!wasBreak)
    {
      flush(0);
      int n=(int)style->spacePara;
      if (n<1)
       n=1;
       while( n-- )
	{ PUTC('\n'); cury++; }
      curx=0;
    }
   else
    {
      curx=0;
      if (win)
	win->put(cury, curx, "");
    }
   int indent=(int)style->leftIndent;
   if( isFirst )
      indent=(int)style->indent1st;
   for( int i=indent+shift; i; i-- )
     PUTC(' ');

   if (style->freeFormat)
     spaceCount=-1;
   else
     spaceCount = 0;
   softPara = 0;
   lateBreak = 0;
   wasBreak = 1;
   waitDigit = 0;
   newPara = 1 ;
}

void HText::newIndent()
{
   curx=0;

   if(win)
     win->put(cury, curx, "", -1, color);
   for( int i=(int)style->leftIndent+shift ; i; i-- )
     PUTC(' ');
   if (style->freeFormat)
     spaceCount=-1;
   else
     spaceCount = 0;
}

// internal HText interface ]
// class TableData

TableData::TableData( int r_span, int c_span, int Align, int Header ) :
		first_obj(0), last_obj(0),
		cols(0), rowspan(r_span), align(Align),
		isHeader(Header), style(0), parent(0),
		hor_line(1), add_count(r_span-1)

{
  static int TD_ID=0;	// у каждой ячейки есть свой уникальный номер
  id = ++TD_ID;
  if ( rowspan < 1 )
     rowspan = 1;
  if ( c_span < 1 )
     c_span = 1;
  cols = new Collection( c_span, 0 );
  for( ; c_span > 0 ; c_span-- )
      cols->insert(0);
}

TableData::~TableData()
{
  freeAll();
  if ( cols )
   {
     cols->removeAll();
     delete cols;
   }
}

// class Table

Table::Table( BOOL Border, Table *prev_t, TableRow *prev_r, TableData *prev_c ) :
	min_width(0), max_width(0), tbl_width(0),
	line(0), lock_id(0), hor_flag(0), tr_prev(0), caption(0),
	border(Border), prev_table(prev_t),
	prev_row(prev_r), prev_cell(prev_c), parent(0)
{
}

Table::~Table()
{
  freeAll();
  while( columns.getCount() > 0 )
   {
     delete (ColumnInfo*)columns.at(0);
     columns.atRemove(0);
   }
}

void Table::init()
{
  TableRow *tr=0, *tr2=0;
  TableData *td=0, *td2=0;
  ColumnInfo *ci=0;
  int i=0, j=0, k=0, w=0, col_count=0;
  // заполнение списка столбцов
  for( i=0; i < count; i++ )
   {
     tr = (TableRow*)items[i];
     col_count=0;
     for( j=tr->first(); j; j=tr->next() )
      {
	td = (TableData*)tr->getCurrent();
	if ( td->parent )	// фиктивная ячейка
	 {
	   td = td->parent;
	   col_count += td->cols->getCount();
	 }
	else if ( td->cols ) {
	// заполнение списка охватываемых столбцов для текущей ячейки
	for( k=0; k < td->cols->getCount(); k++ )
	 {
	   while( col_count >= columns.getCount() )
	      columns.insert( new ColumnInfo );
	   ci = (ColumnInfo*)columns.at( col_count++ );
	   td->cols->atPut( k, ci );
	 }
	}
	// вставка фиктивной ячейки в следующую строку таблицы, если надо
	if ( td->add_count > 0 && i+1 < count )
	 {
	   td2 = new TableData;
	   td2->parent = td;
	   td2->hor_line = 0;
	   tr2 = (TableRow*)items[i+1];
	   tr2->first();
	   if ( td->cols && col_count <= td->cols->getCount() )
	      tr2->insertBefore( td2 );
	   else
	    {
	      for( int l=1; 1; )
	       {
		 TableData *td3 = (TableData*)tr2->getCurrent();
		 if ( td3->parent )
		    td3 = td3->parent;
		 l += td3->cols ? td3->cols->getCount() : 1;
		 if ( l >= col_count || !tr2->next() )
		    break;
	       }
	      tr2->insert( td2 );
	    }
	   td->add_count--;
	 }
	if ( td != (TableData*)tr->getCurrent() )	// была фиктивная ячейка
	   continue;
	if ( !ci || td->cols && td->cols->getCount() > 1 )	// ячейка охватывает два или более столбцов
	   continue;

	// корректируем минимальный и максимальный размеры столбца
	int maxsize=0, max=0;
	for( w=0, k=td->first(); k; k=td->next() )
	 {
	   w = ((Element*)td->getCurrent())->width();
	   if ( w > ci->minsize )
	      ci->minsize = w;
	   if ( w < 0 )
	    {
	      if ( maxsize < max )
		 maxsize = max;
	      max = 0;
	    }
	   else if ( w > 0 )
	    {
	      w = ((Element*)td->getCurrent())->width( INT_MAX );
	      if ( max > 0 )
		 max++;
	      max += w;
	    }
	 }
	if ( maxsize < max )
	   maxsize = max;
	if ( maxsize > ci->maxsize )
	   ci->maxsize = maxsize;
      }
   }

  // корректировка минимальных и максимальных размеров столбцов
  // с учетом ячеек, охватывающих несколько столбцов
  for( i=0; i < count; i++ )
   {
     tr = (TableRow*)items[i];
     for( j=tr->first(); j; j=tr->next() )
      {
	td = (TableData*)tr->getCurrent();
	if ( td->parent || td->cols->getCount() < 2 )
	   continue;
	int common_minsize=0, common_maxsize=0, maxsize=0;
	for( k=td->cols->getCount()-1; k >= 0; k-- )
	 {
	   if ( common_maxsize > 0 )
	      common_maxsize++;
	   if ( common_minsize > 0 )
	      common_minsize++;
	   ci = (ColumnInfo*)td->cols->at(k);
	   common_minsize += ci->minsize;
	   common_maxsize += ci->maxsize;
	 }
	for( w=0, k=td->first(); k; k=td->next() )
	 {
	   w = ((Element*)td->getCurrent())->width();
	   if ( w > common_minsize )
	    {
	      int delta = w - common_minsize;
	      ci->minsize += delta;
	      common_minsize += delta;
	    }
	   if ( w > 0 )
	    {
	      if ( maxsize > 0 )
		 maxsize++;
	      maxsize += w;
	      if ( maxsize > common_maxsize )
	       {
		 int delta = maxsize - common_maxsize;
		 ci->maxsize += delta;
		 common_maxsize += delta;
	       }
	    }
	 }
      }
   }

  // вычисление размеров таблицы
  min_width=0;
  max_width=0;
  for( i=0; i < columns.getCount(); i++ )
   {
     ci = (ColumnInfo*)columns.at(i);
     ci->size = ci->minsize;
     if ( min_width > 0 )
	min_width++;
     if ( max_width > 0 )
	max_width++;
     min_width += ci->minsize;
     max_width += ci->maxsize;
   }
  tbl_width = min_width;

  // вставка недостающих ячеек в конце каждой строки
  for( i=0; i < count; i++ )
   {
     tr = (TableRow*)items[i];
     k = 0;
     for( j=tr->first(); j; j=tr->next() )
      {
	td = (TableData*)tr->getCurrent();
	if ( td->parent )
	   td = td->parent;
	k += td->cols->getCount();
      }
     for( ; k < columns.getCount(); k++ )
      {
	td = new TableData( 1, 1 );
	ci = (ColumnInfo*)columns.at( k );
	td->cols->atPut( 0, ci );
	tr->append( td );
      }
   }

  if ( parent && count > 0 )
     parent->empty = 0;

  caption = caption_buf.str();
}

void Table::setWidth( int new_width )
{
  int i=0, j=0;
  ColumnInfo *ci=0;
  new_width -= 2;
  tbl_width = min_width;
  if ( new_width < 0 )
     new_width = 0;
  if ( max_width < new_width )
   {
     for( i=0; i < columns.getCount(); i++ )
      {
	ci = (ColumnInfo*)columns.at(i);
	ci->size = ci->maxsize;
      }
     tbl_width = max_width;
   }
  else if ( tbl_width < new_width )
   {
     // увеличение ширины каждого столбца с ненулевой шириной
     int zero_columns=0;
     for( i=0; i < columns.getCount(); i++ )
	if ( ((ColumnInfo*)columns.at(i))->maxsize == 0 )
	   zero_columns++;
     short f = (new_width - tbl_width) / (columns.getCount() - zero_columns);
     for( i=0; i < columns.getCount(); i++ )
      {
	ci = (ColumnInfo*)columns.at(i);
	if ( ci->maxsize == 0 )
	   continue;
	ci->size = ci->minsize + f;
	tbl_width += f;
      }
     for( i=0, j=new_width-tbl_width; i < j; i++ )
      {
	ci = (ColumnInfo*)columns.at(i);
	if ( ci->maxsize == 0 )
	   continue;
	ci->size++;
	tbl_width++;
      }
   }
  else for( i=0; i < columns.getCount(); i++ )
   {
     ci = (ColumnInfo*)columns.at(i);
     ci->size = ci->minsize;
   }

  TableRow *tr=0;
  TableData *td=0;
  Element *el=0;
  for( i=0; i < count; i++ )
   {
     tr = (TableRow*)items[i];
     for( j=tr->first(); j; j=tr->next() )
      {
	td = (TableData*)tr->getCurrent();
	if ( td->parent )
	   continue;
	// вычисляем ширину ячейки
	int j=0, k=0, td_width=0;
	for( k=td->cols->getCount()-1; k >= 0; k-- )
	 {
	   if ( td_width > 0 )
	      td_width++;
	   td_width += ((ColumnInfo*)td->cols->at(k))->size;
	 }
	for( j=td->first(); j; j=td->next() )
	 {
	   el = (Element*)td->getCurrent();
	   el->setWidth( td_width );
	 }
      }
   }
}

void Table::print( HText *text )
{
  do {
     text->new_line();
  } while( printRow( text ) );
  text->new_line();
}

int Table::printRow( HText *text )
{
  // печатаем заголовок таблицы
  if ( caption )
   {
     int i=strlen(caption), j=0, k=0, sp=0, sp_count=0;
     if ( i > tbl_width+2 )
	for( i=tbl_width+2; i >= 0 && caption[i] != ' ' && caption[i] != '\t'; i-- );
     if ( i <= 0 )
	i = tbl_width+2;
     else for( j=0, k=(tbl_width+2-i)/2; j < k; j++ )
	text->PUTC(' ');
     for( j=0; *caption && (!parent || j < i+sp_count); j++, caption++ )
      {
	switch( *caption )
	 {
	   case '\r':
	       continue;
	   case '\t':
	   case '\n':
	       *caption=' ';
	   case ' ':
	       if ( sp > 0 )
		{
		  sp_count++;
		  continue;
		}
	       sp++;
	       break;
	   default:
	       sp = 0;
	 }
	text->PUTC( *caption );
      }
     j -= sp_count;
     for( ; *caption && (*caption == ' ' || *caption == '\t'); caption++ );
     for( j+=k-1; j <= tbl_width; j++ )
	text->PUTC(' ');
     if ( !*caption )
	caption = 0;
     return 1;
   }

  if ( count <= 0 || line > count )
     return 0;

  char ch=0;
  int k=0, w=0, first_flag=1, col_count=0, lock_td_flag=0;
  TableData *td=0, *td2=0;
  ColumnInfo *ci=0;
  Element *el;
  TableRow *tr = (TableRow*)items[ line < count ? line : line-1 ];

  hor_flag=0;
  tr->first();

  HTStyle *style = text->style;
  HTStyle *subStyle = text->subStyle;
  HTColor color = text->color;

  while( 1 )
   {
     if ( !(td = (TableData*)tr->getCurrent()) )
	return 0;		// таблица пустая
     if ( td->parent )		// ячейка фиктивная
	td = td->parent;
     if ( line >= count )	// печать нижней границы таблицы
	td->hor_line = 1;

     td2 = 0;

     // поиск в верхней строке ячейки в зтом же столбце
     if ( tr_prev && tr != tr_prev )
      {
	int cycle = 1;
	for( k=tr_prev->first(); cycle && k; k=tr_prev->next() )
	 {
	   td2 = (TableData*)tr_prev->getCurrent();
	   if ( td2->parent )
	      td2 = td2->parent;
	   if ( td == td2 )
	      break;
	   for( col_count=0; cycle && col_count < td2->cols->getCount(); col_count++ )
	    {
	      ci = (ColumnInfo*)td2->cols->at(col_count);
	      for( w=0; w < td->cols->getCount(); w++ )
		if ( ci == (ColumnInfo*)td->cols->at(w) )
		  { cycle = 0; col_count--; break; }	// ячейка найдена
	    }
	 }
	if ( cycle )
	   td2 = 0;
      }

     // вывод символа - разделителя ячеек
     if ( border == NO )
	ch = ' ';
     else if ( td->hor_line )
      {
	if ( line >= count )
	   ch = tbl_border[ first_flag ? 2 : 3 ];
	else if ( !tr_prev )
	   ch = tbl_border[ first_flag ? 8 : 9 ];
	else if ( hor_flag==2 )
	   ch = tbl_border[ td2 && col_count<=0 ? 6 : 9 ];
	else
	   ch = tbl_border[ 5 ];
      }
     else if ( hor_flag == 2 )
	ch = tbl_border[7];
     else
	ch = tbl_border[1];

     text->style = style;
     text->subStyle = subStyle;
     text->color = color;

     text->PUTC( ch, 1 );

     first_flag = 0;
     if ( hor_flag == 2 )
	hor_flag = 1;

     // вычисляем ширину ячейки
     int td_width=0;
     for( k=td->cols->getCount()-1; k >= 0; k-- )
      {
	if ( td_width > 0 )
	   td_width++;
	td_width += ((ColumnInfo*)td->cols->at(k))->size;
      }

     w = 0;
     if ( td->hor_line )	// вывод горизонтального разделителя
      {
	td->hor_line = 0;
	int w2=0, flag=1;
	for( ; w < td_width; w++ )
	 {
	   if ( border == NO )
	      ch = ' ';
	   else
	    {
	      ch = tbl_border[0];
	      if ( td2 )	// вычисление пересечения с ячейками верхней строки
	       {
		 if ( flag )
		  {
		    flag = 0;
		    for( ; col_count < td2->cols->getCount(); col_count++ )
		     {
		       if ( w2 > 0 )
			  w2++;
		       w2 += ((ColumnInfo*)td2->cols->at(col_count))->size;
		     }
		  }
		 if ( w == w2 )	// пересечение с ячейками в верхней строке
		  {
		    ch = tbl_border[3];
		    td2 = (TableData*)tr_prev->getCurrent();
		    if ( td2 && td2->parent )
		       td2 = td->parent;
		    col_count = 0;
		    flag = 1;
		  }
	       }
	    }
	   text->PUTC( ch, 1 );
	 }
	hor_flag = 2;
	if ( !tr->next() )	// конец строки
	 {
	   ch = ' ';
	   if ( border == YES )
	      ch = tbl_border[ line >= count ? 4 : (tr_prev ? 7 : 10) ];
	   text->PUTC( ch, 1 );
	   if ( line < count )
	      return 1;
	   line++;
	   return 0;
	 }
	continue;
      }

     int draw_len=0;	// длина выводимого в текущей строке
     int first_flag2=1, el_w=0, all_w=0;
     for( k=td->first(); k; k=td->next() )
      {
	el = (Element*)td->getCurrent();
	all_w = draw_len + (draw_len > 0 ? 1 : 0);
	el_w = el->width( td_width - all_w );
	if ( el->break_before && !first_flag2 || el_w < 0 || all_w + el_w > td_width )
	   break;
	if ( el_w > 0 )
	 {
	   if ( draw_len > 0 )
	      draw_len++;
	   draw_len += el_w;
	   first_flag2 = 0;
	 }
      }

     int spaces = 0;
     switch( td->align )
      {
	case 1:	// выравнивание по центру
	    spaces = (td_width - draw_len) / 2;
	    break;
	case 2:	// выравнивание по правому краю
	    spaces = td_width - draw_len;
	    break;
      }
     for( int s=spaces; s > 0; text->PUTC(' '), s-- );

     // печать текущей строки в текущей ячейке
     if ( td->style )
      {
	text->style = td->style;
	text->subStyle = td->subStyle;
	text->color = td->color;
      }
     first_flag2 = 1;
     for( td->first(); 1; )
      {
	if ( tr->td_with_obj == td )	// блокировка указателя на первую ячейку с get-объектом
	   lock_td_flag = 1;
	if ( !(el = (Element*)td->getCurrent()) )
	 {
	   if ( lock_id == td->id )	// вывод закончен, снятие блокировки
	      lock_id = 0;
	   break;
	 }
	if ( !first_flag2 && el->break_before )
	 {
	   el->break_before = 0;
	   if ( td->rowspan <= 1 )
	      lock_id = td->id;
	   break;
	 }
	all_w = spaces + w + (w>0 ? 1 : 0);
	el_w = el->width( td_width - all_w );
	if ( el_w < 0 || spaces + w + (w>0 && el_w>0 ? 1 : 0) + el_w > td_width )
	 {
	   if ( el_w < 0 )
	      td->free_cur();	// удаляем обработанный злемент
	   if ( td->getCurrent() )
	    {
	      if ( td->rowspan <= 1 )
		 lock_id = td->id;	// блокировка до конца вывода содержимого ячейки
	    }
	   else if ( lock_id == td->id )	// вывод закончен, снятие блокировки
	      lock_id = 0;
	   break;
	 }
	if ( el_w > 0 )
	 {
	   if ( w > 0 )
	      { w++; text->PUTC(' '); }
	   w += el_w;
	 }

	int index = -1;
	if ( !lock_td_flag && tr->td_with_obj && tr->td_with_obj->first_obj )
	   index = tr->td_with_obj->first_obj->index;
	else if ( td->last_obj )
	   index = td->last_obj->index + 1;

	getobj *g = el->insertTo( text, index, td_width - all_w );
	if ( g )
	 {
	   td->last_obj = g;
	   if ( !td->first_obj )
	      td->first_obj = g;
	   if ( !tr->td_with_obj || !lock_td_flag )
	    {
	      tr->td_with_obj = td;
	      lock_td_flag = 1;
	    }
	 }

	first_flag2 = 0;
	if ( el->empty )
	   td->free_cur();		// удаляем обработанный злемент
	else if ( parent )
	 {
	   if ( td->rowspan <= 1 )
	      lock_id = td->id;	// блокировка до конца вывода содержимого ячейки
	   break;
	 }
      }
     // заполнение пробелами до конца ячейки
     for( w+=spaces; w < td_width; w++, text->PUTC(' ') );

     td->style = text->style;
     td->subStyle = text->subStyle;
     td->color = text->color;

     if ( tr->next() )	// НЕ конец строки в таблице
	continue;

     text->style = style;
     text->subStyle = subStyle;
     text->color = color;

     if ( border == YES )
	text->PUTC( tbl_border[1], 1 );

     if ( !hor_flag && !lock_id )	// переход на следующую строку в таблице
      {
	tr_prev = tr;
	line++;
	for( k=tr->first(); k; k=tr->next() )
	 {
	   td = (TableData*)tr->getCurrent();
	   if ( td->parent )
	      td = td->parent;
	   td->rowspan--;
	 }
      }
     break;
   }
  return 1;
}

