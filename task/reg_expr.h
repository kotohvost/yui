/*
	$Id: reg_expr.h,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#ifndef REG_EXPR_H
#define REG_EXPR_H

#ifndef RE_NREGS
#define RE_NREGS       16  /* number of registers available */
typedef struct re_registers
{
  int start[RE_NREGS];  /* start offset of region */
  int end[RE_NREGS];    /* end offset of region */
} *regexp_registers_t;
#endif

struct Re_pattern_buffer;

class Regexpr
{
  Regexpr(const Regexpr&) 	    {}  // no X(X&)
  void  operator = (const Regexpr&) {}  // no assignment

  struct Re_pattern_buffer *buf;
  unsigned ignorcase:1
	   ;
  void reg_free();

public:
  Regexpr(int ignoreCase=0 );
  Regexpr(const char *regex, int ignoreCase=0 );
  ~Regexpr();

  int OK() const { return buf!=0; }
  operator struct Re_pattern_buffer * () { return buf; }

  char *compile( const char *regex, int ignoreCase=0 );

  char *charset();

     /* return static char[256] array, where 1 means that this char
	can appear in string */

  int syntax(int syntax);

     /* This sets the syntax to use and returns the previous syntax.  The
	syntax is specified by a bit mask of the above defined bits. */

  int match( const char *string, int size, int pos,
	     re_registers *regs);
     /* This tries to match the regexp against the string.  This returns the
	length of the matched portion, or -1 if the pattern could not be
	matched and -2 if an error (such as failure stack overflow) is
	encountered. */

  int match2( const char *string1, int size1,
	      const char *string2, int size2,
	      int pos, re_registers *regs);

     /* This tries to match the regexp to the concatenation of string1 and
	string2.  This returns the length of the matched portion, or -1 if the
	pattern could not be matched and -2 if an error (such as failure stack
	overflow) is encountered. */

  int search( const char *string, int size,
	      int startpos, int range, re_registers *regs);

    /* This rearches for a substring matching the regexp.  This returns the first
       index at which a match is found.  range specifies at how many positions to
       try matching; positive values indicate searching forwards, and negative
       values indicate searching backwards.  mstop specifies the offset beyond
       which a match must not go.  This returns -1 if no match is found, and
       -2 if an error (such as failure stack overflow) is encountered. */

  int search2(const char *string1, int size1,
	      const char *string2, int size2,
	      int startpos, int range,
	      re_registers *regs);

    /* This is like re_search, but search from the concatenation of string1 and
       string2.  */



  /* start sytax is RE_SYNTAX_AWK | RE_ANSI_HEX  */

};


  /* bit definitions for syntax */
#define NO_BK_PARENS		1     /* no quoting for parentheses  */
#define  NO_BK_VBAR		2     /* no quoting for vertical bar */
#define  BK_PLUS_QM		4     /* quoting needed for + and ? */
#define  TIGHT_VBAR		8     /* | binds tighter than ^ and $ */
#define  NEWLINE_OR		16    /* treat newline as or */
#define  CONTEXT_INDEP_OPS	32    /* ^$?*+ are special in all contexts */
#define  ANSI_HEX		64    /* ansi sequences (\n etc) and \xhh */
#define  NO_GNU_EXTENSIONS	128   /* no gnu extensions */

  /* definitions for some common regexp styles */
#define SYNTAX_AWK		(NO_BK_PARENS | NO_BK_VBAR | CONTEXT_INDEP_OPS)
#define SYNTAX_EGREP		(SYNTAX_AWK | NEWLINE_OR)
#define SYNTAX_GREP		(BK_PLUS_QM | NEWLINE_OR)
#define SYNTAX_EMACS		0


#endif /* REGEXPR_H */
