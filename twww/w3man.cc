extern "C"{
#include "HTError.h"
#include "rw.h"
}

#include <reg_expr.h>
#include <dir.h>
#include <sys/wait.h>

#if defined(SOLARIS_SPARC) || defined(SOLARIS_X86)
  #include <fcntl.h>
#endif

#if defined(SINIX)
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
#endif

#include <unistd.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <program.h>
#include <window.h>
#include "w3man.h"

enum ManMode {
    MAN_MAN,
    MAN_APRO,
    MAN_INFO,
    MAN_INDEX
};

class ManStream;

struct _HTStream {
	CONST HTStreamClass *	isa;
	HTStream *		out;
	ManStream * man;
};

static int testNewArt( ManStream *man, char *&command, char *node );

static char *manMsg()
{
  return
"<HEAD>\
<ISINDEX>\
<TITLE>Hypertext Man Pages</TITLE>\
</HEAD>\
<BODY>\
<H1>Hypertext Man Pages</H1><br><br>\
This is the index page for the man pages.  Enter your search command\
in the <B>Search Keyword</B> dialog for your browser.  You can enter these\
searches at any point while viewing the man pages.\
<P><br><br>\
Several search forms are supported:<br><br>\
<DL>\
<DT> <I>name()</I>\
<DD> lookup name in man pages; e.g., <I>ls()</I>\
<DT> <I>name(section)</I>\
<DD> lookup name in specific section; e.g., <I>ls(1)</I>\
<DT> <I>keywords</I>\
<DD> locate commands by keyword lookup (apropos); e.g., <I>dos</I>\
</DL>\
</BODY>";
}

// ][  converters

#define LEN_STRING	500
#define LEN_WORD	200
#define LEN_HEAD	100

class ManStream
{
   friend int testNewArt( ManStream *man, char *&command, char *node );
private:
   HTStream *fout;
   char *inbuff;
   int pos_inbuff;
   long charCount;
   char *url;

   char *word;
   char *manName;
   ManMode mode;
   char *headStr;
   char *infofile;
   int dlCount;

   unsigned flSel:2,
	    flRef:1,
	    flMenu:2,
	    flEndMenuItem:1,
	    flDoubleCR:1,
	    flFirstStr:1,
	    flQuota:1;

   int posSecondStr;
   char *s_delim;

   re_registers regs, r_word, r_incl, r_head, r_sect;
   Regexpr ex, ex_incl, ex_head;

   void outman();
   void outapropos();
   void outinfo();
   char *full_incl_name( char *fname );
   void fill( int begPos );
   char getWord( char *s, int &pos, char *w );
   void m_fputc( char ch );
   void m_fputs( char *str );
   void fputc( char ch, HTStream *fout );
   void fputs( char *s, HTStream *fout );

public:
   ManStream( HTStream *Fout, HTRequest *request, ManMode Mode );
   ~ManStream();
   void put( char ch );
   void putHead();
   void putTail();
   void flush();
};

static void Man_free (HTStream * me)
{
    me->man->flush();
    me->man->putTail();
    (*me->out->isa->_free)(me->out);
    delete me->man;
    free(me);
}

static void Man_abort (HTStream * me, HTError e)
{
    (*me->out->isa->abort)(me->out, e);
    delete me->man;
    free(me);
}

static void Man_put_character (HTStream * me, char c)
{
    me->man->put(c);
}

static void Man_put_string (HTStream * me, CONST char* s)
{
    while( *s )
      me->man->put(*s++);
}

static void Man_write (HTStream * me, CONST char* s, int l)
{
    while( --l >= 0 )
      me->man->put(*s++);
}

static const HTStreamClass ManClass =
{
	"Man",
	Man_free,
	Man_abort,
	Man_put_character,
	Man_put_string,
	Man_write
};

extern "C" HTStream* ManToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{
  HTStream *ret= new HTStream;
  ret->out=output_stream;
  ret->isa = &ManClass;
  ret->man=new ManStream( output_stream, request, MAN_MAN );
  ret->man->putHead();
  return ret;
}

extern "C" HTStream* InfoToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{
  HTStream *ret= new HTStream;
  ret->out=output_stream;
  ret->isa = &ManClass;
  ret->man=new ManStream( output_stream, request, MAN_INFO );
  ret->man->putHead();
  return ret;
}

extern "C" HTStream* AproposToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{
  HTStream *ret= new HTStream;
  ret->out=output_stream;
  ret->isa = &ManClass;
  ret->man=new ManStream( output_stream, request, MAN_APRO );
  ret->man->putHead();
  return ret;
}

extern "C" HTStream* ManIndexToHTML (
	HTRequest *		request,
	void *			param,
	HTFormat		input_format,
	HTFormat		output_format,
	HTStream *		output_stream)
{
  HTStream *ret= new HTStream;
  ret->out=output_stream;
  ret->isa = &ManClass;
  ret->man=new ManStream( output_stream, request, MAN_INDEX );
  return ret;
}

