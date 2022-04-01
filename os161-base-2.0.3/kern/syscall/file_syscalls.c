#include <types.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <lib.h>

ssize_t sys_read(int filehandle, void *buf, size_t size)
{
    size_t pos = 0;
    int ch;
    char *char_buf = (char *)buf;

    int s = filehandle;
    s++;

    for (pos=0; pos<size; pos++) {
        ch = getch();
        // TODO: add processing on the character read
        char_buf[pos] = ch;
    }

    return size;
}

ssize_t sys_write(int filehandle, const void *buf, size_t size)
{
    size_t i;
    const char *char_buf = (const char *)buf;
    
    int s = filehandle;
    s++;

    for (i=0; i<size; i++) {
        putch(char_buf[i]);
    }

    return size;
}