/*
 	$Id: yuimacro.tab.c,v 3.2.2.1 2007/07/24 09:58:08 shelton Exp $
*/
#define YY_parse_h_included

/*  A Bison parser, made from yuimacro.y  */


#line 1 "/usr/local/lib/bison.cc"
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* HEADER SECTION */
#ifndef _MSDOS
#ifdef MSDOS
#define _MSDOS
#endif
#endif
/* turboc */
#ifdef __MSDOS__
#ifndef _MSDOS
#define _MSDOS
#endif
#endif

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc)
#include <alloca.h>
#else /* not sparc */
#if defined (_MSDOS)
#include <malloc.h>
#ifndef __TURBOC__
/* MS C runtime lib */
#ifndef __ZTC__
#define alloca _alloca
#endif
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */
#ifdef c_plusplus
#ifndef __cplusplus
#define __cplusplus
#endif
#endif
#ifdef __cplusplus
#ifndef YY_USE_CLASS
#define YY_USE_CLASS
#endif
#else
#ifndef __STDC__
#define const
#endif
#endif
#include <stdio.h>
#define YYBISON 1

/* #line 72 "/usr/local/lib/bison.cc" */
#define YY_parse_PARSE  yyparse
#define YY_parse_CLASS  Yuimacro
#define YY_parse_MEMBERS  					\
	char *script;					\
	char *ptr;					\
	char *endptr;					\
	varCollection local;				\
	varCollection global;				\
        int running;					\
        int continuing;					\
                                			\
	int skipComments();				\
	int backslash();				\
	int follow(int expect, int ifyes, int ifno);	\
	int follow2(int expect1, int expect2, int ifyes1, int ifyes2, int ifno);\
        void set_pos(short pos);			\
        vardata * set_var(char *ident, vardata *val);	\
        vardata * get_var(char *ident);			\
        vardata * call (vardata *obj, long hash, int argc);\
							\
	static Createdata creator;			\
     public:						\
        Yuimacro(char *Script);				\
	Yuimacro(const char *Script, int len);		\
        ~Yuimacro();					\
        int run(int argc, char **argv);			\
        void clearLocal();				\

#define YY_parse_CONSTRUCTOR_INIT  :	\
	script(0),		\
	ptr(0),			\
	endptr(0),		\
	continuing(0),		\
	running(0)		\

#define YY_parse_CONSTRUCTOR_CODE  	\

#line 44 "yuimacro.y"


// ][

#include "collect.h"

class vardata
{
  short refcount;
  unsigned fLock:1;
public:
  static void addref(vardata *data);
  static void delref(vardata *data);

  vardata();
  virtual ~vardata();

  virtual int toInt();
  virtual long type();
  virtual int cmp(vardata *);
  virtual vardata *copy();
  virtual vardata *process(long msgtype, int argc, YYSTYPE *stack);
};


class strdata: public vardata
{
  char *str;
  void assign(char *Str, int len);
  void assign(char *Str);
  void free();

public:
  strdata();
  ~strdata();
  virtual int toInt();
  virtual long type();
  virtual int cmp(vardata *);
  virtual vardata *copy();
  virtual vardata *process(long msgtype, int argc, YYSTYPE *stack);
};

class intdata: public vardata
{
  long long data;

public:
  intdata():data(0){}

  virtual int toInt();
  virtual long type();
  virtual int cmp(vardata *);
  virtual vardata *copy();
  virtual vardata *process(long msgtype, int argc, YYSTYPE *stack);
};


class varCollection: public SortedCollection
{
  void *keyOf(void *item) { return &(((vardata*)item)->type()); }
  int compare(void *key1, void *key2) { return *(long*)key1 < *(long*)key2 ? -1 : (*(long*)key1 < *(long*)key2  ? 1: 0 ) ;}
  void freeItem(void *item) { vardata::delref((vardata*)item); }
public:
  varCollection(){}
  ~varCollection(){freeAll();}
  vadata *vat(int i){ return (vardata*)at(i); }
};



#line 117 "yuimacro.y"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define YYSTACKSIZE 128
#define RUN if (running) {
#define RDEL(p) } else { delete (p) ; }
#define NUR }

static char word_delim[]="+-*/%=!,.:;<>$|@#^~`(){}[]&\\\n\r\t ";

// ][

#line 132 "yuimacro.y"
typedef union
{
  int i;
  vardata *obj;
  char *name;

  struct {
    short pos;
    char val;
    char prun;
  } w;

} yy_parse_stype;
#define YY_parse_STYPE yy_parse_stype

#line 72 "/usr/local/lib/bison.cc"
/* %{ and %header{ and %union, during decl */
#define YY_parse_BISON 1
#ifndef YY_parse_COMPATIBILITY
#ifndef YY_USE_CLASS
#define  YY_parse_COMPATIBILITY 1
#else
#define  YY_parse_COMPATIBILITY 0
#endif
#endif

#if YY_parse_COMPATIBILITY != 0
/* backward compatibility */
#ifdef YYLTYPE
#ifndef YY_parse_LTYPE
#define YY_parse_LTYPE YYLTYPE
#endif
#endif
#ifdef YYSTYPE
#ifndef YY_parse_STYPE
#define YY_parse_STYPE YYSTYPE
#endif
#endif
#ifdef YYDEBUG
#ifndef YY_parse_DEBUG
#define  YY_parse_DEBUG YYDEBUG
#endif
#endif
#ifdef YY_parse_STYPE
#ifndef yystype
#define yystype YY_parse_STYPE
#endif
#endif
#endif

#ifndef YY_parse_PURE

/* #line 107 "/usr/local/lib/bison.cc" */

#line 107 "/usr/local/lib/bison.cc"
/*  YY_parse_PURE */
#endif

/* section apres lecture def, avant lecture grammaire S2 */

/* #line 111 "/usr/local/lib/bison.cc" */

#line 111 "/usr/local/lib/bison.cc"
/* prefix */
#ifndef YY_parse_DEBUG

/* #line 113 "/usr/local/lib/bison.cc" */

#line 113 "/usr/local/lib/bison.cc"
/* YY_parse_DEBUG */
#endif


#ifndef YY_parse_LSP_NEEDED