extern "C" int HTLoadMan( HTRequest *request )
{
    if ( !request || !request->anchor )
	return HT_INTERNAL;

    int status=-1;
    char *command=0, *msg=0, *node=0;
    char **c1=&msg, **c2=&node;	// защита от разрушения после vfork при -O2
    ManStream *man=0;

    char *cmd= HTAnchor_physical(request->anchor);
    if (!strncmp(cmd, "man:",4 ))
     {
       char *beg=(char*)strchr(cmd, '?');
       if (!beg)
	 beg=(char*)strchr(cmd, ':');
       if (!beg || !*(++beg) )
	 msg=manMsg();
       else
	{
	  if ( !strchr(beg, '(') )
	   {
	    command=new char[10 +strlen(beg)];
	    sprintf(command, "apropos %s", beg);
	    node = strdup(beg);
	   }
	  else
	   {
	    char *str = strdup( beg );
	    char **ap, *argv[10], *ch=str;
	    int n=0;
	    for( ap = argv; (*ap = strtok( ch, " \t()\r\n")) != NULL; )
	     {
	       ch = 0;
	       if (**ap != '\0')
		 { ++ap; n++; }
	     }
	    int le=0;
	    for(int i=0; i<n; i++)
	      le+=strlen(argv[i])+2;
	    if (n>1)
	     {
	      command=new char[le+8];
	      if (! strcmp(argv[1], "*") )
		sprintf(command, "man -a %s", argv[0]);
	      else
		sprintf(command, "man %s %s", argv[1], argv[0]);
	     }
	    else if (n>0)
	     {
	      command=new char[le+8];
	      sprintf(command, "whatis %s", argv[0]);
	      node = strdup(argv[0]);
	     }
	    ::free(str);
	   }
	}
     }
    else if (!strncmp(cmd, "info:",4 ))
     {
       char *beg=(char*)strchr(cmd, '?');
       if (!beg)
	 beg=(char*)strchr(cmd, ':');
       if (!beg || !*(++beg) )
	 command=strdup("info -o -");
       else
	{
	  char *beg1;
	  if (!(beg1=(char*)strchr(beg, ':' )))
	   {
	    command=new char[22+strlen(beg)];
	    sprintf(command, "info -o - -n \"%s\"", beg);
	   }
	  else
	   {
	    int fle=beg1-beg;
	    beg1++;
	    int nle=strlen(beg1);

	    char *s=command=new char[22+fle+nle];
	    memcpy(s, "info -o - -f \"", 14);
	    s+=14;
	    memcpy(s, beg, fle);
	    s+=fle;
	    memcpy(s, "\" -n \"", 6);
	    s+=6;
	    memcpy(s, beg1, nle);
	    s+=nle;
	    memcpy(s, "\"", 1);
	    s+=1;
	    *s=0;
	   }
	}
     }

    int pid=-1, sockfd=-1, b_read=0;

reloadMan:
    if ( !msg && command )
     {
       int pi, po[2];
       pipe(po);
	// set O_NONBLOCK to 1
       int arg=fcntl(po[0], F_GETFL, 0);
       fcntl(po[0], F_SETFL,  arg | O_NONBLOCK);
       pi=open("/dev/null", O_RDONLY );
       arg=fcntl(pi, F_GETFL, arg);
       fcntl(pi, F_SETFL,  arg | O_NONBLOCK);

       if ( !(pid=vfork()) )
	 {
	    dup2(pi,0);
	    dup2(po[1],1);
	    int null=open("/dev/null", O_WRONLY);
	    dup2(null, 2);

	    close(pi);
	    close(po[1]);

	    for(int i=3; i<20; i++)
		close(i);

	    setgid(getgid());
	    setuid(getuid());

	    if ( strpbrk(command, "*|&><;\"'`$()[]{}\\") )
	      execl( "/bin/sh", "/bin/sh", "-c", command, 0 );
	    else
	     {
	      // split program to words
	      char *string=strdup(command);
	      char **vect=new char *[64];
	      int i=0;
	      for (char *s=strtok( string, " \t"); s && i<63 ; s=strtok(0, " \t"), i++ )
		vect[i]=s;
	      vect[i]=0;
	      execvp(vect[0], vect);
	      printf("command '%s' not found\n", command);
	      fflush(stdout);
	      execl( "/bin/sh", "/bin/sh", "-c", "exit", 0 );
	     }
	    exit(111);
	 }
       close(po[1]);
       close(pi);
       sockfd = po[0];
     }

    HTStream *target=0;
    if ( msg )
      {
	  target = ManIndexToHTML( request, 0, HTAtom_for("text/html"),
		       WWW_HTML, HTStreamStack( WWW_HTML, request, YES) );
	  (*target->isa->put_string)(target,msg);
	  status = HT_LOADED;
	  goto end_process;
      }
//    else if (HTAnchor_cacheHit(anchor))
//        HTAnchor_setFormat(anchor, WWW_MIME);
    else if ( !strncmp(command, "apropos", 7 ) || !strncmp(command, "whatis", 6 ) )
	  target = AproposToHTML( request, 0, HTAtom_for("text/apropos"),
		       WWW_HTML, HTStreamStack( WWW_HTML, request, YES) );
    else if ( !strncmp(command, "man", 3 )  )
	  target = ManToHTML( request, 0, HTAtom_for("text/man"),
		       WWW_HTML, HTStreamStack( WWW_HTML, request, YES) );
    else if ( !strncmp(command, "info", 4 )  )
	  target = InfoToHTML( request, 0, HTAtom_for("text/info"),
		       WWW_HTML, HTStreamStack( WWW_HTML, request, YES) );

    request->isoc = HTInputSocket_new( sockfd );
    while( 1 )
     {
       int b_read = NETREAD( sockfd, request->isoc->input_buffer, INPUT_BUFFER_SIZE );
       if ( b_read <= 0 )
	  break;
       (*target->isa->put_block)(target, request->isoc->input_buffer, b_read);
     }
    HTInputSocket_free( request->isoc );

    if ( testNewArt( target->man, command, node ) )
      {
	if ( sockfd >=0 )
	  { close( sockfd ); sockfd=-1; }
	if ( target )
	  {
	    target->isa->_free( target );
	    target = 0;
	  }
	if ( pid != -1 )
	  {
	    waitpid( pid, &status, WNOHANG );
	    pid = -1;
	  }
	goto reloadMan;
      }

    if ( sockfd >=0 )
       close( sockfd );

    if ( pid != -1 )
       waitpid( pid, &status, WNOHANG );

    status = HT_LOADED;

end_process:
    FREE( command );
    FREE( node );
    if ( target && target->man )
       target->isa->_free( target );
    return status;
}

