#ifndef TMACHINE_H
#define TMACHINE_H

#include "tcaps.h"


struct TMachine;

typedef int (*TAction)(struct TMachine *mp, Terminfo *tp, void *par, int nparam, ... );


struct TMachine *new_TMachine(Terminfo *info, TAction *actions, void *par);
void delete_TMachine(struct TMachine *mp);

int do_TMachine(struct TMachine *mp, unsigned char byte);
int param_TMachine(struct TMachine *mp, int pno);
Terminfo *tinfo_TMachine(struct TMachine *mp);

#endif
