#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"
#include "sysfunc.h"
#include "rand.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->tickets = 1;
  p->ticks = 0;
  p->queue = 1;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  pid = np->pid;
  np->state = RUNNABLE;
  np->tickets = proc->tickets; //clone the tickets from source
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){

    	/*if it is a thread then clear the parent value of the thread. else will cause problem in wait method*/
      if(p->pgdir != proc->pgdir){
    	  p->parent = initproc;
		  if(p->state == ZOMBIE)
			wakeup1(initproc);
      }
      else{
    	  /*when parent exits , if it has threads then need to set parent of thread to 0 .
    	  When shell is waiting for parent , it checks if parent has children. so if the parent field of thread is not cleared then in wait the
    	  threa's parent will be set to initproc . then shell will start again . some confusion will happen */
    	  p->parent = 0;
    	  p->state =ZOMBIE;
      }

    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int kids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    kids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      /*if the p is a thread of parent then do not wait. wait only for child.*/
      if(p->pgdir == proc->pgdir && p->pid!=0 && p->state == ZOMBIE)//TODO Why p->state ==ZOMBIE
//      if(p->pgdir == proc->pgdir)
         continue;
      kids = 1;

      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!kids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    //Declare maximum ticket count -- static
    //Declare ticket variable and ticktock variable, non-static

    int tickets = 0;
    int ticktock = 0;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE) {
        continue;
        }
      tickets += p->tickets;

      if (p->queue == 1) {
        ticktock++;
      }

      if (tickets > 0) {
        int winner = random_at_most(20);
        tickets = winner+1;
      }

      if (p->queue == 2 && ticktock > 0) {
          continue;
      }

      int j = 1;
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      if (p->queue == 1) {
        p->ticks++;
      }
      else {
        p->ticks++;
      }
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      if (p->queue == 1) {
        p->queue = 2;
        ticktock--;
      }
      else if (1==j && p->state == RUNNABLE) {
        proc = p;
        p->state = RUNNING;
        p->ticks++;
        switchuvm(p);
        swtch(&cpu->scheduler, proc->context);
        switchkvm();
      }
      proc = 0;
      p->tickets = tickets;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int getpinfo(void) {
  struct pstat *ps;
  struct proc *p;
  int i;

  if (argptr(0, (void *)&ps, sizeof(*ps)) < 0) {
    return -1;
  }
  acquire(&ptable.lock);
  for (i=0; i < NPROC; i++) {
    p = &ptable.proc[i];
    ps->pid[i] = p->pid;
    if (p->state == UNUSED) {
      ps->inuse[i] = 0;
    }
    else {
      ps->inuse[i] = 1;
    }
    ps->ticks[i] = p->ticks;
    ps->tickets[i] = p->tickets;
  }
  release(&ptable.lock);
  return 0;
}

int settickets(void) {
  int num;
  if (argint(0, &num) < 0) {
    return -1;
  }
  if (num < -1) {
    return -1;
  }
  proc->tickets = num;
  return 0;
}

int clone(void)
{
	uint ustack[2];
	void(*fcn)(void*);
	void *arg;
	void *stack;

	if(argptr(0, (void*)&fcn, sizeof(fcn) < 0))
	{
		return -1;
	}
	if(argptr(1, (void*)&arg, sizeof(arg) < 0))
	{
		return -1;
	}
	if(argptr(2, (void*)&stack, sizeof(stack) < 0))
	{
		return -1;
	}

	int i, pid;
	struct proc *np;

	if((uint)stack%PGSIZE!=0){
		return -1;
	}
	if((proc->sz - (uint)stack) < PGSIZE){
		return -1;
	}

	// We will allocate the process.
	if((np = allocproc()) == 0)
	return -1;

	// Copy current process state.
	np->pgdir = proc->pgdir;
	np->sz = proc->sz;
	np->parent = proc;
	*np->tf = *proc->tf;
	np->tf->esp = (uint)stack+PGSIZE;
	np->tstack = (uint)stack;

	//set stack values
	ustack[0] = 0xffffffff;
	ustack[1] = (uint)arg;
	np->tf->esp -= (2)*4;
	copyout(np->pgdir, np->tf->esp, ustack, (2)*4);

	//Clear %eax so that fork returns 0 in the child proc.
	np->tf->eax = 0;
	np->tf->eip = (uint)fcn;
	np->tf->ebp = np->tf->esp;

	 for(i = 0; i < NOFILE; i++) {
	 	if(proc->ofile[i]) {
	      		np->ofile[i] = filedup(proc->ofile[i]);
		}
		np->cwd = idup(proc->cwd);
	 }

	pid = np->pid;
	np->state = RUNNABLE;
	safestrcpy(np->name, proc->name, sizeof(proc->name));
	return pid;
}

int
join(void)
{
  void **stack;
  if(argptr(0, (void*)&stack, sizeof(stack) < 0))
			return -1;
  if((proc->sz-(uint)stack)< sizeof(void**))
	  return -1;
  struct proc *p;
  int havethreads, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie threads.
	havethreads = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    	/*if not a thread then keep scanning the pg table*/
      if(p->pgdir != proc->pgdir )
        continue;
      if(p->parent != proc )
              continue;
	  havethreads = 1;

      if(p->state == ZOMBIE ){
    	pid = p->pid;
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        *((int*)((int*)stack))=p->tstack;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havethreads || proc->killed){
      release(&ptable.lock);
      return -1;
    }
    // Wait for children to exit.
    sleep(proc, &ptable.lock);
  }
}