GLOBALDEF HTProtocol HTMan = { "man", HTLoadMan, NULL, NULL };
GLOBALDEF HTProtocol HTInfo = { "info", HTLoadMan, NULL, NULL };

// ][
#define ex_word ex_incl
#define ex_menu ex_incl
#define ex_sect ex_incl
#define ex_first ex_head

ManStream::ManStream( HTStream *Fout, HTRequest *request, ManMode Mode ) :
	fout(Fout),
	pos_inbuff(0),
	mode(Mode),
	flSel(0),
	flRef(0),
	flMenu(0),
	flEndMenuItem(0),
	flDoubleCR(0),
	flFirstStr(1),
	flQuota(0),
	posSecondStr(0),
	s_delim(" \t\n{}()[]:;,?*=+!@$%&\\\"'")
{
   dlCount=0;
   charCount=0;
   inbuff=new char[LEN_STRING];
   word=new char[LEN_WORD];
   headStr=new char[LEN_HEAD];
   infofile=(char*)malloc(1);
   infofile[0]=0;
   headStr[0]=0;

   HTParentAnchor *anchor = request ? request->anchor : 0;
   url=strdup(HTAnchor_physical(anchor));
   char *cmd=(char*)strchr(url, '?');
   if (!cmd || !*++cmd)
     manName=strdup("");
   else
     manName=strdup(cmd);

   switch( mode )
    {
     case MAN_MAN:
      //ex.compile( "[^(]+\\((1|2|3|3C|3X11|4|5|6|7|8|l|L|n|o)\\)", 0 ); // выражение в скобках является секцией
      ex.compile( "[^(]+\\((1|2|3|4|5|6|7|8|l|L|n|o)([a-zA-Z1-9]*)\\)", 0 ); // выражение в скобках является секцией
      ex_incl.compile( "<(.+\\.h)>", 0 );				   // имя файла
      //ex_head.compile( ".*\\((1|2|3|3C|3X11|4|5|6|7|8|l|L|n|o)\\).*", 0 );   // заголовок  типа 'DATE(1)   BSD Reference Manual   DATE(1)'
      ex_head.compile( ".*\\((1|2|3|3C|3X11|4|5|6|7|8|l|L|n|o)([a-zA-Z]*)\\).*", 0 );   // заголовок  типа 'DATE(1)   BSD Reference Manual   DATE(1)'
      break;
     case MAN_APRO:
      //ex.compile( "([^(]*)(\\([^)]+\\))[ \t-]*(.*)", 0 );
      ex.compile( "([^(]*)(\\(([1-9]+)[^)]*\\))[ \t-]*(.*)", 0 );
      ex_word.compile( "[ \t]*([^, \t]+)", 0 );	// выделение первого слова
      break;
     case MAN_INFO:
      //ex.compile( "(^\\* Menu:)|(`([^']+)')|(\"([^\"]*)\")|\\*[Nn]ote[ \t\n]*([^:]+):[ \t\n]*(\\(.+\\)[^.,:]*)\\:*|\\*[Nn]ote[ \t\n]*([^:]+):([^.,:]+)\\:*|\\*[Nn]ote[ \t\n]*([^:]+)::|\"([^\"]+)", 0 );
      ex.compile( "(^\\* Menu:)|`(('.'|[^'])+)'|(\"([^\"]*)\")|\\*[Nn]ote[ \t\n]*([^:]+):[ \t\n]*(\\(.+\\)[^.,:]*)\\:*|\\*[Nn]ote[ \t\n]*([^:]+):([^.,:]+)\\:*|\\*[Nn]ote[ \t\n]*([^:]+)::|\"([^\"]+)", 0 );
		      //      (1)     |    (2)                3                            |       4            5                        6         7(8)      |  9(10)
      ex_menu.compile( "(\\*[Nn]ote.*)|\\*([^:]+):[ \t]*(\\([^.,/\\(\\)]*[.,/]+.*\\))[^ \t]|\\*([^:]+):[ \t]*([^.,:\n]+)[^ \t]|\\*[ \t]*([^:]+)::|`(([^']+))'|(\"([^\"]*)\")", 0 );
      break;
     default:
      break;
    }
}