/* #line 118 "/usr/local/lib/bison.cc" */

#line 118 "/usr/local/lib/bison.cc"
 /* YY_parse_LSP_NEEDED*/
#endif



/* DEFAULT LTYPE*/
#ifdef YY_parse_LSP_NEEDED
#ifndef YY_parse_LTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YY_parse_LTYPE yyltype
#endif
#endif
/* DEFAULT STYPE*/
      /* We used to use `unsigned long' as YY_parse_STYPE on MSDOS,
	 but it seems better to be consistent.
	 Most programs should declare their own type anyway.  */

#ifndef YY_parse_STYPE
#define YY_parse_STYPE int
#endif
/* DEFAULT MISCELANEOUS */
#ifndef YY_parse_PARSE
#define YY_parse_PARSE yyparse
#endif
#ifndef YY_parse_LEX
#define YY_parse_LEX yylex
#endif
#ifndef YY_parse_LVAL
#define YY_parse_LVAL yylval
#endif
#ifndef YY_parse_LLOC
#define YY_parse_LLOC yylloc
#endif
#ifndef YY_parse_CHAR
#define YY_parse_CHAR yychar
#endif
#ifndef YY_parse_NERRS
#define YY_parse_NERRS yynerrs
#endif
#ifndef YY_parse_DEBUG_FLAG
#define YY_parse_DEBUG_FLAG yydebug
#endif
#ifndef YY_parse_ERROR
#define YY_parse_ERROR yyerror
#endif
#ifndef YY_parse_PARSE_PARAM
#ifndef __STDC__
#ifndef __cplusplus
#ifndef YY_USE_CLASS
#define YY_parse_PARSE_PARAM
#ifndef YY_parse_PARSE_PARAM_DEF
#define YY_parse_PARSE_PARAM_DEF
#endif
#endif
#endif
#endif
#ifndef YY_parse_PARSE_PARAM
#define YY_parse_PARSE_PARAM void
#endif
#endif
/* TOKEN C */
#if YY_parse_COMPATIBILITY != 0
/* backward compatibility */
#ifdef YY_parse_LTYPE
#ifndef YYLTYPE
#define YYLTYPE YY_parse_LTYPE
#else
/* WARNING obsolete !!! user defined YYLTYPE not reported into generated header */
#endif
#endif
#ifndef YYSTYPE
#define YYSTYPE YY_parse_STYPE
#else
/* WARNING obsolete !!! user defined YYSTYPE not reported into generated header */
#endif
#ifdef YY_parse_PURE
#ifndef YYPURE
#define YYPURE YY_parse_PURE
#endif
#endif
#ifdef YY_parse_DEBUG
#ifndef YYDEBUG
#define YYDEBUG YY_parse_DEBUG
#endif
#endif
#ifndef YY_parse_ERROR_VERBOSE
#ifdef YYERROR_VERBOSE
#define YY_parse_ERROR_VERBOSE YYERROR_VERBOSE
#endif
#endif
#ifndef YY_parse_LSP_NEEDED
#ifdef YYLSP_NEEDED
#define YY_parse_LSP_NEEDED YYLSP_NEEDED
#endif
#endif


/* #line 225 "/usr/local/lib/bison.cc" */
#define	BREAK	258
#define	CONTINUE	259
#define	IF	260
#define	WHILE	261
#define	ELSE	262
#define	FOR	263
#define	RETURN	264
#define	NEW	265
#define	LSHIFT	266
#define	RSHIFT	267
#define	GLOBAL	268
#define	IDENT	269
#define	NAME	270
#define	CONSTANT	271
#define	REFASSIGN	272
#define	OR	273
#define	AND	274
#define	GE	275
#define	LE	276
#define	EQ	277
#define	NE	278
#define	CONTAIN	279
#define	NOTCONTAIN	280
#define	SIMILAR	281
#define	NOTSIMILAR	282
#define	POW	283
#define	INCR	284
#define	DECR	285
#define	UNARYMINUS	286
#define	METHOD	287


#line 225 "/usr/local/lib/bison.cc"
 /* #defines tokens */
#else
/* CLASS */
#ifndef YY_parse_CLASS
#define YY_parse_CLASS parse
#endif
#ifndef YY_parse_INHERIT
#define YY_parse_INHERIT
#endif
#ifndef YY_parse_MEMBERS
#define YY_parse_MEMBERS
#endif
#ifndef YY_parse_LEX_BODY
#define YY_parse_LEX_BODY
#endif
#ifndef YY_parse_ERROR_BODY
#define YY_parse_ERROR_BODY
#endif
#ifndef YY_parse_CONSTRUCTOR_PARAM
#define YY_parse_CONSTRUCTOR_PARAM
#endif
#ifndef YY_parse_CONSTRUCTOR_CODE
#define YY_parse_CONSTRUCTOR_CODE
#endif
#ifndef YY_parse_CONSTRUCTOR_INIT
#define YY_parse_CONSTRUCTOR_INIT
#endif

class YY_parse_CLASS YY_parse_INHERIT
{
public: /* static const int token ... */

/* #line 256 "/usr/local/lib/bison.cc" */
static const int BREAK;
static const int CONTINUE;
static const int IF;
static const int WHILE;
static const int ELSE;
static const int FOR;
static const int RETURN;
static const int NEW;
static const int LSHIFT;
static const int RSHIFT;
static const int GLOBAL;
static const int IDENT;
static const int NAME;
static const int CONSTANT;
static const int REFASSIGN;
static const int OR;
static const int AND;
static const int GE;
static const int LE;
static const int EQ;
static const int NE;
static const int CONTAIN;
static const int NOTCONTAIN;
static const int SIMILAR;
static const int NOTSIMILAR;
static const int POW;
static const int INCR;
static const int DECR;
static const int UNARYMINUS;
static const int METHOD;


#line 256 "/usr/local/lib/bison.cc"
 /* decl const */
public:
 int YY_parse_PARSE (YY_parse_PARSE_PARAM);
 virtual void YY_parse_ERROR(char *msg) YY_parse_ERROR_BODY;
#ifdef YY_parse_PURE
#ifdef YY_parse_LSP_NEEDED
 virtual int  YY_parse_LEX (YY_parse_STYPE *YY_parse_LVAL,YY_parse_LTYPE *YY_parse_LLOC) YY_parse_LEX_BODY;
#else
 virtual int  YY_parse_LEX (YY_parse_STYPE *YY_parse_LVAL) YY_parse_LEX_BODY;
#endif
#else
 virtual int YY_parse_LEX() YY_parse_LEX_BODY;
 YY_parse_STYPE YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
 YY_parse_LTYPE YY_parse_LLOC;
#endif
 int   YY_parse_NERRS;
 int    YY_parse_CHAR;
#endif
#if YY_parse_DEBUG != 0
 int YY_parse_DEBUG_FLAG;   /*  nonzero means print parse trace     */
#endif
public:
 YY_parse_CLASS(YY_parse_CONSTRUCTOR_PARAM);
public:
 YY_parse_MEMBERS
};
/* other declare folow */

