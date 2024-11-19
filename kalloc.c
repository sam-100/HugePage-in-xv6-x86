// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

int is_aligned(uint ptr, uint offset) {
  return (ptr%offset == 0);
}

char*
kalloc_huge(void)
{
  if(kmem.use_lock)
    acquire(&kmem.lock);

  // search for consecutive pages
  struct run *curr = kmem.freelist, *prev = 0, *start = 0;
  while(curr)
  {
    // check alignment
    if(!is_aligned((uint)curr+PGSIZE, HUGEPGSIZE))
    {
      prev=curr;
      curr = curr->next;
      continue;
    }
    
    // search ... 
    int count = HUGEPGSIZE/PGSIZE-1;
    struct run *ptr = curr;
    while(ptr && count-- && ((uint)ptr - (uint)ptr->next == PGSIZE))
      ptr = ptr->next;

    // if found, set start and break
    if(count == -1)
    {
      start=ptr;
      break;
    }

    // update curr pointer
    prev=curr;
    curr = ptr->next;    
  }

  // if not found, exit
  if(!curr || !start)
  {
    cprintf("kalloc() failed!\n");
    if(kmem.use_lock)
      release(&kmem.lock);
    exit();
  }

  // allocate
  if(prev == 0)
    kmem.freelist = start->next;
  else
    prev->next = start->next;

  // clean all pages
  memset((void*)start, 0, HUGEPGSIZE);

  // return starting address
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)start;
}


int kfreespace(void) {
  if(kmem.use_lock)
    acquire(&kmem.lock);
  
  struct run *ptr = kmem.freelist;
  int cnt = 0;
  while(ptr)
  {
    cnt++;
    ptr=ptr->next;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
  return cnt;
}