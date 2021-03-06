#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <array.h>
#include <machine/tlb.h>
#include <machine/spl.h>
#include <coremap.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */


struct addrspace *
as_create(void)
{
	//kprintf("as creating...\n");
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		kprintf("as NULL..\n");
		return NULL;
	}

	/*
	 * Initialize as needed.initilize those variavbles 
	 */
	as->as_vbase1=0;
	as->as_pbase1=0;
	as->as_npages1=0;
	as->as_vbase2=0;
	as->as_pbase2=0;
	as->as_npages2=0;
	as->as_stackpbase=0;
	as->as_heapstart=0;
	as->as_heapend=0;
	as->as_heappbase=0;
	as->as_heapstart = 0;
	as->as_heapend = 0;
	as->pagetable = array_create();
	//kprintf("Created addr space!\n");
	return as;
}
/*
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	
	(void)old;
	
	*ret = newas;
	return 0;
}
*/
void
as_destroy(struct addrspace *as)
{

	kprintf("enter as destroy!\n");
	array_destroy(as->pagetable);
	kfree(as);
	kprintf("leave as destroy!\n");
}


static struct addrspace *lastas = NULL;
void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	int spl;
	size_t npages; 
	spl = splhigh();
	//kprintf("the passed in virtual memory  %x \n",vaddr);
	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
	//kprintf("aftering masking with PAGE_SIZE the passed in virtual memory  %x \n",vaddr);
	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;
	//kprintf("sz %d\n ",sz);
	struct pagetableentry* pte;
	
	//kprintf("number of npages %d\n",npages);
	//kprintf("The passed in varrd in as deine region %x\n",vaddr );
	/* We don't use these - all pages are read-write */
	//(void)readable;
	//(void)writeable;
	//(void)executable;
	unsigned i ;
	for( i = 0; i < npages; i++)
	{
		pte = kmalloc(sizeof(struct pagetableentry));

	
		pte->vaddr = PAGE_SIZE *i +vaddr; // the PAGE_SIZE  was defined in VM(machine dependent)
		
		//kprintf("address in as define region%x\n",pte->vaddr);
		// pte->paddr = getppage(1);
		pte->valid = 0;
	/* values for p_flags */
	//#define	PF_R		0x4	/* Segment is readable */
	//#define	PF_W		0x2	/* Segment is writable */
	//#define	PF_X		0x1	/* Segment is executable */

		pte->status = readable | writeable | executable ;//
		array_add(as->pagetable, pte);
	}
	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		//kprintf("returning from as defined region Vbase1 ==0\n");
		splx(spl);
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		as->as_heapstart = vaddr + PAGE_SIZE*npages;
		as->as_heapend = as->as_heapstart;
		//kprintf("returning from as defined region Vbase2 ==0\n");
		splx(spl);
		as->as_heapstart = vaddr + PAGE_SIZE*npages;
		as->as_heapend = as->as_heapstart;
		kprintf("init heapstart, start at %x\n", as->as_heapstart);
		return 0;
		
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	splx(spl);
	return EUNIMP;
	
}

int
as_prepare_load(struct addrspace *as)
{
	
	assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);
	assert(as->as_heappbase == 0);

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		//kprintf("as->as_pbase1 == 0\n");
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(VM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
	
	as->as_heappbase = getppages(HEAPPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	//kprintf("Entering as_define_stack\n");
	//assert(as->as_stackpbase != 0);
	// the USERSTAACK is the top of the stack, and stack grows downward.
	struct pagetableentry* pte;
	unsigned  i; // for c 99
	for(i = 0; i<VM_STACKPAGES; i++) // VM stackpages are defined in vm.h
	{
		pte = kmalloc(sizeof(struct pagetableentry));
		pte->vaddr = USERSTACK- i*PAGE_SIZE;
		pte->valid = 0;
		pte->status = 0x7; // 111 at the end means read write and ex
		//kprintf("beforeentring array_added\n");
		array_add(as->pagetable, pte);
	}

	//kprintf("just came out from for loop in as stack\n");
	*stackptr = USERSTACK; 
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	kprintf("CALL AS COPY\n");
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
	new->as_heapstart = old->as_heapstart;
	new->as_heapend = old->as_heapend;

	if (as_prepare_load(new)) {
		//kprintf("as_load fail\n");
		as_destroy(new);
		return ENOMEM;
	}

	assert(new->as_pbase1 != 0);
	assert(new->as_pbase2 != 0);
	assert(new->as_stackpbase != 0);
	assert(new->as_heappbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		VM_STACKPAGES*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_heappbase),
		(const void *)PADDR_TO_KVADDR(old->as_heappbase),
		HEAPPAGES*PAGE_SIZE);
	
	*ret = new;
	return 0;
}


