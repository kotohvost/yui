#ifdef DMALLOC
#include <dmalloc.h>
#endif

/*  									 HTML.c
**	STRUCTURED STREAM TO RICH HYPERTEXT CONVERTER
**
**	(c) COPYRIGHT MIT 1995.
**	Please first read the full copyright statement in the file COPYRIGH.
**
**	This generates of a hypertext object.  It converts from the
**	structured stream interface fro HTMl events into the style-
**	oriented iunterface of the HText interface.  This module is
**	only used in clients and shouldnot be linked into servers.
**
**	Override this module if making a new GUI browser.
**
** HISTORY:
**	 8 Jul 94  FM	Insulate free() from _free structure element.
**
*/

/* Library include files */
extern "C" {
#include "HTUtils.h"
#include "HTChunk.h"
#include "HText.h"
#include "HTMLGen.h"
#include "HTParse.h"
}

extern "C" void HText_appendPara (HText * text );

#ifndef TDEST
#define TDEST stderr
#endif

#ifndef HTML_FORM_ACTION
#define HTML_FORM_ACTION	0
#endif
#ifndef HTML_FORM_METHOD
#define HTML_FORM_METHOD	4
#endif

#ifndef HTML_TEXTAREA_COLS
#define HTML_TEXTAREA_COLS	0
#endif
#ifndef HTML_TEXTAREA_NAME
#define HTML_TEXTAREA_NAME	4
#endif
#ifndef HTML_TEXTAREA_ROWS
#define HTML_TEXTAREA_ROWS	5
#endif

#ifndef HTML_SELECT_MULTIPLE
#define HTML_SELECT_MULTIPLE	0
#endif
#ifndef HTML_SELECT_NAME
#define HTML_SELECT_NAME	1
#endif
#ifndef HTML_SELECT_SIZE
#define HTML_SELECT_SIZE	2
#endif

#ifndef HTML_OPTION_SELECTED
#define HTML_OPTION_SELECTED	2
#endif

#ifndef HTML_OPTION_VALUE
#define HTML_OPTION_VALUE	3
#endif

#ifndef HTML_FRAME_NAME
#define HTML_FRAME_NAME		0
#endif

#ifndef HTML_FRAME_NORESIZE
#define HTML_FRAME_NORESIZE	1
#endif

#ifndef HTML_FRAME_SRC
#define HTML_FRAME_SRC		2
#endif

#ifndef HTML_FRAMESET_COLS
#define HTML_FRAMESET_COLS	0
#endif

#ifndef HTML_FRAMESET_ROWS
#define HTML_FRAMESET_ROWS	1
#endif

#define RUN_HTEXT 1
#define HTML_BASE_HREF 0
#define HTML_BASE_TARGET 1

extern "C" void HText_BeginAnchor (HText * text, HTChildAnchor * anc, BOOL haveHREF, const char *a_target );
extern HTStyleSheet * styleSheet;	/* Application-wide */
extern "C" void HText_putListPrefix(HText *text, char *prefix);
extern "C" void HText_appendSoftPara(HText *text);
extern "C" void HText_beginForm(HText *text, const char *action, const char *method);
extern "C" void HText_endForm(HText *text);
extern "C" void HText_beginFrame(HText *text, const char *src, const char *name, int noresize);
extern "C" void HText_beginFrameset(HText *text, const char *cols, const char *rows);
extern "C" void HText_endFrameset(HText *text);
extern "C" void HText_beginInput(HText *text, const char *name, const char *type,
		const char *value, const char *size,
		const char *maxlength, BOOL checked );
extern "C" void HText_beginTextArea(HText *text, const char *name,
		const char *cols, const char *rows);
extern "C" void HText_endTextArea(HText *text);
extern "C" void HText_beginSelect(HText *text, const char *name,
		const char *size, BOOL multiple);
extern "C" void HText_endSelect(HText *text);
extern "C" void HText_beginOption(HText *text, const char *value, int selected);
extern "C" void HText_endOption(HText *text);
extern "C" void HText_base(HText *text, const char *base_href, const char *base_target);

extern "C" void HText_beginTable(HText *text, BOOL border);
extern "C" void HText_endTable(HText *text);
extern "C" void HText_setCaption(HText *text, const char *data);
extern "C" void HText_row(HText *text);
extern "C" void HText_cell(HText *text, int rowspan, int colspan, const char *align, BOOL isHeader);

#define HTML_TABLE_BORDER 0
#define HTML_TD_ALIGN 0
#define HTML_TD_COLSPAN 1
#define HTML_TD_ROWSPAN 2

#undef PUBLIC
#define PUBLIC extern "C"

/*	Module-wide style cache
*/
PRIVATE int 		got_styles = 0;
PRIVATE HTStyle *styles[HTMLP_ELEMENTS];
PRIVATE HTStyle *ListStyles[7];
PRIVATE HTStyle *MenuStyles[7];
PRIVATE HTStyle *GlossaryStyles[7];
PRIVATE HTStyle *GlossaryCompactStyles[7];

PRIVATE HTStyle *default_style;


/*		HTML Object
**		-----------
*/
//#define MAX_NESTING 20		/* Should be checked by parser */
#define MAX_NESTING 10		/* Should be checked by parser */

typedef struct _stack_element {
	HTStyle *	style;
	int		tag_number;
} stack_element;

struct _HTStructured {
    CONST HTStructuredClass * 	isa;
    HTParentAnchor * 		node_anchor;
    HTRequest *			request;
    HText * 			text;

    HTStream*			target;			/* Output stream */
    HTStreamClass		targetClass;		/* Output routines */

    HTChunk 			title;		/* Grow by 128 */

    char *			comment_start;	/* for literate programming */
    char *			comment_end;

    CONST SGML_dtd*		dtd;

    HTTag *			current_tag;
    BOOL			style_change;
    HTStyle *			new_style;
    HTStyle *			old_style;
    BOOL			in_word;  /* Have just had a non-white char */
    BOOL			skip_flag;

    stack_element 		stack[MAX_NESTING];
    stack_element 		*sp;		      /* Style stack pointer */
    int				overflow;  /* Keep track of overflow nesting */

    int				olStack[MAX_NESTING];
    int				listStack[MAX_NESTING];
    int				olLevel;
    int				listLevel;
    char *			base_href;  /* base href */
};

