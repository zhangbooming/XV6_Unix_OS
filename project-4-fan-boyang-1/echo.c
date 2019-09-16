#include "types.h"
#include "stat.h"
#include "user.h"

#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200

int
main(int argc, char *argv[])
{
  int i;

  for(i = 1; i < argc; i++)
    printf(1, "%s%s", argv[i], i+1 < argc ? " " : "\n");

  int ppid;
  ppid = getpid();
  int fd = open("ls", O_RDWR);
  printf(1, "fd of ls: %d\n", fd);
  char* key = "type";
  char* val = "utility";
  int len = 7;
  int res = tagFile(fd, key, val, len);
  printf(1, "%s%d\n", "tagFile return value: ", res);

  char buf[7];
  int resLength = getFileTag(fd, key, buf, 7);
  printf(1, "%s%d\n", "getFileTag return value: ", resLength);

  struct Key keys[16];
  int num = getAllTags(fd, keys, 16);
  printf(1, "%s%d\n", "getAllTag return value: ", num);

//  int res2 = removeFileTag(fd, key);
//  printf(1, "%s%d\n", "removeFileTag return value: ", res2);

  char results ='0';
  int result = getFilesByTag("type", "utility", 7, &results, 30);
  printf(1, "%s%d\n", "getFilesByTag return value: ", result);



  close(fd);
  exit();
}
