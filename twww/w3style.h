#ifndef _W3STYLE_H_
#define _W3STYLE_H_

typedef double HTCoord;
typedef int HTColor;

typedef struct {
    short               kind;           /* only NX_LEFTTAB implemented*/
    HTCoord             position;       /* x coordinate for stop */
} HTTabStop;

typedef long int HTLMFont;      /* For now */

#define HT_NON_BREAK_SPACE ((char)1)    /* For now */

#define HT_FONT         0
#define HT_CAPITALS     1
#define HT_BOLD         2
#define HT_UNDERLINE    4
#define HT_INVERSE      8
#define HT_DOUBLE       0x10

#define HT_BLACK        0
#define HT_WHITE        1
#define HT_HI           2
#define HT_FRAME        3

struct _HTStyle {

/*      Style management information
*/
    struct _HTStyle     *next;          /* Link for putting into stylesheet */
    char *              name;           /* Style name */
    char *              SGMLTag;        /* Tag name to start */


/*      Character attributes    (a la NXRun)
*/
    HTFont              font;           /* Font id */
    HTCoord             fontSize;       /* The size of font, not independent */
    HTColor             color;  /* text gray of current run */
    int                 superscript;    /* superscript (-sub) in points */

    HTAnchor            *anchor;        /* Anchor id if any, else zero */

/*      Paragraph Attribtes     (a la NXTextStyle)
*/
    HTCoord             indent1st;      /* how far first line in paragraph is
                                 * indented */
    HTCoord             leftIndent;     /* how far second line is indented */
    HTCoord             rightIndent;    /* (Missing from NeXT version */
    short               alignment;      /* quad justification */
    HTCoord             lineHt;         /* line height */
    HTCoord             descentLine;    /* descender bottom from baseline */
    HTTabStop           *tabs;          /* array of tab stops, 0 terminated */

    BOOL                wordWrap;       /* Yes means wrap at space not char */
    BOOL                freeFormat;     /* Yes means \n is just white space */
    HTCoord             spaceBefore;    /* Omissions from NXTextStyle */
    HTCoord             spaceAfter;
    HTCoord             spacePara;
    int                 paraFlags;      /* Paragraph flags, bits as follows: */

#define PARA_KEEP       1       /* Do not break page within this paragraph */
#define PARA_WITH_NEXT  2       /* Do not break page after this paragraph */

#define HT_JUSTIFY 0            /* For alignment */
#define HT_LEFT 1
#define HT_RIGHT 2
#define HT_CENTER 3

    const char *	strBefore;
    const char *	strAfter;
    const char *	strList;
    int			onlyColor; /* 1 keep current font metric and set color */
				   /* 2 keep current font metric and set quotation */
#define CHANGE_COLOR 1
#define CHANGE_QUOTA 2
};

#endif
