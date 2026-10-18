/* Owned native thread, event, file and process-local mutex handles. Closing
   a thread handle never terminates its worker. Waiters and file operations
   retain allocations until their outstanding leases end. Mutex names share
   objects within this process, not a cross-process kernel namespace. */
#include "engine_handles.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <unistd.h>
#include <string.h>
struct mac_owned_handle
{
    struct mac_owned_handle *next;
    pthread_t thread;
    pthread_cond_t changed;
    unsigned long (*entry)(void *);
    void *argument;
    unsigned long exit_code;
    unsigned long id;
    unsigned int references;
    int suspended, finished;
    int kind; /* 0 thread, 1 manual-reset event, 2 auto-reset event, 3 file, 4 mutex, 5 find */
    void *payload;
    void (*destroy_payload)(void *);
};
static pthread_mutex_t mac_handle_lock = PTHREAD_MUTEX_INITIALIZER;
static struct mac_owned_handle *mac_handles;
struct mac_mutex_object
{
    struct mac_mutex_object *next;
    pthread_cond_t changed;
    pthread_t owner;
    char *name;
    unsigned int references,recursion;
    int owned,abandoned;
};
static struct mac_mutex_object *mac_mutex_objects;
static pthread_key_t mac_mutex_owner_key;
static pthread_once_t mac_mutex_owner_once=PTHREAD_ONCE_INIT;
static void mac_mutex_owner_exit(void *value)
{
    struct mac_mutex_object *object;
    (void)value;
    pthread_mutex_lock(&mac_handle_lock);
    for(object=mac_mutex_objects;object;object=object->next)
        if(object->owned && pthread_equal(object->owner,pthread_self()))
        {
            object->owned=0;object->recursion=0;object->abandoned=1;
            pthread_cond_broadcast(&object->changed);
        }
    pthread_mutex_unlock(&mac_handle_lock);
}
static void mac_mutex_owner_key_create(void)
{
    if(pthread_key_create(&mac_mutex_owner_key,mac_mutex_owner_exit))abort();
}
static int mac_mutex_track_owner(void)
{
    return pthread_once(&mac_mutex_owner_once,mac_mutex_owner_key_create)==0 &&
        pthread_setspecific(mac_mutex_owner_key,(void *)1)==0;
}
static void mac_mutex_destroy(void *value)
{
    struct mac_mutex_object *object=value,**entry;
    if(--object->references)return;
    for(entry=&mac_mutex_objects;*entry!=object;entry=&(*entry)->next){}
    *entry=object->next;
    pthread_cond_destroy(&object->changed);free(object->name);free(object);
}
static unsigned long mac_next_id;
static pthread_once_t mac_error_once = PTHREAD_ONCE_INIT;
static pthread_key_t mac_error_key;
static void mac_error_key_create(void)
{
    if (pthread_key_create(&mac_error_key,NULL)) abort();
}
void SetLastError(unsigned long error)
{
    if(pthread_once(&mac_error_once,mac_error_key_create) ||
       pthread_setspecific(mac_error_key,(void *)(uintptr_t)error)) abort();
}
unsigned long GetLastError(void)
{
    if(pthread_once(&mac_error_once,mac_error_key_create)) abort();
    return (unsigned long)(uintptr_t)pthread_getspecific(mac_error_key);
}
static struct mac_owned_handle *mac_handle_find(void *value)
{
    struct mac_owned_handle *h=NULL;
    for(h=mac_handles;h;h=h->next)if(h==value)return h;
    SetLastError(6); return NULL;
}
static void mac_handle_release(struct mac_owned_handle *h)
{
    if(--h->references==0)
    {
        if(h->destroy_payload)h->destroy_payload(h->payload);
        pthread_cond_destroy(&h->changed);free(h);
    }
}
/* Windows priorities are relative to the thread's normal priority. Darwin's
   Tiger thread-precedence policy is also a signed, per-thread relative
   importance. Keep the value intact; Mach decides whether this caller may
   change it. */
