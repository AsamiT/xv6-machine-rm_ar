#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#include "param.h"
#include "syscall.h"
#include "traps.h"
#include "fs.h"
#include "user.h"

#define PGSIZE 4096

int lock_init(lock_t *lk)
{
	lk->flag = 0;
	return 0;
}

void lock_acquire(lock_t *lk){
	while(xchg(&lk->flag, 1) != 0)
	    ;
}

void lock_release(lock_t *lk){
	xchg(&lk->flag, 0);
}

int thread_create(void (*start_routine)(void*), void *arg) {
	lock_t lk;
	lock_init(&lk);
	lock_acquire(&lk);
	void *stack= malloc(PGSIZE*2);
	lock_release(&lk);

	if((uint)stack % PGSIZE) {
		stack = stack + (PGSIZE - (uint)stack % PGSIZE);
	}

	int result = clone(start_routine,arg,stack);
	return result;
}

int thread_join(){
	void *stack = malloc(sizeof(void*));
	int result= join(&stack);

	lock_t lk;
	lock_init(&lk);
	lock_acquire(&lk);
	free(stack);
	lock_release(&lk);

	return result;
}
