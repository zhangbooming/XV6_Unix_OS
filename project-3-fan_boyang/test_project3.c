#include "types.h"
#include "stat.h"
#include "user.h"

void run(void* id) {
  printf(1, "hello\n");
  volatile int i;
  for(i=0; i<100000000; i++);
  printf(1, "Child %d done\n", *(int*)id);
}

int main(int argc, char *argv[]) {
  int one = 1;
  int two = 2;
  thread_create(run, &one);
  thread_create(run, &two);
  printf(1, "now exit without waiting for children\n");
  exit();
}