struct _HTStream {
    CONST HTStreamClass *	isa;
    /* .... */
};

/*		Forward declarations of routines
*/
PRIVATE void get_styles (void);

PRIVATE void actually_set_style (HTStructured * me);
PRIVATE void change_paragraph_style (HTStructured * me, HTStyle * style);

/*	Style buffering avoids dummy paragraph begin/ends.
*/
#define UPDATE_STYLE if (me->style_change) { actually_set_style(me); }


#ifdef OLD_CODE
/* The following accented characters are from peter Flynn, curia project */

/* these ifdefs don't solve the problem of a simple terminal emulator
** with a different character set to the client machine. But nothing does,
** except looking at the TERM setting */


	{ "ocus" , "&" },       /* for CURIA */
#ifdef IBMPC
	{ "aacute" , "\240" },	/* For PC display */
	{ "eacute" , "\202" },
	{ "iacute" , "\241" },
	{ "oacute" , "\242" },
	{ "uacute" , "\243" },
	{ "Aacute" , "\101" },
	{ "Eacute" , "\220" },
	{ "Iacute" , "\111" },
	{ "Oacute" , "\117" },
	{ "Uacute" , "\125" },
#else
	{ "aacute" , "\341" },	/* Works for openwindows -- Peter Flynn */
	{ "eacute" , "\351" },
	{ "iacute" , "\355" },
	{ "oacute" , "\363" },
	{ "uacute" , "\372" },
	{ "Aacute" , "\301" },
	{ "Eacute" , "\310" },
	{ "Iacute" , "\315" },
	{ "Oacute" , "\323" },
	{ "Uacute" , "\332" },
#endif
	{ 0,	0 }  /* Terminate list */
};
#endif


/* 	Entity values -- for ISO Latin 1 local representation
**
**	This MUST match exactly the table referred to in the DTD!
*/
static char * ISO_Latin1[] = {
	"\306",	/* capital AE diphthong (ligature) */
	"\301",	/* capital A, acute accent */
	"\302",	/* capital A, circumflex accent */
	"\300",	/* capital A, grave accent */
	"\305",	/* capital A, ring */
	"\303",	/* capital A, tilde */
	"\304",	/* capital A, dieresis or umlaut mark */
	"\307",	/* capital C, cedilla */
	"\320",	/* capital Eth, Icelandic */
	"\311",	/* capital E, acute accent */
	"\312",	/* capital E, circumflex accent */
	"\310",	/* capital E, grave accent */
	"\313",	/* capital E, dieresis or umlaut mark */
	"\315",	/* capital I, acute accent */
	"\316",	/* capital I, circumflex accent */
	"\314",	/* capital I, grave accent */
	"\317",	/* capital I, dieresis or umlaut mark */
	"\321",	/* capital N, tilde */
	"\323",	/* capital O, acute accent */
	"\324",	/* capital O, circumflex accent */
	"\322",	/* capital O, grave accent */
	"\330",	/* capital O, slash */
	"\325",	/* capital O, tilde */
	"\326",	/* capital O, dieresis or umlaut mark */
	"\336",	/* capital THORN, Icelandic */
	"\332",	/* capital U, acute accent */
	"\333",	/* capital U, circumflex accent */
	"\331",	/* capital U, grave accent */
	"\334",	/* capital U, dieresis or umlaut mark */
	"\335",	/* capital Y, acute accent */
	"\341",	/* small a, acute accent */
	"\342",	/* small a, circumflex accent */
	"\346",	/* small ae diphthong (ligature) */
	"\340",	/* small a, grave accent */
	"\046",	/* ampersand */
	"\345",	/* small a, ring */
	"\343",	/* small a, tilde */
	"\344",	/* small a, dieresis or umlaut mark */
	"\347",	/* small c, cedilla */
	"(C)", /* copy */
	"\351",	/* small e, acute accent */
	"\352",	/* small e, circumflex accent */
	"\350",	/* small e, grave accent */
	"\360",	/* small eth, Icelandic */
	"\353",	/* small e, dieresis or umlaut mark */
	"\076",	/* greater than */
	"\355",	/* small i, acute accent */
	"\356",	/* small i, circumflex accent */
	"\354",	/* small i, grave accent */
	"\357",	/* small i, dieresis or umlaut mark */
	"\074",	/* less than */
	" ", /* nbsp */
	"\361",	/* small n, tilde */
	"\363",	/* small o, acute accent */
	"\364",	/* small o, circumflex accent */
	"\362",	/* small o, grave accent */
	"\370",	/* small o, slash */
	"\365",	/* small o, tilde */
	"\366",	/* small o, dieresis or umlaut mark */
	"\042", /* double quote sign - June 94 */
	"(R)", /*reg*/
	"\337",	/* small sharp s, German (sz ligature) */
	"\376",	/* small thorn, Icelandic */
	"\372",	/* small u, acute accent */
	"\373",	/* small u, circumflex accent */
	"\371",	/* small u, grave accent */
	"\374",	/* small u, dieresis or umlaut mark */
	"\375",	/* small y, acute accent */
	"\377",	/* small y, dieresis or umlaut mark */
};


