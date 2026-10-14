/*
GUEST_SYSCALL.C

musl's system calls (arch/arm64_32/syscall_arch.h). The guest cannot enter
the kernel itself: the structures it would pass (timespec, stat, iovec) have
ILP32 layouts, and memory it maps must stay below 4 GB. The host performs
each call, converting where needed (host_syscall.c).

The calls use the guest's own interface (halo_guest_syscall.h), which the
host translates to its kernel's. The checks below stop the build if musl's
headers for this guest architecture disagree with it.

The result is a long, 32 bits, as musl's __syscall returns. Every result
the guest's calls give fits in it; the 64-bit file offset of lseek comes
back through _llseek's pointer instead (musl's lseek uses _llseek when the
numbering has it, as on 32-bit architectures).
*/

#include "guest_host.h"
#include "halo_guest_syscall.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/uio.h>
#include <time.h>
#include <kstat.h>

#define SAME(name) _Static_assert(name == GUEST_##name, "musl's " #name " differs from the guest interface's")

/* the numbers */
SAME(SYS_getcwd);
SAME(SYS_epoll_pwait);
SAME(SYS_dup);
SAME(SYS_dup3);
SAME(SYS_fcntl);
SAME(SYS_ioctl);
SAME(SYS_flock);
SAME(SYS_mkdirat);
SAME(SYS_unlinkat);
SAME(SYS_renameat);
SAME(SYS_statfs);
SAME(SYS_fstatfs);
SAME(SYS_ftruncate);
SAME(SYS_faccessat);
SAME(SYS_chdir);
SAME(SYS_fchmod);
SAME(SYS_fchmodat);
SAME(SYS_openat);
SAME(SYS_close);
SAME(SYS_pipe2);
SAME(SYS_getdents64);
SAME(SYS__llseek);
SAME(SYS_read);
SAME(SYS_write);
SAME(SYS_readv);
SAME(SYS_writev);
SAME(SYS_pread64);
SAME(SYS_pwrite64);
SAME(SYS_preadv);
SAME(SYS_pwritev);
SAME(SYS_pselect6);
SAME(SYS_ppoll);
SAME(SYS_readlinkat);
SAME(SYS_newfstatat);
SAME(SYS_fstat);
SAME(SYS_fsync);
SAME(SYS_fdatasync);
SAME(SYS_utimensat);
SAME(SYS_exit);
SAME(SYS_exit_group);
SAME(SYS_waitid);
SAME(SYS_set_tid_address);
SAME(SYS_futex);
SAME(SYS_nanosleep);
SAME(SYS_getitimer);
SAME(SYS_setitimer);
SAME(SYS_timer_create);
SAME(SYS_timer_gettime);
SAME(SYS_timer_settime);
SAME(SYS_clock_gettime);
SAME(SYS_clock_getres);
SAME(SYS_clock_nanosleep);
SAME(SYS_sched_yield);
SAME(SYS_kill);
SAME(SYS_tkill);
SAME(SYS_tgkill);
SAME(SYS_sigaltstack);
SAME(SYS_rt_sigaction);
SAME(SYS_rt_sigprocmask);
SAME(SYS_rt_sigtimedwait);
SAME(SYS_times);
SAME(SYS_uname);
SAME(SYS_getrlimit);
SAME(SYS_getrusage);
SAME(SYS_umask);
SAME(SYS_gettimeofday);
SAME(SYS_getpid);
SAME(SYS_getppid);
SAME(SYS_getuid);
SAME(SYS_geteuid);
SAME(SYS_getgid);
SAME(SYS_getegid);
SAME(SYS_gettid);
SAME(SYS_sysinfo);
SAME(SYS_sendmsg);
SAME(SYS_recvmsg);
SAME(SYS_brk);
SAME(SYS_munmap);
SAME(SYS_mremap);
SAME(SYS_clone);
SAME(SYS_execve);
SAME(SYS_mmap);
SAME(SYS_mprotect);
SAME(SYS_madvise);
SAME(SYS_wait4);
SAME(SYS_prlimit64);
SAME(SYS_renameat2);
SAME(SYS_getrandom);
SAME(SYS_membarrier);
SAME(SYS_statx);
SAME(SYS_clone3);