ManStream::~ManStream()
{
   delete inbuff;
   delete word;
   delete manName;
   delete headStr;
   delete url;
   delete infofile;
}

void ManStream::put( char ch )
{
 charCount++;
 if (pos_inbuff>=LEN_STRING-2)
   flush();

 switch(mode)
   {
   case MAN_MAN:
   case MAN_INDEX:
   case MAN_APRO:
     inbuff[pos_inbuff++]=ch;
     if( ch=='\n' )
	flush();
     break;

   case MAN_INFO:
     inbuff[pos_inbuff++]=ch;
     if( ch=='\n')
      {
	if( posSecondStr )
	   flush();
	else
	   posSecondStr=pos_inbuff+1;
      }
     break;
   }
}

void ManStream::flush()
{
   inbuff[pos_inbuff]=0;
   switch(mode)
    {
     case MAN_MAN:
      outman();
      pos_inbuff=0;
      break;
     case MAN_APRO:
      outapropos();
      pos_inbuff=0;
      break;
     case MAN_INFO:
      outinfo();
      break;
     case MAN_INDEX:
      fputs( inbuff, fout );
      pos_inbuff=0;
    }
}

void ManStream::outman()
{
   int i=0, sLen=strlen(inbuff), le;
   if( !headStr[0] ) // пропуск заголовка типа 'DATE(1)   BSD Reference Manual   DATE(1)'
    {
      int ret2 = ex_head.match( inbuff, sLen, 0, &r_head );
      if( ret2 >=0 )
       {
	 fputs( inbuff, fout );
	 strcpy( headStr, inbuff );
	 return;
       }
    }
   else if( !strcmp( inbuff, headStr) )
    {
      fputs( inbuff, fout );
      return;
    }

   while( inbuff[i] )
    {
      flSel=flRef=flQuota=0;
      if( getWord( inbuff, i, word ) )
       {
	 if( flRef )   // слово типа 'select(1)'
	  {
	    fputs( "<A HREF=\"man:?", fout );
	    // Секции типа `3x` заменяются на `3`
	    // в regs лежит информация о word (заполняется в getWord() )
	    for( int j=0; j<regs.end[1]; j++ )
	       if( !strchr( " \t", word[j] ) )
		  fputc( word[j], fout );
	    fputc( ')', fout );
	    /*for( int j=0; word[j]; j++ )
	       if( !strchr( " \t", word[j] ) )
		  fputc( word[j], fout );
	    */
	    fputs( "\">", fout );
	    m_fputs( word );
	    fputs( "</A>", fout );
	  }
	 else if( flQuota )   // слово типа ``asdf''
	  {
	    fputs( "<I>", fout );
	    m_fputs( word );
	    for( ; inbuff[i] && inbuff[i]=='\''; i++ )
	       fputc( '\'', fout );
	    fputs( "</I>", fout );
	  }
	 else if( !strcmp( "#include", word ) )
	  {
	    fputs( "<B>#include</B>", fout );
	    flSel=0;
	    getWord( inbuff, i, word );
	    if( ex_incl.match( word , strlen(word), 0, &r_incl )>=0 )
	     {
	       char word1[40];
	       memcpy( word1, word+r_incl.start[1], le=(r_incl.end[1]-r_incl.start[1]) );
	       word1[ le ]=0;

	       fputs( "<B>&#60;<A HREF=\"file:", fout );
	       fputs( full_incl_name(word1), fout );

	       fputs( "\">", fout );
	       fputs( word1, fout );
	       fputs( "</A>&#62;</B>", fout );
	     }
	  }
	 else
	  {
	    if( flSel==1 )
	     {
	       fputs( "<B>", fout );
	       m_fputs( word );
	       fputs( "</B>", fout );
	     }
	    else if( flSel==2 )
	     {
	       fputs( "<I>", fout );
	       m_fputs( word );
	       fputs( "</I>", fout );
	     }
	    else
	       m_fputs( word );
	  }
       }
    }
}