/* #line 284 "/usr/local/lib/bison.cc" */
const int YY_parse_CLASS::BREAK=258;
const int YY_parse_CLASS::CONTINUE=259;
const int YY_parse_CLASS::IF=260;
const int YY_parse_CLASS::WHILE=261;
const int YY_parse_CLASS::ELSE=262;
const int YY_parse_CLASS::FOR=263;
const int YY_parse_CLASS::RETURN=264;
const int YY_parse_CLASS::NEW=265;
const int YY_parse_CLASS::LSHIFT=266;
const int YY_parse_CLASS::RSHIFT=267;
const int YY_parse_CLASS::GLOBAL=268;
const int YY_parse_CLASS::IDENT=269;
const int YY_parse_CLASS::NAME=270;
const int YY_parse_CLASS::CONSTANT=271;
const int YY_parse_CLASS::REFASSIGN=272;
const int YY_parse_CLASS::OR=273;
const int YY_parse_CLASS::AND=274;
const int YY_parse_CLASS::GE=275;
const int YY_parse_CLASS::LE=276;
const int YY_parse_CLASS::EQ=277;
const int YY_parse_CLASS::NE=278;
const int YY_parse_CLASS::CONTAIN=279;
const int YY_parse_CLASS::NOTCONTAIN=280;
const int YY_parse_CLASS::SIMILAR=281;
const int YY_parse_CLASS::NOTSIMILAR=282;
const int YY_parse_CLASS::POW=283;
const int YY_parse_CLASS::INCR=284;
const int YY_parse_CLASS::DECR=285;
const int YY_parse_CLASS::UNARYMINUS=286;
const int YY_parse_CLASS::METHOD=287;


#line 284 "/usr/local/lib/bison.cc"
 /* const YY_parse_CLASS::token */
/*apres const  */
YY_parse_CLASS::YY_parse_CLASS(YY_parse_CONSTRUCTOR_PARAM) YY_parse_CONSTRUCTOR_INIT
{
#if YY_parse_DEBUG != 0
YY_parse_DEBUG_FLAG=0;
#endif
YY_parse_CONSTRUCTOR_CODE;
};
#endif

/* #line 294 "/usr/local/lib/bison.cc" */


#define	YYFINAL		109
#define	YYFLAG		-32768
#define	YYNTBASE	52

#define YYTRANSLATE(x) ((unsigned)(x) <= 287 ? yytranslate[x] : 66)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    40,     2,     2,     2,    35,     2,     2,    45,
    46,    33,    31,    49,    32,    43,    34,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    44,    23,
    17,    21,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    50,     2,    51,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    47,     2,    48,    41,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    18,    19,    20,    22,    24,    25,    26,    27,    28,
    29,    30,    36,    37,    38,    39,    42
};

#if YY_parse_DEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     6,    11,    12,    18,    20,    23,    27,
    30,    31,    37,    40,    43,    46,    49,    52,    55,    59,
    60,    62,    66,    68,    70,    74,    78,    81,    87,    94,
    99,   106,   108,   112,   116,   120,   124,   128,   132,   136,
   140,   144,   148,   152,   156,   160,   164,   168,   171,   174,
   177,   178,   184,   185,   191
};

#endif

static const short yyrhs[] = {    -1,
    52,    57,     0,    44,     0,     5,    45,    62,    46,     0,
     0,     6,    56,    45,    62,    46,     0,    53,     0,    62,
    53,     0,    47,    52,    48,     0,    54,    57,     0,     0,
    54,    57,     7,    58,    57,     0,    55,    57,     0,     3,
    53,     0,     4,    53,     0,     9,    53,     0,    59,    53,
     0,    13,    14,     0,    59,    49,    14,     0,     0,    62,
     0,    60,    49,    62,     0,    16,     0,    14,     0,    14,
    17,    62,     0,    14,    18,    61,     0,    10,    14,     0,
    10,    14,    45,    60,    46,     0,    61,    43,    14,    45,
    60,    46,     0,    61,    50,    60,    51,     0,    61,    50,
    60,    51,    17,    62,     0,    61,     0,    45,    62,    46,
     0,    62,    31,    62,     0,    62,    32,    62,     0,    62,
    33,    62,     0,    62,    34,    62,     0,    62,    35,    62,
     0,    62,    36,    62,     0,    62,    11,    62,     0,    62,
    12,    62,     0,    62,    25,    62,     0,    62,    26,    62,
     0,    62,    22,    62,     0,    62,    24,    62,     0,    62,
    21,    62,     0,    62,    23,    62,     0,    37,    62,     0,
    38,    62,     0,    32,    62,     0,     0,    62,    19,    65,
    63,    62,     0,     0,    62,    20,    65,    64,    62,     0,
     0
};

#if YY_parse_DEBUG != 0
static const short yyrline[] = { 0,
   170,   171,   173,   175,   181,   182,   189,   190,   191,   193,
   197,   203,   207,   222,   223,   224,   225,   228,   229,   232,
   233,   234,   237,   238,   239,   240,   241,   242,   243,   244,
   245,   247,   248,   249,   250,   251,   253,   254,   255,   257,
   258,   259,   260,   261,   262,   263,   264,   265,   266,   267,
   269,   270,   277,   278,   287
};

