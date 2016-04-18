#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <curthread.h>
#include <thread.h>
#include <process.h>
#include <synch.h>
#include <addrspace.h>
#include <array.h>
#include <vm.h>
#include <array.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

// to test the marker - 2016/03/20
void
mips_syscall(struct trapframe *tf)
{
    int callno;
    int32_t retval;
    int err;

    assert(curspl == 0);

    callno = tf->tf_v0;

    /*
     * Initialize retval to 0. Many of the system calls don't
     * really return a value, just 0 for success and -1 on
     * error. Since retval is the value returned on success,
     * initialize it to 0 by default; thus it's not necessary to
     * deal with it except for calls that return other values,
     * like write.
     */

    retval = 0;

    switch (callno)
    {
    case SYS_reboot:
        err = sys_reboot(tf->tf_a0);
        break;
    case SYS_fork:
        err = sys_fork(tf, &retval);
        if(err < 0)
        {
            err = retval;
        }
        break;
    case SYS_write:
        err = write(tf->tf_a0, (const void *)tf->tf_a1, tf->tf_a2);
        break;
    case SYS_read:
        err = read(tf->tf_a0, (void *)tf->tf_a1, tf->tf_a2, &retval);
        // retval = 1;
        break;
    case SYS_getpid:
        retval = getpid();
        err = 0;
        break;
    case SYS__exit:
        _exit(tf->tf_a0);
        break;

    case SYS_waitpid:
        err = waitpid(tf->tf_a0, (int *)tf->tf_a1, tf->tf_a2, &retval);
        if(err < 0)
        {
            err = retval;
        }
        else
        {
            retval = err;
            err = 0;
        }
        break;
    case SYS_sbrk:
	err = sys_sbrk((userptr_t)tf->tf_a0, &retval);
	break;


    /* Add stuff here */

    default:
        kprintf("Unknown syscall %d\n", callno);
        err = ENOSYS;
        break;
    }


    if (err)
    {
        /*
         * Return the error code. This gets converted at
         * userlevel to a return value of -1 and the error
         * code in errno.
         */
        tf->tf_v0 = err;
        tf->tf_a3 = 1;      /* signal an error */
    }
    else
    {
        /* Success. */
        tf->tf_v0 = retval;
        tf->tf_a3 = 0;      /* signal no error */
    }

    /*
     * Now, advance the program counter, to avoid restarting
     * the syscall over and over again.
     */

    tf->tf_epc += 4;

    /* Make sure the syscall code didn't forget to lower spl */
    assert(curspl == 0);
}

int sys_fork(struct trapframe *tf, int *retval)
{
    struct thread *child_thread;
    struct trapframe *child_trapframe = kmalloc(sizeof(struct trapframe));
    struct addrspace *child_space;
    struct addrspace *parent_space;

    if (child_trapframe == NULL)
    {
        kprintf("Out of memory, ENoMEM\n");
        // No memory to allocate for child's trapframe, return error.
        *retval = ENOMEM;
        return -1;
    }

    memcpy(child_trapframe, tf, sizeof(struct trapframe));

    // get the parent address space.
    parent_space = curthread->t_vmspace;

    assert(parent_space != NULL);

    int copy_error = as_copy(parent_space, &child_space);
    if(copy_error != 0)
    {
        // As written in dumbvm.c, as_copy will return either ENOMEM for error or 0 for success
        *retval = ENOMEM;
        //kprintf("Address space Out of Memory, ENoMEM\n");
        kfree(child_trapframe);
        return -1;
    }


    int thread_fork_error = thread_fork(curthread->t_name, child_trapframe, ( unsigned long )child_space, md_forkentry, &child_thread);

    if (thread_fork_error != 0)
    {
        *retval = ENOMEM;
        //kprintf("Thread_fork out of memory, ENoMEM \n");
        kfree(child_trapframe);
        return -1;
    }


    *retval = child_thread -> t_pid;
    return 0;
}

