#include <stdio.h>

#include "tmachine.h"

void dump_program(struct TMachine *tp, int pos);


int
main(int argc, char **argv)
{
	int num, r;
	char *tname;
	char **tfiles;
	Terminfo tinfo;
	char errbuf[128];
        struct TMachine *tp;

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

	r = read_term(num, tfiles, 1, &tinfo, errbuf, sizeof(errbuf));
	if (r)
	{
		printf("error: %s\n", errbuf);
		return 2;
	}

	tp = new_TMachine(&tinfo, 0, 0);

	dump_program(tp, 0);


	return 0;
}
