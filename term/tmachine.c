#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include "tmachine.h"

#define DBG

#ifdef DBG
#include <stdio.h>
void dump_program(struct TMachine *tp, int pc);

#endif

#define NEW(type,num)	((type*)calloc((num),sizeof(type)))
#define ARR(type,num,ptr) ((ptr)=(type*)realloc((ptr),sizeof(type)*(num)))

#define GET_SHORT(tp,pos)	(*(short *)((tp)->program+pos))
#define INCR_SHORT(pos)	 ((pos)+=sizeof(short))
#define INCR_BYTE(pos)	 ((pos)+=sizeof(unsigned char))
#define GET_BYTE(tp,pos)		(*(unsigned char*)((tp)->program+pos))

#define CODE_END 0
#define CODE_SWITCH 1
#define CODE_CALL 2
#define CODE_D_CONV 3
#define CODE_C_CONV 4
#define CODE_INCR 5
#define CODE_P1 6
#define CODE_P2 7
#define CODE_P3 8
#define CODE_P4 9
#define CODE_P5 10
#define CODE_P6 11
#define CODE_P7 12
#define CODE_P8 13
#define CODE_P9 14
#define CODE_P0 15
#define CODE_O_CONV 16
#define CODE_CONST 17

#define CODE_ADD 18
#define CODE_SUB 19
#define CODE_DIV 20
#define CODE_MUL 21
#define CODE_MOD 22
#define CODE_OR  23
#define CODE_AND 24
#define CODE_INT 25

typedef struct Jtab
{
	int def;
	int tabnum;
	int *ctab;
	int *jtab;

	int dignum;
	int *dctab;
	int *djtab;
	int *ddtab;
}
Jtab;

struct CharRef;

struct TMachine
{
	Terminfo *info;
	void *par;

	char *program;
	int plen;

	int pc;			/* program counter == state */
	int params[9];		/* */
	int pno;		/* current parameter pointer */

	int pflag;
	struct CharRef *lastref;
};

typedef struct
{
	char *str;
	int no;
	char convaddr;
	int percent:1;
}
Bucket;

typedef struct CharRef
{
	unsigned char ch;
	struct CharRef *ref;
	Jtab *tp;
}
CharRef;

static void init_Jtab(Jtab * tp);
static void destroy_Jtab(Jtab * tp);
static void add_Jtab(Jtab * tp, int ch, int jmp);
static void dig_Jtab(Jtab * tp, int ch, int jmp, int dig);

static int make_fa(struct TMachine *tp, Bucket * bucks, int beg, int end, int pos,
		   CharRef * back, Jtab * jlast, int dig, int lastref);
static int code_short(struct TMachine *tp, short s);
static int code_byte(struct TMachine *tp, unsigned char b);
static void store_short(struct TMachine *tp, int pos, short s);
static void store_byte(struct TMachine *tp, int pos, unsigned char b);
static int code_pos(struct TMachine *tp);

int
param_TMachine(struct TMachine *mp, int pno)
{
	if (pno > 0 && pno < 10)
		return mp->params[pno - 1];
	return 0;
}

Terminfo *
tinfo_TMachine(struct TMachine * mp)
{
	return mp->info;
}

static int
cmp_bucket(const void *p1, const void *p2)
{
	return strcmp(((const Bucket *) p1)->str, ((const Bucket *) p2)->str);
}

static void
init_Jtab(Jtab * tp)
{
	memset(tp, 0, sizeof(Jtab));
}

static void
destroy_Jtab(Jtab * tp)
{
	free(tp->jtab);
	free(tp->ctab);
	free(tp->djtab);
	free(tp->dctab);
	free(tp->ddtab);
}

static void
add_Jtab(Jtab * tp, int ch, int jmp)
{
	int n, np;

	np = tp->tabnum;

#if 0
	for (i = 0; i < np; ++i)
		if (tp->ctab[i] == ch)
			return;
#endif

	n = ++(tp->tabnum);
	ARR(int, n, tp->jtab);
	ARR(int, n, tp->ctab);

	(tp->jtab)[np] = jmp;
	(tp->ctab)[np] = ch;
}

