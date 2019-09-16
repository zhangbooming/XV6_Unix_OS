#include "types.h"
#include "user.h"
// make sure that struct Key is included via either types.h or user.h above

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
    ppid = getpid();

    int fd1 = open("ls", O_RDWR);
    printf(1, "%s%d\n", "the value of fd1: ", fd1);
//    char* key1 = "type";
//    char* val1 = "utility";
//    int len1 = 7;
//    int res1 = tagFile(fd1, key1, val1, len1);
//    printf(1, "%s%d\n", "the return value of tagFile is: ", res1);
    //close(fd1);


    //懂了,open的时候,O_RDWR和这个O_RDONLY打开的不是一个文件.....

    //int fd = open("ls", O_RDONLY);
    //printf(1, "%s%d\n", "the value of fd: ", fd);

    struct Key keys[16];
    int numTags = getAllTags(fd1, keys, 16);
    printf(1, "%s%d\n", "the return value of getAllTags is: ", numTags);

    if(numTags < 0){
        exit();
    }
    if(numTags > 16){
        numTags = 16;
    }
    char buffer[18];
    int i;
    printf(1, "Here is a list of this file's tags:\n");
    for(i = 0; i < numTags; i++){
        //printf(1,"%s\n" ,"进入了numTags这个大的循环中, 开始去找对应的值, 然后,从buffer中读取的");
        int res = getFileTag(fd1, keys[i].key, buffer, 18);
        if(res > 0){
            printf(1, "%s: %s\n", keys[i].key, buffer);
        }
    }
    //printf(1,"%s\n" ,"出了循环");
    close(fd1);
    //printf(1,"%s\n" ,"也就是关闭文件成功");
    exit();

}