// this function read thr data from file
// and return the count of bytes read

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <syscall.h>
#include <kern/unistd.h>

int read(int fd, void *buf, size_t buflen, int *retval)
{
	// make sure the buffer pointed to a valid space
	if(buf == NULL)
		return EFAULT;

	// read from user and save them to the file point by buf
	char i	= getch();	
	copyout(&i,(userptr_t)buf, sizeof(int ));
	
	// return the number of char read by getch()
	*retval = sizeof(char);
	return 0;
}
