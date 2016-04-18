#include <coremap.h>
#include <synch.h>
#include <curthread.h>
#include <thread.h>
#include <lib.h>
#include <machine/vm.h>
#include <machine/spl.h>


#define COREMAP_TO_PADDR(i) (((paddr_t)PAGE_SIZE)*((i)+cm_base))
#define PADDR_TO_COREMAP(page)  (((page)/PAGE_SIZE) - cm_base)

// since we can only know the ram size during the run time
// need to dynamiclly allocate the coremap array
static struct coremap_entry *coremap;
struct spinlock *cm_spinlock;
static unsigned total_page_num;
static unsigned cm_size; // record the coremap size
static unsigned cm_base; // num of pages even before we init cm
static unsigned cm_entry; // index of the first non-base page
static unsigned cm_freepage; // index of the first free page

// initialize the coremap, calculate ramsize and pnumber
void coremap_create()
{
   
    // create the cm_spinlock
    cm_spinlock = spinlock_create("cm_spinlock");
    // get the total ram size in the machine
    u_int32_t paddr_start;
    u_int32_t paddr_end;
    ram_getsize(&paddr_start, &paddr_end);

    // ensure page-aligned
    assert((paddr_start & PAGE_FRAME) == paddr_start);
    assert((paddr_end & PAGE_FRAME) == paddr_end);

    // Total page number
    total_page_num = (paddr_end - paddr_start) / PAGE_SIZE;
    // size of CM itself
    cm_size = total_page_num * sizeof(struct coremap_entry);
    cm_size = ROUNDUP(cm_size, PAGE_SIZE);

    // steal pages
    coremap = (struct coremap_entry *)PADDR_TO_KVADDR(paddr_start);
    paddr_start = paddr_start + cm_size;
    // ensure core map not oversize
    assert(paddr_start < paddr_end);

    cm_base = paddr_start / PAGE_SIZE;
    cm_entry = (paddr_end / PAGE_SIZE) - cm_base;
    cm_freepage = cm_entry;

    assert(cm_entry + (cm_size / PAGE_SIZE) == total_page_num);

    unsigned i;
    // initialize coremap entries
    for (i = 0; i < cm_entry; i++)
    {
        coremap[i].as = NULL;
        coremap[i].va = PADDR_TO_KVADDR(COREMAP_TO_PADDR(i));
        coremap[i].state = P_FREE;
    } 
    //kprintf("coremap created!\n free pages: %d\n", cm_freepage);
}

// get npages free page from memory and return
paddr_t getppages(unsigned long npages)
{
    // iterate to find the first available free pages
    if(coremap == NULL)
    {
	//kprintf("coremap not set up yet, stealing memory...\n");
	return before_getppages(npages);
    }
    spinlock_acquire(cm_spinlock);
    //kprintf("coremap set up, finding free pages from coremap...\n");
    paddr_t pa;
    int index = -1;
    unsigned i;
    if (cm_freepage > 0)
    {
        for (i = 0; i < cm_entry; i++)
        {
            if (coremap[i].state == P_FREE)
            {
                index = i;
                break;
            }
        }
    }else
	return 0;
    coremap[i].state = P_DIRTY;
    coremap[i].as = curthread->t_vmspace;
    coremap[i].n = npages;
    
    unsigned page_need = i + npages;
    // make sure next npages are available as well
    for (i = i + 1; i < page_need; i++)
    {
        /* check if they are occupied */
        if (coremap[i].state != P_FREE)
        {
            /* should swap/do sth here */
            panic("page been occupied \n");
        }
	coremap[i].state = P_DIRTY;
	coremap[i].as = curthread->t_vmspace;
    }

    // update number of free pages
    cm_freepage -= npages;
    spinlock_release(cm_spinlock);
    pa = COREMAP_TO_PADDR(index);
    //kprintf("free pages returned...\n");
    //kprintf("Remaining free pages %d\n", pa);
    return pa;
}

paddr_t
before_getppages(unsigned long npages)
{
    paddr_t addr;
    int spl = splhigh();
    addr = ram_stealmem(npages);
    splx(spl);

    return addr;	
}

// realse the pages 
void releasepages(paddr_t paddr)
{
	unsigned index = PADDR_TO_COREMAP(paddr);
	unsigned long npages = coremap[index].n;
	coremap[index].n = 0;
	unsigned i;
	for(i = 0; i < npages; i++){
		coremap[index + i].state = P_FREE;	
	}
}
