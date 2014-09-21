#include <stdio.h>
#include <string.h>

#include "tcaps.h"

#define PR_BOOL(cap) printf(#cap": %d\n", tinfo.bools[NO_##cap])
#define PR_NUM(cap) printf(#cap": %d\n", tinfo.nums[NO_##cap])
#define PR_STR(cap) {printf(#cap": ");pr_str(&tinfo,NO_##cap);putchar('\n');}
#define PR_KEY(cap) {printf(#cap": ");pr_key(&tinfo,NO_##cap);putchar('\n');}

static void
pr_estr(const char *str)
{
	const unsigned char *s = (const unsigned char *) str;
	int c;

	for (; (c = *s); ++s)
	{
		if (c < 32 || c >= 127)
			printf("\\%03o", c);
		else
			putchar(c);
	}
}

static void
pr_str(Terminfo * tinfo, int no)
{
	if (tinfo->strings[no] == -1)
		printf("(null)");
	else
		pr_estr(tinfo->buf + tinfo->strings[no]);
}

static void
pr_key(Terminfo * tinfo, int no)
{
	if (tinfo->keys[no] == -1)
		printf("(null)");
	else
		pr_estr(tinfo->buf + tinfo->keys[no]);
}

int
main(int argc, char **argv)
{
	int num, r;
	char *tname;
	char **tfiles;
	Terminfo tinfo;
	char errbuf[128];

	if (argc < 3)
	{
		printf("usage: %s termname tcapfile1 [... tcapfileN]\n", argv[0]);
		return 1;
	}

	num = argc - 2;
	tfiles = argv + 2;
	tname = argv[1];

	init_Terminfo(&tinfo);
	tinfo.name = tname;

	r = read_tcap(num, tfiles, 1, &tinfo, errbuf, sizeof(errbuf));
	if (r)
	{
		printf("error: %s\n", errbuf);
		return 2;
	}

	/*printf("entry:\n%s\n", tinfo.entry);*/

	printf("bool capabilities:\n");
	PR_BOOL(bw);
	PR_BOOL(am);
	PR_BOOL(bce);
	PR_BOOL(ccc);
	PR_BOOL(xhp);
	PR_BOOL(xt);
	PR_BOOL(xenl);
	PR_BOOL(eo);
	PR_BOOL(gn);
	PR_BOOL(hc);
	PR_BOOL(km);
	PR_BOOL(hs);
	PR_BOOL(hls);
	PR_BOOL(os);
	PR_BOOL(sam);
	PR_BOOL(hz);
	PR_BOOL(ul);
	PR_BOOL(xon);

	PR_BOOL(bs);
	PR_BOOL(mir);
	PR_BOOL(msgr);
	PR_BOOL(pt);
	PR_BOOL(C2);
	PR_BOOL(CY);

	printf("\nnum capabilities:\n");
	PR_NUM(cols);
	PR_NUM(it);
	PR_NUM(lines);
	PR_NUM(xmc);
	PR_NUM(colors);
	PR_NUM(pairs);
	PR_NUM(ncv);
	PR_NUM(wsl);
	PR_NUM(btns);

	PR_NUM(Nf);
	PR_NUM(Nb);

	printf("\nstr capabilities:\n");
	PR_STR(cbt);
	PR_STR(bel);
	PR_STR(cr);
	PR_STR(csr);
	PR_STR(tbc);
	PR_STR(mgc);
	PR_STR(clear);
	PR_STR(el1);
	PR_STR(el);
	PR_STR(ed);
	PR_STR(hpa);
	PR_STR(cwin);
	PR_STR(cup);
	PR_STR(cud1);
	PR_STR(home);
	PR_STR(civis);
	PR_STR(cub1);
	PR_STR(cnorm);
	PR_STR(cuf1);
	PR_STR(ll);
	PR_STR(cuu1);
	PR_STR(cvvis);
	PR_STR(dl1);
	PR_STR(dsl);
	PR_STR(smacs);
	PR_STR(blink);
	PR_STR(bold);
	PR_STR(smcup);
	PR_STR(dim);
	PR_STR(prot);
	PR_STR(rev);
	PR_STR(invis);
	PR_STR(smso);
	PR_STR(smul);
	PR_STR(smxon);
	PR_STR(ech);
	PR_STR(rmacs);
	PR_STR(sgr0);
	PR_STR(rmcup);
	PR_STR(rmso);
	PR_STR(rmul);
	PR_STR(rmxon);
	PR_STR(ff);
	PR_STR(fsl);
	PR_STR(is1);
	PR_STR(is2);
	PR_STR(is3);
	PR_STR(initc);
	PR_STR(initp);
	PR_STR(ich1);
	PR_STR(il1);
	PR_STR(oc);
	PR_STR(op);
	PR_STR(dch);
	PR_STR(dl);
	PR_STR(cud);
	PR_STR(ich);
	PR_STR(indn);
	PR_STR(il);
	PR_STR(cub);
	PR_STR(cuf);
	PR_STR(rin);
	PR_STR(cuu);
	PR_STR(rep);
	PR_STR(rs1);
	PR_STR(rs2);
	PR_STR(rs3);
	PR_STR(rc);
	PR_STR(vpa);
	PR_STR(sc);
	PR_STR(ind);
	PR_STR(ri);
	PR_STR(sgr);
	PR_STR(setb);
	PR_STR(scp);
	PR_STR(setf);
	PR_STR(wind);
	PR_STR(ht);
	PR_STR(tsl);
	PR_STR(uc);
	PR_STR(xoffc);
	PR_STR(xonc);
	PR_STR(smsc);
	PR_STR(rmsc);
	PR_STR(getm);
	PR_STR(kmous);
	PR_STR(minfo);
	PR_STR(reqmp);
	PR_STR(scesc);
	PR_STR(setab);
	PR_STR(setaf);

	PR_STR(rs);
	PR_STR(enacs);
	PR_STR(smam);
	PR_STR(rmam);
	PR_STR(hts);
	PR_STR(rmir);
	PR_STR(dch1);
	PR_STR(vt);
	PR_STR(sb);

	PR_STR(Cs);
	PR_STR(Ce);
	PR_STR(Cf);
	PR_STR(Cb);
	PR_STR(Ct);
	PR_STR(g1);
	PR_STR(g2);
	PR_STR(Mf);
	PR_STR(Mb);

	PR_STR(smkx);
	PR_STR(rmkx);

	printf("\nkey capabilities:\n");
	PR_KEY(acsc);
	PR_KEY(ka1);
	PR_KEY(kb2);
	PR_KEY(ka3);
	PR_KEY(kc1);
	PR_KEY(kc3);
	PR_KEY(kbs);
	PR_KEY(kcbt);
	PR_KEY(kctab);
	PR_KEY(kdch1);
	PR_KEY(kcud1);
	PR_KEY(kf0);
	PR_KEY(kf1);
	PR_KEY(kf2);
	PR_KEY(kf3);
	PR_KEY(kf4);
	PR_KEY(kf5);
	PR_KEY(kf6);
	PR_KEY(kf7);
	PR_KEY(kf8);
	PR_KEY(kf9);
	PR_KEY(kf10);
	PR_KEY(kf11);
	PR_KEY(kf12);
	PR_KEY(kf13);
	PR_KEY(kf14);
	PR_KEY(kf15);
	PR_KEY(kf16);
	PR_KEY(kf17);
	PR_KEY(kf18);
	PR_KEY(kf19);
	PR_KEY(kf20);
	PR_KEY(kf21);
	PR_KEY(kf22);
	PR_KEY(kf23);
	PR_KEY(kf24);
	PR_KEY(kf25);
	PR_KEY(kf26);
	PR_KEY(kf27);
	PR_KEY(kf28);
	PR_KEY(kf29);
	PR_KEY(kf30);
	PR_KEY(kf31);
	PR_KEY(kf32);
	PR_KEY(kf33);
	PR_KEY(kf34);
	PR_KEY(kf35);
	PR_KEY(kf36);
	PR_KEY(kf37);
	PR_KEY(kf38);
	PR_KEY(kf39);
	PR_KEY(kf40);
	PR_KEY(kf41);
	PR_KEY(kf42);
	PR_KEY(kf43);
	PR_KEY(kf44);
	PR_KEY(kf45);
	PR_KEY(kf46);
	PR_KEY(kf47);
	PR_KEY(kf48);
	PR_KEY(kf49);
	PR_KEY(kf50);
	PR_KEY(kf51);
	PR_KEY(kf52);
	PR_KEY(kf53);
	PR_KEY(kf54);
	PR_KEY(kf55);
	PR_KEY(kf56);
	PR_KEY(kf57);
	PR_KEY(kf58);
	PR_KEY(kf59);
	PR_KEY(kf60);
	PR_KEY(kf61);
	PR_KEY(kf62);
	PR_KEY(kf63);
	PR_KEY(khome);
	PR_KEY(kich1);
	PR_KEY(kcub1);
	PR_KEY(kll);
	PR_KEY(knp);
	PR_KEY(kpp);
	PR_KEY(kcuf1);
	PR_KEY(kind);
	PR_KEY(kri);
	PR_KEY(khts);
	PR_KEY(kcuu1);

	PR_KEY(kclr);

	PR_KEY(kfnd);
	PR_KEY(kbeg);
	PR_KEY(kcan);
	PR_KEY(kclo);
	PR_KEY(kcmd);
	PR_KEY(kcpy);
	PR_KEY(kcrt);
	PR_KEY(kend);
	PR_KEY(kent);
	PR_KEY(kext);



	return 0;
}
