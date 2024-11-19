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
  int offset = va & 0xfff;
  pde_t *pgdir = myproc()->pgdir;
  pde_t pde = pgdir[PDX(va)];
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

void enable_paging_extension() {
    uint cr4;
    asm volatile ("mov %%cr4, %0" : "=r" (cr4));
    cr4 |= CR4_PSE;
    asm volatile ("mov %0, %%cr4" :: "r" (cr4));
}

int is_paging_extension_enabled() {
    uint cr4;
    asm volatile ("mov %%cr4, %0" : "=r" (cr4));
    return (cr4 & CR4_PSE) != 0;
}

int 
sys_promote(void) {
  enable_paging_extension();
  lcr3(V2P(myproc()->pgdir));   
  if(!is_paging_extension_enabled())
  {
    cprintf("Paging is disabled.\n");
    exit();
  }

  // 1. Get arguments
  void *va;
  int size;
  argptr(0, (char **)&va, sizeof(va));
  argint(1, &size);
  void *end = va+size;


  va = (void*)HUGEPGROUNDUP((uint)va);

  for(void *ptr=va; ptr+HUGEPGSIZE < end; ptr += HUGEPGSIZE)  // iterating at huge page intervals
  {
    cprintf("curr va = %p, pdx = %d\n", ptr, PDX(ptr));
    void *buffer = (void*)V2P(kalloc_huge());
    cprintf("Address: %p\n", buffer);

    memmove(P2V(buffer), ptr, HUGEPGSIZE);
    deallocate_pagetable(ptr);
    cprintf("Deallocated 1024 pages at %p\n", ptr);

    // inserting the address and setting pse bit on
    pde_t *pde = &myproc()->pgdir[PDX(ptr)];
    *pde &= 0xfff;                                      // clear old address
    *pde |= PTE_ADDR(buffer);                           // add new buffer's physical address
    *pde |= PTE_P | PTE_W | PTE_U | PTE_PS;             // set pageset bit

    // Invalidate TLB
    lcr3(V2P(myproc()->pgdir));   
  }

  return 0;
}

void 
sys_printPDE(void)
{
  void *va;
  argptr(0, (char**)&va, sizeof(va));

  cprintf("PDE = %p\n", myproc()->pgdir[PDX(va)]);
}

int 
sys_get_free_pa_space(void)
{
  return kfreespace();
}