char *ManStream::full_incl_name( char *fname )
{
   const char *delim=" \t:";
   static char *path = ":/usr/local/include:/usr/include";
   char *incl_path = getenv( "INCLUDE_PATH" );
   char *main_path = new char[ strlen(path)+( incl_path ? strlen(incl_path):0 )+1 ];
   char *m_path=main_path;

   *main_path=0;
   if( incl_path )
      strcat( main_path, incl_path );
   strcat( main_path, path );
   if( strchr( delim, *m_path )!=NULL )
      m_path++;

   int len_fname=strlen(fname);
   static char buf[255];
   strncpy( buf, (const char *)m_path, 255 );
   int pos;
   for( char *ptr=0; *m_path && ptr<m_path ; m_path=ptr+1 )
    {
      if( (ptr=(char*)strpbrk( (const char*)m_path, delim ))==NULL )
	 ptr=m_path +strlen(m_path);
      pos=ptr-m_path;
      strncpy( buf, (const char *)m_path, pos );
      buf[ pos ]=FILESEP;
      buf[ pos+1 ]=0;
      strcat( buf, (const char *)fname );
      buf[ pos+len_fname+1 ]=0;
      if( !access( buf, F_OK) )
	 break;
    }
   delete main_path;
   return buf;
}

void ManStream::outapropos()
{
   int i=0;
   int ret = ex.match( inbuff, strlen(inbuff), 0, &regs );
   if( ret >= 0 )
      ret=ex_word.match( inbuff+regs.start[1], regs.end[1]-regs.start[1], 0, &r_word );

   if( ret>=0 )
    {
      dlCount++;
      fputs( "\n<DT><A HREF=\"man:?", fout );

      for( i=r_word.start[0]; i<r_word.end[0]; i++ )
	 fputc( inbuff[i], fout );

      //for( i=regs.start[2]; i<regs.end[2]; i++ )
      // Секции типа `3x` заменяются на `3`
      for( i=regs.start[2]; i<regs.end[3]; i++ )
	 fputc( inbuff[i], fout );
      //fputs( "\">", fout );
      fputs( ")\">", fout );

      for( i=regs.start[1]; i<regs.end[2]; i++ )
	 fputc( inbuff[i], fout );
      fputs( "</A>\n<DD>", fout );

      //for( i=regs.start[3]; i<regs.end[3]; i++ )
      for( i=regs.start[4]; i<regs.end[4]; i++ )
	 fputc( inbuff[i], fout );
    }
   else
      m_fputs( inbuff );
}


static int testNewArt( ManStream *man, char *&command, char *node )
{
  if ( man->mode==MAN_APRO && man->dlCount==1 && command && node )
   {
      command = (char*)realloc( command, strlen(node)+10 );
      sprintf( command, "man %s", node );
      return 1;
   }
  return 0;
}

void ManStream::fill( int begPos )
{
   if( begPos )
    {
      int i;
      for( i=0; inbuff[i+begPos]; i++ )
	 inbuff[i]=inbuff[i+begPos];
      inbuff[i]=0;
      posSecondStr=i;
    }
   pos_inbuff=posSecondStr;
}

