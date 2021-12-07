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
  int dev;
  uint64 id;
  struct logheader lh;
};
struct log log[NPARALLELLOGGING];

uint64 commit_enqueue = 0, commit_dequeue = 0;
struct spinlock commit_idx_lk;
int is_committing;
#define INDEX(x) (x%NPARALLELLOGGING)

static void recover_from_log(void);
static void commit();
static void init_log_struct(int dev, struct superblock *sb);
static void start_recovery();
static void read_all_head();
static void recover_from_logs();

void
initlog(int dev, struct superblock *sb)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log.lock, "log");
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

void set_is_committing(int val) {
	acquire(&commit_idx_lk);
	is_committing = val;
	release(&commit_idx_lk);
}

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

    minimum = min(minimum, log[i].id);
    maximum = max(maximum, log[i].id);
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
    set_enqueue(maximum);
    set_dequeue(minimum);
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

    log.start = sb->logstart + i*(sb->nlog);
    log.size = (sb->nlog)/NPARALLELLOGGING;
    log.dev = dev;
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

bool is_log_full(int idx) {
	
	acquire(&log[idx].lock);
	if(log[idx].lh.n + (log[idx].outstanding+1)*MAXOPBLOCKS > LOGSIZE/NPARALLELLOGGING) {
		release(&log[idx].lock);
		return true;
	}
	release(&log[idx].lock);
	return false;
}

// called at the start of each FS system call.
void
begin_op(void)
{
	acquire(&commit_idx_lk);
	while(1) {
		if(commit_enqueue - commit_dequeue > 4)
			panic("more than 4 log structure at a time");
		if(((commit_enqueue - commit_dequeue) == 4) && is_log_full(INDEX(enqueue))) {
			sleep(&commit_idx_lk, &commit_idx_lk);
		} else if(is_log_full(INDEX(enqueue))) {
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
	acquire(&log[INDEX(id)].lock);
	log[INDEX(id)].outstanding -= 1;
	if(log[INDEX(id)].outstanding == 0) {
		log[INDEX(id)].commit_ready = 1;
		if(id == get_commit_dequeue()) {
			commit(); //possible race condition
			log[id].commit_ready = 0;
			wakeup(&commit_idx_lk);
			release(&log[INDEX(id)].lock);
			return;
		}
		wakeup(&commit_idx_lk);
		release(&log[INDEX(id)].lock);
		
		if(is_committing) {
			return;
		}
		uint64 dq = get_commit_dequeue();
		acquire(&log[INDEX(dq)]);
		if(!log[INDEX(dq)].commit_ready) {
			release(&log[INDEX(dq)]);
			return;
		}
		release(&log[INDEX(dq)]);
		wakeup(&log[INDEX(dq)]);
		//TODO: acquire lock enq
		sleep(&log[INDEX(commit_enqueue)], &log[INDEX(commit_enqueue)].lock);
		commit();
		myproc()->fs_log_id = 0;
		//TODO: release lock enq
		increment_dequeue();
		uint64 dq = get_commit_dequeue();
		acquire(&log[INDEX(dq)]);
		if(!log[INDEX(dq)].commit_ready) {
			release(&log[INDEX(dq)]);
			return;
		}
		commit();
		release(&log[INDEX(dq)]);
	}
	
	
	/*
	 1. do we need commit lock here?
	 2. take seq no. from myproc
	 3. acquire log[seqno%4] lock
	 4. subtract outstanding
	 5.	if outstanding == 0
	 
		1. set it to commit ready
		2. if enq == deq
		   1. commit it
		   2. incrmnt deq
				wakeup commit lock
		   3. return
	 3.
			1. acquire commit lock
			2. check if is_committing global one -- commit lock
			3. acquire log lock[deque]   --
			4. check if its commit ready
			5. wakeup(log deq)
			6. sleep log enq
			   is_committing = true
			   
			7. commit it
	 wakeup commit lock
			8. deq++
			   is_committing = false
				myproc - seq id = 0
			9. check if ready
			10. wakeup(log deq)
	 wakeup commit lock
	 
			end
	
	 
	 */
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;
  if(log.committing)
    panic("log.committing");
  if(log.outstanding == 0){
    do_commit = 1;
    log.committing = 1;
  } else {
    // begin_op() may be waiting for log space,
    // and decrementing log.outstanding has decreased
    // the amount of reserved space.
    wakeup(&log);
  }
  release(&log.lock);

  if(do_commit){
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit();
    acquire(&log.lock);
    log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

// Copy modified blocks from cache to log.
static void
write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // log block
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

static void
commit()
{
  set_is_committing(1);
  if (log.lh.n > 0) {
    write_log();     // Write modified blocks from cache to log
    write_head();    // Write header to disk -- the real commit
    install_trans(0); // Now install writes to home locations
    log.lh.n = 0;
    write_head();    // Erase the transaction from the log
  }
  set_is_committing(0);
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

  acquire(&log.lock);
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // log absorbtion
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n) {  // Add new block to log?
    bpin(b);
    log.lh.n++;
  }
  release(&log.lock);
}

