#include <stdint.h>
#include <sys/stat.h>
extern int errno;

// Minimal syscalls stubs to satisfy linker when using -nostdlib
int _write(int file, char *ptr, int len) {
    (void)file; (void)ptr; (void)len;
    return len;
}

int _read(int file, char *ptr, int len) {
    (void)file; (void)ptr; (void)len;
    return 0;
}

int _close(int file) { (void)file; return -1; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
void _exit(int status) { (void)status; while (1) {} }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = 22; return -1; }
int _getpid(void) { return 1; }

// Simple heap implementation
extern char end; // provided by linker
static char *heap_end;
void* _sbrk(ptrdiff_t incr) {
    if (heap_end == 0) heap_end = &end;
    char *prev = heap_end;
    heap_end += incr;
    return (void*)prev;
}