void ManStream::outinfo()
{
  int le;
  if( flFirstStr )
   {
     int pos=0;
     ex_first.compile( "File: ([^ \t,:]+)", 0);
     int ret=ex_first.search( inbuff, pos_inbuff, 0, pos_inbuff, &regs );
     if( ret >=0 )
      {
	int le=regs.end[1]-regs.start[1];
	infofile=(char*)realloc(infofile, le+1);
	memcpy( infofile, inbuff+regs.start[1], le );
	infofile[ le ]=0;
	char *e=(char*)strrchr(infofile, '.');
	if (e && !strncmp(e, ".info", 5) )
	  *e=0;
	memcpy( word, inbuff+regs.start[0], le=(regs.end[0]-regs.start[0]) );
	word[ le ]=0;
	m_fputs( word );
	pos=regs.end[0];
      }

     char *mas[3]={ "Next: ([^\t,:]+)" ,
		    "Prev[a-z]*: ([^\t\n,:]+)" ,
		    "Up: ([^\t\n,:]+)" };

     int i, f;
     for( i=0, f=1; i<3; i++ )
      {
	ex_first.compile( mas[i], 0);
	ret=ex_first.search( inbuff, pos_inbuff, pos, pos_inbuff-pos, &regs );
	if( ret >= 0 )
	 {
	   memcpy( word, inbuff+pos, le=(regs.start[0]-pos) );
	   word[ le ]=0;
	   m_fputs( word );
	   if (f) { f=0;fputc('\n', fout); }
	   fputc('\n', fout);

	   memcpy( word, inbuff+regs.start[0], le=(regs.start[1]-regs.start[0]) );
	   word[ le ]=0;
	   m_fputs( word );
	   if (i==2)
	     fputs("  ", fout);

	   fputs( "<A HREF=\"info:?", fout );
	   m_fputs( infofile );
	   fputc( ':', fout );
	   memcpy( word, inbuff+regs.start[1], le=(regs.end[1]-regs.start[1]) );
	   word[ le ]=0;
	   m_fputs( word );
	   fputs( "\">", fout );
	   m_fputs( word );
	   fputs( "</A>", fout );
	   pos=regs.end[0];
	 }
      }

     memcpy( word, inbuff+pos, le=(posSecondStr-pos) );
     word[ le ]=0;
     m_fputs( word );
     if (f)
       fputc('\n',fout);

     flFirstStr=0;
     fill( posSecondStr );
     return;
   }

   int n_menu=1;
   int n_sel=2;
   int n_sel2=4;
   int n_note=8;
   int n_note2=10;
   int n_note3=6;
   int n_quota=11;
   int i, rs, fl_note;
   for( i=0; inbuff[i] && i<posSecondStr; )
    {
      n_sel=2;
      n_sel2=4;
      if( inbuff[i]=='\n' )
       {
	 if(flMenu!=1)
	   fputc( '\n', fout );
	 i++;
	 for( ; inbuff[i] && strchr( " \t", inbuff[i]); i++ )
	    fputc( inbuff[i], fout );
	 if( inbuff[i]=='\n' )
	    flDoubleCR=1;
	 fill( i );
	 return;
       }

      if( flMenu )
	 rs=ex_menu.search( inbuff, pos_inbuff, i, pos_inbuff-i, &regs );
      else
	 rs=ex.search( inbuff, pos_inbuff, i, pos_inbuff-i, &regs );

      if( rs<0 || regs.start[0] >=posSecondStr || (flMenu && regs.end[6]>=posSecondStr) )
       {
	 int fl=0;
	 if( flMenu && flDoubleCR && !i && !strchr( " \t", inbuff[i] ) )
	  { fputs( "<H2>", fout ); fl=1; } // SubMenu have self header
	 for( ; i<posSecondStr; i++ )
	   if (flMenu!=1 /*|| inbuff[i]!='\n'*/)
	     m_fputc( inbuff[i] );
	 if( inbuff[posSecondStr]=='\n' )
	    flDoubleCR=1;
	 if( fl )
	    fputs( "</H2>", fout );
	 break;
       }

      memcpy( word, inbuff+i, le=(regs.start[0]-i) );
      word[ le ]=0;
      m_fputs( word );
      fl_note=0;
      if( flMenu )
       {
	 if( flMenu==1 )
	  {
	    flMenu=2;
	    fputs( "<DL>\n", fout );
	  }
	 if( regs.start[1]>=0 )  // Appear '*Note'-> menu ended
	  {
	     if( flDoubleCR )
	      {
		flMenu=0;
		fputs( "</DL>\n<PRE>\n", fout );
	      }
	     fl_note=1;
	     int prev_end=regs.end[1];
	     i=regs.start[1];
	     rs=ex.search( inbuff, pos_inbuff, i, pos_inbuff-i, &regs );
	     if( rs<0 || regs.start[0] >= posSecondStr )
	      {
		memcpy( word, inbuff+i, le=(prev_end-i) );
		word[ le ]=0;
		m_fputs( word );
		i=prev_end;
		continue;
	      }
	  }
	 else if( regs.start[7]<0 && regs.start[9]<0 )
	  {
	    flDoubleCR=0;
	    fputs( "<DT><A HREF=\"info:?", fout );
	    m_fputs( infofile );
	    fputc( ':', fout );
	  }

	if( !fl_note )
	 {
	   if( regs.start[3]>=0 )
	    {
	      memcpy( word, inbuff+regs.start[3], le=(regs.end[3]-regs.start[3]) );
	      word[ le ]=0;
	      m_fputs( word );
	      fputs( "\">", fout );
	      memcpy( word, inbuff+regs.start[2], le=(regs.end[2]-regs.start[2]) );
	      word[ le ]=0;
	      i=regs.end[0];
	    }
	   else if( regs.start[6]>=0 )
	    {
	      memcpy( word, inbuff+regs.start[6], le=(regs.end[6]-regs.start[6]) );
	      word[ le ]=0;
	      m_fputs( word );
	      fputs( "\">", fout );
	      i=regs.end[0];
	    }
	   else if( regs.start[5]>=0 )
	    {
	      memcpy( word, inbuff+regs.start[5], le=(regs.end[5]-regs.start[5]) );
	      word[ le ]=0;
	      m_fputs( word );
	      fputs( "\">", fout );
	      memcpy( word, inbuff+regs.start[4], le=(regs.end[4]-regs.start[4]) );
	      word[ le ]=0;
	      i=regs.end[0];
	    }

	   if( regs.start[7]>=0 || regs.start[9]>=0 )
	    {
	      n_sel=7;
	      n_sel2=9;
	    }
	   else
	    {
	      m_fputs( word );
	      fputs( "</A>", fout );
	      fputs( "\n<DD>", fout );
	      if( regs.start[4]>=0 && inbuff[regs.end[5]]=='\n' ) // ошибка в меню
		 i=regs.start[5];
	      continue;
	    }
	  }
       }

      if( !flMenu && regs.start[n_menu]>=0 )
       {
	 flMenu=1;
	 i=regs.end[0];
       }
      else if( regs.start[n_sel]>=0 || regs.start[n_sel2]>=0 )
       {
	 int num=n_sel;
	 char ch='I';
	 if( regs.start[n_sel2]>=0 )
	  {
	    num=n_sel2;
	    ch='B';
	  }
	 fputc( '<', fout );
	 fputc( ch, fout );
	 fputc( '>', fout );
	 memcpy( word, inbuff+regs.start[num], le=(regs.end[num]-regs.start[num]) );
	 word[ le ]=0;
	 m_fputs( word );
	 fputs( "</", fout );
	 fputc( ch, fout );
	 fputc( '>', fout );
	 i=regs.end[0];
       }
      else if( regs.start[n_note]>=0 || regs.start[n_note2]>=0 || regs.start[n_note3]>=0 )
       {
	 int k, num=n_note3, num_ref=n_note3+1;	// *Note.*:.*[,.:]
	 if( regs.start[n_note2]>=0 )		// *Note.*::
	    num=num_ref=n_note2;
	 else if( regs.start[n_note]>=0 )	// *Note.*: (gdb.info)C.)
	    { num=n_note; num_ref=n_note+1; }

	 memcpy( word, inbuff+regs.start[0], le=(regs.start[num]-regs.start[0]) );
	 word[ le ]=0;
	 if( strchr( word, '\n' ) )
	    fputc( '\n', fout );

	 memcpy( word, inbuff+regs.start[num_ref], le=(regs.end[num_ref]-regs.start[num_ref]) );
	 word[ le ]=0;
	 fputs( "<A HREF=\"info:?", fout );
	 m_fputs( infofile );
	 fputc( ':', fout );
	 for( k=0; word[k]; k++ )
	    if( word[k]=='\n' )
	       word[k]=' ';

	 word[k]=0;
	 m_fputs( word );
	 fputs( "\">", fout );

	 memcpy( word, inbuff+regs.start[num], le=(regs.end[num]-regs.start[num]) );
	 word[ le ]=0;

	 char *w=strdup(word);
	 char *e=0;
	 for( k=1; word[k]; k++ )
	    if( word[k]=='\n' && !e )
		 e=w+k;

	 if (e)
	   *e++=0;
	 m_fputs( w );

	 fputs( "</A>", fout );
	 if (e)
	   { fputc('\n', fout); fputs( e, fout); }
	 i=regs.end[0];
	 ::free(w);
       }
      else if( regs.start[n_quota]>=0 && !flQuota )
       {
	 char ch='B';
	 flQuota=1;

	 fputc( '<', fout );
	 fputc( ch, fout );
	 fputc( '>', fout );
	 memcpy( word, inbuff+regs.start[n_quota], le=(regs.end[n_quota]-regs.start[n_quota]) );
	 word[ le ]=0;
	 m_fputs( word );
	 i=regs.end[0];
       }
      else if( regs.start[n_quota]>=0 && flQuota )
       {
	 flQuota=0;
	 char ch='B';

	 memcpy( word, inbuff+regs.start[n_quota], le=(regs.end[n_quota]-regs.start[n_quota]) );
	 word[ le ]=0;
	 m_fputs( word );
	 fputs( "</", fout );
	 fputc( ch, fout );
	 fputc( '>', fout );
	 i=regs.end[0];
       }
    }
   fill( i );
   return;
}

