#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <synch.h>
#include <process.h>
#include <thread.h>
#include <curthread.h>

// an array contains all process
// and a lock to implement atomic
struct process parray[MAX_NUM_PROCS];
struct lock *parray_lock = NULL;

// init all the process
void process_bootstrap()
{
    int i;
    parray_lock = lock_create("parray_lock");
	//kprintf("parry_lock created\n");
    for (i = 0; i < MAX_NUM_PROCS; i++)
    {
        parray[i].parent = 0;
        parray[i].self = NULL;
        parray[i].occupied = 0;
        parray[i].exitcode = -1;
        parray[i].exit_sem = sem_create("exit_sem", 0);
	// kprintf("process %d inited\n", i);
    }
}

// get a valid PID and return it
// if there is no valid PID return -1
int get_pid()
{
    int i;
    for (i = 1; i <= MAX_NUM_PROCS; i++)
    {
        if (parray[i - 1].occupied != 1)
        {
            parray[i - 1].occupied = 1;
            //kprintf("PID %d be used\n", i);
            return i;
        }
    }
    kprintf("NO valid PID\n");
    return -1;
}

int create_process (struct thread *_thread)
{
    lock_acquire(parray_lock);
    int result;

    // assign pid to this thread
    int children_pid;
    result = assign_pid(&children_pid);
    if (result < 0)
    {
        kprintf("bad PID \n");
        return 1;
    }
    _thread->t_pid = children_pid;

    // Child thread successfully created.
    parray[children_pid - 1].parent = curthread->t_pid;
    parray[children_pid - 1].self = _thread;
    parray[children_pid - 1].occupied = 1;

    lock_release(parray_lock);
    return 0;
}

// destory process
void destroy_process (int _exitcode)
{
    //V(parray[(int)pid - 1].exit_sem);
    lock_acquire(parray_lock);
    parray[curthread->t_pid - 1].exitcode = _exitcode;
    parray[curthread->t_pid - 1].self = NULL;
    parray[curthread->t_pid - 1].occupied = 0;
    lock_release(parray_lock);
}

// assign PID to a thread
// return -1 if failed
int assign_pid(int *pid)
{

    int _pid = get_pid();
    if (_pid < 0)
    {
        panic("used up PID\n");
        return -1;
    }
    else if (_pid > MAX_NUM_PROCS)
    {
        panic("PID overflow");
        return -1;
    }
    // assign the pid get to the pid
    *pid = _pid;
    return 0;
}

pid_t find_child()
{
	int i;
	pid_t retval;
	pid_t cur_pid = curthread->t_pid;
	for(i = 0; i < MAX_NUM_PROCS; i++){
		if(parray[i].parent == cur_pid){
			retval = parray[i].self->t_pid;
			return retval;	
		}			
	}
	kprintf("NO CHILD!");
	return -1;
}
