#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  int n;
  int block[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // how many FS sys calls are executing.
  int commit_ready; //ready to commit
  int committing;
  int dev;
  uint64 id;
  struct logheader lh;
};
struct log log[NPARALLELLOGGING];

uint64 commit_enqueue = 0, commit_dequeue = 0;
struct spinlock commit_idx_lk;
//int is_committing;
#define INDEX(x) ((x) % NPARALLELLOGGING)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void recover_from_log(int idx);
static void commit();
static void init_log_struct(int dev, struct superblock *sb);
static void start_recovery();
static void read_all_head();
static void recover_from_logs();
static void read_head(int log_idx);
static void recover_from_log(int idx);

void
initlog(int dev, struct superblock *sb)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log->lock, "log");
  initlock(&commit_idx_lk, "commit index lock");
  init_log_struct(dev, sb);
  start_recovery();
}

//Utility function for enqueue and dequeue

void increment_enqueue() {
  acquire(&commit_idx_lk);
  commit_enqueue++;
  release(&commit_idx_lk);
}

void increment_dequeue() {
  acquire(&commit_idx_lk);
  commit_dequeue++;
  release(&commit_idx_lk);
}

void set_enqueue(uint64 val) {

  acquire(&commit_idx_lk);
  commit_enqueue = val;
  release(&commit_idx_lk);
}

void set_dequeue(uint64 val) {

  acquire(&commit_idx_lk);
  commit_dequeue = val;
  release(&commit_idx_lk);
}

//void set_is_committing(int val) {
//	acquire(&commit_idx_lk);
//	is_committing = val;
//	release(&commit_idx_lk);
//}

uint64 get_commit_dequeue() {
	uint64 val;
	acquire(&commit_idx_lk);
	val = commit_dequeue;
	release(&commit_idx_lk);
	return val;
}

//--

void get_min_max(int* arr) {

  int minimum = 0, maximum = 0;
  for(int i=0; i<NPARALLELLOGGING;i++) {
    if(log[i].id == 0) {
      continue;
    }
    if(minimum == 0) {
      minimum = log[i].id;
    }

    minimum = MIN(minimum, log[i].id);
    maximum = MAX(maximum, log[i].id);
  }
  arr[0] = minimum;
  arr[1] = maximum;
}

void start_recovery() {

  read_all_head();
  int arr[2];
  get_min_max(arr);
  if(arr[0] == 0) {
    set_enqueue(1);
    set_dequeue(1);
  } else {
    set_enqueue(arr[1]);
    set_dequeue(arr[0]);
  }
  recover_from_logs();
}

static void recover_from_logs() {

  //dequeue <= enqueue
  while(commit_dequeue <= commit_enqueue) {
    recover_from_log();
  }
}

static void read_all_head() {

  for(int i=0; i<NPARALLELLOGGING;i++) {
    read_head(i);
  }
}

static void init_log_struct(int  dev, struct superblock *sb) {

  for(int i=0; i<NPARALLELLOGGING;i++) {

    log[i].start = sb->logstart + i*(sb->nlog);
    log[i].size = (sb->nlog);
    log[i].dev = dev;
  }  
}