static void
dig_Jtab(Jtab * tp, int ch, int jmp, int dig)
{
	int n, np;

	np = tp->dignum;

#if 0
	for (i = 0; i < np; ++i)
		if (tp->dctab[i] == ch)
			return;
#endif

	n = ++(tp->dignum);
	ARR(int, n, tp->djtab);
	ARR(int, n, tp->dctab);
	ARR(int, n, tp->ddtab);

	(tp->djtab)[np] = jmp;
	(tp->dctab)[np] = ch;
	(tp->ddtab)[np] = dig;
}

#ifdef DBG

static void
pr_char(int c)
{
	if (c < 32 || c >= 127)
		printf("\\%03o", c);
	else
		putchar(c);
}

static void
pr_str(const char *str)
{
	const unsigned char *s = (const unsigned char *) str;
	int c;

	for (; (c = *s); ++s)
		pr_char(c);
}
#endif

struct TMachine *
new_TMachine(Terminfo * info, TAction * actions, void *par)
{
	int i, num;
	struct TMachine *tp;
	Bucket *bucks;

	tp = NEW(struct TMachine, 1);

	tp->info = info;
	tp->par = 0;

	num = 0;
	for (i = 0; i < MAX_STR; ++i)
		if (info->strings[i] != -1)
		{
			char *s;

			switch (i)
			{
			case NO_xoffc:
			case NO_xonc:
				continue;
			}

			s = info->buf + info->strings[i];
			if (!*s)
				continue;
			++num;
		}

	bucks = NEW(Bucket, num);

	num = 0;
	for (i = 0; i < MAX_STR; ++i)
		if (info->strings[i] != -1)
		{
			char *s;

			switch (i)
			{
			case NO_xoffc:
			case NO_xonc:
				continue;
			}
			s = info->buf + info->strings[i];

			if (!*s)
				continue;
			bucks[num].str = s;
			bucks[num].no = i;
			++num;
		}

	qsort(bucks, num, sizeof(Bucket), cmp_bucket);

#ifdef DBG
	printf("\nnum=%d:\n", num);
	for (i = 0; i < num; ++i)
	{
		printf("no=%d ,str='", bucks[i].no);
		pr_str(bucks[i].str);
		printf("'\n");
	}
#endif

	tp->program = 0;
	tp->plen = 0;
	make_fa(tp, bucks, 0, num - 1, 0, 0, 0, 0, 0);

#ifdef DBG
	printf("\nprogram length=%d\n", tp->plen);
#endif

	tp->pno = 0;
	tp->pc = 0;

	free(bucks);

	return tp;
}

void
delete_TMachine(struct TMachine *mp)
{
}

int
do_TMachine(struct TMachine *mp, unsigned char byte)
{
	return 0;
}

static int
fa_end(struct TMachine *tp, Bucket * bp, Jtab *jtp, int dig)
{
	int r;

#ifdef DBG
	printf("fa_end: no=%d str=", bp->no);
	pr_str(bp->str);
	putchar('\n');
#endif

	r = code_pos(tp);
	code_byte(tp, CODE_CALL);
	code_short(tp, bp->no);

	if (dig)
		dig_Jtab(jtp, 0, r, dig - 1);

	return r;
}

static void
end_sw(struct TMachine *tp, int start, int numpos, Jtab * jp)
{
	int jmp, i;

	/*printf("end_sw jtab=%p num=%d dnum=%d\n", jp, jp->tabnum, jp->dignum);*/

	jmp = code_pos(tp);
	store_short(tp, start, jmp);
	store_byte(tp, numpos, jp->tabnum);
	store_byte(tp, numpos+1, jp->dignum);

	code_short(tp, jp->def);

	for (i = 0; i < jp->tabnum; ++i)
		code_byte(tp, jp->ctab[i]);
	for (i = 0; i < jp->dignum; ++i)
		code_byte(tp, jp->dctab[i]);

	for (i = 0; i < jp->dignum; ++i)
		code_short(tp, jp->ddtab[i]);

	for (i = 0; i < jp->tabnum; ++i)
		code_short(tp, jp->jtab[i]);
	for (i = 0; i < jp->dignum; ++i)
		code_short(tp, jp->djtab[i]);

}

