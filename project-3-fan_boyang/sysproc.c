#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

struct
{
  uint flag;
}lock_sbrk;

// lock_sbrk->flag = 0;

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  //获得某个锁, 在sbrk处进行调用
  //acquire(somelock);
  while(xchg(&lock_sbrk.flag, 1) != 0);
  addr = myproc()->sz;
  if(growproc(n) < 0) {
    //失败的话,依旧要释放锁,因为失败了,所以,pgdir没有变化,也没有分配成功的,不需要操作了
    //release(somelock);
    xchg(&lock_sbrk.flag, 0);
    return -1;
  }
  //分配成功,对proc进行广播的, 但是这个函数里面没有ptable如果,在这个位置进行ptable的遍历是非常复杂的
  // 所以,还是放到proc.c文件中进行sz的广播的.
  //release(somelock);

  xchg(&lock_sbrk.flag, 0);
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_clone(void){
    //这里面就是要去进程里面拿参数的,这个照抄exe就可以了
    int fcn;
    int arg;
    int stack;
    if(argint(0, &fcn) <0 || argint(1, &arg) <0 || argint(2, &stack) <0)
        return -1;
    return clone((void*)fcn, (void*)arg, (void*)stack);
}

int
sys_join(void){
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return join(pid);

}

int
sys_cvsleep(void){
  void *cv, *tmp;

  if(argptr(0, (void *)&cv, sizeof(void *)) < 0)
    return -1;

  if(argptr(1, (void *)&tmp, sizeof(void *)) < 0)
    return -1;
  
  lock_t *lock = (lock_t *)tmp;
  cv_sleep(cv, lock);
  return 0;
}

int
sys_cvwake(void){
  void *cv;
  
  if(argptr(0, (void *)&cv, sizeof(void *)) < 0)
    return -1;
  cv_wake(cv);
  return 0;
}