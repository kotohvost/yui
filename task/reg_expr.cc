/*
	$Id: reg_expr.cc,v 3.2.2.1 2007/07/24 09:58:07 shelton Exp $
*/
#include <stdlib.h>
#include <string.h>

#include "reg_expr.h"

extern "C" {
#include "regexpr.h"
}

#ifdef DMALLOC
 #include <dmalloc.h>
#endif

static unsigned char ignorCaseTranslateTable[256]=
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,
27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,
51,52,53,54,55,56,57,58,59,60,61,62,63,64,'a','b','c','d','e','f','g','h',
'i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
91,92,93,94,95,96,'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
'o','p','q','r','s','t','u','v','w','x','y','z',123,124,125,126,127
#ifdef __MSDOS__
// russian alternate charset (866 codepage)
,
//'‰','ˆ','½','°','€','Š','¤','‚','¥','»','¸','±',' ','¾','¹','º',
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
//'·','ª','©','¢','¯','«','†','','','Ž','‹','…','¼','Œ','„','¶',
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
//'‰','ˆ','½','°','€','Š','¤','‚','¥','»','¸','±',' ','¾','¹','º',
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
//'·','ª','©','¢','¯','«','†','','','Ž','‹','…','¼','Œ','„','¶',
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
241,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};
// IBM PC charset
/*
,
135,129,130,131,132,133,134,135,136,137,138,139,140,141,132,134,
130,145,146,147,148,149,150,151,152,153,129,155,156,157,158,159,
160,161,162,163,164,164,166,167,168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};
*/
#else
// KOI-8 part
,
/*
128,129,130,131,132,133,134,135,136,137,138,139,140,140,142,143,
144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,164,166,167,168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223
*/
 0xc4, 0xb3, 0xda, 0xbf, 0xc0, 0xd9, 0xc3, 0xb4, 0xc2, 0xc1, 0xc5, 0xdf, 0xdc, 0xdb, 0xdd, 0xde,
 0xb0, 0xb1, 0xb2, 0xf4, 0xfe, 0xf9, 0xfb, 0xf7, 0xf3, 0xf2, 0xff, 0xf5, 0xf8, 0xfd, 0xfa, 0xf6,
 0xcd, 0xba, 0xd5, 0xf1, 0xd6, 0xc9, 0xb8, 0xb7, 0xbb, 0xd4, 0xd3, 0xc8, 0xbe, 0xbd, 0xbc, 0xc6,
 0xc7, 0xcc, 0xb5, 0xf0, 0xb6, 0xb9, 0xd1, 0xd2, 0xcb, 0xcf, 0xd0, 0xca, 0xd8, 0xd7, 0xce, 0xfc,
 0xee, 0xa0, 0xa1, 0xe6, 0xa4, 0xa5, 0xe4, 0xa3, 0xe5, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae,
 0xaf, 0xef, 0xe0, 0xe1, 0xe2, 0xe3, 0xa6, 0xa2, 0xec, 0xeb, 0xa7, 0xe8, 0xed, 0xe9, 0xe7, 0xea,
 0xee, 0xa0, 0xa1, 0xe6, 0xa4, 0xa5, 0xe4, 0xa3, 0xe5, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae,
 0xaf, 0xef, 0xe0, 0xe1, 0xe2, 0xe3, 0xa6, 0xa2, 0xec, 0xeb, 0xa7, 0xe8, 0xed, 0xe9, 0xe7, 0xea,
};
#endif

