/*
 
 @name: lock_test.c
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
int global = 0;
lock_t lock;
int num_threads = 30;
int loops = 1000;
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
	lock_init(&lock);
	
	int i;
	for (i = 0; i < num_threads; i++) {
		int thread_pid = thread_create(worker, 0);
		assert(thread_pid > 0);
	}
	for (i = 0; i < num_threads; i++) {
		int join_pid = thread_join();
		assert(join_pid > 0);
	}
	
	assert(global == num_threads * loops);
	printf(1, "TEST PASSED!\n");
	exit();
	
}

void worker(void *arg_ptr) {
    int i, j, tmp;
	for (i = 0; i < loops; i++) {
		lock_acquire(&lock);
		tmp = global;
		for(j=0; j<50; j++);
		global = tmp + 1;
		lock_release(&lock);
	}
	exit();
}
