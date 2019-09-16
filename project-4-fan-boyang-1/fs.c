// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
static void itrunc(struct inode*);
// there should be one superblock per disk device, but we run with
// only one device
struct superblock sb; 

// Read the super block.
void
readsb(int dev, struct superblock *sb)
{
  struct buf *bp;

  bp = bread(dev, 1);
  memmove(sb, bp->data, sizeof(*sb));
  brelse(bp);
}

// Zero a block.
static void
bzero(int dev, int bno)
{
  struct buf *bp;

  bp = bread(dev, bno);
  memset(bp->data, 0, BSIZE);
  log_write(bp);
  brelse(bp);
}

// Blocks.

// Allocate a zeroed disk block.
static uint
balloc(uint dev)
{
  int b, bi, m;
  struct buf *bp;

  bp = 0;
  for(b = 0; b < sb.size; b += BPB){
    bp = bread(dev, BBLOCK(b, sb));
    for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
      m = 1 << (bi % 8);
      if((bp->data[bi/8] & m) == 0){  // Is block free?
        bp->data[bi/8] |= m;  // Mark block in use.
        log_write(bp);
        brelse(bp);
        bzero(dev, b + bi);
        return b + bi;
      }
    }
    brelse(bp);
  }
  panic("balloc: out of blocks");
}

// Free a disk block.
static void
bfree(int dev, uint b)
{
  struct buf *bp;
  int bi, m;

  readsb(dev, &sb);
  bp = bread(dev, BBLOCK(b, sb));
  bi = b % BPB;
  m = 1 << (bi % 8);
  if((bp->data[bi/8] & m) == 0)
    panic("freeing free block");
  bp->data[bi/8] &= ~m;
  log_write(bp);
  brelse(bp);
}

// Inodes.
//
// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at
// sb.startinode. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a cache of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The cached
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//   is non-zero. ialloc() allocates, and iput() frees if
//   the reference and link counts have fallen to zero.
//
// * Referencing in cache: an entry in the inode cache
//   is free if ip->ref is zero. Otherwise ip->ref tracks
//   the number of in-memory pointers to the entry (open
//   files and current directories). iget() finds or
//   creates a cache entry and increments its ref; iput()
//   decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//   cache entry is only correct when ip->valid is 1.
//   ilock() reads the inode from
//   the disk and sets ip->valid, while iput() clears
//   ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//   the information in an inode and its content if it
//   has first locked the inode.
//
// Thus a typical sequence is:
//   ip = iget(dev, inum)
//   ilock(ip)
//   ... examine and modify ip->xxx ...
//   iunlock(ip)
//   iput(ip)
//
// ilock() is separate from iget() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. iget() increments ip->ref so that the inode
// stays cached and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The icache.lock spin-lock protects the allocation of icache
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold icache.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

struct {
  struct spinlock lock;
  struct inode inode[NINODE];
} icache;

void
iinit(int dev)
{
  int i = 0;
  
  initlock(&icache.lock, "icache");
  for(i = 0; i < NINODE; i++) {
    initsleeplock(&icache.inode[i].lock, "inode");
  }

  readsb(dev, &sb);
  cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
 inodestart %d bmap start %d\n", sb.size, sb.nblocks,
          sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
          sb.bmapstart);
}

static struct inode* iget(uint dev, uint inum);

