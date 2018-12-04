/*
 
 @name: rjm_jointest2.c
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
int global = 1;
int pid;

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
    if((uint)stack % PGSIZE) { //if stack is modulo PGSIZE
        stack = stack + (PGSIZE - (uint)stack % PGSIZE); //stack = stack + (stack mod pgsize)
    }
    
    int arg = 42;
    int clone_pid = clone(worker, &arg, stack); //clone the process with the stack
    assert(clone_pid > 0); //assert to see if the clone worked
    
    sbrk(PGSIZE);
    void **join_stack = (void**) ((uint)sbrk(0) - 4);
    assert(join((void**) ((uint)join_stack + 2)) == -1);
    assert(join(join_stack) == clone_pid);
    assert(stack == *join_stack);
    assert(global == 2);
    
    printf(1, "TEST PASSED\n"); //pronounce our success!
    exit(); //follow the rabbit hole
}

void worker(void *arg_ptr) {
    int arg = *(int*)arg_ptr;
    assert(arg == 42);
    assert(global == 1);
    global++;
    exit();
}
