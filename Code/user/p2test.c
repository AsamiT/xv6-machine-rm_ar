#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pstat.h"

int main(int argc, char *argv[])
{
        int x = settickets(3);

        struct pstat *stat1;
        stat1 = malloc(sizeof(*stat1));
        int y = getpinfo(stat1);


        int counter = 0;
        for(counter = 0; counter < NPROC; counter++) {
                printf(1, "PID %d, \n", stat1->pid[counter]);
                printf(1, "in-use %d, \n", stat1->inuse[counter]);
                printf(1, "high-ticks %d, \n", stat1->hticks[counter]);
                printf(1, "low-ticks %d, \n", stat1->lticks[counter]);
        }
        y = y+0;
        printf(1, "hello world %d, \n", x);
        exit();
}