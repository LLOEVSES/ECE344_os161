// This function is used to perform the printing
// and return the number of byte written

#include <kern/unistd.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>


int write(int filehandle, const void *buf, size_t size)
{
    // check if the address space pointed by buf is not valid
    if (buf == NULL)
    {
        return EFAULT;
    }
    else
        return kprintf("%c", *(const char *)buf);
}
