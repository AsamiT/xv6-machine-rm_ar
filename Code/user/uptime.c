/**

  uptime.c
  @author: Robert Maloy
  @date: 09/13/2018
  @desc: Program to test implementation of new executable programs.

  **/

#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
  printf(1, "Uptime: %d\n", uptime());
  exit();
}