static const char * const yytname[] = {   "$","error","$illegal.","BREAK","CONTINUE",
"IF","WHILE","ELSE","FOR","RETURN","NEW","LSHIFT","RSHIFT","GLOBAL","IDENT",
"NAME","CONSTANT","'='","REFASSIGN","OR","AND","'>'","GE","'<'","LE","EQ","NE",
"CONTAIN","NOTCONTAIN","SIMILAR","NOTSIMILAR","'+'","'-'","'*'","'/'","'%'",
"POW","INCR","DECR","UNARYMINUS","'!'","'~'","METHOD","'.'","';'","'('","')'",
"'{'","'}'","','","'['","']'","operlist","delim","if","while","@1","oper","@2",
"global","arglist","obj","expr","@3","@4","beg",""
};
#endif

static const short yyr1[] = {     0,
    52,    52,    53,    54,    56,    55,    57,    57,    57,    57,
    58,    57,    57,    57,    57,    57,    57,    59,    59,    60,
    60,    60,    61,    61,    61,    61,    61,    61,    61,    61,
    61,    62,    62,    62,    62,    62,    62,    62,    62,    62,
    62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
    63,    62,    64,    62,    65
};

static const short yyr2[] = {     0,
     0,     2,     1,     4,     0,     5,     1,     2,     3,     2,
     0,     5,     2,     2,     2,     2,     2,     2,     3,     0,
     1,     3,     1,     1,     3,     3,     2,     5,     6,     4,
     6,     1,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
     0,     5,     0,     5,     0
};

static const short yydefact[] = {     1,
     0,     0,     0,     0,     5,     0,     0,     0,    24,    23,
     0,     0,     0,     3,     0,     1,     7,     0,     0,     2,
     0,    32,     0,    14,    15,     0,     0,    16,    27,    18,
     0,     0,    50,    48,    49,     0,     0,    10,    13,     0,
    17,     0,    20,     0,     0,    55,    55,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
     0,     0,    20,    25,    26,    33,     9,    11,    19,     0,
     0,    21,    40,    41,    51,    53,    46,    44,    47,    45,
    42,    43,    34,    35,    36,    37,    38,    39,     4,     0,
     0,     0,    20,     0,    30,     0,     0,     6,    28,    12,
     0,    22,     0,    52,    54,    29,    31,     0,     0
};

static const short yydefgoto[] = {     1,
    17,    18,    19,    27,    20,    92,    21,    71,    22,    23,
    96,    97,    75
};

static const short yypact[] = {-32768,
    56,   -31,   -31,   -20,-32768,   -31,     3,    15,    18,-32768,
    39,    39,    39,-32768,    39,-32768,-32768,   115,   115,-32768,
   -35,   -40,   198,-32768,-32768,    39,   -14,-32768,    12,-32768,
    39,     2,-32768,-32768,-32768,   144,   101,    43,-32768,    44,
-32768,    49,    39,    39,    39,-32768,-32768,    39,    39,    39,
    39,    39,    39,    39,    39,    39,    39,    39,    39,-32768,
   162,    39,    39,   224,   -40,-32768,-32768,-32768,-32768,    19,
   -28,   224,   263,   263,-32768,-32768,   273,   273,   273,   273,
   273,   273,    62,    62,    31,    31,    31,-32768,-32768,   180,
   -27,   115,    39,    39,    51,    39,    39,-32768,-32768,-32768,
   -19,   224,    39,   241,   257,-32768,   224,    73,-32768
};

static const short yypgoto[] = {    58,
     5,-32768,-32768,-32768,   -13,-32768,-32768,   -39,    46,   -11,
-32768,-32768,    28
};


#define	YYLAST		309


static const short yytable[] = {    33,
    34,    35,    42,    36,    38,    39,    24,    25,    14,    43,
    28,     7,    14,    40,    61,     9,    29,    10,    99,    64,
    94,    94,    95,    91,    26,    41,   106,    60,    30,    94,
    62,    72,    73,    74,    31,    32,    77,    78,    79,    80,
    81,    82,    83,    84,    85,    86,    87,    88,     7,    68,
    90,    72,     9,   101,    10,   108,    63,    69,     2,     3,
     4,     5,    70,    93,     6,     7,    59,   103,     8,     9,
    11,    10,   109,    37,    76,    12,    13,    65,   100,     0,
     0,    72,   102,    15,   104,   105,     0,    11,     0,     0,
     0,   107,    12,    13,    56,    57,    58,    59,     0,    14,
    15,     0,    16,     2,     3,     4,     5,     0,     0,     6,
     7,     0,     0,     8,     9,     0,    10,     2,     3,     4,
     5,     0,     0,     6,     7,     0,     0,     8,     9,     0,
    10,     0,    11,     0,     0,     0,     0,    12,    13,     0,
     0,     0,     0,     0,    14,    15,    11,    16,    67,     0,
     0,    12,    13,     0,    44,    45,     0,     0,    14,    15,
     0,    16,    46,    47,    48,    49,    50,    51,    52,    53,
     0,     0,    44,    45,    54,    55,    56,    57,    58,    59,
    46,    47,    48,    49,    50,    51,    52,    53,     0,    66,
    44,    45,    54,    55,    56,    57,    58,    59,    46,    47,
    48,    49,    50,    51,    52,    53,     0,    89,    44,    45,
    54,    55,    56,    57,    58,    59,    46,    47,    48,    49,
    50,    51,    52,    53,     0,    98,     0,     0,    54,    55,
    56,    57,    58,    59,    44,    45,     0,     0,     0,     0,
     0,    14,    46,    47,    48,    49,    50,    51,    52,    53,
     0,    44,    45,     0,    54,    55,    56,    57,    58,    59,
    47,    48,    49,    50,    51,    52,    53,    44,    45,     0,
     0,    54,    55,    56,    57,    58,    59,    48,    49,    50,
    51,    52,    53,    44,    45,     0,     0,    54,    55,    56,
    57,    58,    59,    54,    55,    56,    57,    58,    59,     0,
     0,     0,     0,    54,    55,    56,    57,    58,    59
};

