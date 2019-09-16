struct stat;
struct rtcdate;
typedef struct{
  uint flag;
} lock_t;

typedef struct {
  lock_t *lock;
} cond_t;
// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);

char* sbrk(int);
int sleep(int);
int uptime(void);
//int clone(void(*fn)(void*), void* arg, void* stack)
//int clone(void* (*)(void*), void*, void*)
int clone(void(*)(void*), void* arg, void* stack);
int join(int);
void cvsleep(void*, lock_t*);
void cvwake(void*);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

//-----------------uthreadlib.c-----------------
//thread wrappers
int thread_create(void(*)(void*), void* arg);
int thread_join(int);
//Locks
void lock_acquire(lock_t* lock);
void lock_release(lock_t* lock);
void lock_init(lock_t* lock);

void cv_wait(cond_t* conditionVariable, lock_t* lock);
void cv_signal(cond_t* conditionVariable);