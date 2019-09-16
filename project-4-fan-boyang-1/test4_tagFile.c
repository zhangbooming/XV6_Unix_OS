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

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   kill(ppid); \
   exit(); \
}

int
main(int argc, char *argv[])
{
   ppid = getpid();
   int fd = open("ls", O_RDWR);
   printf(1, "fd of ls: %d\n", fd);
   char* key = "type";
   char* val = "utility";
   int len = 7;
   int res = tagFile(fd, key, val, len);

   tagFile(fd, "bbyy", "12345", 5);
   assert(res > 0);

   char buf[7];
   int valueLength = getFileTag(fd, key, buf, 7);
   assert(valueLength == len);


   struct Key keys[16];
   int numTags = getAllTags(fd, keys, 16);

   //close(fd);

   int i;
   for(i = 0; i < len; i++){
      char v_actual = buf[i];
      char v_expected = val[i];
      assert(v_actual == v_expected);
   }


   printf(1, "%s%d\n", "the return value of getAllTags is: ", numTags);

   printf(1, "TEST PASSED\n");

   //exit();

   //----------------------------------------------------------------------
   ppid = getpid();
   //int fd1 = open("ls", O_RDWR);
   printf(1, "%s%d\n", "the value of fd1: ", fd);
//    char* key1 = "type";
//    char* val1 = "utility";
//    int len1 = 7;
//    int res1 = tagFile(fd1, key1, val1, len1);
//    printf(1, "%s%d\n", "the return value of tagFile is: ", res1);
   //close(fd1);


   //懂了,open的时候,O_RDWR和这个O_RDONLY打开的不是一个文件.....

   //int fd = open("ls", O_RDONLY);
   //printf(1, "%s%d\n", "the value of fd: ", fd);

   struct Key keys1[16];
   int numTags1 = getAllTags(fd, keys1, 16);
   printf(1, "%s%d\n", "the return value of getAllTags is: ", numTags1);

   if(numTags1 < 0){
      exit();
   }
   if(numTags1 > 16){
      numTags1 = 16;
   }
   char buffer[18];
   printf(1, "Here is a list of this file's tags:\n");
   for(i = 0; i < numTags1; i++){
      //printf(1,"%s\n" ,"进入了numTags这个大的循环中, 开始去找对应的值, 然后,从buffer中读取的");
      int res = getFileTag(fd, keys1[i].key, buffer, 18);
      if(res > 0){
         printf(1, "%s: %s\n", keys1[i].key, buffer);
      }
   }
   //printf(1,"%s\n" ,"出了循环");
   close(fd);
   //printf(1,"%s\n" ,"也就是关闭文件成功");
   exit();



}