char ManStream::getWord( char *sourse, int &pos, char *word )
{
   word[0]=0;
   flSel=0;
   int p_flSel=0, p_pos=-1, p_j=0;
   while( strchr(s_delim, sourse[pos]) && sourse[pos] ) // пропуск символов-разделителей
    {
      if( sourse[pos+1]=='\b' && sourse[pos+2]==sourse[pos] )
       {
	 if( sourse[pos]=='_' )
	  {
	    if( !flSel )
	       fputs( "<I>", fout );
	    else if( flSel==1 )
	       fputs( "</I><B>", fout );
	    flSel=2;
	  }
	 else
	  {
	    if( !flSel )
	       fputs( "<B>", fout );
	    else if( flSel==2 )
	       fputs( "</B><I>", fout );
	    flSel=1;
	  }
	 m_fputc( sourse[pos] );
	 pos+=3;
       }
      else
       {
	 if( flSel==1 )
	    fputs( "</B>", fout );
	 else if( flSel )
	    fputs( "</I>", fout );
	 flSel=0;
	 fputc( sourse[pos++], fout );
       }
    }
   if( flSel==1 )
      fputs( "</B>", fout );
   else if( flSel )
      fputs( "</I>", fout );
   flSel=0;
   int fl_break, j;
   for( fl_break=0, j=0; sourse[pos] && !fl_break; pos++, j++ )
    {
      word[j]=0;
      if( !flRef && !flQuota && (( strchr( s_delim, sourse[pos] ))
				 || (flQuota=(sourse[pos]=='`')) ) )
       {
	 if( (flRef=( sourse[pos]=='(' )) || flQuota )
	  {
	    p_pos=pos;
	    p_flSel=flSel;
	    p_j=j;
	    if( flQuota )
	     {
	       j--;
	       continue;
	     }
	  }
	 else
	    break;
       }
      else if( sourse[pos]==')' && !flQuota )
	 fl_break=1;
      else if( sourse[pos]=='\'' )
       {
	 pos++;
	 fl_break=1;
	 break;
       }

      if( sourse[pos]=='\b' )
       {
	 if( !flSel )
	    flSel=1;
	 if( j && word[--j]=='_' && sourse[pos+1]!='_' )
	    flSel=2;
	 if( sourse[pos+1]=='(' && j )
	    flRef=1;
	 else if( sourse[pos+1]==')' && flRef )
	  {
	    word[j++]=sourse[++pos];
	    pos++;
	    break;
	  }
	 word[j]=sourse[++pos];
       }
      else if( !flRef && !flSel && (j && sourse[pos+1]=='\b' ) )
       {
	 word[j]=sourse[pos];
	 break;
       }
      else if( !flSel )
	 word[j]=sourse[pos];
      else if( flSel && (j==0 || sourse[pos+1]=='\b' ) )
       {
	 if( !flRef && ((flSel==2 && sourse[pos]!='_') || (flSel==1 && sourse[pos]=='_' && sourse[pos+2]!='_' )) )
	    break;
	 if( flRef && sourse[pos+1]=='\b' ) // && sourse[pos+2]==')' )
	    pos+=2;
	 word[j]=sourse[pos];
       }
      else if( flRef || p_pos>=0 )
	 word[j]=sourse[pos];
      else if( flQuota || p_pos>=0 )
	 word[j]=sourse[pos];
      else
	 break;
    }
   word[j]=0;
   if( flQuota && sourse[pos-1]!='\'' )
    {
      fputc( '`', fout );  // '\''
      flQuota=0;
    }
   if( flRef && ex.match( word, strlen(word), 0, &regs ) < 0 )
    {
      flRef=0;
      if( p_pos>=0 )
       {
	 flSel=p_flSel;
	 pos=p_pos;
	 word[p_j]=0;
       }
    }
   return word[0] | flQuota | flSel | flRef;
}