static const short yycheck[] = {    11,
    12,    13,    43,    15,    18,    19,     2,     3,    44,    50,
     6,    10,    44,    49,    26,    14,    14,    16,    46,    31,
    49,    49,    51,    63,    45,    21,    46,    23,    14,    49,
    45,    43,    44,    45,    17,    18,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    10,     7,
    62,    63,    14,    93,    16,     0,    45,    14,     3,     4,
     5,     6,    14,    45,     9,    10,    36,    17,    13,    14,
    32,    16,     0,    16,    47,    37,    38,    32,    92,    -1,
    -1,    93,    94,    45,    96,    97,    -1,    32,    -1,    -1,
    -1,   103,    37,    38,    33,    34,    35,    36,    -1,    44,
    45,    -1,    47,     3,     4,     5,     6,    -1,    -1,     9,
    10,    -1,    -1,    13,    14,    -1,    16,     3,     4,     5,
     6,    -1,    -1,     9,    10,    -1,    -1,    13,    14,    -1,
    16,    -1,    32,    -1,    -1,    -1,    -1,    37,    38,    -1,
    -1,    -1,    -1,    -1,    44,    45,    32,    47,    48,    -1,
    -1,    37,    38,    -1,    11,    12,    -1,    -1,    44,    45,
    -1,    47,    19,    20,    21,    22,    23,    24,    25,    26,
    -1,    -1,    11,    12,    31,    32,    33,    34,    35,    36,
    19,    20,    21,    22,    23,    24,    25,    26,    -1,    46,
    11,    12,    31,    32,    33,    34,    35,    36,    19,    20,
    21,    22,    23,    24,    25,    26,    -1,    46,    11,    12,
    31,    32,    33,    34,    35,    36,    19,    20,    21,    22,
    23,    24,    25,    26,    -1,    46,    -1,    -1,    31,    32,
    33,    34,    35,    36,    11,    12,    -1,    -1,    -1,    -1,
    -1,    44,    19,    20,    21,    22,    23,    24,    25,    26,
    -1,    11,    12,    -1,    31,    32,    33,    34,    35,    36,
    20,    21,    22,    23,    24,    25,    26,    11,    12,    -1,
    -1,    31,    32,    33,    34,    35,    36,    21,    22,    23,
    24,    25,    26,    11,    12,    -1,    -1,    31,    32,    33,
    34,    35,    36,    31,    32,    33,    34,    35,    36,    -1,
    -1,    -1,    -1,    31,    32,    33,    34,    35,    36
};

#line 294 "/usr/local/lib/bison.cc"
 /* fattrs + tables */

/* parser code folow  */


/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: dollar marks section change
   the next  is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (YY_parse_CHAR = YYEMPTY)
#define YYEMPTY         -2
#define YYEOF           0
#define YYACCEPT        return(0)
#define YYABORT         return(1)
#define YYERROR         goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL          goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do                                                              \
  if (YY_parse_CHAR == YYEMPTY && yylen == 1)                               \
    { YY_parse_CHAR = (token), YY_parse_LVAL = (value);                 \
      yychar1 = YYTRANSLATE (YY_parse_CHAR);                                \
      YYPOPSTACK;                                               \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    { YY_parse_ERROR ("syntax error: cannot back up"); YYERROR; }   \
while (0)

#define YYTERROR        1
#define YYERRCODE       256

#ifndef YY_parse_PURE
/* UNPURE */
#define YYLEX           YY_parse_LEX()
#ifndef YY_USE_CLASS
/* If nonreentrant, and not class , generate the variables here */
int     YY_parse_CHAR;                      /*  the lookahead symbol        */
YY_parse_STYPE      YY_parse_LVAL;              /*  the semantic value of the */
				/*  lookahead symbol    */
int YY_parse_NERRS;                 /*  number of parse errors so far */
#ifdef YY_parse_LSP_NEEDED
YY_parse_LTYPE YY_parse_LLOC;   /*  location data for the lookahead     */
			/*  symbol                              */
#endif
#endif


#else
/* PURE */
#ifdef YY_parse_LSP_NEEDED
#define YYLEX           YY_parse_LEX(&YY_parse_LVAL, &YY_parse_LLOC)
#else
#define YYLEX           YY_parse_LEX(&YY_parse_LVAL)
#endif
#endif
#ifndef YY_USE_CLASS
#if YY_parse_DEBUG != 0
int YY_parse_DEBUG_FLAG;                    /*  nonzero means print parse trace     */
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif
#endif



/*  YYINITDEPTH indicates the initial size of the parser's stacks       */

#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif


#if __GNUC__ > 1                /* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)       __builtin_memcpy(TO,FROM,COUNT)
#else                           /* not GNU C or C++ */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */

#ifdef __cplusplus
static void __yy_bcopy (char *from, char *to, int count)
#else
#ifdef __STDC__
static void __yy_bcopy (char *from, char *to, int count)
#else
static void __yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
#endif
#endif
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}
#endif

int
#ifdef YY_USE_CLASS
 YY_parse_CLASS::
#endif
     YY_parse_PARSE(YY_parse_PARSE_PARAM)
#ifndef __STDC__
#ifndef __cplusplus
#ifndef YY_USE_CLASS
/* parameter definition without protypes */
YY_parse_PARSE_PARAM_DEF
#endif
#endif
#endif
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YY_parse_STYPE *yyvsp;
  int yyerrstatus;      /*  number of tokens to shift before error messages enabled */
  int yychar1;          /*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];     /*  the state stack                     */
  YY_parse_STYPE yyvsa[YYINITDEPTH];        /*  the semantic value stack            */

  short *yyss = yyssa;          /*  refer to the stacks thru separate pointers */
  YY_parse_STYPE *yyvs = yyvsa;     /*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YY_parse_LSP_NEEDED
  YY_parse_LTYPE yylsa[YYINITDEPTH];        /*  the location stack                  */
  YY_parse_LTYPE *yyls = yylsa;
  YY_parse_LTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YY_parse_PURE
  int YY_parse_CHAR;
  YY_parse_STYPE YY_parse_LVAL;
  int YY_parse_NERRS;
#ifdef YY_parse_LSP_NEEDED
  YY_parse_LTYPE YY_parse_LLOC;
#endif
#endif

  YY_parse_STYPE yyval;             /*  the variable used to return         */
				/*  semantic values from the action     */
				/*  routines                            */

  int yylen;

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  YY_parse_NERRS = 0;
  YY_parse_CHAR = YYEMPTY;          /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YY_parse_LSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YY_parse_STYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YY_parse_LSP_NEEDED
      YY_parse_LTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
