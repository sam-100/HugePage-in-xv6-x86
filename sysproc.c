#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int 
sys_getpa(void) 
{
  int va;
  argint(0, &va);
  pde_t *pgdir = myproc()->pgdir;
  pde_t pde = pgdir[PDX(va)];
  if(pde & PTE_PS)                            // Huge page 
  {
    uint baseaddr = PTE_ADDR(pde);
    int offset = 0x3fffff;
    return (int)(baseaddr + offset);    
  }
  int offset = va & 0xfff;
  uint *pgtable = P2V(PTE_ADDR(pde));
  pte_t pte = pgtable[PTX(va)];
  int address = ((pte & 0xfffff000) + offset);
  return address;
}

int 
sys_getpagesize(void)
{
  int va;
  argint(0, &va);

  // get page dir entry
  pte_t *pgdir = myproc()->pgdir;
  pde_t pde = pgdir[PDX(va)];

  // check if pse is set
  if(pde & PTE_PS)
    return 1;

  // get page table entry
  pte_t *pgtable = P2V(PTE_ADDR(pde));
  pte_t pte = pgtable[PTX(va)];

  // check if page is present
  if(!(pte & PTE_P))
    cprintf("Page table entry is absent!\n");

  return 0;
}

void promote_page(void *va) {
    void *buffer = kalloc_huge();
    if(buffer == 0)
      return;
    buffer = (void*)V2P(buffer);
    memmove(P2V(buffer), va, HUGEPGSIZE);
    deallocate_pagetable(va);

    // inserting the address and setting pse bit on
    pde_t *pde = &myproc()->pgdir[PDX(va)];
    *pde &= 0xfff;                                      // clear old address
    *pde |= PTE_ADDR(buffer);                           // add new buffer's physical address
    *pde |= PTE_P | PTE_W | PTE_U | PTE_PS;             // set pageset bit

}

void demote_page(void *va) {
    // 2.1. If its not huge page, skip.
    pte_t *pgdir = myproc()->pgdir;
    pte_t *pde = &pgdir[PDX(va)];
    if(!(*pde & PTE_PS))
      return;

    // 2.2. allocate a page table and all of its pages.
    // 2.3. copy all data to newly allocated page table.
    pte_t *pgtable = (pte_t*)kalloc();
    memset(pgtable, 0, PGSIZE);
    for(int i=0; i<NPTENTRIES; i++)
    {
      void *buffer = (void*)kalloc();
      pgtable[i] |= (V2P(buffer));
      pgtable[i] |= PTE_P | PTE_U | PTE_W;
      memmove(buffer, va+i*PGSIZE, PGSIZE);
    }
    
    // 2.4. deallocate huge page.
    kfree_huge(P2V(PTE_ADDR(*pde)));

    // 2.5. insert va of pagetable in pde and set appropriate flags.
    *pde = 0;
    *pde |= (V2P(pgtable) & ~0xfff);
    *pde |= PTE_P | PTE_U | PTE_W;
}

int 
sys_promote(void) {
  // 1. Get arguments
  void *va;
  int size;
  argptr(0, (char **)&va, sizeof(va));
  argint(1, &size);
  void *end = va+size;

  va = (void*)HUGEPGROUNDUP((uint)va);
  for(void *ptr=va; ptr+HUGEPGSIZE < end; ptr += HUGEPGSIZE)  // iterating at huge page intervals
  {
    promote_page(ptr);
  }

  // Invalidate TLB
  lcr3(V2P(myproc()->pgdir));   
  return 0;
}


int 
sys_demote(void) {
  // 1. Get arguments
  void *va;
  int size;
  argptr(0, (char**)&va, sizeof(va));
  argint(1, &size);
  
  size -= (int)(HUGEPGROUNDUP((uint)va)-(uint)va);
  va = (void*)HUGEPGROUNDUP((uint)va);

  // 2. For each pde from va to va+size 
  for(void* ptr = va; ptr < va+size; ptr += HUGEPGSIZE)
  {
    demote_page(ptr);
  }
  // 3. return
  return 0;
}

int 
sys_huge_page_count(void) {
  void *va;
  int size;
  argptr(0, (char**)&va, sizeof(int));
  argint(1, &size);

  return huge_page_count(va, size);
}

int 
sys_get_free_pa_space(void)
{
  return kfreespace();
}