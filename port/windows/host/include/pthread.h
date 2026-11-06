/*
PTHREAD.H

The mutexes and condition variables the files the Windows x64 host shares
with the Linux and Android hosts use (host_sdl.c), on Windows' slim
reader/writer locks and condition variables.
*/

#ifndef WIN64_HOST_PTHREAD_H
#define WIN64_HOST_PTHREAD_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

typedef SRWLOCK pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef int pthread_mutexattr_t;
typedef int pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER CONDITION_VARIABLE_INIT

static inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attributes)
{
	(void)attributes;
	InitializeSRWLock(mutex);
	return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	(void)mutex;
	return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	AcquireSRWLockExclusive(mutex);
	return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	ReleaseSRWLockExclusive(mutex);
	return 0;
}

static inline int pthread_cond_init(pthread_cond_t *condition, const pthread_condattr_t *attributes)
{
	(void)attributes;
	InitializeConditionVariable(condition);
	return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t *condition)
{
	(void)condition;
	return 0;
}

static inline int pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex)
{
	SleepConditionVariableSRW(condition, mutex, INFINITE, 0);
	return 0;
}

static inline int pthread_cond_signal(pthread_cond_t *condition)
{
	WakeConditionVariable(condition);
	return 0;
}

static inline int pthread_cond_broadcast(pthread_cond_t *condition)
{
	WakeAllConditionVariable(condition);
	return 0;
}

#endif