// Copy committed blocks from log to their home location
static void
install_trans(int recovering, int idx)
{
  int tail;

  for (tail = 0; tail < log[idx].lh.n; tail++) {
    struct buf *lbuf = bread(log[idx].dev, log[idx].start+tail+1); // read log block
    struct buf *dbuf = bread(log[idx].dev, log[idx].lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    if(recovering == 0)
      bunpin(dbuf);
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(int log_idx)
{
  struct buf *buf = bread(log[log_idx].dev, log[log_idx].start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log[log_idx].lh.n = lh->n;
  for (i = 0; i < log[log_idx].lh.n; i++) {
    log[log_idx].lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(int idx)
{
  struct buf *buf = bread(log[idx].dev, log[idx].start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log[idx].lh.n;
  for (i = 0; i < log[idx].lh.n; i++) {
    hb->block[i] = log[idx].lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(int idx)
{
  install_trans(idx, 1); // if committed, copy from log to disk
  log[idx].lh.n = 0;
  write_head(idx); // clear the log
}

int is_log_full(int idx) {
	
	acquire(&log[idx].lock);
	if(log[idx].lh.n + (log[idx].outstanding+1)*MAXOPBLOCKS > LOGSIZE) {
		release(&log[idx].lock);
		return 1;
	}
	release(&log[idx].lock);
	return 0;
}

//has commit lock already acquired
int commit_ready(int idx) {
	
	acquire(&log[idx].lock);
	int val = log[idx].commit_ready;
	release(&log[idx].lock);
	return val;
}

// called at the start of each FS system call.
void
begin_op(void)
{
	acquire(&commit_idx_lk);
	while(1) {
		if(commit_enqueue - commit_dequeue > 4)
			panic("more than 4 log structure at a time");
		if(((commit_enqueue - commit_dequeue) == 4) && is_log_full(INDEX(commit_enqueue))) {
			sleep(&commit_idx_lk, &commit_idx_lk);
		} else if(commit_ready(INDEX(commit_enqueue))) {
			commit_enqueue++;
		} else if(is_log_full(INDEX(commit_enqueue))) {
			commit_enqueue++;
			break;
		}
	}
	myproc()->fs_log_id = commit_enqueue;
	release(&commit_idx_lk);
	
	acquire(&log[myproc()->fs_log_id].lock);
	log[INDEX(myproc()->fs_log_id)] += 1;
	release(&log[myproc()->fs_log_id].lock);
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
	uint64 id = myproc()->fs_log_id;
	myproc()->fs_log_id = 0;
	acquire(&log[INDEX(id)].lock);
	log[INDEX(id)].outstanding -= 1;
	if(log[INDEX(id)].outstanding != 0) { //not last end_op
		wakeup(&commit_idx_lk);
		release(&log[INDEX(id)].lock);
		return;
	}
	log[INDEX(id)].commit_ready = 1;
	if(id == commit_dequeue) {  //TODO: lock?
		commit();
		wakeup(&commit_idx_lk);
		release(&log[INDEX(id)].lock);
		increment_dequeue();
		return;
	}
	wakeup(&commit_idx_lk);
	sleep(&log[INDEX(id)], &log[INDEX(id)].lock);
	commit();
	wakeup(&commit_idx_lk);
	release(&log[INDEX(id)].lock);
	increment_dequeue();
	
	uint64 dq = get_commit_dequeue();
	acquire(&log[INDEX(dq)].lock);
	if(!log[INDEX(dq)].commit_ready) {
		release(&log[INDEX(dq)].lock);
		return;
	}
	wakeup(&log[INDEX(dq)]);
	release(&log[INDEX(dq)].lock);
}

// Copy modified blocks from cache to log.
static void
write_log(int idx)
{
  int tail;

  for (tail = 0; tail < log[idx].lh.n; tail++) {
    struct buf *to = bread(log[idx].dev, log[idx].start+tail+1); // log block
    struct buf *from = bread(log[idx].dev, log[idx].lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

static void
commit(int idx)
{
  if(log[INDEX(idx)].committing) {
	return;  // no need to return status since some other process is handling it
  }
  log[INDEX(idx)].committing = 1;
  if (log[idx].lh.n > 0) {
    write_log(idx);     // Write modified blocks from cache to log
    write_head(idx);    // Write header to disk -- the real commit
    install_trans(0, idx); // Now install writes to home locations
    log[idx].lh.n = 0;
    write_head(idx);    // Erase the transaction from the log
  }
  log[INDEX(id)].committing = 0;
  log[INDEX(id)].commit_ready = 0;
  return;
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache by increasing refcnt.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;
  uint64 id = myproc()->fs_log_id;
  acquire(&log[INDEX(id)].lock);
  if (log[INDEX(id)].lh.n >= LOGSIZE || log[INDEX(id)].lh.n >= log[INDEX(id)].size - 1)
    panic("too big a transaction");
  if (log[INDEX(id)].outstanding < 1)
    panic("log_write outside of trans");

  for (i = 0; i < log[INDEX(id)].lh.n; i++) {
    if (log[INDEX(id)].lh.block[i] == b->blockno)   // log absorbtion
      break;
  }
  log[INDEX(id)].lh.block[i] = b->blockno;
  if (i == log[INDEX(id)].lh.n) {  // Add new block to log?
    bpin(b);
    log[INDEX(id)].lh.n++;
  }
  release(&log[INDEX(id)].lock);
}