static int
gen_cmd(struct TMachine * tp, CharRef * ref)
{
	int jmp=0;
	char *buf, *s;
	int l, c;
	CharRef *cbp;
	int incrFlag=0;

	for (l=0, cbp = ref;;)
		if (cbp->ref)
		{
			cbp = cbp->ref;
			++l;
		}
		else
			break;

	buf = (char*) malloc(l+1);
	buf[l]=0;

	for (s=buf+l, cbp = ref;;)
		if (cbp->ref)
		{
			cbp = cbp->ref;
			--s;
			*s = cbp->ch;
		}
		else
			break;

#ifdef DBG
	printf("gen_cmd: str='%s'\n", buf);
#endif
	jmp = code_pos(tp);

	for(s=buf; (c=*s++); )
	{
		switch(c)
		{
		default:
			continue;
		case '%':
			switch( (c=*s++))
			{
				case 0:
					goto ret;
				case 'i':
				case 'I':
					incrFlag=1;
					break;
				case 'p':
				case 'P':
					switch( (c=*s++) )
					{
					case '1':
						code_byte(tp, CODE_P1);
						break;
					case '2':
						code_byte(tp, CODE_P2);
						break;
					case '3':
						code_byte(tp, CODE_P3);
						break;
					case '4':
						code_byte(tp, CODE_P4);
						break;
					case '5':
						code_byte(tp, CODE_P5);
						break;
					case '6':
						code_byte(tp, CODE_P6);
						break;
					case '7':
						code_byte(tp, CODE_P7);
						break;
					case '8':
						code_byte(tp, CODE_P8);
						break;
					case '9':
						code_byte(tp, CODE_P9);
						break;
					case '0':
						code_byte(tp, CODE_P0);
						break;
					case 0:
						goto ret;
					}
					break;
				case '\'':
					c = *s++;
					if (!c)
						goto ret;
					code_byte(tp, CODE_CONST);
					code_byte(tp, c);
					break;
				case '+':
					code_byte(tp, CODE_ADD);
					break;
				case '-':
					code_byte(tp, CODE_SUB);
					break;
				case '/':
					code_byte(tp, CODE_DIV);
					break;
				case '*':
					code_byte(tp, CODE_MUL);
					break;
				case 'm':
					code_byte(tp, CODE_MOD);
					break;
				case '|':
					code_byte(tp, CODE_OR);
					break;
				case '&':
					code_byte(tp, CODE_AND);
					break;
				case '{':
					{
						int c;

						c = atoi(s);
						code_byte(tp, CODE_INT);
						code_short(tp,c);
						while( *s && *s!= '}')
							++s;
						if (!*s)
							goto ret;
					}
					break;
			}
			break;
		}
	}
ret:
	if (incrFlag)
		code_byte(tp, CODE_INCR);

	return jmp;
}


