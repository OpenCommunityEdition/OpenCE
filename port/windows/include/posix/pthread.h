/*
PTHREAD.H

The subset of POSIX threads the platform layer uses, implemented over Windows
threads, critical sections and condition variables
(port/windows/src/win32_posix.c). The types are declared without windows.h,
which cannot be included next to the Xbox SDK headers.
*/

#ifndef __HALO_WINDOWS_PTHREAD_H
#define __HALO_WINDOWS_PTHREAD_H

#include <stddef.h>
#include <time.h>

typedef unsigned long pthread_t;

typedef struct
{
	void *section; /* CRITICAL_SECTION, allocated on first use */
} pthread_mutex_t;

typedef struct
{
	void *variable; /* CONDITION_VARIABLE, which is pointer-sized */
} pthread_cond_t;

typedef struct
{
	size_t stack_size;
	int detached;
} pthread_attr_t;

typedef struct
{
	int type;
} pthread_mutexattr_t;

typedef struct
{
	int unused;
} pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER { 0 }
#define PTHREAD_COND_INITIALIZER { 0 }

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

/* critical sections are always recursive */
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL

int pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*start)(void *), void *argument);
int pthread_detach(pthread_t thread);
pthread_t pthread_self(void);
int pthread_equal(pthread_t thread1, pthread_t thread2);

int pthread_attr_init(pthread_attr_t *attributes);
int pthread_attr_destroy(pthread_attr_t *attributes);
int pthread_attr_setdetachstate(pthread_attr_t *attributes, int state);
int pthread_attr_setstacksize(pthread_attr_t *attributes, size_t size);

int pthread_mutexattr_init(pthread_mutexattr_t *attributes);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attributes);
int pthread_mutexattr_settype(pthread_mutexattr_t *attributes, int type);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attributes);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *condition, const pthread_condattr_t *attributes);
int pthread_cond_destroy(pthread_cond_t *condition);
int pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex);
/* deadline on the CLOCK_REALTIME clock */
int pthread_cond_timedwait(pthread_cond_t *condition, pthread_mutex_t *mutex, const struct timespec *deadline);
int pthread_cond_signal(pthread_cond_t *condition);
int pthread_cond_broadcast(pthread_cond_t *condition);

#endif
