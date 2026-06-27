// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13

struct {
  
  struct buf buf[NBUF];
  struct spinlock lock[NBUCKET];
  struct spinlock eviction_lock;
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

static int
bhash(uint dev, uint blockno)
{
  return (dev ^ blockno) % NBUCKET;
}

static void
binsert(int bucket, struct buf *b)
{
  b->next = bcache.head[bucket].next;
  b->prev = &bcache.head[bucket];
  bcache.head[bucket].next->prev = b;
  bcache.head[bucket].next = b;
}

static void
bremove(struct buf *b)
{
  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = 0;
  b->prev = 0;
}

void
binit(void)
{
  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.lock[i], "bcache");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

 initlock(&bcache.eviction_lock, "bcache_eviction");

  for(int i = 0; i < NBUF; i++){
    struct buf *b = &bcache.buf[i];

    b->dev = -1;
    b->blockno = 0;
    b->valid = 0;
    b->refcnt = 0;

    initsleeplock(&b->lock, "buffer");
    binsert(i % NBUCKET, b);
  }
}
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket = bhash(dev, blockno);

  acquire(&bcache.lock[bucket]);
  for(b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[bucket]);

  acquire(&bcache.eviction_lock);

  acquire(&bcache.lock[bucket]);
  for(b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucket]);
      release(&bcache.eviction_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[bucket]);

  for(int i = 0; i < NBUCKET; i++){
    acquire(&bcache.lock[i]);

    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0){
        bremove(b);
        release(&bcache.lock[i]);

        acquire(&bcache.lock[bucket]);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        binsert(bucket, b);
        release(&bcache.lock[bucket]);

        release(&bcache.eviction_lock);
        acquiresleep(&b->lock);
        return b;
      }
    }

    release(&bcache.lock[i]);
  }

  release(&bcache.eviction_lock);
  panic("bget: no buffers");
}

struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid){
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket = bhash(b->dev, b->blockno);

  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  if(b->refcnt == 0){
    bremove(b);
    binsert(bucket, b);
  }
  release(&bcache.lock[bucket]);
}

void
bpin(struct buf *b)
{
  int bucket = bhash(b->dev, b->blockno);

  acquire(&bcache.lock[bucket]);
  b->refcnt++;
  release(&bcache.lock[bucket]);
}

void
bunpin(struct buf *b)
{
  int bucket = bhash(b->dev, b->blockno);

  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}