#ifdef YY_parse_LSP_NEEDED
		 &yyls1, size * sizeof (*yylsp),
#endif
		 &yystacksize);

      yyss = yyss1; yyvs = yyvs1;
#ifdef YY_parse_LSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  YY_parse_ERROR("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YY_parse_STYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YY_parse_LSP_NEEDED
      yyls = (YY_parse_LTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YY_parse_LSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (YY_parse_CHAR == YYEMPTY)
    {
#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Reading a token: ");
#endif
      YY_parse_CHAR = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (YY_parse_CHAR <= 0)           /* This means end of input. */
    {
      yychar1 = 0;
      YY_parse_CHAR = YYEOF;                /* Don't call YYLEX any more */

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(YY_parse_CHAR);

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	{
	  fprintf (stderr, "Next token is %d (%s", YY_parse_CHAR, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, YY_parse_CHAR, YY_parse_LVAL);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Shifting token %d (%s), ", YY_parse_CHAR, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (YY_parse_CHAR != YYEOF)
    YY_parse_CHAR = YYEMPTY;

  *++yyvsp = YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
  *++yylsp = YY_parse_LLOC;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


/* #line 699 "/usr/local/lib/bison.cc" */

  switch (yyn) {

case 4:
#line 175 "yuimacro.y"
{
                                  yyval.w.prun=running;
                                  if (running)
        			    running=yyval.w.val= yyvsp[-1].obj->toInt();
				;
    break;}
case 5:
#line 181 "yuimacro.y"
{ yyval.w.pos=ptr-script; ;
    break;}
case 6:
#line 182 "yuimacro.y"
{
                                  yyval.w.prun=running;
                                  if (running)
        			    running=yyval.w.val= yyvsp[-1].obj->toInt() ;
        			;
    break;}
case 8:
#line 190 "yuimacro.y"
{ RUN  CellData::delref(yyvsp[-1].obj); NUR ;
    break;}
case 10:
#line 193 "yuimacro.y"
{
                                    running = yyvsp[-1].w.prun;
	  	    		;
    break;}
case 11:
#line 197 "yuimacro.y"
{
				  if (running)
                                    running = 0;
                                  else if (yyvsp[-2].w.prun )
                                    running = 1;
				;
    break;}
case 12:
#line 203 "yuimacro.y"
{
                                    running = yyvsp[-4].w.prun;
	  			;
    break;}
case 13:
#line 207 "yuimacro.y"
{
				  if (!running)
				   {
                                    if (yyvsp[-1].w.prun && continuing)
                                      {
                                        continuing=0;
                                        set_pos(yyvsp[-1].w.pos);
                                      }
                                    else
                                      running = yyvsp[-1].w.prun;
                                   }
				  else
				    set_pos(yyvsp[-1].w.pos);
	  			;
    break;}
case 14:
#line 222 "yuimacro.y"
{ RUN  running=0; NUR ;
    break;}
case 15:
#line 223 "yuimacro.y"
{ RUN  running=0; continuing=1; NUR ;
    break;}
case 16:
#line 224 "yuimacro.y"
{ RUN  running=0; YYABORT;	NUR ;
    break;}
case 18:
#line 228 "yuimacro.y"
{;
    break;}
case 19:
#line 229 "yuimacro.y"
{;
    break;}
case 20:
#line 232 "yuimacro.y"
{ yyval.i=0; ;
    break;}
case 21:
#line 233 "yuimacro.y"
{ yyval.i=1; ;
    break;}
case 22:
#line 234 "yuimacro.y"
{ yyval.i=yyvsp[-2].i+1; ;
    break;}
case 23:
#line 237 "yuimacro.y"
{ RUN yyval.obj=yyvsp[0].obj; NUR 		;
    break;}
case 24:
#line 238 "yuimacro.y"
{ RUN yyval.obj=get_var(yyvsp[0].name); NUR 	;
    break;}
case 25:
#line 239 "yuimacro.y"
{ RUN yyval.obj=set_var(yyvsp[-2].name,yyvsp[0].obj); NUR 	;
    break;}
case 26:
#line 240 "yuimacro.y"
{ RUN yyval.obj=set_var(yyvsp[-2].name,yyvsp[0].obj); NUR    ;
    break;}
case 27:
#line 241 "yuimacro.y"
{ RUN yyval.obj=call(&creater, hashstr(yyvsp[0].name),0); NUR  ;
    break;}
case 28:
#line 242 "yuimacro.y"
{ RUN yyval.obj=call(&creater, hashstr(yyvsp[-3].name),yyvsp[-1].i); NUR ;
    break;}
case 29:
#line 243 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-5].obj,hashstr(yyvsp[-3].name),yyvsp[-1].i); NUR ;
    break;}
case 30:
#line 244 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-3].obj,HASH_fetch,yyvsp[-1].i); NUR  ;
    break;}
case 31:
#line 245 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-5].obj,HASH_fetch,yyvsp[-3].i+1); NUR ;
    break;}
case 32:
#line 247 "yuimacro.y"
{ RUN yyval.obj=yyvsp[0].obj->copy(); vardata::delref(yyvsp[0].obj); NUR ;
    break;}
case 33:
#line 248 "yuimacro.y"
{ RUN yyval.obj=yyvsp[-1].obj; NUR ;
    break;}
case 34:
#line 249 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_add,2); NUR ;
    break;}
case 35:
#line 250 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_sub,2); NUR ;
    break;}
case 36:
#line 251 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_mul,2); NUR ;
    break;}
case 37:
#line 253 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_div,2); NUR ;
    break;}
case 38:
#line 254 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_mod,2); NUR ;
    break;}
case 39:
#line 255 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_pow,2); NUR ;
    break;}
case 40:
#line 257 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_lshift,2); NUR ;
    break;}
case 41:
#line 258 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_rshift,2); NUR ;
    break;}
case 42:
#line 259 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_eq,2); NUR ;
    break;}
case 43:
#line 260 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_ne,2); NUR ;
    break;}
case 44:
#line 261 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_ge,2); NUR ;
    break;}
