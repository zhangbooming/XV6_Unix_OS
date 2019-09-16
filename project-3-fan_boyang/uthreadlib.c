#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "x86.h"
#define PGSIZE (4096)


void* stackArr[64];
int pidArr[64];


int
thread_create(void(*start_routine)(void*), void* arg){
    void *stack = malloc(PGSIZE*2);
    void* realStack = stack;
    if((uint)stack % PGSIZE) {
        stack = stack + (PGSIZE - (uint)stack % PGSIZE);
    }
    int clone_pid = clone(start_routine, arg, stack);

    //记录clone_pid对应的这个*stack的指针

    if( clone_pid > 0 ) {
        int i;
        for (i = 0; i < 64; i++) {
            if (pidArr[i] == 0) {
                pidArr[i] = clone_pid;
                stackArr[i] = realStack;
                break;
            }
        }
    }else{
        free(realStack);
    }



    return clone_pid;
}

int
thread_join(int pid){
    // 这个直接调用就好了, 但是疑惑在于 要求 Cleans up the completed thread's user stack
    // 也就是对于这个user stack我们没有记录它,只有我们记录了这个stack,后面才能给清理掉,这个是一个隐患.
    int join_pid = join(pid);

    //找到join_pid(子线程的pid)对应的*stack,调用free函数进行清理.

    if ( join_pid > 0 ) {
        int j;
        for (j = 0; j < 64; j++) {
            if (pidArr[j] == join_pid) {
                pidArr[j] = 0;
                free(stackArr[j]);
                stackArr[j] = 0;
                break;
            }
        }
    }

    if(join_pid > 0)
      return 0;
    return join_pid;
}

void lock_init(lock_t *lock) {
  lock->flag = 0;
}

void lock_acquire(lock_t *lock) {
  while(xchg(&lock->flag, 1) != 0);
}

void lock_release(lock_t *lock) {
  xchg(&lock->flag, 0);
}

void cv_wait(cond_t* conditionVariable, lock_t* lock){
  cvsleep(conditionVariable, lock);
  return;
}

void cv_signal(cond_t* conditionVariable){
  cvwake(conditionVariable);
  return;
}