/* 	Entity values -- for NeXT local representation
**
**	This MUST match exactly the table referred to in the DTD!
**
*/
#if 0
static char * NeXTCharacters[] = {
	"\341",	/* capital AE diphthong (ligature) 	*/
	"\202",	/* capital A, acute accent		*/
	"\203",	/* capital A, circumflex accent 	*/
	"\201",	/* capital A, grave accent 		*/
	"\206",	/* capital A, ring 			*/
	"\204",	/* capital A, tilde 			*/
	"\205",	/* capital A, dieresis or umlaut mark	*/
	"\207",	/* capital C, cedilla 			*/
	"\220",	/* capital Eth, Icelandic 		*/
	"\211",	/* capital E, acute accent 				*/
	"\212",	/* capital E, circumflex accent 			*/
	"\210",	/* capital E, grave accent 				*/
	"\213",	/* capital E, dieresis or umlaut mark 			*/
	"\215",	/* capital I, acute accent 				*/
	"\216",	/* capital I, circumflex accent 	these are	*/
	"\214",	/* capital I, grave accent		ISO -100 hex	*/
	"\217",	/* capital I, dieresis or umlaut mark			*/
	"\221",	/* capital N, tilde 					*/
	"\223",	/* capital O, acute accent 				*/
	"\224",	/* capital O, circumflex accent 			*/
	"\222",	/* capital O, grave accent 				*/
	"\351",	/* capital O, slash 		'cept this */
	"\225",	/* capital O, tilde 					*/
	"\226",	/* capital O, dieresis or umlaut mark			*/
	"\234",	/* capital THORN, Icelandic */
	"\230",	/* capital U, acute accent */
	"\231",	/* capital U, circumflex accent */
	"\227",	/* capital U, grave accent */
	"\232",	/* capital U, dieresis or umlaut mark */
	"\233",	/* capital Y, acute accent */
	"\326",	/* small a, acute accent */
	"\327",	/* small a, circumflex accent */
	"\361",	/* small ae diphthong (ligature) */
	"\325",	/* small a, grave accent */
	"\046",	/* ampersand */
	"\332",	/* small a, ring */
	"\330",	/* small a, tilde */
	"\331",	/* small a, dieresis or umlaut mark */
	"\333",	/* small c, cedilla */
	"\335",	/* small e, acute accent */
	"\336",	/* small e, circumflex accent */
	"\334",	/* small e, grave accent */
	"\346",	/* small eth, Icelandic 	*/
	"\337",	/* small e, dieresis or umlaut mark */
	"\076",	/* greater than */
	"\342",	/* small i, acute accent */
	"\344",	/* small i, circumflex accent */
	"\340",	/* small i, grave accent */
	"\345",	/* small i, dieresis or umlaut mark */
	"\074",	/* less than */
	"\347",	/* small n, tilde */
	"\355",	/* small o, acute accent */
	"\356",	/* small o, circumflex accent */
	"\354",	/* small o, grave accent */
	"\371",	/* small o, slash */
	"\357",	/* small o, tilde */
	"\360",	/* small o, dieresis or umlaut mark */
	"\042", /* double quote sign - June 94 */
	"\373",	/* small sharp s, German (sz ligature) */
	"\374",	/* small thorn, Icelandic */
	"\363",	/* small u, acute accent */
	"\364",	/* small u, circumflex accent */
	"\362",	/* small u, grave accent */
	"\366",	/* small u, dieresis or umlaut mark */
	"\367",	/* small y, acute accent */
	"\375",	/* small y, dieresis or umlaut mark */
};
#endif

/* 	Entity values -- for IBM/PC Code Page 850 (International)
**
**	This MUST match exactly the table referred to in the DTD!
**
*/
/* @@@@@@@@@@@@@@@@@ TBD */



/*		Set character set
**		----------------
*/

PRIVATE char** p_entity_values = ISO_Latin1;	/* Pointer to translation */

PUBLIC void HTMLUseCharacterSet (HTMLCharacterSet i)
{
    /*
    p_entity_values = (i == HTML_NEXT_CHARS) ? NeXTCharacters
					     : ISO_Latin1;
    */
    p_entity_values = ISO_Latin1;
}


/*		Flattening the style structure
**		------------------------------
**
On the NeXT, and on any read-only browser, it is simpler for the text to have
a sequence of styles, rather than a nested tree of styles. In this
case we have to flatten the structure as it arrives from SGML tags into
a sequence of styles.
*/

/*		If style really needs to be set, call this
*/
PRIVATE void actually_set_style (HTStructured * me)
{
    if (!me->text) {			/* First time through */
	    me->text = HText_new2( me->node_anchor, me->target);
	    HText_beginAppend(me->text);
	    HText_setStyle(me->text, me->new_style);
	    me->in_word = NO;
	    me->skip_flag = NO;
    } else {
	    HText_setStyle(me->text, me->new_style);
    }
    me->old_style = me->new_style;
    me->style_change = NO;
}

/*      If you THINK you need to change style, call this
*/

PRIVATE void change_paragraph_style (HTStructured * me, HTStyle *style)
{
    if (me->new_style!=style) {
	me->style_change = YES;
	me->new_style = style;
    }
    me->in_word = NO;
}

/*_________________________________________________________________________
**
**			A C T I O N 	R O U T I N E S
*/

/*	Character handling
**	------------------
*/
PRIVATE void HTML_put_character (HTStructured * me, char c)
{
    if ( me->skip_flag )
	return;
    if ( me->target )
	me->targetClass.put_character( me->target, c );
#if RUN_HTEXT
    switch (me->sp[0].tag_number) {
    case HTML_COMMENT:
	break;					/* Do Nothing */

    case HTML_TITLE:
    case HTML_CAPTION:
	HTChunkPutc(&me->title, c);
	break;

    case HTML_LISTING:				/* Litteral text */
    case HTML_XMP:
    case HTML_PLAINTEXT:
    case HTML_PRE:
    case HTML_A:
/*	We guarrantee that the style is up-to-date in begin_litteral
*/

    case HTML_I:
    case HTML_U:
    case HTML_TT:
    case HTML_B:
    case HTML_EM:
    case HTML_Q:
	HText_appendCharacter(me->text, c);
	break;
    default:					/* Free format text */
	if (me->style_change) {
	    if ((c=='\n') || (c==' ')) return /*HT_OK*/;	/* Ignore it */
	    UPDATE_STYLE;
	}
	if ( c == '\n' )
	   me->in_word = NO;
	else
	   me->in_word = YES;
	HText_appendCharacter( me->text, c );
    } /* end switch */
#endif
//    return HT_OK;
}



/*	String handling
**	---------------
**
**	This is written separately from put_character becuase the loop can
**	in some cases be promoted to a higher function call level for speed.
*/
PRIVATE void HTML_put_string (HTStructured * me, CONST char* s)
{
    if ( me->skip_flag )
	return;
    if ( me->target )
	me->targetClass.put_string( me->target, s );
#if RUN_HTEXT
    switch (me->sp[0].tag_number) {
    case HTML_COMMENT:
	break;					/* Do Nothing */

    case HTML_TITLE:
    case HTML_CAPTION:
	HTChunkPuts(&me->title, s);
	break;

    case HTML_LISTING:				/* Litteral text */
    case HTML_XMP:
    case HTML_PLAINTEXT:
    case HTML_PRE:
    case HTML_A:

    case HTML_I:
    case HTML_U:
    case HTML_TT:
    case HTML_B:
    case HTML_EM:
    case HTML_Q:

/*	We guarrantee that the style is up-to-date in begin_litteral
*/
	HText_appendText(me->text, s);
	break;
    default:					/* Free format text */
	{
	    CONST char *p = s;
	    if (me->style_change) {
		for (; *p && ((*p=='\n') || (*p==' ')); p++)  ;  /* Ignore leaders */
		if (!*p) return;
		UPDATE_STYLE;
	    }
	    for(; *p; p++) {
		if (me->style_change) {
		    if ((*p=='\n') || (*p==' ')) continue;  /* Ignore it */
		    UPDATE_STYLE;
		}
		if (*p=='\n') {
		    me->in_word = NO;
		    HText_appendCharacter(me->text, ' ');
		} else {
		    HText_appendCharacter(me->text, *p);
		    me->in_word = YES;
		}
	    } /* for */
	}
    } /* end switch */
#endif
}