case 45:
#line 262 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_le,2); NUR ;
    break;}
case 46:
#line 263 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_gt,2); NUR ;
    break;}
case 47:
#line 264 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[-2].obj,HASH_lt,2); NUR ;
    break;}
case 48:
#line 265 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[0].obj,HASH_incr,2); NUR ;
    break;}
case 49:
#line 266 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[0].obj,HASH_decr,2); NUR ;
    break;}
case 50:
#line 267 "yuimacro.y"
{ RUN yyval.obj=call(yyvsp[0].obj,HASH_inv,1); NUR ;
    break;}
case 51:
#line 269 "yuimacro.y"
{ RUN running = !yyvsp[-2].obj->toInt(); NUR ;
    break;}
case 52:
#line 270 "yuimacro.y"
{
	  		  if (running)
                            { yyval.obj=yyvsp[0].obj; vardata::delref(yyvsp[-4].obj); }
                          else if (yyvsp[-2].w.prun)
                            { yyval.obj=yyvsp[-4].obj; vardata::delref(yyvsp[0].obj); }
	                  running = yyvsp[-2].w.prun;
	                ;
    break;}
case 53:
#line 277 "yuimacro.y"
{ RUN running = yyvsp[-2].obj->toInt(); NUR ;
    break;}
case 54:
#line 278 "yuimacro.y"
{
	  		  if (running)
                            { yyval.obj=yyvsp[0].obj; vardata::delref(yyvsp[-4].obj); }
                          else if (yyvsp[-2].w.prun)
                            { yyval.obj=yyvsp[-4].obj; vardata::delref(yyvsp[0].obj); }
	                  running = yyvsp[-2].w.prun;
	  		;
    break;}
case 55:
#line 287 "yuimacro.y"
{ yyval.w.prun=running;;
    break;}
}

#line 699 "/usr/local/lib/bison.cc"
   /* the action file gets copied in in place of this dollarsign  */
  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YY_parse_LSP_NEEDED
  yylsp -= yylen;
#endif

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YY_parse_LSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = YY_parse_LLOC.first_line;
      yylsp->first_column = YY_parse_LLOC.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++YY_parse_NERRS;

