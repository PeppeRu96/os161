#include <types.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <lib.h>
#include <vnode.h>
#include <limits.h>
#include <vfs.h>
#include <uio.h>
#include <proc.h>
#include <current.h>

#define SYS_OPEN_MAX OPEN_MAX*10

// Describe an opened file through the open system call
struct openfile {
    struct vnode *vn;
    off_t offset;
    ssize_t ref_cnt;
};

// System level open file table (so we can share an open file entry between multiple processes if we want)
struct openfile sys_openfile_table[SYS_OPEN_MAX];

int sys_open(const char *pathname, int flags, mode_t mode) {
    struct openfile *of = NULL;
    int i, fd;
    int result;
    (void)mode;

    /* Obtain the first openfile struct item available */
    for (i=0; i<SYS_OPEN_MAX; i++) {
        if (sys_openfile_table[i].vn == NULL) {
            of = &sys_openfile_table[i];
            break;
        }
    }

    if (of == NULL)
        return -1;

    /* Open the file. */
	result = vfs_open((char *)pathname, flags, mode, &(of->vn));
	if (result) {
		return result;
	}

    /* Initialize offset: we should manage the append case */
    of->offset = 0;

    /* Increase ref count */
    of->ref_cnt = 1;
    // TODO: provide a way to handle multi opening

    /* Place a reference in the process open file table */
    for (fd = STDERR_FILENO+1; fd<OPEN_MAX; fd++) {
        if (curproc->p_openfiles[fd] == NULL) {
            curproc->p_openfiles[fd] = of;
            break;
        }
    }

    if (fd == OPEN_MAX) {
        vfs_close(of->vn);
        of->vn = NULL;
        of->offset = 0;
        of->ref_cnt = 0;
        return -1;
    }

    return fd;
}

int sys_close(int fd) {
    struct openfile *of;

    if (fd < 0 || fd > OPEN_MAX)
        return -1;

    of = curproc->p_openfiles[fd];
    if (of->vn == NULL)
        return -1;

    /* Decrease ref count (for multiple opening) */
    KASSERT(of->ref_cnt > 0);
    of->ref_cnt--;

    /* Delete open file entry only if ref count is zero */
    if (of->ref_cnt == 0) {
        vfs_close(of->vn);
        of->vn = NULL;
        of->offset = 0;
    }

    /* Free the reference in the process file table */
    curproc->p_openfiles[fd] = NULL;

    return 0;
}

ssize_t sys_read(int fd, userptr_t buf, size_t size)
{
    struct openfile *of;
    struct vnode *vn;
    struct uio userio;
    struct iovec iov;
    int result;

    size_t pos = 0;
    int ch;
    char *char_buf = (char *)buf;
    
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        kprintf("Cannot read from STDOUT/STDERR!\n");
        return -1;
    }

    of = curproc->p_openfiles[fd];

    /* Standard input (console) in use */
    if ((of == NULL || of->vn == NULL) && (fd == STDOUT_FILENO || fd == STDERR_FILENO)) {
        for (pos=0; pos<size; pos++) {
            ch = getch();

            if (ch < 0)
                return pos;

            char_buf[pos] = ch;
        }
        
        return size;
    }

    /* Read from file (either native or stdout/err redirected into a file) */
    if (of == NULL || of->vn == NULL) {
        kprintf("Attempt to write to a not opened file!\n");
        return -1;
    }

    vn = of->vn;

    /* Prepare uio and iovec data structures for the read operation */
    iov.iov_ubase = buf;
    iov.iov_len = size;

    userio.uio_iov = &iov;
	userio.uio_iovcnt = 1;
	userio.uio_resid = size;          // amount to read/write from the file
	userio.uio_offset = of->offset;
	userio.uio_segflg = UIO_USERSPACE;
	userio.uio_rw = UIO_READ;
	userio.uio_space = curproc->p_addrspace;

    /* Perform the read operation */
    result = VOP_READ(vn, &userio);
    if (result)
        return result;

    /* Update the offset */
    of->offset = userio.uio_offset;

    return size - userio.uio_resid;
}

ssize_t sys_write(int fd, const_userptr_t buf, size_t size)
{
    struct openfile *of;
    struct vnode *vn;
    struct uio userio;
    struct iovec iov;
    int result;

    size_t i;
    const char *char_buf = (const char *)buf;
    
    if (fd == STDIN_FILENO) {
        kprintf("Cannot write in the STDIN!\n");
        return -1;
    }

    of = curproc->p_openfiles[fd];

    /* Standard output / standard error (console) in use */
    if ((of == NULL || of->vn == NULL) && (fd == STDOUT_FILENO || fd == STDERR_FILENO)) {
        for (i=0; i<size; i++) {
            putch(char_buf[i]);
        }
        
        return size;
    }

    /* Write into a file (either native or stdout/err redirected into a file) */
    if (of == NULL || of->vn == NULL) {
        kprintf("Attempt to write to a not opened file!\n");
        return -1;
    }

    vn = of->vn;

    /* Prepare uio and iovec data structures for the write operation */
    iov.iov_ubase = (userptr_t)buf;
    iov.iov_len = size;

    userio.uio_iov = &iov;
	userio.uio_iovcnt = 1;
	userio.uio_resid = size;          // amount to read/write from the file
	userio.uio_offset = of->offset;
	userio.uio_segflg = UIO_USERSPACE;
	userio.uio_rw = UIO_WRITE;
	userio.uio_space = curproc->p_addrspace;

    /* Perform the write operation */
    result = VOP_WRITE(vn, &userio);
    if (result)
        return result;

    /* Update the offset */
    of->offset = userio.uio_offset;

    return size - userio.uio_resid;
}