int SetThreadPriority(void *value,int priority)
{
    struct mac_owned_handle *h=NULL;
    thread_precedence_policy_data_t policy;
    thread_t thread;
    kern_return_t result;
    if(priority < -15 || priority > 15){SetLastError(87);return 0;}
    if((intptr_t)value==(intptr_t)-2) thread=pthread_mach_thread_np(pthread_self());
    else
    {
        pthread_mutex_lock(&mac_handle_lock);
        h=mac_handle_find(value);
        if(h && (h->kind || h->finished)){SetLastError(6);h=NULL;}
        if(!h){pthread_mutex_unlock(&mac_handle_lock);return 0;}
        ++h->references;
        thread=pthread_mach_thread_np(h->thread);
        pthread_mutex_unlock(&mac_handle_lock);
    }
    policy.importance=(integer_t)priority;
    result=thread_policy_set(thread,THREAD_PRECEDENCE_POLICY,
        (thread_policy_t)&policy,THREAD_PRECEDENCE_POLICY_COUNT);
    if((intptr_t)value!=(intptr_t)-2)
    {
        pthread_mutex_lock(&mac_handle_lock);
        mac_handle_release(h);
        pthread_mutex_unlock(&mac_handle_lock);
    }
    if(result!=KERN_SUCCESS){SetLastError(result==KERN_NO_ACCESS?5:87);return 0;}
    return 1;
}