static int
fa_gen(struct TMachine *tp, Bucket * bucks, int beg, int end, int pos,
       int pch, CharRef * ref, CharRef * rch, Jtab * jtp, int dig, int lastref)
{
	int jmp;
	char *s;

	jmp = code_pos(tp);
	s = bucks[beg].str+pos;

	if (ref)
	{
		int cp;

		cp = code_pos(tp);
		if (dig)
		{
			if (!isdigit(pch))
			{
				/*printf("end of digit  detected: beg=%d end=%d pos=%d pch='%c' (%d)\n", beg, end, pos,pch>32?pch:' ', pch);*/
				jmp=make_fa(tp, bucks, beg, end, pos, 0, 0, 0, lastref);
				dig_Jtab(jtp, pch, jmp, dig - 1);
				return jmp;
			}
			else
			{
				if (dig<11)
					dig+= (pch-'0')*10;
				else if (dig<101)
					dig+= (pch-'0')*100;
				else if (dig<1001)
					dig+= (pch-'0')*1000;

				goto def;
			}
		}
		else
		{
			switch (pch)
			{
			case 'c':
				/* generate command */
				if (ref->ch == '%')
				{
					CharRef *cbp;

					for (cbp = ref;;)
						if (cbp->ref)
							cbp = cbp->ref;
						else
							break;

					if (!cbp->tp->def)
					{
						jmp = code_byte(tp, CODE_C_CONV);
						cbp->tp->def = jmp;
						gen_cmd(tp, ref);
					}
					make_fa(tp, bucks, beg, end, pos, 0, 0, 0, lastref);
				}
				else
					goto def;
				break;
			case 'd':
			case '2':
			case '3':
				/* generate command */
				if (ref->ch == '%')
				{
					printf("%%d detected: beg=%d end=%d pos=%d\n", beg, end, pos);
					jmp = code_byte(tp, CODE_D_CONV);
					gen_cmd(tp, ref);
					make_fa(tp, bucks, beg, end, pos, 0, jtp, 1, jmp);
				}
				else
					goto def;
				break;
			case 'o':
				if (ref->ch == '%')
				{
					printf("%%o detected: beg=%d end=%d pos=%d\n", beg, end, pos);
					jmp = code_byte(tp, CODE_O_CONV);
					gen_cmd(tp, ref);
					make_fa(tp, bucks, beg, end, pos, 0, jtp, 1, jmp);
				}
				else
					goto def;
				break;
			default:
			      def:
				rch->ref = ref;
				rch->ch = pch;
				rch->tp = 0;
				jmp = make_fa(tp, bucks, beg, end, pos, rch, jtp, dig,lastref);
				break;
			}
		}
		return cp;
	}
	else if (pch == '%')
	{
		int cp;

		rch->ref = ref;
		rch->ch = pch;
		rch->tp = jtp;

		cp = make_fa(tp, bucks, beg, end, pos, rch, jtp, 0, lastref);

		return cp;
	}
	else if (isdigit(pch))
	{
		int cp;

		rch->ref = ref;
		rch->ch = pch;
		rch->tp = jtp;

		cp = make_fa(tp, bucks, beg, end, pos, rch, jtp, pch-'0'+1, lastref );

		return cp;
	}
	else
	{
		/* if (dig)
		{
			jmp = code_byte(tp, CODE_D_CONV);
			if (lastref)
				gen_cmd(tp, lastref);
		}
		else */
			jmp = code_pos(tp);

		make_fa(tp, bucks, beg, end, pos, 0, 0, 0, 0);

		/*printf("%s_Jtab num=%d, char='%c'(%d), jtab=%p, dig=%d\n",
			dig?"dig":"add",
		       jtp->tabnum, pch < 32 ? ' ' : pch, pch, jtp, dig);*/

		if (dig)
		{
			if (lastref)
				dig_Jtab(jtp, pch, lastref, dig - 1);
			else
				dig_Jtab(jtp, pch, jmp, dig - 1);
		}
		else
			add_Jtab(jtp, pch, jmp);

		return jmp;
	}

}

/*
 *    recursive finite automata builder
 */
