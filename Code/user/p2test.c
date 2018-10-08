#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pstat.h"
int main(int argc, char *argv[])
{
        int x = settickets(15);
        struct pstat *stat1;
        stat1 = malloc(sizeof(*stat1));
        int y = getpinfo(stat1);
        int counter = 0;
        for(counter = 0; counter < NPROC; counter++) {
          if (stat1->pid[counter] > 0) {
                printf(1, "-------\n");
                printf(1, "PID: %d\n", stat1->pid[counter]);
                printf(1, "in-use: %d\n", stat1->inuse[counter]);
                printf(1, "ticks: %d\n", stat1->ticks[counter]);
                printf(1, "tickets: %d\n", stat1->tickets[counter]);
        }
        }
        y = y+0;
        exit();
}
