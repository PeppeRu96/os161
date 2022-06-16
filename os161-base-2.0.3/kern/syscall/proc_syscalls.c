#include <types.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <synch.h>

pid_t sys_getpid( struct proc *p ) {
    return proc_getpid(p);
}

pid_t sys_waitpid(pid_t pid, int *returncode, int flags) {
    struct proc *p;
    int ret_status;

    p = proc_from_pid(pid);
    if (p == NULL)
        return -1;

    ret_status = proc_wait(p);
    *returncode = ret_status;

    (void)flags;

    return pid;
}

void sys__exit(int status) {
    struct proc *p = curproc;

    // No need to destroy the address space here because the operation
    // is already performed by the proc_wait -> proc_destroy

    // Before ending the thread, detach this thread from its process
    // This is done because after lock_release, the waiting process
    // could have already destroyed the process before the
    // thread_exit call..
    // This is performed to make the independence between
    // the thread_exit() and its actual process, so they can have
    // a separate life cycle.
    proc_remthread(curthread);

    // Save the exit state, set the ZOMBIE status and signal on the cv
    lock_acquire(p->p_state_lk);

    p->exit_status = status;
    p->proc_state = PROC_STATUS_ZOMBIE;
    cv_signal(p->p_cv, p->p_state_lk);

    lock_release(p->p_state_lk);


    // Terminate the current thread (actually it goes zombie)
    thread_exit();

    panic("thread_exit returned (should not happen)\n");
}