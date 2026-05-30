#include <errno.h>
#include <sys/types.h>

/* Define these as weak symbols so you can override them */

/* Stub for sbrk - memory allocation */
caddr_t _sbrk(int incr)
{
    extern char _end;
    static char *heap_end = &_end;
    char *prev_heap_end   = heap_end;
    heap_end += incr;
    return (caddr_t) prev_heap_end;
}

/* Stub for kill - signal handling */
int _kill(pid_t pid, int sig)
{
    errno = EINVAL;
    return -1;
}

/* Stub for getpid - process ID */
pid_t _getpid(void)
{
    return 1;
}