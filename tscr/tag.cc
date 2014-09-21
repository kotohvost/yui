/*
	$Id: tag.cc,v 3.2.2.2 2007/07/24 11:28:12 shelton Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "tag.h"

TagProcessor::TagProcessor( int b_chars ) : first(0), next(0), prev(0),
		base_chars(b_chars), packed(0), start(0),
		end(b_chars), prev_symbol(0), state(TAG_NONE)
{
}

TagProcessor::~TagProcessor()
{
  clear();
}

void TagProcessor::clear()
{
  if ( !first )
     return;
  for( int i=0, j=end-start; i < j; i++ )
    if ( first[i].chain )
      freeChain( first[i].chain, first[i].start, first[i].end );
  reset();
}

void TagProcessor::freeChain( struct TagElement *chain, unsigned char start, unsigned char end )
{
  if ( !chain )
     return;
  for( int i=0, j=end-start; i < j; i++ )
    if ( chain[i].chain )
       freeChain( chain[i].chain, chain[i].start, chain[i].end );
  ::free( chain );
}

void TagProcessor::pack()
{
  if ( packed || !first )
     return;
  for( int i=0, flag=1; i < base_chars; i++ )
   {
     if ( first[i].chain )
      {
	if ( flag )
	  { start=i; flag=0; }
	end = i+1;
      }
   }
  if ( start >= 0 && end < base_chars )
     packChain( first, (unsigned char)start, (unsigned char)end );
  packed = 1;
  reset();
}

void TagProcessor::packChain( struct TagElement *&chain, unsigned char start, unsigned char end )
{
  if ( !chain || start >= end )
     return;
  memmove( chain, chain + start, (end - start)*sizeof(struct TagElement) );
  chain = (struct TagElement*)realloc( chain, (end - start)*sizeof(struct TagElement) );
  for( int i=0, j=end-start; i < j; i++ )
    if ( chain[i].chain )
      packChain( chain[i].chain, chain[i].start, chain[i].end );
}

void TagProcessor::insertTag( TagInfo *tag )
{
  int name_len = 0;
  if ( !tag || !tag->name )
     return;
  if ( !first )
     first=(struct TagElement*)calloc( base_chars, sizeof(struct TagElement) );
  next = first;
  prev = 0;
  for( int i=0, ch=0; tag->name[i]; i++ )
   {
     if ( !tagSymbol( (ch=tag->name[i]) ) )
	continue;
     if ( !next[ch].chain && tag->name[i+1] )
      {
	next[ch].chain = (struct TagElement*)calloc( base_chars, sizeof(struct TagElement) );
	next[ch].start = 0xff;
	next[ch].end = 0;
      }
     if ( prev )
      {
	if ( prev->start > ch )
	   prev->start = ch;
	if ( prev->end <= ch )
	   prev->end = ch+1;
      }
     prev = &next[ch];
     next = next[ch].chain;
     name_len++;
   }
  if ( prev )
   {
     prev->param = tag->param ? 1 : 0;
     prev->type = tag->type;
     prev->finish = 1;
     prev->hend = tag->hend;
     prev->name_len = name_len;
   }
}

int TagProcessor::checkTag( int ch, int &found_type, int &name_len )
{
  found_type = -1;
  if ( !first )
      return TAG_NONE;
  if ( tagSymbol( ch ) ) {
      if ( prev_symbol && state == TAG_NONE )
	 return TAG_NONE;
      register int s = (next==first ? start : prev->start);
      register int e = (next==first ? end : prev->end);
      register int i = ch>=s && ch<e ? ch - s : -1;
      prev_symbol = 1;
      if ( i < 0 ) {
	  reset();
	  return TAG_NONE;
      }
      prev = &next[i];
      next = next[i].chain;
      int ret = (state == TAG_NONE ? TAG_START : TAG_NAME);
      state = TAG_NAME;
      return ret;
  }

  prev_symbol = isalnum( ch );

  int ret = TAG_NONE;
  if ( state == TAG_NAME && !prev_symbol && prev && prev->finish ) {
      ret = TAG_FOUND;
      found_type = prev->type;
      name_len = prev->name_len;
  }

  reset();
  return ret;
}

void TagProcessor::reset()
{
  state = TAG_NONE;
  next = first;
  prev = 0;
}

int TagProcessor::tagSymbol( int &ch )
{
  switch( ch )
   {
     case '_':
	 ch -= 95; return 1;
     case 'a'...'z':
	 return (ch-=70);
     case 'A'...'Z':
	 return (ch-=64);
   }
  return 0;
}

int TagProcessor::startTag( int ch )
{
  //return ispunct( ch ) || isspace( ch );
  return 0;
}

int TagProcessor::endTag( int ch )
{
  //return ispunct( ch ) || isspace( ch );
  return 0;
}

/*----------------------- HTML ------------------------------*/

HTMLTagProcessor::HTMLTagProcessor() : TagProcessor( 32 ),
				lock_value(0), flag_slash(0)
{
}

