#include <types.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <lib.h>

ssize_t sys_read(int filehandle, void *buf, size_t size)
{
    size_t pos = 0;
    int ch;
    char *char_buf = (char *)buf;

    if (filehandle != STDIN_FILENO) {
        kprintf("sys_read supported only from stdin\n");
        return -1;
    }

    for (pos=0; pos<size; pos++) {
        ch = getch();

        if (ch < 0)
            return pos;

        char_buf[pos] = ch;
    }

    return size;
}

ssize_t sys_write(int filehandle, const void *buf, size_t size)
{
    size_t i;
    const char *char_buf = (const char *)buf;
    
    if (filehandle != STDOUT_FILENO && fd != STDERR_FILENO) {
        kprintf("sys_write supported only to stdout\n");
        return -1;
    }

    for (i=0; i<size; i++) {
        putch(char_buf[i]);
    }

    return size;
}