#ifndef _COREMAP_H_
#define _COREMAP_H_
#include <types.h>
//#include <vm.h>
extern struct spinlock *cm_spinlock;
// The coremap class is the helper for us to 
// manage physical pages
// States a page can be in. 
typedef enum {
	P_FREE,		
	P_FIXED,	
	P_DIRTY,	
	P_CLEAN,	
} page_state_t;


//----------------Struct & Varables------------

// Structure coremap_entry is used to store the
// phisical page infomations
struct coremap_entry
{
    // where this page mapped to
    struct addrspace *as;
    vaddr_t va;
    // keep track of n pages
    unsigned long n;
    // page state 
    page_state_t state;

    // other info may used:
    
};

// since we can only know the ram size during the run time
// need to dynamiclly allocate the coremap array
// static struct coremap_entry *coremap;
// struct lock *cm_lock;
// static unsigned total_page_num;
// static unsigned cm_size; // record the coremap size
// static unsigned cm_base; // num of pages even before we init cm
// static unsigned cm_entry; // index of the first non-base page
// static unsigned cm_freepage; // index of the first free page


//----------------Functions---------------------

// initialize the coremap, calculate ramsize and pnumber
void coremap_create();

// get npages free page from memory and return
paddr_t getppages(unsigned long npages);

 
paddr_t before_getppages(unsigned long npages);

// 
void releasepages(paddr_t paddr);

#endif /*_COREMAP_H_*/