void
md_forkentry(struct trapframe *tf, unsigned long child_addr)
{
   /*
		this function is basically for the child thread, remember the threadfor in sys_fork, it calls the md_forkentry  and passed in new TF and addressspace and the child_thread.
	 */
	//kprintf("just enter forkentry\n");
	struct trapframe* modifiy_tf;
	struct trapframe dumbvalue;
	struct addrspace* new_addr = (struct addrspace*) child_addr;
	modifiy_tf= kmalloc(sizeof(struct trapframe));
	//kprintf("just enter memcpy modified tf and tf\n");
	memcpy(modifiy_tf, tf, sizeof(struct trapframe));
	//kprintf("just after memcpy modified tf and tf\n");
	//*modifiy_tf = *tf;

	modifiy_tf->tf_v0 = 0;
	modifiy_tf->tf_a3 = 0;
	modifiy_tf->tf_epc +=4;
	curthread->t_vmspace = new_addr;
	//activie the addressspace
	//kprintf("activate thread -- pid: %d\n", curthread->t_pid);
	as_activate(curthread->t_vmspace);
	//memcpy(dumbvalue, tf, sizeof(struct trapframe));
	//kprintf("just enter dumbvalue =*tf\n");
	dumbvalue = *modifiy_tf;
	//kprintf("just leave dumbvalue =*tf\n");
	mips_usermode(&dumbvalue);
}

pid_t
getpid()
{

    return curthread->t_pid;
}

void
_exit(int _exitcode)
{
    //kprintf("PID %d exitcode %d\n", curthread->t_pid, _exitcode);
    // lock_acquire(parray_lock);
    //    parray[curthread->t_pid].exitcode = _exitcode;
    //    parray[curthread->t_pid].self = NULL;
    //    parray[curthread->t_pid].occupied = 0;
    //    lock_release(parray_lock);
    destroy_process(_exitcode);
    //kprintf("PID %d done exit\n", curthread->t_pid);
    thread_exit();
}

pid_t waitpid(pid_t pid, int *status, int options, int *retval)
{
    // make sure the option is 0
    if(options != 0)
    {
        kprintf("option is not 0 \n");
        *retval = EINVAL;
        return -1;
    }

    // the exit code is a invlid pointer
    if(status == NULL)
    {
        kprintf("invalid exit code \n");
        *retval = EFAULT;
        return -1;
    }

    // the child already exit
    if(parray[(int)pid - 1].occupied == 0)
    {
        *retval = 0;
        *status = parray[(int)pid - 1].exitcode;
        //kprintf("thread %d already exit\n", pid);
        return pid;
    }

    // check if we are the parent of that pid
    if(parray[(int)pid - 1].parent != curthread->t_pid)
    {
        kprintf("PID %d Not parent!\n", pid);
        *retval = EINVAL;
        return -1;
    }

    // P to exit semaphore to make sure the child process exit
    //kprintf("waiting pid %d\n", pid);
    //kprintf("this thread is %d\n",parray[(int)pid - 1].occupied);
    P(parray[(int)pid - 1].exit_sem);

    *status = parray[(int)pid - 1].exitcode;
    //destroy_process(pid);
    //V(parray[(int)pid - 1].exit_sem);
    //kprintf("SUCCESS wait pid %d\n", pid);
    *retval = 0;

    return pid;
}