//PAGEBREAK!
// Allocate an inode on device dev.
// Mark it as allocated by  giving it type type.
// Returns an unlocked but allocated and referenced inode.
struct inode*
ialloc(uint dev, short type)
{
  int inum;
  struct buf *bp;
  struct dinode *dip;

  for(inum = 1; inum < sb.ninodes; inum++){
    bp = bread(dev, IBLOCK(inum, sb));
    dip = (struct dinode*)bp->data + inum%IPB;
    if(dip->type == 0){  // a free inode
      memset(dip, 0, sizeof(*dip));
      dip->type = type;

      log_write(bp);   // mark it allocated on the disk
      brelse(bp);

      return iget(dev, inum);
    }
    brelse(bp);
  }
  panic("ialloc: no inodes");
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk, since i-node cache is write-through.
// Caller must hold ip->lock.
void
iupdate(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  bp = bread(ip->dev, IBLOCK(ip->inum, sb));
  dip = (struct dinode*)bp->data + ip->inum%IPB;
  dip->type = ip->type;
  dip->major = ip->major;
  dip->minor = ip->minor;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
  dip->tag = ip->tag;
//  cprintf("%s\n", "-------iupdate函数------------");
//  cprintf("%s%d\n", "ip->tag: ", ip->tag);
//  cprintf("%s%d\n", "dip->tag: ", dip->tag);
  memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
  //memmove((uint*)dip->tag, (uint*)ip->tag, 4);
  log_write(bp);
  brelse(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode*
iget(uint dev, uint inum)
{
  struct inode *ip, *empty;

  acquire(&icache.lock);

  // Is the inode already cached?
  empty = 0;
  for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
      ip->ref++;
      release(&icache.lock);
      return ip;
    }
    if(empty == 0 && ip->ref == 0)    // Remember empty slot.
      empty = ip;
  }

  // Recycle an inode cache entry.
  if(empty == 0)
    panic("iget: no inodes");

  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;
  release(&icache.lock);

  return ip;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode*
idup(struct inode *ip)
{
  acquire(&icache.lock);
  ip->ref++;
  release(&icache.lock);
  return ip;
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void


ilock(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  if(ip == 0 || ip->ref < 1)
    panic("ilock");

  acquiresleep(&ip->lock);

  if(ip->valid == 0){
    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct dinode*)bp->data + ip->inum%IPB;
    ip->type = dip->type;
    ip->major = dip->major;
    ip->minor = dip->minor;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    ip->tag = dip->tag;   //inode和dinode之间的通信

    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
    brelse(bp);
    ip->valid = 1;
    if(ip->type == 0)
      panic("ilock: no type");
  }
}

// Unlock the given inode.
void
iunlock(struct inode *ip)
{
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    panic("iunlock");

  releasesleep(&ip->lock);
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode cache entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void
iput(struct inode *ip)
{
  //cprintf("进入到了iput函数中了, ip->valid: %d , ip->nlick: %d \n", ip->valid, ip->nlink);
  acquiresleep(&ip->lock);
  if(ip->valid && ip->nlink == 0){
    acquire(&icache.lock);
    int r = ip->ref;
    release(&icache.lock);
    if(r == 1){
      // inode has no links and no other references: truncate and free.
      //cprintf("ip->ref = 1, 所以要进行itrunc和iupdate操作\n");
      itrunc(ip);
      ip->type = 0;
      iupdate(ip);
      ip->valid = 0;
    }
  }
  releasesleep(&ip->lock);

  acquire(&icache.lock);
  ip->ref--;
  release(&icache.lock);
}

// Common idiom: unlock, then put.
void
iunlockput(struct inode *ip)
{
  iunlock(ip);
  iput(ip);
}

//PAGEBREAK!
// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;

  if(bn < NINDIRECT){
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0)
      ip->addrs[NDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }

  panic("bmap: out of range");
}

// Truncate inode (discard contents).
// Only called when the inode has no links
// to it (no directory entries referring to it)
// and has no in-memory reference to it (is
// not an open file or current directory).
static void
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;


  if(ip->tag){
    bfree(ip->dev, ip->tag); // 因为address都在这里进行了操作,并且进行了清零操作,所以,也在这里模仿
    ip->tag =0;
  }

  for(i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < NINDIRECT; j++){
      if(a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }

  ip->size = 0;
  iupdate(ip);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void
stati(struct inode *ip, struct stat *st)
{
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

//PAGEBREAK!
// Read data from inode.
// Caller must hold ip->lock.
int
readi(struct inode *ip, char *dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
      return -1;
    return devsw[ip->major].read(ip, dst, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > ip->size)
    n = ip->size - off;

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(dst, bp->data + off%BSIZE, m);
    brelse(bp);
  }
  return n;
}

// PAGEBREAK!
// Write data to inode.
// Caller must hold ip->lock.
int
writei(struct inode *ip, char *src, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
      return -1;
    return devsw[ip->major].write(ip, src, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > MAXFILE*BSIZE)
    return -1;

  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
    bp = bread(ip->dev, bmap(ip, off/BSIZE));
    m = min(n - tot, BSIZE - off%BSIZE);
    memmove(bp->data + off%BSIZE, src, m);
    log_write(bp);
    brelse(bp);
  }

  if(n > 0 && off > ip->size){
    ip->size = off;
    iupdate(ip);
  }
  return n;
}

//PAGEBREAK!
// Directories

int
namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode*
dirlookup(struct inode *dp, char *name, uint *poff)
{
  uint off, inum;
  struct dirent de;

  if(dp->type != T_DIR)
    panic("dirlookup not DIR");

  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlookup read");
    if(de.inum == 0)
      continue;
    if(namecmp(name, de.name) == 0){
      // entry matches path element
      if(poff)
        *poff = off;
      inum = de.inum;
      return iget(dp->dev, inum);
    }
  }

  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int
dirlink(struct inode *dp, char *name, uint inum)
{
  int off;
  struct dirent de;
  struct inode *ip;

  // Check that name is not present.
  if((ip = dirlookup(dp, name, 0)) != 0){
    iput(ip);
    return -1;
  }

  // Look for an empty dirent.
  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if(de.inum == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("dirlink");

  return 0;
}

//PAGEBREAK!
// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char*
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while(*path != '/' && *path != 0)
    path++;
  len = path - s;
  if(len >= DIRSIZ)
    memmove(name, s, DIRSIZ);
  else {
    memmove(name, s, len);
    name[len] = 0;
  }
  while(*path == '/')
    path++;
  return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode*
namex(char *path, int nameiparent, char *name)
{
  struct inode *ip, *next;

  if(*path == '/')
    ip = iget(ROOTDEV, ROOTINO);
  else
    ip = idup(myproc()->cwd);

  while((path = skipelem(path, name)) != 0){
    ilock(ip);
    if(ip->type != T_DIR){
      iunlockput(ip);
      return 0;
    }
    if(nameiparent && *path == '\0'){
      // Stop one level early.
      iunlock(ip);
      return ip;
    }
    if((next = dirlookup(ip, name, 0)) == 0){
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  if(nameiparent){
    iput(ip);
    return 0;
  }
  return ip;
}

struct inode*
namei(char *path)
{
  char name[DIRSIZ];
  return namex(path, 0, name);
}

struct inode*
nameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}

/*-------------------------------代码---------------------------------------*/
int
itagFile(struct inode *ip, char *key, char *value, int valuelength){

  if(ip ==0 || ip->type != T_FILE )
    return -1;      //file对应的inode不存在,或者是inode对应的type不是文件类型

  struct buf *bp;

  begin_op();
  ilock(ip);
  // 在ilock的时候,因为我在这个操作中没有用到file对应的size,ilock会进行处理的,所以,不用担心.
  //ip->size = 140*512;
  if(ip->tag == 0){
      ip->tag = balloc(ip->dev);  //这个一定不为0
  }
  bp = bread(ip->dev, ip->tag); //分配了block之后,就从内存中拿到buffer,


  struct Tag tag;
  uint off;

  //for(off =512*139; off <512*140; off += sizeof(tag)){
  for(off = 0; off<512; off+= sizeof(tag)){
//    if( readi(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag))
//      panic("tagFileread fail");
    memmove((char*)&tag, bp->data+off, sizeof(tag));  //把buffer里面的对应的偏移量放到临时变量tag中
    if(tag.used == 0)
      continue;
    if(strncmp(key, tag.key, 10) == 0){
      strncpy(tag.value, value, 18);
//      if( writei(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag) )
//        panic("tagFilewrite fail");
      memmove(bp->data+off, (char*)&tag, sizeof(tag));  //把临时变量tag中的数据写入到bp中,
      //brelse(bp); //buffer放到内存中
      log_write(bp);
      brelse(bp);

      iupdate(ip);
      //cprintf("%s%d\n", "替换的位置off: ", off);
      iunlock(ip);
      end_op();
      return 1;  // 因为已经找到了key,并且发生了替换,结束
    }
  }

  // 此时, 是没有找到这个对应的key,所以,就是要找到一个空的位置进行插入的
  //for(off =512*139; off<512*140; off+= sizeof(tag)){
  for(off = 0; off<512; off+=sizeof(tag)){
//    if( readi(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag) )
//      panic("tagFileread fail");

    memmove((char*)&tag, bp->data+off, sizeof(tag));  //把buffer里面的对应的偏移量放到临时变量tag中
    if(tag.used == 0){
      //这个字符串操作可能有点问题,
      strncpy(tag.key, key, 10);
      strncpy(tag.value, value, 18);
      tag.used = 1;
//      if( writei(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag) )
//        panic("tagFilewrite fail");
      memmove(bp->data+off, (char*)&tag, sizeof(tag));  //把临时变量tag中的数据写入到bp中,
      log_write(bp);
      brelse(bp);

      iupdate(ip);
      //cprintf("%s%d\n", "插入的位置off: ", off);
      iunlock(ip);
      end_op();
      return 1;  //标明已经完成上述操作
    }
  }

  brelse(bp);

  iupdate(ip);
  iunlock(ip);
  end_op();
  return -1;
  //可能情况就是第一个block来存储这个东西的已经满了.
}

int
iremoveFileTag(struct inode *ip, char *key){
  if(ip ==0 || ip->type != T_FILE )
    return -1;      //file对应的inode不存在,或者是inode对应的type不是文件类型


    struct buf *bp;
  begin_op();
  ilock(ip);
  // 在ilock的时候,因为我在这个操作中没有用到file对应的size,ilock会进行处理的,所以,不用担心.
//  if(ip->size != 512*140){
//    iunlock(ip);
//    end_op();
//    return -1;
//  }
    if(ip->tag == 0){
        iunlock(ip);   //都没有对应的tag对应的block都没有分配,则直接返回错误.
        end_op();
        return -1;
    }
    bp = bread(ip->dev, ip->tag);  //如果没有报错,那么就去除对应的缓存.

  struct Tag tag;
  uint off;

  //for(off =512*139; off <512*140; off += sizeof(tag)){
  for(off =0; off<512; off += sizeof(tag)){

//    if( readi(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag))
//      panic("tagFileread fail");
    memmove((char*)&tag, bp->data+off, sizeof(tag));
    if(tag.used == 0)
      continue;
    if(strncmp(key, tag.key, 10) == 0){
      //找到之后,要开展清理工作
      //tag.used = 0;
      //还是直接进行覆盖就好了
      memset(&tag, 0, sizeof(tag));
      //其实,只要对这个进行处理就好了,但是,有一个函数,就是赋值等于0的函数,拿过来比较保险的
      //strncpy(tag.value, value, valuelength);
//      if( writei(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag) )
//        panic("tagFilewrite fail");
      memmove(bp->data+off, (char*)&tag, sizeof(tag));
      log_write(bp);
      brelse(bp);

      iupdate(ip);
      //cprintf("%s%d\n", "清理的位置off: ", off);
      iunlock(ip);
      end_op();
      return 1;  // 因为已经找到了key,并且发生了替换,结束
    }
  }

  brelse(bp);

  iupdate(ip);
  iunlock(ip);
  end_op();
  return -1;
  //扫描了一遍,都没有发现有这个的key的存在的

}

int
igetFileTag(struct inode *ip, char *key, char *buffer, int length){
  if(ip ==0 || ip->type != T_FILE )
    return -1;      //file对应的inode不存在,或者是inode对应的type不是文件类型

  struct buf *bp;
  begin_op();
  ilock(ip);
  // 在ilock的时候,因为我在这个操作中没有用到file对应的size,ilock会进行处理的,所以,不用担心.

//  if(ip->size != 512*140){
//    end_op();
//    iunlock(ip);
//    return -1;
//  }
    if(ip->tag == 0){
        iunlock(ip);   //都没有对应的tag对应的block都没有分配,则直接返回错误.
        end_op();
        return -1;
    }
    bp = bread(ip->dev, ip->tag);  //如果没有报错,那么就去除对应的缓存.


  struct Tag tag;
  uint off;

  int resLength;


  //for(off =512*139; off <512*140; off += sizeof(tag)){
  for(off = 0; off<512; off += sizeof(tag)){
//    if( readi(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag))
//      panic("tagFileread fail");
    memmove((char*)&tag, bp->data+off, sizeof(tag));
    if(tag.used == 0)
      continue;
    if(strncmp(key, tag.key, 10) == 0){
      //发现了目标的key了,
      resLength = strlen(tag.value);
      resLength = min(18, resLength);

      if(length >= 18){
        strncpy(buffer, tag.value, 18);
      }else{
        strncpy(buffer, tag.value, length);
      }

      brelse(bp);
      //cprintf("%s%d\n", "查找的位置off: ", off);
      iunlock(ip);
      end_op();
      return resLength;  // 因为已经找到了key,并且发生了替换,结束
    }
  }

  brelse(bp);
  iunlock(ip);
  end_op();
  return -1;
}

int
igetAllTags(struct inode *ip, char *keys, int maxTags){
  if(ip ==0 || ip->type != T_FILE )
    return -1;      //file对应的inode不存在,或者是inode对应的type不是文件类型

    struct buf *bp;
    int numTags;
  numTags = 0;

  begin_op();
  ilock(ip);
  // 在ilock的时候,因为我在这个操作中没有用到file对应的size,ilock会进行处理的,所以,不用担心.
//  if(ip->size != 512*140){
//      iunlock(ip);
//    end_op();
//    return 0;
//  }
    if(ip->tag == 0){
        iunlock(ip);   //都没有对应的tag对应的block都没有分配,则直接返回错误.
        end_op();
        return -1;
    }
    bp = bread(ip->dev, ip->tag);  //如果没有报错,那么就去除对应的缓存.


  struct Tag tag;
  uint off;

  //for(off =512*139; off <512*140; off += sizeof(tag)){
  for(off = 0; off<512; off += sizeof(tag)){
//    if( readi(ip, (char*)&tag, off, sizeof(tag)) != sizeof(tag))
//      panic("tagFileread fail");
    memmove((char*)&tag, bp->data+off, sizeof(tag));
    if(tag.used == 0)
      continue;

    numTags++;
    //表示已经有了一个tag
    if(numTags <= maxTags){
      strncpy(keys, tag.key, 10);
      keys +=10;
    }

  }

  brelse(bp);
  iunlock(ip);
  end_op();

  return numTags;
}

int
igetFilesByTag(char *key, char *value, int valueLength, char* results, int resultsLength){

  //cprintf("%s%d\n", "valueLength: ", valueLength);
  //("%s%d\n", "resultsLength:  ", resultsLength);
  // 从'/'文件开始,往下进行递归,使用dirlookup函数,

  struct inode *ip;
  ip = iget(ROOTDEV, ROOTINO);

  int resultsOff =0;    // 在results偏移量, 所以,要传入这个地址,
  int count = 0;      // 也是要传入地址的, 并且是地址的指针count == 0;

  //cprintf("%s\n", "进入到igetFilesByTag函数中");
  //return 100;
  dirHelper(ip, (int*)&count, (int*)&resultsOff, key, value, valueLength, results, resultsLength);
  return count;
}

//void            dirHelper(struct inode*, char*, char*, int, int*, char*, int*);
void
dirHelper(struct inode *ip, int *count, int *resultsOff, char *key, char *value, int valueLength, char* results, int resultsLength ){
  uint off;
  struct dirent de;
  struct inode *nextIp;

  //cprintf("%s\n", "进入到dirHelper函数中");
  ilock(ip);

  if(ip->type != T_DIR)
    panic("dirHelper must be T_DIR");
  for( off =0; off<ip->size; off += sizeof(de) ){
    if( readi(ip, (char*)&de, off, sizeof(de)) != sizeof(de) )
      panic("read dirent fail");
    if(de.inum == 0 || strncmp(".", de.name, 1) == 0 || strncmp("..", de.name, 2) == 0)
      continue;

    nextIp = iget(ip->dev, de.inum);
    //cprintf("%s%d%s%d\n", "发现了de.inum不为0, 其de.inum的值为: ", de.inum, "其de.name为: ", de.name);
    //-----------对文件的处理---------------
    if( nextIp->type == T_FILE ){
      //如果发现为文件
      //cprintf("%s%s\n", "---进入到的文件名为: ", de.name);
      if( fileKeyValue(nextIp, key, value, valueLength) == 1 ){
        *count += 1;
        //cprintf("%s\n", "找到了一个文件, 此时count++操作");
        int len = min(DIRSIZ, strlen(de.name));
        if(*resultsOff + len +1 <= resultsLength){
          //此时能够继续往里面去写,
          //cprintf("%s%s\n","写入的文件名字为:", de.name);
          strncpy(results+(*resultsOff), de.name, len);
          strncpy(results+(*resultsOff+len), 0, 1);
          //空间足够的话,写入长度为len的文件进去的;
          *resultsOff += (len+1);
        }
      }
    }

    //----------对目录的处理------------
    if( nextIp->type == T_DIR ){
      //cprintf("%s%s\n", "---进入到的目录名为: ", de.name);
      dirHelper(nextIp, count, resultsOff, key, value, valueLength, results, resultsLength);
    }

  }
  iunlock(ip);

}

int
fileKeyValue(struct inode *ip, char *key, char *value, int valueLength){
  if(ip ==0 || ip->type != T_FILE )
    return -1;      //file对应的inode不存在,或者是inode对应的type不是文件类型

  struct buf *bp;
  begin_op();
  ilock(ip);

  if(ip->tag == 0){
    iunlock(ip);   //都没有对应的tag对应的block都没有分配,则直接返回错误.
    end_op();
    return -1;
  }
  bp = bread(ip->dev, ip->tag);  //如果没有报错,那么就去除对应的缓存.

  struct Tag tag;
  uint off;

  int resLength;

  for(off = 0; off<512; off += sizeof(tag)){
    memmove((char*)&tag, bp->data+off, sizeof(tag));
    if(tag.used == 0)
      continue;
    if(strncmp(key, tag.key, 10) == 0){
      //发现了目标的key了,
      resLength = strlen(tag.value);
      resLength = min(18, resLength);
      if(resLength == valueLength){
        //长度value长度一致,才进行比较操作的
        if( strncmp(value, tag.value, valueLength) == 0 ){
          brelse(bp);
          //cprintf("%s%d\n", "查找的位置off: ", off);
          iunlock(ip);
          end_op();
          return 1;  // 因为已经找到了key,并且发生了替换,结束
        }
      }
    }

  }

  brelse(bp);
  iunlock(ip);
  end_op();
  return -1;

}