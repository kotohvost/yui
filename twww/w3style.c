#ifdef DMALLOC
#include <dmalloc.h>
#endif

/*	A real style sheet for the Character Grid browser
**
**	The dimensions are all in characters!
*/

#include <HTStyle.h>
#include "w3style.h"


/*	Tab arrays:
*/
PRIVATE HTTabStop tabs_8[] = {
	{ 0, 8 }, {0, 16}, {0, 24}, {0, 32}, {0, 40},
	{ 0, 48 }, {0, 56}, {0, 64}, {0, 72}, {0, 80},
	{ 0, 88 }, {0, 96}, {0, 104}, {0, 112}, {0, 120},
	{ 0, 128 }, {0, 136}, {0, 144}, {0, 152}, {0, 160},
	{0, 168}, {0, 176},
	{0, 0 }		/* Terminate */
};

PRIVATE HTTabStop tabs_16[] = {
	{ 0, 16 }, {0, 32}, {0, 48}, {0, 64}, {0, 80},
	{0, 96}, {0, 112},
	{0, 0 }		/* Terminate */
};

PRIVATE HTTabStop tabs_24[] = {
	{0, 24}, {0, 48},		/* 3 columns */
	{0, 0 }		/* Terminate */
};

/* Template:
**	link to next, name, tag,
**	font, size, colour, 		superscript, anchor id,
**	indents: 1st, left, right, alignment	lineheight, descent,	tabs,
**	word wrap, free format, spaceBefore, spaceAfter, spacePara, flags.
**	strBefore, strAfter, onlyColor
*/

PRIVATE HTStyle HTStyleNormal = {
	0,  "Normal", "P",
	HT_FONT, 1.0, 0,		0, 0,
	4, 4, 4, HT_LEFT,		1, 0,	tabs_8,
	YES, YES, 2, 0,	2,		0
	,0,0,0};

PRIVATE HTStyle HTStyleBlockQuote = {
	&HTStyleNormal,  "BlockQuote", "BLOCKQUOTE",
	HT_FONT, 1.0, 0,		0, 0,
	8, 8, 4, HT_LEFT,		1, 0,	tabs_8,
	YES, YES, 2, 2,	2,		0
	,0 ,0, 0, 0 };

PRIVATE HTStyle HTStyleDirect = {
	&HTStyleBlockQuote,  "Dir", "DIR",
	HT_FONT, 1.0, 0,		0, 0,
	4, 4, 0, HT_LEFT,		1, 0,	tabs_16,
	NO, YES, 1, 1,	2,		0
	, 0, 0, 0,  0};

PRIVATE HTStyle HTStyleList = {
	&HTStyleDirect,  "List", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	4, 4, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	, 0, 0, "-", 0 };

PRIVATE HTStyle HTStyleList1 = {
	&HTStyleList,  "List1", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	8, 8, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 ,0, "-", 0 };

PRIVATE HTStyle HTStyleList2 = {
	&HTStyleList1,  "List2", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	16, 16, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 ,0, "-", 0 };

PRIVATE HTStyle HTStyleList3 = {
	&HTStyleList2,  "List3", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	20, 20, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 ,0,"-" ,0 };

PRIVATE HTStyle HTStyleList4 = {
	&HTStyleList3,  "List4", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	24, 24, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 ,0,"-", 0 };

PRIVATE HTStyle HTStyleList5 = {
	&HTStyleList4,  "List5", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	28, 28, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 ,0,"-", 0 };

PRIVATE HTStyle HTStyleList6 = {
	&HTStyleList5,  "List6", "UL",
	HT_FONT, 1.0, 0,		0, 0,
	32, 32, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 ,0,"-", 0};

PRIVATE HTStyle HTStyleMenu = {
	&HTStyleList6,  "Menu", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	4, 4, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 , 0,"*", 0 };

PRIVATE HTStyle HTStyleMenu1 = {
	&HTStyleMenu,  "Menu1", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	8, 8, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 , 0,"*" , 0 };

PRIVATE HTStyle HTStyleMenu2= {
	&HTStyleMenu1,  "Menu2", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	16, 16, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 , 0,"*", 0 };

