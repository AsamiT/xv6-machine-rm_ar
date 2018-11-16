/**

  null.c
  @author: Robert Maloy
  @date: 10/19/2018
  @desc: This program is intended to invoke a null pointer dereferencing on the XV6 Operating system.

  **/

#include "types.h"
#include "user.h"

int main(int argc, char *argv[]) {
	int *ptr; //pointer declaration
	int deref; //deference integer
	printf(1,"code %p\n", main);
	printf(1, "stack %p\n", &ptr);
	printf(1, "heap %p\n", ptr);
	printf(1, "ptr address %x\n", *ptr);
	deref = 3;
	ptr = deref;
	*ptr = 0;
	return 0;
}
