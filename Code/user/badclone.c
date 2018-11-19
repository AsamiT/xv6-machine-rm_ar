/*
 
 @name: badclone.c
 @date: Nov 2018
 
 This program is a clone of the tester program provided by Akshay Uttamani on YouTube, as provided
 by the professor. Other than preprocessor declarations, defining assert, and some minor changes
 to improve the program as a whole, I do not claim any copyrights to this program.
 
 */

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "x86.h"
#define PGSIZE (4096)

/* global declarations */
int pid;
int global = 1;

#define assert(x) if (x) {} else { \
    printf(1, "%s: %d ", __FILE__, __LINE__); \
    printf(1, "assert failed (%s)\n", # x); \
    printf(1, "TEST FAILED\n"); \
    kill(pid); \
    exit(); \
}

/*
 
what does this define do?
This define gives us the functionality of "assert", which simply does debug checks
to make sure we're on track. If it detects a mismatch between the "x" value provided,
it will identify where the error is occurring, and terminate the program.
 
 */

void worker(void *arg_ptr); // declaring it here because reasons

int main(int argc, char *argv[]) {
    pid = getpid(); //pid number
    void *stack = malloc(PGSIZE*2); //allocate a memory area twice the page size we defined above
    assert(stack != NULL); //check to make sure stack was properly allocated
    if((uint)stack % PGSIZE == 0) { //if stack is modulo PGSIZE
        stack += 4;
    }
    
    assert(clone(worker, 0, stack) == -1);
    
    stack = sbrk(0);
    if((uint)stack % PGSIZE) {
        stack = stack + (PGSIZE - (uint)stack % PGSIZE);
    }
    sbrk( ((uint)stack - (uint)sbrk(0)) + PGSIZE/2);
    assert((uint)stack % PGSIZE == 0);
    assert((uint)sbrk(0) - (uint)stack == PGSIZE/2);
    
    assert(clone(worker, 0, stack) == -1);
    printf(1, "TEST PASSED\n");
    exit();
}

void worker(void *arg_ptr) {
    assert(global == 1);
    global = 5;
    exit();
}