/*	Buffer write
**	------------
*/
PRIVATE void HTML_write (HTStructured * me, CONST char* s, int l)
{
    if ( me->target )
	me->targetClass.put_block( me->target, s, l );
#if RUN_HTEXT
    while (l-- > 0)
	HTML_put_character(me, *s++);
#endif
}

PRIVATE void set_list_style(HTStructured * me, CONST BOOL *present)
{
  int no=me->listLevel;
  if (no<0)
    no=0;
  else if( no>6)
    no=6;
  switch ( me->listStack[me->listLevel] )
   {
    case 0: /*definition list*/
	{
	HTStyle **tbl;
	if (present && present[DL_COMPACT])
	  tbl=GlossaryCompactStyles;
	else
	  tbl=GlossaryStyles;
	if (--no<0)
	  no=0;
	change_paragraph_style(me, tbl[no]);
	}
	break;
   case 1: /*unordered list*/
   case 2: /*ordered list*/
	{
	change_paragraph_style(me, ListStyles[no]);
	}
	break;
   case 3: /* menu */
	{
	change_paragraph_style(me, MenuStyles[no]);
	}
	break;
   }
}


extern "C" void addSlash(char **str);

PRIVATE HTChildAnchor * myHTAnchor_findChildAndLink (
					HTStructured * 	me,
					HTParentAnchor *	parent,
					CONST char *		tag,
					CONST char *		href,
					HTLinkType *		ltype )
{
    HTChildAnchor * child = HTAnchor_findChild(parent, tag);
    if ( href && *href )
     {
	char *relative_to = HTAnchor_address((HTAnchor *) parent);
	char *parsed_address;
	HTAnchor *dest;
	parsed_address = HTParse( href, me->base_href ? me->base_href : relative_to, PARSE_ALL );
	dest = HTAnchor_findAddress(parsed_address);
	HTAnchor_link((HTAnchor *) child, dest, ltype);

	free(parsed_address);
	free(relative_to);
     }
    return child;
}


