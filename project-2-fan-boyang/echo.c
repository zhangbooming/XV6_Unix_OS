#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i;

  for(i = 1; i < argc; i++)
    printf(1, "%s%s", argv[i], i+1 < argc ? " " : "\n");
  char *p = malloc(sizeof(char)*1024*1024*200);
  *p = 'a';
  int pid;
  pid = fork();
  if(pid == 0) {
    printf(1, "%s%d\n", "Children process pid is: \n", pid);
    exit();
  }
  if(pid == -1) {
    printf(1, "%s\n", "failure\n");
    exit();
  }else{
    printf(1, "%s%d\n","Parent produce a children process, which id is: \n", pid );
  }
  exit();
}
