// #include "uthreadlib.c"
#include "types.h"
#include "user.h"
int ppid;
#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   kill(ppid); \
   exit(); \
}
#define TIMES 1000000

void dosomework(void*);

int global = 0;
lock_t lock;

int 
main(int argc, char *argv[]) {
    lock_init(&lock);
    ppid = getpid();

    int tid1 = thread_create(dosomework, 0);
    assert(tid1 > 0);
    int tid2 = thread_create(dosomework, 0);
    assert(tid2 > 0 );
    thread_join(tid1);
    thread_join(tid2);
    printf(1, "[DEBUG] global = %d...\n", global);
    assert(global == 2*TIMES);
    printf(1, "TEST PASSED...\n");
    exit();
}

void
dosomework(void* arg) {
    int i, j;
    int x;
    
    for(i = 0; i < TIMES; i++ ) {
        // lock_acquire(&lock);
        x = global + 1;
        for(j = 0; j < 1; j++) {
            global = x;
        }
        // lock_release(&lock);
    }
    
    exit();
}