/*	Start Element
**	-------------
*/
PRIVATE void HTML_start_element (
	HTStructured * 	me,
	int			element_number,
	CONST BOOL*		present,
	CONST char **		value)
{
    if ( me->target )
     {
	HTTag tag = HTMLP_dtd.tags[element_number];
	me->targetClass.put_character( me->target, '<' );
	me->targetClass.put_block( me->target, tag.name, strlen(tag.name) );
	if ( present )
	  for( int i=0; i < tag.number_of_attributes; i++ )
	   {
	     if ( !present[i] )
		continue;
	     me->targetClass.put_character( me->target, ' ' );
	     me->targetClass.put_block( me->target, tag.attributes[i].name, strlen(tag.attributes[i].name) );
	     if ( !value || !value[i] )
		continue;
	     me->targetClass.put_character( me->target, '=' );
	     me->targetClass.put_block( me->target, value[i], strlen(value[i]) );
	   }
	me->targetClass.put_character( me->target, '>' );
     }
#if RUN_HTEXT

    if ( me->skip_flag == YES )
       goto skip_switch;

    switch (element_number) {
    case HTML_TABLE:
	{
	BOOL border = NO;
	if ( present && present[HTML_TABLE_BORDER] &&
	     (!value[HTML_TABLE_BORDER] || value[HTML_TABLE_BORDER][0] != '0') )
	   border = YES;
	UPDATE_STYLE;
	HText_beginTable( me->text, border );
	}
	break;
    case HTML_TR:
	HText_row(me->text);
	return;
    case HTML_TH:
    case HTML_TD:
	{
	  int rowspan=1, colspan=1;
	  if ( present && present[HTML_TD_ROWSPAN] )
	     rowspan = atoi( value[HTML_TD_ROWSPAN] );
	  if ( present && present[HTML_TD_COLSPAN] )
	     colspan = atoi( value[HTML_TD_COLSPAN] );
	  HText_cell( me->text, rowspan, colspan,
		      present && present[HTML_TD_ALIGN] ? value[HTML_TD_ALIGN] : 0,
		      element_number == HTML_TH ? YES : NO );
	}
	return;
    case HTML_BASE:
	{
	 char *href=NULL;
	 const char *target=NULL;
	 if ( present && present[HTML_BASE_HREF] )
	  {
	    StrAllocCopy( href, value[HTML_BASE_HREF] );
	    FREE(me->base_href);
	    me->base_href=href;
	  }
	 if ( present && present[HTML_BASE_TARGET] )
	    target = value[HTML_BASE_TARGET];
	 HText_base( me->text, href, target );
	}
	break;
    case HTML_A:
	{
	    HTChildAnchor * source;
	    char *href = NULL;
	    const char *a_target=NULL;
	    if (present && present[HTML_A_HREF])
		StrAllocCopy(href, value[HTML_A_HREF]);

	    source = myHTAnchor_findChildAndLink(
		me,
		me->node_anchor,				/* parent */
		present && present[HTML_A_NAME] ? value[HTML_A_NAME] : 0,	/* Tag */
		present && present[HTML_A_HREF] ? href : 0,		/* Addresss */
		present && present[HTML_A_REL] && value[HTML_A_REL] ?
			(HTLinkType*)HTAtom_for(value[HTML_A_REL]) : 0 );

	    if (present && present[HTML_A_TITLE] && value[HTML_A_TITLE]) {
		HTParentAnchor * dest =
		    HTAnchor_parent(
			HTAnchor_followMainLink((HTAnchor*)source)
				    );
		if (!HTAnchor_title(dest))
			HTAnchor_setTitle(dest, value[HTML_A_TITLE]);
	    }
	    if ( present && present[HTML_A_TARGET] )
	       a_target = value[HTML_A_TARGET];
	    UPDATE_STYLE;
	    HText_BeginAnchor( me->text, source, present[HTML_A_HREF], a_target );
	    FREE(href);				 /* Leak fix Henrik 17/02-94 */
	}
	break;

    case HTML_FORM:
	{
	    char *action=0, *method=0;
	    HTChildAnchor *source;
	    HTAnchor *link_dest;

	    if (present && present[HTML_FORM_ACTION])  {
		StrAllocCopy( action, value[HTML_FORM_ACTION] );
		action = HTSimplify( action );
		source = myHTAnchor_findChildAndLink(me, me->node_anchor,
						     0, action, 0);
		link_dest = HTAnchor_followMainLink((HTAnchor *)source);
		{
		  char *s = HTAnchor_address(link_dest);
		  if (s)
		    StrAllocCopy(action, s);
		  else
		    StrAllocCopy(action, "");
		  free(s);
		}
	    }
	    if (present && present[HTML_FORM_METHOD])
		method = (char*)value[HTML_FORM_METHOD];

	    HText_beginForm( me->text, action, method );
	    FREE (action);
	}
	break;
    case HTML_FRAME:
	{
	    char *name=0, *src=0;
	    int noresize=0;
	    if (present && present[HTML_FRAME_SRC])  {
		StrAllocCopy( src, value[HTML_FRAME_SRC] );
		src = HTSimplify( src );
		HTChildAnchor *source = myHTAnchor_findChildAndLink(me, me->node_anchor,
						     0, src, 0);
		HTAnchor *link_dest = HTAnchor_followMainLink((HTAnchor *)source);
		{
		  char *s = HTAnchor_address(link_dest);
		  if (s)
		    StrAllocCopy(src, s);
		  else
		    StrAllocCopy(src, "");
		  ::free(s);
		}
	    }
	    if (present && present[HTML_FRAME_NAME])
		name = (char*)value[HTML_FRAME_NAME];
	    if (present && present[HTML_FRAME_NORESIZE])
		noresize = 1;

	    HText_beginFrame( me->text, src, name, noresize );
	    FREE (src);
	}
	break;
    case HTML_NOFRAMES:
    case HTML_SCRIPT:
    case HTML_STYLE:
	me->skip_flag = YES;
	break;
    case HTML_FRAMESET:
	{
	    char *cols=0, *rows=0;
	    if (present && present[HTML_FRAMESET_COLS])
		cols = (char *) value[HTML_FRAMESET_COLS];
	    if (present && present[HTML_FRAMESET_ROWS])
		rows = (char *) value[HTML_FRAMESET_ROWS];
	    HText_beginFrameset( me->text, cols, rows );
	}
	break;
    case HTML_INPUT:
	{
	    char *name=0, *type=0, *val=0, *size=0, *maxlength=0;
	    BOOL checked=NO;

	    /* before any input field add a space if necessary */
	    UPDATE_STYLE;
	    HTML_put_character(me,' ');

	    if (present && present[HTML_INPUT_NAME])
		name = (char *) value[HTML_INPUT_NAME];
	    if (present && present[HTML_INPUT_TYPE])
		type = (char *) value[HTML_INPUT_TYPE];
	    if (present && present[HTML_INPUT_VALUE])
		val = (char *) value[HTML_INPUT_VALUE];
	    if (present && present[HTML_INPUT_CHECKED])
		checked = YES;
	    if (present && present[HTML_INPUT_SIZE])
		size = (char *) value[HTML_INPUT_SIZE];
	    if (present && present[HTML_INPUT_MAX])
		maxlength = (char *) value[HTML_INPUT_MAX];

	    HText_beginInput(me->text, name, type, val, size, maxlength, checked );
	}
	break;

    case HTML_TEXTAREA:
	{
	    char *name=0, *cols=0, *rows=0;
	    if (present && present[HTML_TEXTAREA_NAME])
	       name = (char*) value[HTML_TEXTAREA_NAME];
	    if (present && present[HTML_TEXTAREA_COLS])
	       cols = (char*) value[HTML_TEXTAREA_COLS];
	    if (present && present[HTML_TEXTAREA_ROWS])
	       rows = (char*) value[HTML_TEXTAREA_ROWS];
	    HText_beginTextArea(me->text, name, cols, rows);
	}
	break;

    case HTML_SELECT:
	{
	    char *name=0, *size=0;
	    BOOL multiple=NO;
	    if (present && present[HTML_SELECT_NAME])
		StrAllocCopy(name, value[HTML_SELECT_NAME]);
	    if (present && present[HTML_SELECT_MULTIPLE])
		multiple=YES;
	    if (present && present[HTML_SELECT_SIZE])
		StrAllocCopy(size, value[HTML_SELECT_SIZE]);

	    HText_beginSelect(me->text, name, size, multiple);
	}
	break;

    case HTML_OPTION:
	{
	    /* an option is a special case of an input field */
	    BOOL selected=NO;
	    const char *val=0;
	    if ( present && present[HTML_OPTION_SELECTED] )
		 selected=YES;
	    if ( present && present[HTML_OPTION_VALUE] )
		 val=value[HTML_OPTION_VALUE];
	    HText_beginOption(me->text, val, selected);
	}
	break;

    case HTML_CAPTION:
    case HTML_TITLE:
	HTChunkClear(&me->title);
	break;

    case HTML_ISINDEX:
	HTAnchor_setIndex(me->node_anchor);
	break;

    case HTML_BR:
	HText_appendLineBreak(me->text);
	me->in_word = NO;
	break;

    case HTML_HR:
	HText_appendHorizontalRule (me->text);
	me->in_word = NO;
	break;

    case HTML_P:
	UPDATE_STYLE;
	HText_appendPara(me->text);
	me->in_word = NO;
	break;

    case HTML_DL:
	me->listLevel++;
	me->listStack[me->listLevel]=0; /*not ordered*/
	set_list_style(me, present);
	HText_appendSoftPara(me->text);
	UPDATE_STYLE;
	break;

    case HTML_DT:
	UPDATE_STYLE;
	HText_appendParagraph(me->text);
	me->in_word = NO;
	break;

    case HTML_DD:
	UPDATE_STYLE;
	HText_appendLineBreak(me->text);
	me->in_word = NO;
	break;

    case HTML_UL:
	me->listLevel++;
	me->listStack[me->listLevel]=1; /*unordered*/
	set_list_style(me, present);
	break;

    case HTML_OL:
	me->listLevel++;
	me->listStack[me->listLevel]=2; /* ordered*/
	me->olLevel++;
	me->olStack[me->olLevel]=0;
	set_list_style(me, present);
	break;

    case HTML_MENU:
	me->listLevel++;
	me->listStack[me->listLevel]=3; /*not ordered*/
	set_list_style(me, present);
	break;

    case HTML_DIR:
	change_paragraph_style(me, styles[element_number]);
	break;

    case HTML_LI:
	{
	  int no=me->listLevel;
	  UPDATE_STYLE;
	  if ( no >6 ) no=6;
	  else if (no<0)  no=0;
	  switch ( me->listStack[no] )
	  {
	   case 3: /* menu list */
	   case 1: /* unordered list */
	      HText_putListPrefix(me->text, 0);
	      break;
	   case 2:
	    { /* ordered list */
	      char buf[21];
	      int i;
	      char *b=buf;
	      buf[0]=buf[20]=0;
	      me->olStack[me->olLevel]++;
	      for(i=0; i<=me->olLevel; i++)
	       {
#if defined(SOLARIS_SPARC) || defined(SINIX)
		 sprintf( b, "%d.", me->olStack[i] );
#else
		 snprintf( b, sizeof(buf)-1-(b-buf), "%d.", me->olStack[i] );
#endif
		 b=buf+strlen(buf);
	       }
	      HText_putListPrefix(me->text, buf);
	      break;

	    }
	    break;
	   case 0: /* definition list */
	   default:
	      HText_appendParagraph(me->text);
	      break;
	  }
	}
	me->in_word = NO;
	break;

    case HTML_LISTING:				/* Litteral text */
    case HTML_XMP:
    case HTML_PLAINTEXT:
    case HTML_PRE:
	change_paragraph_style(me, styles[element_number]);
	UPDATE_STYLE;
	if (me->comment_end)
	    HText_appendText(me->text, me->comment_end);
	break;

    case HTML_IMG:			/* Images */
	{
	    HTChildAnchor *source;
	    char *src = NULL;
	    if (present && present[HTML_IMG_SRC])
		StrAllocCopy(src, value[HTML_IMG_SRC]);
	    source = myHTAnchor_findChildAndLink(
					       me,
					       me->node_anchor,	   /* parent */
					       0,                     /* Tag */
					       src ? src : 0,    /* Addresss */
					       0);
	    UPDATE_STYLE;
	    HText_appendImage(me->text, source,
		      (CONST char*)(present && present[HTML_IMG_ALT] ? value[HTML_IMG_ALT] : NULL),
		      (CONST char*)(present && present[HTML_IMG_ALIGN] ? value[HTML_IMG_ALIGN] : NULL),
		      present && present[HTML_IMG_ISMAP] ? YES : NO);
	    FREE(src);
	}
	break;

    case HTML_I:
    case HTML_U:
    case HTML_TT:			/* Physical character highlighting */
    case HTML_B:
    case HTML_EM:	/* Logical character highlighting */
    case HTML_Q:	/* Quotation */
	UPDATE_STYLE;
	change_paragraph_style(me, styles[element_number]);
	UPDATE_STYLE;
	break;


    case HTML_HTML:			/* Ignore these altogether */
    case HTML_HEAD:
    case HTML_BODY:


    case HTML_STRONG:			/* Currently ignored */
    case HTML_CODE:
    case HTML_SAMP:
    case HTML_KBD:
    case HTML_VAR:
    case HTML_DFN:
    case HTML_CITE:
	break;

    case HTML_H1:			/* paragraph styles */
    case HTML_H2:
    case HTML_H3:
    case HTML_H4:
    case HTML_H5:
    case HTML_H6:
    case HTML_H7:
    case HTML_ADDRESS:
    case HTML_BLOCKQUOTE:
	change_paragraph_style(me, styles[element_number]);	/* May be postponed */
	break;

    } /* end switch */

skip_switch:
    if (me->dtd->tags[element_number].contents!= SGML_EMPTY) {
	if (me->sp == me->stack) {
	    if (SGML_TRACE)
		fprintf(TDEST, "HTML........ Maximum nesting of %d exceded!\n",
			MAX_NESTING);
	    me->overflow++;
	    return;
	}
	--(me->sp);
	me->sp[0].style = me->new_style;	/* Stack new style */
	me->sp[0].tag_number = element_number;
    }
#endif
}


