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
   // test over write tag 17 times
   ppid = getpid();
   int fd = open("kill", O_RDWR);
   printf(1, "fd of kill: %d\n", fd);
   char* key = "type";

   int index;
   for(index = 0; index < 34; index++){
      if (index < 16)
      {
         char* val = "utility";
         int len = 7;
         int res = tagFile(fd, key, val, len);
         assert(res > 0);

         char buf[7];
         int valueLength = getFileTag(fd, key, buf, 7);
         assert(valueLength == len);


         int i;
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }
      }
      else
      {
         char* val = "total_18characters";
         //printf(1, "current index = %d\n", index);
         int len = 18;
         int res = tagFile(fd, key, val, len);
         assert(res > 0);

         char buf[18];
         int valueLength = getFileTag(fd, key, buf, 18);
         assert(valueLength == len);

         

         int i;
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }
      }

   }
   close(fd);
   printf(1, "Overwrite TEST PASSED\n");

   // test write different tags
   ppid = getpid();
   fd = open("ls", O_RDWR);
   printf(1, "fd of ls: %d\n", fd);

   char* key1 = "type1";
   char* key2 = "type2";
   char* key3 = "type3";
   char* key4 = "type4";
   char* key5 = "type5";
   char* key6 = "type6";
   char* key7 = "type7";
   char* key8 = "type8";
   char* key9 = "type9";
   char* key10 = "type10";
   char* key11 = "type11";
   char* key12 = "type12";
   char* key13 = "type13";
   char* key14 = "type14";
   char* key15 = "type15";
   char* key16 = "type16";

   char* val = "utility";
   int len = 7;
   int res = tagFile(fd, key1, val, len);
   assert(res > 0);
         char buf[7];
         int valueLength = getFileTag(fd, key1, buf, 7);
         assert(valueLength == len);
         int i;
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key2, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key2, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key3, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key3, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key4, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key4, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key5, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key5, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key6, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key6, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key7, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key7, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key8, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key8, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key9, val, len);  
   assert(res > 0);       
         valueLength = getFileTag(fd, key9, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key10, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key10, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key11, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key11, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key12, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key12, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key13, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key13, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key14, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key14, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key15, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key15, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   res = tagFile(fd, key16, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key16, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }

   printf(1, "16 tags has been created\n");

   // a wrong type out of range  16
   char* keyWrong = "typeWrong";
   res = tagFile(fd, keyWrong, val, len);
   assert(res < 0)

   printf(1, "TagOverflow Passed\n");


   // Remove tags
   res = removeFileTag(fd, key1);
   assert(res > 0);

   res = removeFileTag(fd, key5);
   assert(res > 0);

   res = removeFileTag(fd, keyWrong);
   assert(res < 0);

   fd = open("kill", O_RDWR);
   res = removeFileTag(fd, keyWrong);
   assert(res < 0);

   res = removeFileTag(fd, key);
   assert(res > 0);

   printf(1, "RmoveTag Passed\n");

   res = tagFile(fd, key, val, len);
   assert(res > 0);
         valueLength = getFileTag(fd, key, buf, 7);
         assert(valueLength == len);
         for(i = 0; i < len; i++){
            char v_actual = buf[i];
            char v_expected = val[i];
            assert(v_actual == v_expected);
         }
   printf(1, "AgainTag Passed\n");

   exit();
}