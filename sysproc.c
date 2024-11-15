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

static pte_t *getpte(void *va) {
  pde_t *pgdir = myproc()->pgdir;
  pde_t pde = pgdir[PDX(va)];
  uint *pgtable = P2V(PTE_ADDR(pde));
  return &pgtable[PTX(va)];
}

static int 
copy_to_pa(char *va, char *pa, int size) {
  cprintf("copy_to_pa(): va = %p, pa = %p, size = %d\n", va, pa, size);
  char *temp = (char*)kalloc();
  for(int i=0; i<size; i += PGSIZE)         // for each page size chunk
  {
    // map 'pa+i' to temp
    pte_t *pte = getpte((void*)temp);  
    *pte &= 0xfff;                      // clear previous address
    *pte |= (uint)(pa+i);               // map the physical address
    cprintf("Mapped a page\n");

    // copy the page
    for(int j=0; j<PGSIZE; j++)
    {
      cprintf("Fetching from %p\n", va+i+j);
      temp[j] = *(va+i+j);
    }
    cprintf("Written a page\n");
  }

  kfree((char*)temp);
  return 0;
}

int 
sys_promote(void) {
  // 1. Get arguments
  void *va;
  int size;
  argptr(0, (char **)&va, sizeof(va));
  argint(1, &size);
  void *end = va+size;
  cprintf("Virtual address from %p to %p\n", va, end);

  va = (void*)HUGEPGROUNDUP((uint)va);
  cprintf("Aligned Virtual address from %p to %p\n", va, end);

  cprintf("Total pages = %d\n", (int)((end-va)/(4*1024)));
  cprintf("Total Huge pages = %d\n", (int)((end-va)/(4*1024*1024)));

  pde_t *pde = &myproc()->pgdir[PDX(va)];
  *pde |= PTE_PS;

  for(void *ptr=va; ptr+HUGEPGSIZE < end; ptr += HUGEPGSIZE)  // iterating at huge page intervals
  {
    void *buffer = kalloc_huge();                
    if(buffer == 0)
    {
      cprintf("Failed to get physical address.");
      return 1;
    }
    buffer = (void*)V2P(buffer);                       // getting real physical address
    cprintf("after kalloc_huge()\n");
    
    if(!copy_to_pa(ptr, buffer, HUGEPGSIZE))
    {
      cprintf("Failed to copy to physical address %p\n", buffer);
      return 2;
    }
    cprintf("after copy\n");

    // pte_t *pgtable = P2V(PTE_ADDR(*pde));
    if(!deallocate_pagetable(va))
    {
      cprintf("Failed to deallocate page table\n");
      return 3;
    }

    // inserting the address and setting pse bit on
    pde_t *pde = &myproc()->pgdir[PDX(va)];
    *pde &= 0xfff;              // clear old address
    *pde |= PTE_ADDR(buffer);   // add new buffer's physical address
    *pde |= PTE_PS;             // set pageset bit
  }

  return 0;
}