void ManStream::putHead()
{
   switch(mode)
    {
     case MAN_MAN:
      fputs("\n<HEAD>\n<ISINDEX>\n<TITLE>Man ", fout );
      fputs( manName, fout );
      fputs("</TITLE>\n</HEAD>\n", fout );
      fputs("<BODY>\n<PRE>\n", fout );
      break;
     case MAN_APRO:
      fputs( "\n<HEAD>\n<ISINDEX>\n<TITLE>Apropos ", fout );
      fputs( manName, fout );
      fputs( "</TITLE>\n</HEAD>\n<BODY>\nApropos: ", fout );
      fputs( manName, fout );
      fputs( "\n<DL>", fout );
      break;
     case MAN_INFO:
      fputs("\n<HEAD>\n<ISINDEX>\n<TITLE>Info ", fout );
      fputs( manName, fout );
      fputs("</TITLE>\n</HEAD>\n", fout );
      fputs("<BODY>\n<PRE>\n", fout );
      break;
     default:
      break;
    }
}

void ManStream::putTail()
{
   if (!charCount)
    {
      fputs("  Resource <B>", fout);
      fputs(url, fout);
      fputs("</B> not found.", fout);
    }
   switch(mode)
    {
     case MAN_MAN:
      fputs( "\n</PRE>\n</BODY>\n", fout );
      break;
     case MAN_APRO:
      fputs( "\n</DL>\n</BODY>\n", fout );
      break;
     case MAN_INFO:
      if( flMenu )
	fputs( "</DL>\n", fout );
      fputs( "\n</PRE>\n</BODY>\n", fout );
      break;
     default:
      break;
    }
}

void ManStream::m_fputs( char *str )
{
   for( int i=0; str[i]; i++ )
      m_fputc( str[i] );
}

void ManStream::m_fputc( char ch )
{
   switch( ch )
    {
      case '<':
	 fputs( "&#60;", fout );
	 break;
      case '>':
	 fputs( "&#62;", fout );
	 break;
      case '&' :
	 fputs( "&#38;", fout );
	 break;
      case 27:
	 fputs( "<B>[</B>", fout );  //]
	 break;
      default:
	 fputc( ch, fout );
    }
}

void ManStream::fputc( char ch, HTStream *fout)
{
  fout->isa->put_character(fout, ch);
}

void ManStream::fputs( char *s, HTStream *fout)
{
  fout->isa->put_string(fout, s);
}


// ]
