/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char *const *args, unsigned long nargs)
{
    //kprintf("Runing program %s\n", progname);
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result,i;

    // if fails happens we can reload this and free the addrspace we create
    	//struct addrspace *old_addrspace = curthread->t_vmspace;	

    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
	
    if (result)
    {
        // reload old addrspace and activate it
        //curthread->t_vmspace = old_addrspace;
        //if (curthread->t_vmspace)
        //{
        //    as_activate(curthread->t_vmspace);
        //}
        return result;
    }


    /* We should be a new thread. */
    assert(curthread->t_vmspace == NULL);
	
    /* Create a new address space. */
    curthread->t_vmspace = as_create();
	//kprintf(" PID: %d\n", curthread->t_pid);
//	kprintf("as success!\n");
    if (curthread->t_vmspace == NULL)
    {
        vfs_close(v);
        //curthread->t_vmspace = old_addrspace;
        //if (curthread->t_vmspace)
        //{
        //    as_activate(curthread->t_vmspace);
        //}
        return ENOMEM;
    }
	
    /* Activate it. */
    as_activate(curthread->t_vmspace);
	//kprintf("as activatr success!\n");
    /* Load the executable. */
    result = load_elf(v, &entrypoint);
	
    if (result)
    {
        vfs_close(v);
        //curthread->t_vmspace = old_addrspace;
        //if (curthread->t_vmspace)
        //{
        //    as_activate(curthread->t_vmspace);
        //}
        return result;
    }

    /* Done with the file now. */
    vfs_close(v);
	
    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);

    if (result)
    {
        //curthread->t_vmspace = old_addrspace;
        //if (curthread->t_vmspace)
        //{
        //    as_activate(curthread->t_vmspace);
        //}
        return result;
    }
	
    //  create corect size of address for user arg
    vaddr_t *user_arg = kmalloc((nargs + 1) * sizeof(vaddr_t) );
	
    // set the last arg to NULL
    args[nargs] = NULL;
    // copy args to user_arg
    for( i = nargs - 1; i >= 0 ; i --) 
    {
        int arg_length = strlen(args[i]) + 1;
        stackptr -= arg_length;
        user_arg[i] = stackptr;
        copyout(args[i] , stackptr, arg_length );
    }
	
    // the stack pointer must be aligned by 4
    int offset = stackptr % 4;
    if(offset != 0) 
    	stackptr = stackptr - offset;

    //copies user_arg
    stackptr -= (nargs + 1) * sizeof(vaddr_t);
    copyout(user_arg, stackptr, (nargs + 1)*sizeof(vaddr_t));
	kprintf("Going to user mode!!!");
    /* Warp to user mode. */
    md_usermode(nargs /*nargs*/, stackptr /*userspace addr of argv*/,
                stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}

