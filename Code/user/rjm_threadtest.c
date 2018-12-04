/*
 
 @name: rjm_threadtest.c
 @date: Dec 2018
 
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
    int arg = 35; //argument
    int thread_pid = thread_create(worker, &arg); //create thread
    assert(thread_pid > 0);
    
    int join_pid = thread_join();
    assert(join_pid == thread_pid);
    assert(global == 2);
    printf(1, "TEST PASSED\n"); //pronounce our success!
    exit(); //follow the rabbit hole
}

void worker(void *arg_ptr) {
    int arg = *(int*)arg_ptr;
    assert(arg==35);
    assert(global==1);
    global++;
    exit();
}