static void *mac_thread_run(void *value)
{
    struct mac_owned_handle *h=value;
    unsigned long code;
    pthread_mutex_lock(&mac_handle_lock);
    while(h->suspended)pthread_cond_wait(&h->changed,&mac_handle_lock);
    pthread_mutex_unlock(&mac_handle_lock);
    code=h->entry(h->argument);
    pthread_mutex_lock(&mac_handle_lock);
    h->exit_code=code;h->finished=1;
    pthread_cond_broadcast(&h->changed);
    mac_handle_release(h);
    pthread_mutex_unlock(&mac_handle_lock);
    return NULL;
}
#define MAC_MINIMUM_THREAD_STACK (512UL*1024UL)
void *CreateThread(void *attributes,unsigned long stack_size,
    unsigned long (*entry)(void *),void *argument,unsigned long flags,
    unsigned long *thread_id)
{
    struct mac_owned_handle *h;
    pthread_attr_t attr;
    int result;
    long page;
    size_t rounded;
    if(attributes || !entry || (flags & ~4UL)){SetLastError(87);return NULL;}
    h=calloc(1,sizeof(*h));
    if(!h){SetLastError(8);return NULL;}
    result=pthread_cond_init(&h->changed,NULL);
    if(result){free(h);SetLastError(8);return NULL;}
    result=pthread_attr_init(&attr);
    if(result){pthread_cond_destroy(&h->changed);free(h);SetLastError(8);return NULL;}
    result=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if(!result && stack_size)
    {
        page=sysconf(_SC_PAGESIZE);
        if(page<=0 || stack_size>ULONG_MAX-(unsigned long)page)result=EINVAL;
        else
        {
            rounded=((size_t)stack_size+(size_t)page-1)/(size_t)page*(size_t)page;
            if(rounded<PTHREAD_STACK_MIN)rounded=PTHREAD_STACK_MIN;
            /* port: the Xbox's threads ask for a few kilobytes; code built for
               this machine needs more (the profile's write thread overflowed
               its stack in CopyFileA, which is where its frame is) */
            if(rounded<MAC_MINIMUM_THREAD_STACK)rounded=MAC_MINIMUM_THREAD_STACK;
            result=pthread_attr_setstacksize(&attr,rounded);
        }
    }
    h->entry=entry;h->argument=argument;h->suspended=(flags&4)!=0;
    h->exit_code=259;h->references=2;
    pthread_mutex_lock(&mac_handle_lock);
    if(!result)result=pthread_create(&h->thread,&attr,mac_thread_run,h);
    pthread_attr_destroy(&attr);
    if(result)
    {
        pthread_mutex_unlock(&mac_handle_lock);
        pthread_cond_destroy(&h->changed);free(h);SetLastError(result==EINVAL?87:8);return NULL;
    }
    h->id=++mac_next_id;if(!h->id)h->id=++mac_next_id;
    h->next=mac_handles;mac_handles=h;
    if(thread_id)*thread_id=h->id;
    pthread_mutex_unlock(&mac_handle_lock);
    return h;
}
int CloseHandle(void *value)
{
    struct mac_owned_handle *h,**p;
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind>=5){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return 0;}
    for(p=&mac_handles;*p!=h;p=&(*p)->next){}
    *p=h->next;
    mac_handle_release(h);
    pthread_mutex_unlock(&mac_handle_lock);
    return 1;
}
unsigned long ResumeThread(void *value)
{
    struct mac_owned_handle *h;
    unsigned long previous;
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return ~0UL;}
    previous=h->suspended;
    h->suspended=0;pthread_cond_broadcast(&h->changed);
    pthread_mutex_unlock(&mac_handle_lock);
    return previous;
}
int GetExitCodeThread(void *value,unsigned long *code)
{
    struct mac_owned_handle *h;
    if(!code){SetLastError(87);return 0;}
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return 0;}
    *code=h->exit_code;
    pthread_mutex_unlock(&mac_handle_lock);return 1;
}
static uint64_t mac_monotonic_nanoseconds(void)
{
    mach_timebase_info_data_t scale;
    uint64_t ticks;
    if(mach_timebase_info(&scale)!=KERN_SUCCESS || !scale.denom)abort();
    ticks=mach_absolute_time();
    return ticks/scale.denom*scale.numer+(ticks%scale.denom)*scale.numer/scale.denom;
}
static int mac_cond_wait(pthread_cond_t *condition,unsigned long milliseconds,uint64_t deadline)
{
    struct timespec remaining;
    uint64_t now,delta;
    if(milliseconds==~0UL)return pthread_cond_wait(condition,&mac_handle_lock);
    now=mac_monotonic_nanoseconds();
    if(!milliseconds || now>=deadline)return ETIMEDOUT;
    delta=deadline-now;
    remaining.tv_sec=(time_t)(delta/1000000000ULL);
    remaining.tv_nsec=(long)(delta%1000000000ULL);
    /* Tiger's relative wait avoids realtime clock adjustments while the
       monotonic outer deadline prevents spurious wakeups extending a wait. */
    return pthread_cond_timedwait_relative_np(condition,&mac_handle_lock,&remaining);
}
unsigned long WaitForSingleObject(void *value,unsigned long milliseconds)
{
    struct mac_owned_handle *h;
    uint64_t deadline;
    unsigned long result;
    int waited=0;
    deadline=mac_monotonic_nanoseconds()+(uint64_t)milliseconds*1000000ULL;
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind!=0 && h->kind!=1 && h->kind!=2 && h->kind!=4){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return ~0UL;}
    ++h->references;
    if(h->kind==4)
    {
        struct mac_mutex_object *object=h->payload;
        while(object->owned && !pthread_equal(object->owner,pthread_self()) && !waited)
        {
            if(!milliseconds){waited=ETIMEDOUT;break;}
            waited=mac_cond_wait(&object->changed,milliseconds,deadline);
        }
        if(!waited)
        {
            if(object->recursion==UINT_MAX || !mac_mutex_track_owner())
            {result=~0UL;SetLastError(8);}
            else
            {
                result=object->abandoned?128:0;
                object->abandoned=0;object->owned=1;
                object->owner=pthread_self();++object->recursion;
            }
        }
        else {result=waited==ETIMEDOUT?258:~0UL;if(result==~0UL)SetLastError(87);}
        mac_handle_release(h);pthread_mutex_unlock(&mac_handle_lock);return result;
    }
    while(!h->finished && !waited)
    {
        if(!milliseconds){waited=ETIMEDOUT;break;}
        waited=mac_cond_wait(&h->changed,milliseconds,deadline);
    }
    result=h->finished?0:(waited==ETIMEDOUT?258:~0UL);
    if(result==0 && h->kind==2)h->finished=0;
    if(result==~0UL)SetLastError(87);
    mac_handle_release(h);
    pthread_mutex_unlock(&mac_handle_lock);return result;
}