int sys_sbrk(intptr_t amount, vaddr_t *retval)
{
    
    vaddr_t size = (vaddr_t)amount;
    struct addrspace *addrsp;
    vaddr_t heapend;
    size_t pages;
    u_int32_t i;
    
    //save the current heapend
    addrsp = curthread->t_vmspace;        
    heapend = addrsp->as_heapend;
    kprintf("Entered sys_sbrk, trying to allocate %x, heapend: %x\n", size, heapend);

    //sanity checks, if size is zero return the heapend
    if(size==0)
    {
        *retval = heapend;
	//kprintf("current heapend: %x\n", *retval);
        return 0;
    }
    //size can't be less than zero

    if (size%PAGE_SIZE)
    	pages = (size/PAGE_SIZE)+1;
    else
	pages = (size/PAGE_SIZE);
	
	//kprintf("need %d pages, PAGESIZE = %x", pages, PAGE_SIZE);
	if (pages > HEAPPAGES)
		return ENOMEM;
    //allocated size can't exceed the heap limit (i.e. can't fall outside its stackspace)
    if( (addrsp->as_heapend+(pages*PAGE_SIZE)) > (USERSTACK+(VM_STACKPAGES*PAGE_SIZE)) ) 
    {
	//kprintf("Out of heap limit!\n");
        *retval = -1;
        return EINVAL;
    }

    vaddr_t vaddr = ROUNDUP(heapend, PAGE_SIZE);
	//kprintf("ROUNDUP heapend: %x\n", heapend);
    //allocate no_of_pages pages by calling each using allocate_page to allocate
    //single page. Allocation of each page followed by adding page mapping of 
    //vaddr to paddr in the page table. alloc_page() will be responsible to call
    //for this mapping. Note that the virtual address space is contagious but 
    //physical pages are not necessarily contagious
    struct pagetableentry* pte;
    for(i=0; i < pages; i++)
    {
	//kprintf("allocate No. %d page\n", i);
        pte = kmalloc(sizeof(struct pagetableentry));
	
	pte->vaddr = PAGE_SIZE *i +vaddr; // the PAGE_SIZE  was defined in VM(machine dependent)
	pte->valid = 1;
	/* values for p_flags */
	//#define	PF_R		0x4	/* Segment is readable */
	//#define	PF_W		0x2	/* Segment is writable */
	//#define	PF_X		0x1	/* Segment is executable */

	//pte->status = readable | writeable | executable ;//
	//kprintf("Add vaddr %x into page table\n", pte->vaddr);
	array_add(addrsp->pagetable, pte);
    }
    
    //increment the heapend by the size of the total allocation
    addrsp->as_heapend = addrsp->as_heapend+size;
	//kprintf("new heapend = %x", addrsp->as_heapend);
    
    //return the heapend
    *retval = heapend;
	//kprintf("retval = %x\n", heapend);
    return 0;
} 

int sys_sbrk(intptr_t amount, vaddr_t *retval)
{
    vaddr_t size = (vaddr_t)amount;
    struct addrspace *addrsp;
    vaddr_t heapend;
    size_t pages;
    u_int32_t i;
    
    //save the current heapend
    addrsp = curthread->t_vmspace;        
    heapend = addrsp->as_heapend;
    
    //sanity checks, if size is zero return the heapend
    if(size==0)
    {
        *retval = heapend;
        return 0;
    }
    //size can't be less than zero

    if (size%PAGE_SIZE)
    	pages = (size/PAGE_SIZE)+1;
    else
	pages = (size/PAGE_SIZE);

    //allocated size can't exceed the heap limit (i.e. can't fall outside its stackspace)
    if( (addrsp->as_heapend+(pages*PAGE_SIZE)) > (USERSTACK+(VM_STACKPAGES*PAGE_SIZE)) ) 
    {
        *retval = -1;
        return EINVAL;
    }

    vaddr_t vaddr = ROUNDUP(heapend, PAGE_SIZE);
    //allocate no_of_pages pages by calling each using allocate_page to allocate
    //single page. Allocation of each page followed by adding page mapping of 
    //vaddr to paddr in the page table. alloc_page() will be responsible to call
    //for this mapping. Note that the virtual address space is contagious but 
    //physical pages are not necessarily contagious
    struct pagetableentry* pte;
    for(i=0; i < pages; i++)
    {
        pte = kmalloc(sizeof(struct pagetableentry));
	
	pte->vaddr = PAGE_SIZE *i +vaddr; // the PAGE_SIZE  was defined in VM(machine dependent)
	pte->valid = 1;
	/* values for p_flags */
	//#define	PF_R		0x4	/* Segment is readable */
	//#define	PF_W		0x2	/* Segment is writable */
	//#define	PF_X		0x1	/* Segment is executable */

	//pte->status = readable | writeable | executable ;//
	//kprintf("Add vaddr %x into page table\n", pte->vaddr);
	array_add(addrsp->pagetable, pte);
    }
    
    //increment the heapend by the size of the total allocation
    addrsp->as_heapend = addrsp->as_heapend+size;
    
    //return the heapend
    *retval = heapend;

    return 0;
} 