static int
make_fa(struct TMachine *tp, Bucket * bucks, int beg, int end, int pos,
	CharRef * ref, Jtab * jlast, int dig, int lastref)
{
	int i;
	int pch;
	int pbeg;
	Jtab jtab;
	Jtab *jtp;
	int jmp;
	int start=0, numpos=0, cp;
	CharRef rch;

	init_Jtab(&jtab);

	pbeg = beg;
	pch = (unsigned char) bucks[beg].str[pos];
	memset(&rch, 0, sizeof(rch));

	if (beg == end && pch == 0)
	{
		jmp = fa_end(tp, bucks + beg, jlast, dig);
		return jmp;
	}

	if (ref||dig)
	{
		jtp = jlast;
		cp = code_pos(tp);
	}
	else
	{
		jtp = &jtab;

		cp = code_byte(tp, CODE_SWITCH);
		numpos = code_byte(tp, 0);
		code_byte(tp, 0);
		start = code_short(tp, 0);
	}

#ifdef DBG
	printf("make_fa: ref=%d beg=%d end=%d pos=%d jtp=%p jlast=%p dig=%d pch=%c\n",
	       ref ? ref->ch : 0, beg, end, pos, jtp, jlast, dig,pch);
#endif

	for (i = beg; i <= end; ++i)
	{
		int c;
		Bucket *bp;

		bp = bucks + i;
		c = (unsigned char) (bp->str[pos]);
		/*printf("i=%d, c=%d, pch=%d\n", i, c, pch);*/

		if (c != pch)
		{
			fa_gen(tp, bucks, pbeg, i - 1, pos + 1, pch, ref, &rch, jtp, dig, lastref);
			pch = c;
			pbeg = i;
		}
	}

	fa_gen(tp, bucks, pbeg, i - 1, pos + 1, pch, ref, &rch, jtp, dig, lastref);

	if (jtp == &jtab)
		end_sw(tp, start, numpos, &jtab);

	destroy_Jtab(&jtab);

	return cp;
}

static int
code_pos(struct TMachine *tp)
{
	return tp->plen;
}

static int
code_short(struct TMachine *tp, short s)
{
	int r;

	r = tp->plen;
	tp->plen += sizeof(short);
	ARR(char, tp->plen, tp->program);

	*(short *) (tp->program + r) = s;

	return r;
}

static int
code_byte(struct TMachine *tp, unsigned char b)
{
	int r;

	r = tp->plen;
	tp->plen += sizeof(unsigned char);
	ARR(char, tp->plen, tp->program);

	*(unsigned char *) (tp->program + r) = b;

	return r;
}

static void
store_short(struct TMachine *tp, int pos, short s)
{
	*(short *) (tp->program + pos) = s;
}

static void
store_byte(struct TMachine *tp, int pos, unsigned char b)
{
	*(unsigned char *) (tp->program + pos) = b;
}