/*		End Element
**		-----------
**
*/
/*	When we end an element, the style must be returned to that
**	in effect before that element.  Note that anchors (etc?)
**	don't have an associated style, so that we must scan down the
**	stack for an element with a defined style. (In fact, the styles
**	should be linked to the whole stack not just the top one.)
**	TBL 921119
**
**	We don't turn on "CAREFUL" check because the parser produces
**	(internal code errors apart) good nesting. The parser checks
**	incoming code errors, not this module.
*/
PRIVATE void HTML_end_element (HTStructured * me, int element_number)
{
    if ( me->target )
      {
	char *name = HTMLP_dtd.tags[element_number].name;
	me->targetClass.put_block( me->target, "</", 2 );
	me->targetClass.put_block( me->target, name, strlen(name) );
	me->targetClass.put_character( me->target, '>' );
      }
#if RUN_HTEXT
#ifdef CAREFUL			/* parser assumed to produce good nesting */
    if (element_number != me->sp[0].tag_number) {
	fprintf(TDEST, "HTMLText: end of element %s when expecting end of %s\n",
		me->dtd->tags[element_number].name,
		me->dtd->tags[me->sp->tag_number].name);
		/* panic */
    }
#endif

    /* HFN, If overflow of nestings, we need to get back to reality */
    if (me->overflow > 0) {
	me->overflow--;
	return;
    }

    switch(element_number)
    {
      case HTML_TR:
      case HTML_TH:
      case HTML_TD:
	return;
    }

    me->sp++;				/* Pop state off stack */

    if ( me->skip_flag )
     {
       switch( element_number ) {
	   case HTML_NOFRAMES:
	   case HTML_SCRIPT:
	   case HTML_STYLE:
	       me->skip_flag = NO;
	       break;
       }
       return;
     }

    switch(element_number) {

    case HTML_TABLE:
	HText_endTable(me->text);
	break;

    case HTML_A:
	UPDATE_STYLE;
	HText_endAnchor(me->text);
	break;

    case HTML_FORM:
	UPDATE_STYLE;
	HText_endForm(me->text);
	break;

    case HTML_FRAMESET:
	UPDATE_STYLE;
	HText_endFrameset(me->text);
	break;

    case HTML_SELECT:
	UPDATE_STYLE;
	HText_endSelect(me->text);
	break;

    case HTML_OPTION:
//	UPDATE_STYLE;
	HText_endOption(me->text);
	break;

    case HTML_TEXTAREA:
	UPDATE_STYLE;
	HText_endTextArea(me->text);
	break;

    case HTML_TITLE:
	HTChunkTerminate(&me->title);
	HTAnchor_setTitle(me->node_anchor, me->title.data);
	break;

    case HTML_CAPTION:
	HTChunkTerminate(&me->title);
	HText_setCaption(me->text, me->title.data);
	break;

    case HTML_OL:
	me->olLevel--;
    case HTML_DL:
    case HTML_UL:
    case HTML_MENU:
    case HTML_DIR:
	me->listLevel--;
	/*set_list_style(me);*/
	change_paragraph_style(me, me->sp->style);
	break;

    case HTML_I:
    case HTML_U:
    case HTML_TT:			/* Physical character highlighting */
    case HTML_B:
    case HTML_EM:	/* Logical character highlighting */
    case HTML_Q:	/* Quotation */
	change_paragraph_style(me, me->sp->style);
	UPDATE_STYLE;
	break;

    case HTML_P:  /* </P> means <P> !!!!!! */
	UPDATE_STYLE;
	HText_appendPara(me->text);
	me->in_word = NO;
	break;

    case HTML_LISTING:				/* Litteral text */
    case HTML_XMP:
    case HTML_PLAINTEXT:
    case HTML_PRE:
	if (me->comment_start)
	    HText_appendText(me->text, me->comment_start);
	/* Fall through */

    default:

	/* Often won't really change */
	change_paragraph_style(me, me->sp->style);
	break;

    } /* switch */
#endif
}


