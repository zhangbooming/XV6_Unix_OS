/* call tagFile to tag a file.  Call getFileTag to read the tag of that file. */
#include "types.h"
#include "user.h"

#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200

#undef NULL
#define NULL ((void*)0)

int ppid;
volatile int global = 1;

int
main(int argc, char *argv[])
{
    int ppid;
    ppid = getpid();
    int fd = open("ls", O_RDWR);
    printf(1, "fd of ls: %d\n", fd);
    char* key = "type";
    char* val = "utility";
    int len = 7;
    int res = tagFile(fd, key, val, len);
    printf(1, "%s%d\n", "tagFile return value: ", res);


    int fd1 = open("echo", O_RDWR);
    printf(1, "fd of echo: %d\n", fd1);
    char* key1 = "type";
    char* val1 = "utility";
    int len1 = 7;
    int res1 = tagFile(fd1, key1, val1, len1);
    printf(1, "%s%d\n", "tagFile return value: ", res1);

    char buf[7];
    int resLength = getFileTag(fd, key, buf, 7);
    printf(1, "%s%d\n", "getFileTag return value: ", resLength);


    char results ='0';
    int result = getFilesByTag("type", "utility", 7, &results, 30);
    printf(1, "%s%d\n", "getFilesByTag return value: ", result);

    //printf(1, "%s\n", results);



    close(fd);
    close(fd1);
    exit();



}
