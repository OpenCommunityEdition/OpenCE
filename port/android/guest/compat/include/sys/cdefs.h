/* The guest's musl has no <sys/cdefs.h>, and clang's own headers ask for it
   (its stdatomic.h, for one, includes it for the __BEGIN_DECLS pair). This
   stands in. It is searched after every other include directory (the builds
   add it with -idirafter), so a real one would win if the tree ever has it. */
#ifndef _SYS_CDEFS_H
#define _SYS_CDEFS_H

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#define __P(protos) protos
#define __CONCAT(x, y) x##y
#define __STRING(x) #x
#define __ptr_t void *
#define __dead2 __attribute__((__noreturn__))
#define __pure2 __attribute__((__const__))
#define __unused __attribute__((__unused__))
#define __used __attribute__((__used__))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __section(x) __attribute__((__section__(x)))

#endif /* _SYS_CDEFS_H */
