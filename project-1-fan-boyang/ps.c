#include "types.h"
#include "user.h"
#include "ProcessInfo.h"

#define MAX_PROC 64 // maximum number of processes
int main(int argc, char *argv[])
{
    struct ProcessInfo *p = malloc(MAX_PROC * sizeof(struct ProcessInfo));
    struct ProcessInfo *pointer;
    int count = ps(p);

    if (count <= 0)
    {
        printf(2, "ps failed\n");
        exit();
    }

    pointer = p;
    int i = 0;
    enum procstate
    {
        UNUSED,
        EMBRYO,
        SLEEPING,
        RUNNABLE,
        RUNNING,
        ZOMBIE
    };
    static char *states[] = {
        [UNUSED] "unused",
        [EMBRYO] "embryo",
        [SLEEPING] "sleep ",
        [RUNNABLE] "runble",
        [RUNNING] "run   ",
        [ZOMBIE] "zombie"};
    char *state;
    for (; i < MAX_PROC; i++)
    {
        state = states[pointer->state];
        if (pointer->state != UNUSED)
        {
            printf(2, "%d  %d  %s  %d  %s\n", pointer->pid, pointer->ppid, state, pointer->sz, pointer->name);
        }
        pointer++;
    }
    exit();
}