#ifdef YY_parse_ERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      YY_parse_ERROR(msg);
	      free(msg);
	    }
	  else
	    YY_parse_ERROR ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YY_parse_ERROR_VERBOSE */
	YY_parse_ERROR("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (YY_parse_CHAR == YYEOF)
	YYABORT;

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Discarding token %d (%s).\n", YY_parse_CHAR, yytname[yychar1]);
#endif

      YY_parse_CHAR = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;              /* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YY_parse_LSP_NEEDED
  yylsp--;
#endif

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
  *++yylsp = YY_parse_LLOC;
#endif

  yystate = yyn;
  goto yynewstate;
}

/* END */

/* #line 893 "/usr/local/lib/bison.cc" */
#line 290 "yuimacro.y"


// ][



Yuimacro::~Yuimacro()
{
  if (script)
    delete script;
}



Yuimacro::Yuimacro(char *Script):
	script(Script),
	continuing(0), running(1)
{
  ptr=script;
  if (script)
    endptr=script+strlen(script);
  else
    endptr=0;
}

Yuimacro::Yuimacro(const char *Script, int len):
	continuing(0), running(1)
{
  script=new char [len+1];
  memcpy(script, Script, len);
  script[len]=0;
  ptr=script;
  endptr=script+len;
}


int Yuimacro::run(int argc, char **argv)
{
    if (!script)
      return 0;
    ptr=script;


    int ret=yyparse();
    if (ret)
     {
       return 0;
     }
    return 1;
}

void Yuimacro::yyerror(char *name)
{
  fprintf(stderr, "%s\n", name);
  fflush(stderr);
}

int Yuimacro::skipComments()
{
   int commenting=0;
   int c;
     for (int first=1; first || commenting ; )
      {
         first=0;
         c=*ptr;
         if(  !c )
           return -1;
         if (c=='/' && ptr[1]=='/' || c=='#')
           {
             //comment to eol
             while(*ptr && *ptr!='\n')ptr++;
             if( !*ptr)
                return -1;
           }
         else if (c=='/' && ptr[1]=='*')
           {
             // begin comments
             commenting++;
             if ( ! *++ptr ) return -1;
             if ( ! *++ptr ) return -1;
           }
         else if (c=='*' && ptr[1]=='/')
           {
             // end comments
             commenting--;
             if ( ! *++ptr ) return -1;
             if ( ! *++ptr ) return -1;
           }
         else
             if ( ! *++ptr ) return -1;
      }
return 0;
}

int Yuimacro::backslash()
{
  static char transtab[]="b\bf\fn\nr\rt\tv\v";
  char *st;
  int c=*ptr;
  if ( !c )
    return -1;
  if (c!='\\') return c;
  if ( (c=*++ptr)==0 )  return -1;
  if (islower(c) && (st=strchr(transtab,c) )!=0 )
     return st[1];
  if (strchr("xX0123456789",c))
    {
      char dig[5], *av;
      int i,xid,count;
	  if (c=='x'|| c=='X') xid=1; else xid=0;
      if (xid) { av="0123456789abcefdABCDEF";
                 if ( !*++ptr )  return -1;
                 count=2;}
      else {av="0123456789"; count=3;}
      for (i=0; ; )
        {
          c=*ptr;
          if (strchr(av,c))
	    dig[i]=c;
	  else
	    break;
          if ( ++i>=count)
            break;
          if ( !ptr[1] )  return -1;
          if ( !strchr(av, ptr[1]) )
            break;
          if ( !*++ptr )  return -1;
        }
      dig[i]=0;
      i=0;
      if (xid)
         sscanf(dig,"%x",&i);
      else
         sscanf(dig,"%o",&i);
      return i;
    }
  return c;
}

int Yuimacro::follow(int expect, int ifyes, int ifno)
{
  if ( !*++ptr ) return 0;
  if ( *ptr==expect)
      { ptr++; return ifyes;}
  return ifno;
}

int Yuimacro::follow2(int expect1, int expect2, int ifyes1, int ifyes2, int ifno)
{
  if (!*++ptr) return 0;
  if (*ptr==expect1) { ++ptr; return ifyes1; }
  if (*ptr==expect2) { ++ptr; return ifyes2; }
  return ifno;
}

int Yuimacro::yylex()
{
  if (!script)
    return 0;
/*
%token    BREAK CONTINUE IF WHILE ELSE FOR
%token<s> IDENT STRING

  // or # - комментарий до конца строки
  // / *   * /  - комментарий

  \   в конце строки экранирует '\n'
  "" или '' ограничивают строку
*/


 int c ;
 int done, lenword;

 // skip all spaces, tabs & shifted by \ string_ends
beglex:
 for( done=0; !done ;)
    {
      c=*ptr;
      switch(c)
        {
            case 0:
              return 0; //PROGRAM_END
            case ' ':
            case '\t':
            case '\r':
                     ++ptr;
                     continue;
            case '\\':
            	 if(ptr[1]=='\n')
            	    { ptr+=2; continue; }

            default:
                 done=1; break;
        }
    }

    char *p, *curword;
    if (c=='"' || c=='\'' || c=='`')  // begin string
	   {
		  int quote, c1;
		  quote=c;
                  lenword=8;
                  if (running)
                    curword=(char*)malloc(lenword);

		  for(p=curword; (c=*++ptr)!=quote; p++)
			{
                          if (running && p-curword>=lenword-1)
                            {
                              if (lenword>31)
                                lenword+=lenword/4;
                              else
                                lenword*=2;
                              curword=(char*)realloc(curword, lenword);
                            }
			  if (! c ) return 0;
			  if ( c=='\\' &&  ptr[1]=='\n' )
			    {if  (*++ptr) p--; }
			  else
			    {
			      c1=backslash();
			      if ( c1 == -1)
			        return 0;
			      if (running)
			        *p=c1;
			    }
			}
                  ++ptr;
                  if (running)
                   { *p=0; yylval.s = new Cell(curword); }
                  return STRING;
	   }
    // comments detection

    if  ( c=='#' || c=='/' && ( ptr[1]=='/'|| ptr[1]=='*' ) )
     {
       if ( skipComments() )
         return 0;
       goto beglex;
     }

    #if 0
        c=*ptr;
        int indig = isdigit(c) ? 1 :0;
        if ( !indig)
        {
          if ( c=='.' && isdigit(ptr[1]))
            indig=2;
        }
	for( lenword=0 ;  (c=*ptr) &&
		(
                   !strchr(word_delim, c)
		   || indig
		      && isdigit(ptr[1])
                      && (c=='-' || c=='+')
                      && (tolower(ptr[1])=='e' )
                   || indig
                      && c=='.' && ptr[1]!='.'
                )
		   ; ptr++, lenword++ )
	 {
	      curword[lenword]=c;
	 }
     #endif

        if (*p=='$')
         { // ident
           if (!*++ptr) return 0;
           p=strpbrk(ptr, word_delim);
           if (!p ) p=(char*)endptr;
           lenword=p-ptr;
           if (running)
              yylval.s=new Cell(p, lenword);
           ptr=p;
           return IDENT;
         }

        p=strpbrk(ptr, word_delim);
        if (!p )
           p=(char*)endptr;

        lenword=p-ptr;
        curword=(char*)ptr;


	if (lenword)
	   {
                 ptr+=lenword;
                 if (!strcmp(curword,"return")) return RETURN;
                 if (!strcmp(curword,"while")) return WHILE;
                 if (!strcmp(curword,"if")) return IF;
                 if (!strcmp(curword,"for")) return FOR;
                 if (!strcmp(curword,"else")) return ELSE;
                 if (!strcmp(curword,"break")) return BREAK;
                 if (!strcmp(curword,"continue")) return CONTINUE;
            #if 0
                 if (isdigit(c=curword[0]) ) // double or long var
			 {
                           if ( curword[1]=='x' || curword[1]=='X' )
                           	{
					yylval.i = strtol(curword, 0, 16);
					return LONG;
                           	}
			   else if (strpbrk(curword,".eE"))
				  {
					yylval.d = atof(curword);
					return DOUBLE;
				  }
			   else
				  {
					yylval.i = strtol(curword, 0, 10);
					return LONG;
				  }
			 }
		 else // name
	     #endif
		   {
                       if (running)
                         yylval.s=new Cell(curword, lenword);
                       return STRING;
                   }
       }
    else if (! *ptr )
       return 0;
    else
       switch(c=*ptr)
         {
           case '-': return follow('-',DECR,'-');
           case '+': return follow('+',INCR, '+');
           case '*': return follow('*',POW, '*');
           case '>': return follow2('=','>',GE,RSHIFT,'>');
           case '<': return follow2('=','<',LE,LSHIFT,'<');
           case '=': return follow('=',EQ,'=');
           case '!': return follow('=', NE,'!');
           case '|': return follow('|',OR,'|');
           case '&': return follow('&',AND,'&');
           default: ++ptr;
                    return c;
         }
 return 0 ;
}


void Yuimacro::set_pos(short pos)
{
  ptr=script+pos;
}

vardata *Yuimacro::set_var(char *ident, vardata *val)
{
}

vardata * Yuimacro::get_var(char *ident)
{
}

vardata * Yuimacro::call (vardata *obj, long hash, int argc)
{
  vardata *ret=obj->process( hasn, argc, YYSP );
  for(int i=0; i<argc; i++)
    vardata::delref((YYSP-i)->obj)
}

// ][

strdata::strdata(char *Str, int len):
	str(0)
{
  assign(Str, len);
}

strdata::strdata(char *Str):
	str(0)
{
  assign(Str);
}


void strdata::assign(char *Str, int len)
{
  realloc(str, len=1);
  memcpy(str,Str,len);
  str[len]=0;
}

void strdata::assign(char *Str)
{
  if (str)
    ::free(str);
  str=Str;
}

int  strdata::toInt()
{
  if (str)
    return atoi(str);
  else
    return 0;
}

double strdata::toDouble()
{
  if (str)
    return strtod(str,0);
  else
    return 0;
}

void strdata::free()
{
   if (str)
     ::free(str);
   str=0;
}

vardata *strdata::copy()
{
  return new strdata(str, strlen(str));
}

//  ][