void *CreateEventA(void *attributes,int manual_reset,int initial_state,const char *name)
{
    struct mac_owned_handle *h;
    /* Named process-shared objects and security attributes need separate
       ownership services. Never silently turn them into private events. */
    if(attributes || name){SetLastError(50);return NULL;}
    h=calloc(1,sizeof(*h));
    if(!h){SetLastError(8);return NULL;}
    if(pthread_cond_init(&h->changed,NULL)){free(h);SetLastError(8);return NULL;}
    h->kind=manual_reset?1:2;h->finished=initial_state!=0;h->references=1;
    pthread_mutex_lock(&mac_handle_lock);
    h->next=mac_handles;mac_handles=h;
    pthread_mutex_unlock(&mac_handle_lock);
    return h;
}
static int mac_event_signal(void *value,int signaled)
{
    struct mac_owned_handle *h;
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind!=1 && h->kind!=2){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return 0;}
    h->finished=signaled;
    if(signaled)pthread_cond_broadcast(&h->changed);
    pthread_mutex_unlock(&mac_handle_lock);
    return 1;
}
int SetEvent(void *value){return mac_event_signal(value,1);}
int ResetEvent(void *value){return mac_event_signal(value,0);}

/* File services borrow a reference while accessing payloads. The destructor
   runs under the registry lock and must not call back into handle services. */
static void *mac_payload_handle_create(void *payload,void (*destroy)(void *),int kind)
{
    struct mac_owned_handle *h;
    if(!payload || !destroy){SetLastError(87);return NULL;}
    h=calloc(1,sizeof(*h));
    if(!h){SetLastError(8);return NULL;}
    if(pthread_cond_init(&h->changed,NULL)){free(h);SetLastError(8);return NULL;}
    h->kind=kind;h->payload=payload;h->destroy_payload=destroy;h->references=1;
    pthread_mutex_lock(&mac_handle_lock);
    h->next=mac_handles;mac_handles=h;
    pthread_mutex_unlock(&mac_handle_lock);
    return h;
}
static void *mac_payload_handle_acquire(void *value,void **payload,int kind)
{
    struct mac_owned_handle *h;
    if(!payload){SetLastError(87);return NULL;}
    *payload=NULL;
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind!=kind){SetLastError(6);h=NULL;}
    if(h){++h->references;*payload=h->payload;}
    pthread_mutex_unlock(&mac_handle_lock);
    return h;
}
void mac_file_handle_release(void *lease)
{
    if(!lease)return;
    pthread_mutex_lock(&mac_handle_lock);
    mac_handle_release((struct mac_owned_handle *)lease);
    pthread_mutex_unlock(&mac_handle_lock);
}

