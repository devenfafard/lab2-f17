#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

/* LAB 4 */

acquire(&(shm_table.lock));

unsigned int i = 0;
unsigned int found = 0;

for(i = 0; i < 64; i++) // Look through the page table to see if the segment id already exists.
{
  if(shm_table.shm_pages[i].id == id)
  {
    found = 1;
    break;
  }
}

if(!found) // Breaks for the first page
{
  for(i = 0; i < 64; i++)
  {
    if(shm_table.shm_pages[i].id == 0    && 
       shm_table.shm_pages[i].frame == 0 &&
       shm_table.shm_pages[i].refcnt == 0)
    {
      break;
    }
  }

  // If the segment id doesn't exist, allocate a page and store it in the table.
  char *mem;

  shm_table.shm_pages[i].id = id;
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  shm_table.shm_pages[i].frame = mem;
}

struct proc *p = myproc();

shm_table.shm_pages[i].refcnt += 1; // Increase reference count
mappages(p->pgdir, (void *)PGROUNDUP(p->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W | PTE_U); // Add mapping between the virtual address and the physical address
*pointer = (char *)PGROUNDUP(p->sz); // Return virtual address to pointer
p->sz = PGROUNDUP(p->sz) + PGSIZE; // Update the size of the process

release(&(shm_table.lock));


return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {

/* LAB 4 */
acquire(&(shm_table.lock));

unsigned int i;

for (i = 0; i < 64; i++) // If id exists, decrease the reference count. At 0, clear the table.
{
  if(shm_table.shm_pages[i].id == id)
  {
    if(shm_table.shm_pages[i].refcnt > 1)
    {
      shm_table.shm_pages[i].refcnt -= 1;
    }
    else
    {
      shm_table.shm_pages[i].id = 0;
      shm_table.shm_pages[i].frame = 0;
      shm_table.shm_pages[i].refcnt = 0;
    }
  }
}

release(&(shm_table.lock));

return 0; //added to remove compiler warning -- you should decide what to return
}
