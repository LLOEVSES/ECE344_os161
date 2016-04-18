#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_fork(struct trapframe *tf, int* retval);
int write(int filehandle, const void *buf, size_t size);
int read(int filehandle, void *buf, size_t size, int* retval);
pid_t getpid();
void _exit(int _exitcode);
pid_t waitpid(pid_t pid, int * status, int options, int * retval);
int sys_sbrk(intptr_t amount, vaddr_t *retval);
int sys_sbrk(intptr_t amount, vaddr_t *retval);

//=========================================================

#endif /* _SYSCALL_H_ */