/*		Expanding entities
**		------------------
*/
/*	(In fact, they all shrink!)
*/

PRIVATE void HTML_put_entity (HTStructured * me, int entity_number)
{
#if RUN_HTEXT
    HTML_put_string(me, ISO_Latin1[entity_number]);	/* @@ Other representations */
#endif
}

/*	Flush an HTML object
**	--------------------
*/
/*
PUBLIC int HTML_flush (HTStructured * me)
{
#if RUN_HTEXT
    UPDATE_STYLE;			     // Creates empty document here!
    if (me->comment_end)
	HTML_put_string(me,me->comment_end);
    if (me->target)
       return me->targetClass.flush(me->target);
#endif
    return HT_OK;
}
*/

/*	Free an HTML object
**	-------------------
**
** If the document is empty, the text object will not yet exist.
   So we could in fact abandon creating the document and return
   an error code.  In fact an empty document is an important type
   of document, so we don't.
**
**	If non-interactive, everything is freed off.   No: crashes -listrefs
**	Otherwise, the interactive object is left.
*/
PUBLIC void HTML_free (HTStructured * me)
{
#if RUN_HTEXT
    UPDATE_STYLE;		/* Creates empty document here! */
    if (me->comment_end)
		HTML_put_string(me,me->comment_end);
    HText_endAppend(me->text);

    if (me->target) {
	me->targetClass._free(me->target);
    }
    HTChunkClear(&me->title);	/* Henrik 18/02-94 */
    HText_free(me->text);

    free(me);
#endif
}


PRIVATE void HTML_abort (HTStructured * me, HTError  e)

{
#if RUN_HTEXT
    if (me->target) {
	me->targetClass.abort(me->target, e);
    }
    HTChunkClear(&me->title);	/* Henrik 18/02-94 */
    HText_free(me->text);
    free(me);
#endif
}