int HTMLTagProcessor::checkTag( int ch, int &found_type, int &name_len )
{
  static int wait_slash = 0;
  int tag_symbol = tagSymbol( ch );
  if ( !tag_symbol && startTag( ch ) )
   {
     reset();
     state = TAG_NAME;
     wait_slash = 1;
     return TAG_START;
   }
  switch( state )
   {
     case TAG_NAME:
	 if ( tag_symbol )
	  {
	    if ( !next )
	      { reset(); break; }
	    int s = (next==first ? start : prev->start);
	    int e = (next==first ? end : prev->end);
	    int i = ch>=s && ch<e ? ch - s : -1;
	    if ( i < 0 )
	     {
//	       prev = next;
//	       next = 0;
	       reset();
	     }
	    else
	     {
	       if ( !next[i].chain )
		  wait_slash=0;
	       prev = &next[i];
	       next = next[i].chain;
	     }
	  }
	 else switch( ch )
	  {
	    case ' ':
	    case '\t':
		if ( prev && prev->param )
		   state = TAG_PARAM_WAIT;
		else
		  { reset(); break; }
	    case '/':
		if ( wait_slash )
		   flag_slash = 1;
		break;
	    default:
		if ( endTag( ch ) )
		   goto tag_found;
		reset();
	  }
	 wait_slash = 0;
	 break;
     case TAG_PARAM_WAIT:
	 if ( !tag_symbol && (ch == ' ' || ch == '\t') )
	    break;
	 if ( endTag( ch ) )
	    goto tag_found;
	 state = TAG_PARAM_NAME;
	 break;
     case TAG_PARAM_NAME:
	 switch( ch )
	  {
	    case '=':
		state = TAG_VAL_WAIT;
		break;
	    case ' ':
	    case '\t':
		if ( !tag_symbol )
		   state = TAG_EQU_WAIT;
		break;
	    default:
		if ( endTag( ch ) )
		   goto tag_found;
	  }
	 break;
     case TAG_EQU_WAIT:
	 switch( ch )
	  {
	    case '=':
		state = TAG_VAL_WAIT;
		break;
	    default:
		if ( endTag( ch ) )
		   goto tag_found;
		state = TAG_PARAM_NAME;
	  }
	 break;
     case TAG_VAL_WAIT:
	 if ( endTag( ch ) )
	    goto tag_found;
	 switch( ch )
	  {
	    case ' ':
	    case '\t':
		if ( !tag_symbol )
		   break;
	    default:
		state = TAG_VAL_VALUE;
		if ( ch == '"' )
		   lock_value = 1;
	  }
	 break;
     case TAG_VAL_VALUE:
	 switch( ch )
	  {
	    case '"':
		if ( lock_value )
		 {
		   lock_value = 0;
		   if ( !tag_symbol )
		    {
		      state = TAG_PARAM_WAIT;
		      return TAG_VAL_VALUE;
		    }
		 }
		break;
	    case ' ':
	    case '\t':
		if ( !lock_value && !tag_symbol )
		   state = TAG_PARAM_WAIT;
		break;
	    default:
		if ( endTag( ch ) )
		   goto tag_found;
	  }
	 break;
   }
  return state;

tag_found:
  int ret = TAG_NONE;
  if ( prev && prev->finish && (!flag_slash || prev->hend) )
   {
     ret = TAG_FOUND;
     found_type = prev->type;
     name_len = prev->name_len;
   }
  reset();
  return ret;
}

void HTMLTagProcessor::reset()
{
  TagProcessor::reset();
  lock_value = flag_slash = 0;
}

int HTMLTagProcessor::startTag( int ch )
{
  return (ch=='<' ? 1 : 0);
}

int HTMLTagProcessor::endTag( int ch )
{
  return (ch=='>' ? 1 : 0);
}

int HTMLTagProcessor::tagSymbol( int &ch )
{
  switch( ch )
   {
     case '1'...'6':
	 ch -= 49; return 1;
	 return 1;
     case 'a'...'z':
	 return (ch-=91);
     case 'A'...'Z':
	 return (ch-=59);
   }
  return 0;
}

/*----------------------- Comment ------------------------------*/

CommentTagProcessor::CommentTagProcessor() : TagProcessor( 5 )
{
}

int CommentTagProcessor::tagSymbol( int &ch )
{
  switch( ch ) {
     case '/':
	 ch = 0; return 1;
     case '*':
	 return (ch = 1);
     case '#':
	 return (ch = 2);
     case ';':
	 return (ch = 3);
     case '!':
	 return (ch = 4);
  }
  return 0;
}

int CommentTagProcessor::checkTag( int ch, int &found_type, int &name_len )
{
  int ret=TAG_NONE;
  found_type = -1;
  if ( !first )
     return ret;
  if ( tagSymbol( ch ) ) {
     if ( prev_symbol && state == TAG_NONE )
	return ret;
     register int s = (next==first ? start : prev->start);
     register int e = (next==first ? end : prev->end);
     register int i = ch>=s && ch<e ? ch - s : -1;
     if ( i < 0 ) {
	 reset();
	 return ret;
     }
     prev_symbol = 1;
     prev = &next[i];
     if ( prev && prev->finish ) {
	 ret = TAG_FOUND;
	 found_type = prev->type;
	 name_len = prev->name_len;
	 reset();
     } else {
	 next = next[i].chain;
	 ret = (state == TAG_NONE ? TAG_START : TAG_NAME);
	 state = TAG_NAME;
     }
  } else {
     prev_symbol = 0;
     reset();
  }
  return ret;
}