void *CreateMutexA(void *attributes,int initial_owner,const char *name)
{
    struct mac_owned_handle *handle;
    struct mac_mutex_object *object;
    int existing=0;
    if(attributes){SetLastError(50);return NULL;}
    if(name && strlen(name)>259){SetLastError(206);return NULL;}
    handle=calloc(1,sizeof(*handle));
    if(!handle){SetLastError(8);return NULL;}
    if(pthread_cond_init(&handle->changed,NULL)){free(handle);SetLastError(8);return NULL;}
    pthread_mutex_lock(&mac_handle_lock);
    object=NULL;
    if(name)for(object=mac_mutex_objects;object;object=object->next)
        if(object->name && !strcmp(name,object->name)){existing=1;break;}
    if(!object)
    {
        object=calloc(1,sizeof(*object));
        if(!object)goto failed;
        if(name){object->name=strdup(name);if(!object->name){free(object);goto failed;}}
        if(pthread_cond_init(&object->changed,NULL)){free(object->name);free(object);goto failed;}
        if(initial_owner)
        {
            if(!mac_mutex_track_owner())
            {pthread_cond_destroy(&object->changed);free(object->name);free(object);goto failed;}
            object->owned=1;object->recursion=1;object->owner=pthread_self();
        }
        object->next=mac_mutex_objects;mac_mutex_objects=object;
    }
    ++object->references;
    handle->kind=4;handle->references=1;handle->payload=object;handle->destroy_payload=mac_mutex_destroy;
    handle->next=mac_handles;mac_handles=handle;
    pthread_mutex_unlock(&mac_handle_lock);
    SetLastError(existing?183:0);return handle;
failed:
    pthread_mutex_unlock(&mac_handle_lock);
    pthread_cond_destroy(&handle->changed);free(handle);SetLastError(8);return NULL;
}
int ReleaseMutex(void *value)
{
    struct mac_owned_handle *handle;
    struct mac_mutex_object *object;
    pthread_mutex_lock(&mac_handle_lock);
    handle=mac_handle_find(value);
    if(handle && handle->kind!=4){SetLastError(6);handle=NULL;}
    if(!handle){pthread_mutex_unlock(&mac_handle_lock);return 0;}
    object=handle->payload;
    if(!object->owned || !pthread_equal(object->owner,pthread_self()))
    {pthread_mutex_unlock(&mac_handle_lock);SetLastError(288);return 0;}
    if(--object->recursion==0){object->owned=0;pthread_cond_broadcast(&object->changed);}
    pthread_mutex_unlock(&mac_handle_lock);return 1;
}

void *mac_file_handle_create(void *payload,void (*destroy)(void *))
{ return mac_payload_handle_create(payload,destroy,3); }
void *mac_file_handle_acquire(void *value,void **payload)
{ return mac_payload_handle_acquire(value,payload,3); }
void *mac_find_handle_create(void *payload,void (*destroy)(void *))
{ return mac_payload_handle_create(payload,destroy,5); }
void *mac_find_handle_acquire(void *value,void **payload)
{ return mac_payload_handle_acquire(value,payload,5); }
void mac_find_handle_release(void *lease)
{ mac_file_handle_release(lease); }
int mac_find_handle_close(void *value)
{
    struct mac_owned_handle *h,**entry;
    pthread_mutex_lock(&mac_handle_lock);
    h=mac_handle_find(value);
    if(h && h->kind!=5 && h->kind!=MAC_HANDLE_SAVE_FIND && h->kind!=MAC_HANDLE_NICKNAME_FIND){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return 0;}
    for(entry=&mac_handles;*entry!=h;entry=&(*entry)->next){}
    *entry=h->next;mac_handle_release(h);
    pthread_mutex_unlock(&mac_handle_lock);return 1;
}

static int valid_service_kind(int kind)
{ return kind>=MAC_HANDLE_SAVE_FIND && kind<=MAC_HANDLE_SIGNATURE; }
void *mac_service_handle_create(int kind,void *payload,void (*destroy)(void *))
{
    if(!valid_service_kind(kind)){SetLastError(87);return NULL;}
    return mac_payload_handle_create(payload,destroy,kind);
}
void *mac_service_handle_acquire(void *value,int kind,void **payload)
{
    if(payload)*payload=NULL;
    if(!valid_service_kind(kind)){SetLastError(87);return NULL;}
    return mac_payload_handle_acquire(value,payload,kind);
}
int mac_service_handle_close(void *value,int kind)
{
    struct mac_owned_handle *h,**entry;
    if(!valid_service_kind(kind)){SetLastError(87);return 0;}
    pthread_mutex_lock(&mac_handle_lock);h=mac_handle_find(value);
    if(h && h->kind!=kind){SetLastError(6);h=NULL;}
    if(!h){pthread_mutex_unlock(&mac_handle_lock);return 0;}
    for(entry=&mac_handles;*entry!=h;entry=&(*entry)->next){}
    *entry=h->next;mac_handle_release(h);
    pthread_mutex_unlock(&mac_handle_lock);return 1;
}