/*	Get Styles from style sheet
**	---------------------------
*/
PRIVATE void get_styles (void)
{
    got_styles = YES;

    default_style =		HTStyleNamed(styleSheet, "Normal");

    styles[HTML_H1] =		HTStyleNamed(styleSheet, "Heading1");
    styles[HTML_H2] =		HTStyleNamed(styleSheet, "Heading2");
    styles[HTML_H3] =		HTStyleNamed(styleSheet, "Heading3");
    styles[HTML_H4] =		HTStyleNamed(styleSheet, "Heading4");
    styles[HTML_H5] =		HTStyleNamed(styleSheet, "Heading5");
    styles[HTML_H6] =		HTStyleNamed(styleSheet, "Heading6");
    styles[HTML_H7] =		HTStyleNamed(styleSheet, "Heading7");

    styles[HTML_DL] =		HTStyleNamed(styleSheet, "Glossary");
    styles[HTML_UL] =
    styles[HTML_OL] =		HTStyleNamed(styleSheet, "List");
    styles[HTML_MENU] =		HTStyleNamed(styleSheet, "Menu");
    styles[HTML_DIR] =		HTStyleNamed(styleSheet, "Dir");
    /*styles[HTML_DLC] =		HTStyleNamed(styleSheet, "GlossaryCompact");*/
    styles[HTML_ADDRESS]=	HTStyleNamed(styleSheet, "Address");
    styles[HTML_BLOCKQUOTE]=	HTStyleNamed(styleSheet, "BlockQuote");
    styles[HTML_PLAINTEXT] =
    styles[HTML_XMP] =		HTStyleNamed(styleSheet, "Example");
    styles[HTML_PRE] =		HTStyleNamed(styleSheet, "Preformatted");
    styles[HTML_LISTING] =	HTStyleNamed(styleSheet, "Listing");

    styles[HTML_I] =		HTStyleNamed(styleSheet, "Italic");
    styles[HTML_TT] =
    styles[HTML_B] =            HTStyleNamed(styleSheet, "Bold");
    styles[HTML_U] =            HTStyleNamed(styleSheet, "Underline");
    styles[HTML_EM] =           HTStyleNamed(styleSheet, "Emphase");
    styles[HTML_Q] =            HTStyleNamed(styleSheet, "Quota");

    GlossaryStyles[0] =		HTStyleNamed(styleSheet, "Glossary");
    GlossaryStyles[1] =		HTStyleNamed(styleSheet, "Glossary1");
    GlossaryStyles[2] =		HTStyleNamed(styleSheet, "Glossary2");
    GlossaryStyles[3] =		HTStyleNamed(styleSheet, "Glossary3");
    GlossaryStyles[4] =		HTStyleNamed(styleSheet, "Glossary4");
    GlossaryStyles[5] =		HTStyleNamed(styleSheet, "Glossary5");
    GlossaryStyles[6] =		HTStyleNamed(styleSheet, "Glossary6");
    GlossaryCompactStyles[0] = 	HTStyleNamed(styleSheet, "GlossaryCompact");
    GlossaryCompactStyles[1] = 	HTStyleNamed(styleSheet, "GlossaryCompact1");
    GlossaryCompactStyles[2] = 	HTStyleNamed(styleSheet, "GlossaryCompact2");
    GlossaryCompactStyles[3] = 	HTStyleNamed(styleSheet, "GlossaryCompact3");
    GlossaryCompactStyles[4] = 	HTStyleNamed(styleSheet, "GlossaryCompact4");
    GlossaryCompactStyles[5] = 	HTStyleNamed(styleSheet, "GlossaryCompact5");
    GlossaryCompactStyles[6] = 	HTStyleNamed(styleSheet, "GlossaryCompact6");
    ListStyles[0] =		HTStyleNamed(styleSheet, "List");
    ListStyles[1] =		HTStyleNamed(styleSheet, "List1");
    ListStyles[2] =		HTStyleNamed(styleSheet, "List2");
    ListStyles[3] =		HTStyleNamed(styleSheet, "List3");
    ListStyles[4] =		HTStyleNamed(styleSheet, "List4");
    ListStyles[5] =		HTStyleNamed(styleSheet, "List5");
    ListStyles[6] =		HTStyleNamed(styleSheet, "List6");
    MenuStyles[0] =		HTStyleNamed(styleSheet, "Menu");
    MenuStyles[1] =		HTStyleNamed(styleSheet, "Menu1");
    MenuStyles[2] =		HTStyleNamed(styleSheet, "Menu2");
    MenuStyles[3] =		HTStyleNamed(styleSheet, "Menu3");
    MenuStyles[4] =		HTStyleNamed(styleSheet, "Menu4");
    MenuStyles[5] =		HTStyleNamed(styleSheet, "Menu5");
    MenuStyles[6] =		HTStyleNamed(styleSheet, "Menu6");
}
/*				P U B L I C
*/

/*	Structured Object Class
**	-----------------------
*/
PUBLIC CONST HTStructuredClass HTMLPresentation = /* As opposed to print etc */
{
	"text/html",
	HTML_free,
	HTML_abort,
	HTML_put_character, 	HTML_put_string,  HTML_write,
	HTML_start_element, 	HTML_end_element,
	HTML_put_entity
};


/*		New Structured Text object
**		--------------------------
**
**	The structured stream can generate either presentation,
**	or plain text, or HTML.
*/
PUBLIC HTStructured* HTML_new (HTRequest *	request,
				     void *		param,
				     HTFormat		input_format,
				     HTFormat		output_format,
				     HTStream *	output_stream)
{

    HTStructured * me;

    if (output_format != WWW_PLAINTEXT
	&& output_format != WWW_PRESENT
	&& output_format != HTAtom_for("text/x-c")) {
	HTStream * intermediate = HTStreamStack(WWW_HTML, request, NO);
	if (intermediate) return HTMLGenerator(intermediate);
	fprintf(stderr, "** Internal error: can't parse HTML to %s\n",
		HTAtom_name(output_format));
	exit (-99);
    }

    if ((me = (HTStructured*) calloc(1, sizeof(*me))) == NULL)
	outofmem(__FILE__, "HTML_new");

    if (!got_styles) get_styles();

    me->isa = &HTMLPresentation;
    me->dtd = &HTMLP_dtd;
    me->request = request;
    me->node_anchor =  request->anchor;
    me->title.size = 0;
    me->title.growby = 128;
    me->title.allocated = 0;
    me->title.data = 0;
    me->text = 0;
    me->style_change = YES; /* Force check leading to text creation */
    me->new_style = default_style;
    me->old_style = 0;
    me->sp = me->stack + MAX_NESTING - 1;
    me->sp->tag_number = -1;				/* INVALID */
    me->sp->style = default_style;			/* INVALID */

    me->comment_start = NULL;
    me->comment_end = NULL;

    me->olLevel = -1;
    me->listLevel = -1;
    me->base_href = 0;

    me->target = output_stream;
    if (output_stream)
	me->targetClass = *output_stream->isa;	/* Copy pointers */

    me->text = HText_new2(/*me->request,*/ me->node_anchor, output_stream);

    HText_beginAppend(me->text);
    HText_setStyle(me->text, me->new_style);
    me->in_word = NO;
    me->skip_flag = NO;

    return (HTStructured*) me;
}


/*	HTConverter for HTML to plain text
**	----------------------------------
**
**	This will convert from HTML to presentation or plain text.
*/
PUBLIC HTStream* HTMLToPlain (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{
    return SGML_new(&HTMLP_dtd, HTML_new(
	request, NULL, input_format, output_format, output_stream));
}


/*	HTConverter for HTML to C code
**	------------------------------
**
**	C code is like plain text but all non-preformatted code
**	is commented out.
**	This will convert from HTML to presentation or plain text.
*/
PUBLIC HTStream* HTMLToC (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{

    HTStructured * html;

    (*output_stream->isa->put_string)(output_stream, "/* "); /* Before title */
    html = HTML_new(request, NULL, input_format, output_format, output_stream);
    html->comment_start = "\n/* ";
    html->dtd = &HTMLP_dtd;
    html->comment_end = " */\n";	/* Must start in col 1 for cpp */
    return SGML_new(&HTMLP_dtd, html);
}


/*	Presenter for HTML
**	------------------
**
**	This will convert from HTML to presentation or plain text.
**
**	Override this if you have a windows version
*/
#ifndef GUI
PUBLIC HTStream* HTMLPresent (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{
    return SGML_new(&HTMLP_dtd, HTML_new(
	request, NULL, input_format, output_format, output_stream));
}
#endif