PRIVATE HTStyle HTStyleMenu3= {
	&HTStyleMenu2,  "Menu3", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	20, 20, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 , 0,"*",0 };

PRIVATE HTStyle HTStyleMenu4= {
	&HTStyleMenu3,  "Menu4", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	24, 24, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 , 0,"*", 0 };

PRIVATE HTStyle HTStyleMenu5= {
	&HTStyleMenu4,  "Menu5", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	28, 28, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0 , 0,"*", 0 };

PRIVATE HTStyle HTStyleMenu6= {
	&HTStyleMenu5,  "Menu6", "MENU",
	HT_FONT, 1.0, 0,		0, 0,
	32, 32, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	1,		0
	,0,0,"*" ,0};

PRIVATE HTStyle HTStyleGlossary = {
	&HTStyleMenu6,  "Glossary", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	4, 8, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossary1 = {
	&HTStyleGlossary,  "Glossary1", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	8, 12, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossary2 = {
	&HTStyleGlossary1,  "Glossary2", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	12, 16, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossary3 = {
	&HTStyleGlossary2,  "Glossary3", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	16, 20, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossary4 = {
	&HTStyleGlossary3,  "Glossary4", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	24, 28, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossary5 = {
	&HTStyleGlossary4,  "Glossary5", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	32, 36, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossary6 = {
	&HTStyleGlossary5,  "Glossary6", "DL",
	HT_FONT, 1.0, 0,		0, 0,
	36, 40, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact = {
	&HTStyleGlossary6,  "GlossaryCompact", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	4, 4, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact1 = {
	&HTStyleGlossaryCompact,  "GlossaryCompact1", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	8, 8, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact2 = {
	&HTStyleGlossaryCompact1,  "GlossaryCompact2", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	12, 12, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact3 = {
	&HTStyleGlossaryCompact2,  "GlossaryCompact3", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	16, 16, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact4 = {
	&HTStyleGlossaryCompact3,  "GlossaryCompact4", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	24, 24, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact5 = {
	&HTStyleGlossaryCompact4,  "GlossaryCompact5", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	28, 28, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleGlossaryCompact6 = {
	&HTStyleGlossaryCompact5,  "GlossaryCompact6", "DLC",
	HT_FONT, 1.0, 0,		0, 0,
	32, 32, 2, HT_LEFT,		1, 0,	0,
	YES, YES, 1, 1,	1,		0
	,0,0, "", 0};

PRIVATE HTStyle HTStyleExample = {
	&HTStyleGlossaryCompact6,  "Example", "XMP",
	HT_FONT, 1.0, 0,		0, 0,
	4, 4, 2, HT_LEFT,		1, 0,	tabs_8,
	NO, NO, 2, 2,	2,		0
	,0,0, 0, 0};

PRIVATE HTStyle HTStylePreformatted = {
	&HTStyleExample,  	"Preformatted", "PRE",
	HT_FONT, 1.0, 0,		0, 0,
	2, 2, -1, HT_LEFT,		1, 0,	tabs_8,
	NO, NO, 2, 2,	2,		0
	,0,0, 0, 0};

PRIVATE HTStyle HTStyleListing =
	{ &HTStylePreformatted,  "Listing", "LISTING",
	HT_FONT, 1.0, 0,		0, 0,
	0, 0, -1, HT_LEFT,		1, 0,	tabs_8,
	NO, NO, 2, 2,	2,		0
	,0, 0, 0, 0};

PRIVATE HTStyle HTStyleAddress =
	{ &HTStyleListing,  "Address", "ADDRESS",
	HT_FONT, 1.0, 3,		0, 0,
	4, 4, 6, HT_LEFT,		1, 0,	0,
	YES, YES, 2, 2,	2,		0
	,"[ "," ]", 0 , 0};

PRIVATE HTStyle HTStyleHeading1 =
	{ &HTStyleAddress,  "Heading1", "H1",
	HT_FONT+HT_CAPITALS+HT_BOLD, 1.0, 1,		0, 0,
	2, 2, 2, HT_CENTER,		1, 0,	0,
	NO, YES, 2, 2, 2,			0
	,0,0 , 0, 0};

PRIVATE HTStyle HTStyleHeading2 =
	{ &HTStyleHeading1,  "Heading2", "H2",
	HT_FONT+HT_BOLD, 1.0, 2,		0, 0,
	2, 2, 10, HT_LEFT,		1, 0,	0,
	NO, YES, 2, 2, 2,			0
	,0, 0, 0, 0 };

PRIVATE HTStyle HTStyleHeading3 = {
	&HTStyleHeading2,  "Heading3", "H3",
	HT_FONT+HT_CAPITALS, 1.0, 3,		0, 0,
	2, 2, 10, HT_LEFT,		1, 0,	0,
	NO, YES, 2, 2,	2,		0
	,"*** "," ***", 0  , 0 };

PRIVATE HTStyle HTStyleHeading4 = {
	&HTStyleHeading3,  "Heading4", "H4",
	HT_FONT, 1.0, 4,		0, 0,
	4, 4, 10, HT_LEFT,		1, 0,	0,
	NO, YES, 2, 2,	2,		0
	,"### ", " ###", 0 , 0 };

PRIVATE HTStyle HTStyleHeading5 = {
	&HTStyleHeading4,  "Heading5", "H5",
	HT_FONT, 1.0, 5,		0, 0,
	4, 4, 10, HT_LEFT,		1, 0,	0,
	NO, YES, 2, 2,	2,		0
	,"### ", " ###", 0  , 0 };

PRIVATE HTStyle HTStyleHeading6 = {
	&HTStyleHeading5,  "Heading6", "H6",
	HT_FONT, 1.0, 6,		0, 0,
	8, 8, 10, HT_LEFT,		1, 0,	0,
	NO, YES, 2, 2,	2,		0
	,0, 0, 0 , 0};

PRIVATE HTStyle HTStyleHeading7 = {
	&HTStyleHeading6,  "Heading7", "H7",
	HT_FONT, 1.0, 7,		0, 0,
	10, 10, 10, HT_LEFT,		1, 0,	0,
	NO, YES, 2, 2,	2,		0
	,0, 0, 0  , 0};

PRIVATE HTStyle HTStyleBold = {
	&HTStyleHeading7,  "Bold", "B",
	0, 0, 1, 			0, 0,
	0, 0, 0, 0, 			1, 0, 0,
	0, 0, 0, 0,	0,		0,
	0, 0, 0, CHANGE_COLOR };

PRIVATE HTStyle HTStyleItalic = {
	&HTStyleBold,  "Italic", "I",
	0, 0, 2, 			0, 0,
	0, 0, 0, 0, 			1, 0, 0,
	0, 0, 0, 0,	0,		0,
	0, 0, 0, CHANGE_COLOR  };

PRIVATE HTStyle HTStyleEmphase = {
	&HTStyleItalic,  "Emphase", "EM",
	0, 0, 2, 			0, 0,
	0, 0, 0, 0, 			1, 0, 0,
	0, 0, 0, 0,	0,		0,
	0, 0, 0, CHANGE_COLOR 	};

PRIVATE HTStyle HTStyleUnderline = {
	&HTStyleEmphase,  "Underline", "U",
	0, 0, 4, 			0, 0,
	0, 0, 0, 0, 			1, 0, 0,
	0, 0, 0, 0,	0,		0,
	0, 0, 0, CHANGE_COLOR 	};


PRIVATE HTStyle HTStyleQuota = {
	&HTStyleUnderline,  "Quota", "Q",
	0, 0, 0, 			0, 0,
	0, 0, 0, 0, 			1, 0, 0,
	0, 0, 0, 0,	0,		0,
	"\"", "\"", 0, CHANGE_QUOTA	};


/* Style sheet points to the last in the list:
*/
PRIVATE HTStyleSheet sheet = { "default.style", &HTStyleQuota }; /* sheet */

PUBLIC HTStyleSheet * styleSheet = &sheet;