#ifdef EXAMPLE

void print_tag( const char *name )
{
   printf( name );
   printf( "\n" );
}

TagInfo HTMLTags[] =
{
	{ "a",		1,	print_tag },
	{ "address",	0,	print_tag },
	{ "applet",	1,	print_tag },
	{ "area",	1,	print_tag },
	{ "b",		0,	print_tag },
	{ "base",	1,	print_tag },
	{ "basefont",	1,	print_tag },
	{ "bgsound",	1,	print_tag },
	{ "big",	0,	print_tag },
	{ "blockquote",	0,	print_tag },
	{ "body",	1,	print_tag },
	{ "br",		1,	print_tag },
	{ "caption",	1,	print_tag },
	{ "center",	0,	print_tag },
	{ "cite",	0,	print_tag },
	{ "code",	0,	print_tag },
	{ "col",	1,	print_tag },
	{ "colgroup",	1,	print_tag },
	{ "comment",	0,	print_tag },
	{ "dd",		0,	print_tag },
	{ "dfn",	0,	print_tag },
	{ "dir",	1,	print_tag },
	{ "div",	1,	print_tag },
	{ "dl",		1,	print_tag },
	{ "dt",		0,	print_tag },
	{ "em",		0,	print_tag },
	{ "embed",	1,	print_tag },
	{ "font",	1,	print_tag },
	{ "form",	1,	print_tag },
	{ "frame",	1,	print_tag },
	{ "frameset",	1,	print_tag },
	{ "h1",		1,	print_tag },
	{ "h2",		1,	print_tag },
	{ "h3",		1,	print_tag },
	{ "h4",		1,	print_tag },
	{ "h5",		1,	print_tag },
	{ "h6",		1,	print_tag },
	{ "head",	0,	print_tag },
	{ "hr",		1,	print_tag },
	{ "html",	1,	print_tag },
	{ "i",		0,	print_tag },
	{ "iframe",	1,	print_tag },
	{ "img",	1,	print_tag },
	{ "input",	1,	print_tag },
	{ "isindex",	1,	print_tag },
	{ "kbd",	0,	print_tag },
	{ "li",		1,	print_tag },
	{ "link",	1,	print_tag },
	{ "listing",	0,	print_tag },
	{ "map",	1,	print_tag },
	{ "marquee",	1,	print_tag },
	{ "menu",	1,	print_tag },
	{ "meta",	1,	print_tag },
	{ "multicol",	1,	print_tag },
	{ "nextid",	1,	print_tag },
	{ "nobr",	0,	print_tag },
	{ "noframes",	0,	print_tag },
	{ "noscript",	0,	print_tag },
	{ "object",	1,	print_tag },
	{ "ol",		1,	print_tag },
	{ "option",	1,	print_tag },
	{ "p",		1,	print_tag },
	{ "param",	1,	print_tag },
	{ "plaintext",	0,	print_tag },
	{ "pre",	1,	print_tag },
	{ "s",		0,	print_tag },
	{ "samp",	0,	print_tag },
	{ "script",	1,	print_tag },
	{ "select",	1,	print_tag },
	{ "small",	0,	print_tag },
	{ "spacer",	1,	print_tag },
	{ "span",	1,	print_tag },
	{ "strike",	0,	print_tag },
	{ "strong",	0,	print_tag },
	{ "style",	0,	print_tag },
	{ "sub",	0,	print_tag },
	{ "sup",	0,	print_tag },
	{ "table",	1,	print_tag },
	{ "tbody",	0,	print_tag },
	{ "td",		1,	print_tag },
	{ "textarea",	1,	print_tag },
	{ "tfoot",	0,	print_tag },
	{ "th",		1,	print_tag },
	{ "thead",	0,	print_tag },
	{ "title",	0,	print_tag },
	{ "tr",		1,	print_tag },
	{ "tt",		0,	print_tag },
	{ "ul",		1,	print_tag },
	{ "var",	0,	print_tag },
	{ "wbr",	0,	print_tag },
	{ "xmp",	0,	print_tag },
	{ 0, 0 }
};

FILE *f = stdin;

int main( int argc, char **argv )
{
  int ch;
  /* построение дерева цепочек */
  TagProcessor tp;
  for( int i=0; HTMLTags[i].name; i++ )
     tp.insertTag( &HTMLTags[i] );
  /* разбор входного потока по дереву цепочек */
  tp.reset();
  if ( argc > 1 && !(f=fopen( argv[1], "rt" )) )
   {
     printf( "Cannot open input file\n" );
     exit(1);
   }
  while( (ch=fgetc( f )) != EOF )
     tp.checkTag( ch );
  return 0;
}

#endif

