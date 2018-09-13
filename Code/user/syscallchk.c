/**

  syscallchk.c
  @author: Robert Maloy and Amber Raymer
  @date: 09/13/2018
  @desc: This program is intended to invoke a one-time report of how many system calls have been executed since kernel initialization.

  **/

#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
  printf(1, "There have been %d system calls.\n", howmanysys());
  exit();
}