#ifdef DBG
void
dump_program(struct TMachine *tp, int pc)
{
	int j, i;
	unsigned char b, num, code, dnum;
	short s, jp, jp0, jdef;
	int *passed;

	printf("pc %d: ", pc);
	code = GET_BYTE(tp, pc);
	INCR_BYTE(pc);
	switch (code)
	{
	case CODE_END:
		printf("CODE_END\n");
		break;
	case CODE_CALL:
		s = GET_SHORT(tp, pc);
		INCR_SHORT(pc);
		printf("CODE_CALL: %d\n", s);
		break;
	case CODE_SWITCH:
		num = GET_BYTE(tp, pc);
		INCR_BYTE(pc);
		dnum = GET_BYTE(tp, pc);
		INCR_BYTE(pc);
		jp = GET_SHORT(tp, pc);
		INCR_SHORT(pc);
		jdef = GET_SHORT(tp, jp);
		INCR_SHORT(jp);
		printf("CODE_SWITCH: num=%d, dnum=%d start=%d def=%d\nchars:\n", num, dnum, jp, jdef);
		passed = (int *) calloc(num+dnum, sizeof(int));

		for (j = 0; j < num; ++j)
		{
			b = GET_BYTE(tp, jp);
			INCR_BYTE(jp);
			printf("%d: ", j);
			pr_char(b);
			putchar('\n');
		}
		printf("dig chars:\n");
		for (j = 0; j < dnum; ++j)
		{
			b = GET_BYTE(tp, jp);
			INCR_BYTE(jp);
			printf("%d: ", j);
			pr_char(b);
			putchar('\n');
		}

		printf("dig digs:\n");
		for (j = 0; j < dnum; ++j)
		{
			s = GET_SHORT(tp, jp);
			INCR_SHORT(jp);
			printf("%d: %d\n", j, s);
		}
		printf("offsets:\n");
		jp0 = jp;
		for (j = 0; j < num; ++j)
		{
			s = GET_SHORT(tp, jp);
			INCR_SHORT(jp);
			printf("%d: %d\n", j, s);
		}
		printf("dig offsets:\n");
		for (j = 0; j < dnum; ++j)
		{
			s = GET_SHORT(tp, jp);
			INCR_SHORT(jp);
			printf("%d: %d\n", j, s);
		}


		if (jdef)
			dump_program(tp, jdef);
		jp = jp0;
		for (j = 0; j < num+dnum; ++j)
		{
			int found = 0;

			s = GET_SHORT(tp, jp);
			INCR_SHORT(jp);

			for (i = 0; i < num+dnum; ++i)
				if (passed[i] == s)
				{
					found = 1;
					break;
				}
			if (!found)
				dump_program(tp, s);
			else
				printf("already passed (pc=%d)\n", s);

			passed[j] = s;
		}
		free(passed);
		break;
	case CODE_D_CONV:
		printf("CODE_D_CONV\n");
		dump_program(tp, pc);
		break;
	case CODE_O_CONV:
		printf("CODE_O_CONV\n");
		dump_program(tp, pc);
		break;
	case CODE_C_CONV:
		printf("CODE_C_CONV\n");
		dump_program(tp, pc);
		break;
	case CODE_INCR:
		printf("CODE_INCR\n");
		dump_program(tp, pc);
		break;
	case CODE_P1:
		printf("CODE_P1\n");
		dump_program(tp, pc);
		break;
	case CODE_P2:
		printf("CODE_P2\n");
		dump_program(tp, pc);
		break;
	case CODE_P3:
		printf("CODE_P3\n");
		dump_program(tp, pc);
		break;
	case CODE_P4:
		printf("CODE_P4\n");
		dump_program(tp, pc);
		break;
	case CODE_P5:
		printf("CODE_P5\n");
		dump_program(tp, pc);
		break;
	case CODE_P6:
		printf("CODE_P6\n");
		dump_program(tp, pc);
		break;
	case CODE_P7:
		printf("CODE_P7\n");
		dump_program(tp, pc);
		break;
	case CODE_P8:
		printf("CODE_P8\n");
		dump_program(tp, pc);
		break;
	case CODE_P0:
		printf("CODE_P0\n");
		dump_program(tp, pc);
		break;
	case CODE_CONST:
		b = GET_BYTE(tp, pc);
		INCR_BYTE(pc);
		printf("CODE_CONST: '%c' (%d)\n", b<32?' ':b, b);
		dump_program(tp, pc);
		break;
	case CODE_INT:
		s = GET_SHORT(tp, pc);
		INCR_SHORT(pc);
		printf("CODE_INT: %d\n", s);
		dump_program(tp, pc);
		break;
	case CODE_ADD:
		printf("CODE_ADD\n");
		dump_program(tp, pc);
		break;
	case CODE_SUB:
		printf("CODE_SUB\n");
		dump_program(tp, pc);
		break;
	case CODE_DIV:
		printf("CODE_ADD\n");
		dump_program(tp, pc);
		break;
	case CODE_MUL:
		printf("CODE_MUL\n");
		dump_program(tp, pc);
		break;
	case CODE_MOD:
		printf("CODE_MOD\n");
		dump_program(tp, pc);
		break;
	case CODE_OR :
		printf("CODE_OR\n");
		dump_program(tp, pc);
		break;
	case CODE_AND:
		printf("CODE_AND\n");
		dump_program(tp, pc);
		break;
	}
}

#endif
