#ifndef _PROCESS_H_
#define _PROCESS_H_

#define MAX_NUM_PROCS 20

// the main idea of a process is same as a thread
// since we only have single thread process in this lab
// structure below consist of:
// a pid: refer to its parent (if it have one)
// a thread pointer point to the thread of the process
// a bool flag to indicate if this process is occupied
extern struct process parray[MAX_NUM_PROCS];
struct process
{
	pid_t parent;
	struct thread *self;
	int occupied;
	int exitcode;
	struct semaphore * exit_sem;
};

// init all the process
void process_bootstrap();
int create_process (struct thread * _thread);
void destroy_process ();

// get a valid PID and return it
// if there is no valid PID return -1
int get_pid();

// assign PID to a thread
// return -1 if failed
int assign_pid(pid_t *pid);
int find_child();

#endif 