/* the structures */
#define SAME_FIELD(guest, host, field) \
	_Static_assert(offsetof(struct guest, field) == offsetof(struct host, field) && \
		sizeof(((struct guest *)0)->field) == sizeof(((struct host *)0)->field), \
		"the guest interface's " #guest "." #field " differs from musl's")
#define SAME_FIELD_AS(guest, guest_field, host, field) \
	_Static_assert(offsetof(struct guest, guest_field) == offsetof(struct host, field) && \
		sizeof(((struct guest *)0)->guest_field) == sizeof(((struct host *)0)->field), \
		"the guest interface's " #guest "." #guest_field " differs from musl's " #field)
_Static_assert(sizeof(struct kstat) == sizeof(struct halo_guest_kstat), "kstat's size differs");
SAME_FIELD(halo_guest_kstat, kstat, st_dev);
SAME_FIELD(halo_guest_kstat, kstat, st_ino);
SAME_FIELD(halo_guest_kstat, kstat, st_mode);
SAME_FIELD(halo_guest_kstat, kstat, st_nlink);
SAME_FIELD(halo_guest_kstat, kstat, st_uid);
SAME_FIELD(halo_guest_kstat, kstat, st_gid);
SAME_FIELD(halo_guest_kstat, kstat, st_rdev);
SAME_FIELD(halo_guest_kstat, kstat, st_size);
SAME_FIELD(halo_guest_kstat, kstat, st_blksize);
SAME_FIELD(halo_guest_kstat, kstat, st_blocks);
SAME_FIELD_AS(halo_guest_kstat, atime_seconds, kstat, st_atime_sec);
SAME_FIELD_AS(halo_guest_kstat, atime_nanoseconds, kstat, st_atime_nsec);
SAME_FIELD_AS(halo_guest_kstat, mtime_seconds, kstat, st_mtime_sec);
SAME_FIELD_AS(halo_guest_kstat, mtime_nanoseconds, kstat, st_mtime_nsec);
SAME_FIELD_AS(halo_guest_kstat, ctime_seconds, kstat, st_ctime_sec);
SAME_FIELD_AS(halo_guest_kstat, ctime_nanoseconds, kstat, st_ctime_nsec);
_Static_assert(sizeof(struct flock) == sizeof(struct halo_guest_flock), "struct flock's size differs");
SAME_FIELD(halo_guest_flock, flock, l_type);
SAME_FIELD(halo_guest_flock, flock, l_whence);
SAME_FIELD(halo_guest_flock, flock, l_start);
SAME_FIELD(halo_guest_flock, flock, l_len);
SAME_FIELD(halo_guest_flock, flock, l_pid);
_Static_assert(sizeof(struct iovec) == sizeof(struct halo_guest_iovec), "struct iovec's size differs");
_Static_assert(sizeof(struct sysinfo) == sizeof(struct halo_guest_sysinfo), "struct sysinfo's size differs");
SAME_FIELD(halo_guest_sysinfo, sysinfo, uptime);
SAME_FIELD(halo_guest_sysinfo, sysinfo, loads);
SAME_FIELD(halo_guest_sysinfo, sysinfo, totalram);
SAME_FIELD(halo_guest_sysinfo, sysinfo, freeram);
SAME_FIELD(halo_guest_sysinfo, sysinfo, sharedram);
SAME_FIELD(halo_guest_sysinfo, sysinfo, bufferram);
SAME_FIELD(halo_guest_sysinfo, sysinfo, totalswap);
SAME_FIELD(halo_guest_sysinfo, sysinfo, freeswap);
SAME_FIELD(halo_guest_sysinfo, sysinfo, procs);
SAME_FIELD(halo_guest_sysinfo, sysinfo, totalhigh);
SAME_FIELD(halo_guest_sysinfo, sysinfo, freehigh);
SAME_FIELD(halo_guest_sysinfo, sysinfo, mem_unit);
SAME_FIELD_AS(halo_guest_sysinfo, reserved, sysinfo, __reserved);
_Static_assert(sizeof(struct timespec) == sizeof(struct halo_guest_timespec), "struct timespec's size differs");
_Static_assert(sizeof(struct rlimit) == 16, "struct rlimit is not prlimit64's");
_Static_assert(IOV_MAX == GUEST_IOV_MAX, "IOV_MAX differs");
#undef SAME_FIELD

/* open's flags and fcntl's commands (musl's O_ACCMODE also has O_SEARCH,
which is O_PATH) */
SAME(O_RDONLY);
SAME(O_WRONLY);
SAME(O_RDWR);
SAME(O_CREAT);
SAME(O_EXCL);
SAME(O_NOCTTY);
SAME(O_TRUNC);
SAME(O_APPEND);
SAME(O_NONBLOCK);
SAME(O_DSYNC);
SAME(O_ASYNC);
SAME(O_DIRECTORY);
SAME(O_NOFOLLOW);
SAME(O_DIRECT);
SAME(O_LARGEFILE);
SAME(O_NOATIME);
SAME(O_CLOEXEC);
SAME(O_SYNC);
SAME(O_PATH);
SAME(O_TMPFILE);
SAME(F_DUPFD);
SAME(F_GETFD);
SAME(F_SETFD);
SAME(F_GETFL);
SAME(F_SETFL);
SAME(F_GETLK);
SAME(F_SETLK);
SAME(F_SETLKW);
SAME(F_SETOWN);
SAME(F_GETOWN);
SAME(F_SETOWN_EX);
SAME(F_GETOWN_EX);
SAME(F_OFD_GETLK);
SAME(F_OFD_SETLK);
SAME(F_OFD_SETLKW);
SAME(F_DUPFD_CLOEXEC);
SAME(FD_CLOEXEC);

/* the values the host passes unchanged */
SAME(AT_FDCWD);
SAME(AT_SYMLINK_NOFOLLOW);
SAME(AT_REMOVEDIR);
SAME(AT_EMPTY_PATH);
SAME(SEEK_SET);
SAME(SEEK_CUR);
SAME(SEEK_END);
SAME(PROT_READ);
SAME(PROT_WRITE);
SAME(PROT_EXEC);
SAME(MAP_SHARED);
SAME(MAP_PRIVATE);
SAME(MAP_FIXED);
SAME(MAP_ANONYMOUS);
SAME(MAP_NORESERVE);
SAME(MAP_FIXED_NOREPLACE);
SAME(MADV_DONTNEED);
SAME(MADV_FREE);
SAME(POLLIN);
SAME(POLLPRI);
SAME(POLLOUT);
SAME(POLLERR);
SAME(POLLHUP);
SAME(POLLNVAL);
SAME(SIG_BLOCK);
SAME(SIG_UNBLOCK);
SAME(SIG_SETMASK);
SAME(SIGABRT);
SAME(SIGKILL);
SAME(SIGSEGV);
SAME(SIGTERM);
SAME(RLIMIT_CPU);
SAME(RLIMIT_FSIZE);
SAME(RLIMIT_DATA);
SAME(RLIMIT_STACK);
SAME(RLIMIT_CORE);
SAME(RLIMIT_NOFILE);
SAME(RLIMIT_AS);
SAME(TIMER_ABSTIME);
SAME(LOCK_SH);
SAME(LOCK_EX);
SAME(LOCK_NB);
SAME(LOCK_UN);
SAME(F_RDLCK);
SAME(F_WRLCK);
SAME(F_UNLCK);
SAME(EPERM);
SAME(ENOENT);
SAME(EINTR);
SAME(EBADF);
SAME(EAGAIN);
SAME(ENOMEM);
SAME(EACCES);
SAME(EFAULT);
SAME(EEXIST);
SAME(ENOTDIR);
SAME(EISDIR);
SAME(EINVAL);
SAME(ENOSPC);
SAME(ERANGE);
SAME(EDEADLK);
SAME(ENAMETOOLONG);
SAME(ENOSYS);
SAME(ENOTEMPTY);
SAME(ELOOP);
SAME(EOVERFLOW);
SAME(EOPNOTSUPP);
SAME(ETIMEDOUT);
#undef SAME

long __guest_syscall(long long number, long long a, long long b, long long c,
	long long d, long long e, long long f)
{
	return (long)host_syscall(number, a, b, c, d, e, f);
}