static unsigned char translateTable[256]=
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,
27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,
51,52,53,54,55,56,57,58,59,60,61,62,63,64,'A','B','C','D','E','F','G','H',
'I','J','K','L','M','N','O','P','Q','R','S','R','U','V','W','X','Y','Z',
91,92,93,94,95,96,'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
'o','p','q','r','s','t','u','v','w','x','y','z',123,124,125,126,127
#ifdef __MSDOS__
// russian alternate charset (866 codepage)
,
128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
241,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};
// IBM PC charset
/*
,
135,129,130,131,132,133,134,135,136,137,138,139,140,141,132,134,
130,145,146,147,148,149,150,151,152,153,129,155,156,157,158,159,
160,161,162,163,164,164,166,167,168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};
*/
#else
// KOI-8 part
,
 0xc4, 0xb3, 0xda, 0xbf, 0xc0, 0xd9, 0xc3, 0xb4, 0xc2, 0xc1, 0xc5, 0xdf, 0xdc, 0xdb, 0xdd, 0xde,
 0xb0, 0xb1, 0xb2, 0xf4, 0xfe, 0xf9, 0xfb, 0xf7, 0xf3, 0xf2, 0xff, 0xf5, 0xf8, 0xfd, 0xfa, 0xf6,
 0xcd, 0xba, 0xd5, 0xf1, 0xd6, 0xc9, 0xb8, 0xb7, 0xbb, 0xd4, 0xd3, 0xc8, 0xbe, 0xbd, 0xbc, 0xc6,
 0xc7, 0xcc, 0xb5, 0xf0, 0xb6, 0xb9, 0xd1, 0xd2, 0xcb, 0xcf, 0xd0, 0xca, 0xd8, 0xd7, 0xce, 0xfc,
 0xee, 0xa0, 0xa1, 0xe6, 0xa4, 0xa5, 0xe4, 0xa3, 0xe5, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae,
 0xaf, 0xef, 0xe0, 0xe1, 0xe2, 0xe3, 0xa6, 0xa2, 0xec, 0xeb, 0xa7, 0xe8, 0xed, 0xe9, 0xe7, 0xea,
 0x9e, 0x80, 0x81, 0x96, 0x84, 0x85, 0x94, 0x83, 0x95, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
 0x8f, 0x9f, 0x90, 0x91, 0x92, 0x93, 0x86, 0x82, 0x9c, 0x9b, 0x87, 0x98, 0x9d, 0x99, 0x97, 0x9a
};
#endif




Regexpr::Regexpr( int ignoreCase ) : buf(0), ignorcase(ignoreCase)
{
}

Regexpr::Regexpr(const char *regex, int IgnorCase ):
	buf(0)
{
  compile(regex, IgnorCase);
}

Regexpr::~Regexpr()
{
  reg_free();
}

void Regexpr::reg_free()
{
  if (!buf)
    return;

  if( buf->buffer)
    ::free(buf->buffer);

  if (buf->fastmap)
    delete buf->fastmap;

  delete buf;
  buf = 0;
}

char *Regexpr::compile( const char *regex, int IgnorCase )
{
  reg_free();
  ignorcase=IgnorCase;
  buf = new re_pattern_buffer;
  memset(buf, 0, sizeof(*buf));
  if (ignorcase)
    buf->translate=(char*)ignorCaseTranslateTable;
  else
    buf->translate=(char*)translateTable;

  char *ret=re_compile_pattern((char*)regex, strlen(regex), buf);
  if ( ret )
     reg_free();

  if( !ret )
   {
      buf->fastmap=new char[256];
      re_compile_fastmap(buf);
   }

  return ret;
}

char *Regexpr::charset()
{
  if (!buf)
    return 0;
  return re_av_chars(buf);
}

int Regexpr::syntax(int Syntax)
{
       return re_set_syntax(Syntax);
}

int Regexpr::match( const char *string, int slen, int from,
	re_registers *regs)
{
      if (slen==0)
	return -1;
      if (from<0 || from >= slen)
	return -1;
      return re_match( buf, (char*)string, slen, from, regs);
}

int Regexpr::match2( const char *string1, int slen1,
		     const char *string2, int slen2,
		     int from, re_registers *regs )
{
      if ( slen1+slen2==0 )
	return -1;
      if (from<0 || from >= slen1+slen2 )
	return -1;
      return re_match_2( buf, (char*)string1, slen1, (char*)string2, slen1,
			 from, regs, slen1+slen2 );
}

int Regexpr::search( const char *string, int slen, int from,
	 int size, re_registers *regs)
{
/*
      if (slen==0)
	return -1;
      if (from<0 || from >= slen)
	return -1;
      if ( size >= slen-from )
	size=slen-from-1;
*/
      if ( from<0 || from > slen )
	return -1;
      if ( size > slen-from )
	size=slen-from;
      else if (size < 0 && size < -from )
	size = - from ;

      return re_search( buf, (char*)string, slen, from, size, regs);
}

int Regexpr::search2( const char *string1, int slen1,
		      const char *string2, int slen2,
		      int from, int size,
		      re_registers *regs )
{
      if ( slen1+slen2==0 )
	return -1;
      if (from<0 || from >= slen1+slen2 )
	return -1;
      if (  size >= slen1+ slen2 - from )
	size=slen1+slen2-from-1;
      else if (size < 0 && size < -from )
	size = - from ;

      return re_search_2( buf, (char*)string1, slen1, (char*)string2, slen1,
		     from, size, regs, slen1+slen2 );

}
