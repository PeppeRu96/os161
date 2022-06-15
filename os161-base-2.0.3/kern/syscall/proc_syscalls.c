#include <types.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <proc.h>
#include <current.h>

void sys__exit(int status) {
    // TODO: save status in the TCB of the this thread that's going to be terminated so other threads can read it
    (void)status;

    // Destroy the address space associated to this process
    as_destroy(proc_getas());

    // Terminate the current thread (actually it goes zombie)
    thread_exit();

    panic("thread_exit returned (should not happen)\n");
}