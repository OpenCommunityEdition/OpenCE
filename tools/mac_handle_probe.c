#include "engine_handles.h"
#include <stdio.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
static int checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);}}while(0)
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static int entered,released,done;
static unsigned long worker(void *value)
{
    unsigned long result=(unsigned long)value;
    if(GetLastError()!=0)return 99;
    SetLastError(87);
    pthread_mutex_lock(&lock);++entered;pthread_cond_broadcast(&changed);
    while(!released)pthread_cond_wait(&changed,&lock);
    ++done;pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);
    return result;
}
static void *event_completed;
static unsigned long event_worker(void *event)
{
    unsigned long result=WaitForSingleObject(event,5000);
    if(result==0)SetEvent(event_completed);
    return result;
}
static void event_checks(void)
{
    void *event,*first,*second;
    unsigned long code;
    int manual;
    CHECK(CreateEventA(NULL,0,0,"unsupported")==NULL && GetLastError()==50);
    CHECK(SetEvent((void*)1)==0 && GetLastError()==6);
    CHECK(ResetEvent(NULL)==0 && GetLastError()==6);
    for(manual=0;manual<=1;++manual)
    {
        event=CreateEventA(NULL,manual,1,NULL);CHECK(event!=NULL);
        CHECK(ResumeThread(event)==~0UL && GetLastError()==6);
        CHECK(!GetExitCodeThread(event,&code) && GetLastError()==6);
        CHECK(WaitForSingleObject(event,0)==0);
        CHECK(WaitForSingleObject(event,0)==(manual?0UL:258UL));
        CHECK(ResetEvent(event));
        CHECK(WaitForSingleObject(event,5)==258);
        CHECK(SetEvent(event));CHECK(SetEvent(event));
        CHECK(WaitForSingleObject(event,0)==0);
        CHECK(WaitForSingleObject(event,0)==(manual?0UL:258UL));
        CHECK(ResetEvent(event));
        event_completed=CreateEventA(NULL,0,0,NULL);CHECK(event_completed!=NULL);
        first=CreateThread(NULL,0,event_worker,event,0,NULL);
        second=CreateThread(NULL,0,event_worker,event,0,NULL);
        CHECK(first && second);
        CHECK(!SetEvent(first) && GetLastError()==6);
        CHECK(SetEvent(event));
        CHECK(WaitForSingleObject(event_completed,3000)==0);
        if(!manual)
        {
            /* Auto reset releases exactly one waiter. A second SetEvent is
               needed even if both workers had not reached their wait yet. */
            CHECK(WaitForSingleObject(event_completed,20)==258);
            CHECK(SetEvent(event));
        }
        CHECK(WaitForSingleObject(first,3000)==0);
        CHECK(WaitForSingleObject(second,3000)==0);
        CHECK(GetExitCodeThread(first,&code) && code==0);
        CHECK(GetExitCodeThread(second,&code) && code==0);
        CHECK(CloseHandle(first));CHECK(CloseHandle(second));
        CHECK(CloseHandle(event_completed));CHECK(CloseHandle(event));
        CHECK(!ResetEvent(event) && GetLastError()==6);
    }
}
static int file_destroyed,file_value;
static void file_destroy(void *payload)
{
    if(payload==&file_value)++file_destroyed;
}
static void file_lease_checks(void)
{
    void *handle,*lease,*payload;
    handle=mac_file_handle_create(&file_value,file_destroy);CHECK(handle!=NULL);
    CHECK(WaitForSingleObject(handle,0)==~0UL && GetLastError()==6);
    CHECK(!SetEvent(handle) && GetLastError()==6);
    lease=mac_file_handle_acquire(handle,&payload);
    CHECK(lease!=NULL && payload==&file_value);
    CHECK(CloseHandle(handle));CHECK(file_destroyed==0);
    CHECK(mac_file_handle_acquire(handle,&payload)==NULL && GetLastError()==6);
    CHECK(payload==NULL);
    mac_file_handle_release(lease);CHECK(file_destroyed==1);
    handle=CreateEventA(NULL,0,0,NULL);
    CHECK(mac_file_handle_acquire(handle,&payload)==NULL && GetLastError()==6);
    CHECK(CloseHandle(handle));
}
static void service_lease_checks(void)
{
    int kind,other,start=file_destroyed;
    void *handle,*lease,*payload;
    for(kind=MAC_HANDLE_SAVE_FIND;kind<=MAC_HANDLE_SIGNATURE;++kind) {
        handle=mac_service_handle_create(kind,&file_value,file_destroy);CHECK(handle!=NULL);
        CHECK(!CloseHandle(handle) && GetLastError()==6);
        CHECK(!mac_find_handle_acquire(handle,&payload) && payload==NULL);
        CHECK(!mac_file_handle_acquire(handle,&payload) && payload==NULL);
        for(other=MAC_HANDLE_SAVE_FIND;other<=MAC_HANDLE_SIGNATURE;++other)if(other!=kind) {
            CHECK(!mac_service_handle_acquire(handle,other,&payload) && GetLastError()==6 && payload==NULL);
            CHECK(!mac_service_handle_close(handle,other) && GetLastError()==6);
        }
        lease=mac_service_handle_acquire(handle,kind,&payload);CHECK(lease && payload==&file_value);
        if(kind==MAC_HANDLE_SIGNATURE) {
            CHECK(!mac_find_handle_close(handle) && GetLastError()==6);
            CHECK(mac_service_handle_close(handle,kind));
        } else CHECK(mac_find_handle_close(handle));
        CHECK(file_destroyed==start);
        CHECK(!mac_service_handle_acquire(handle,kind,&payload) && GetLastError()==6);
        mac_file_handle_release(lease);++start;CHECK(file_destroyed==start);
    }
    CHECK(!mac_service_handle_create(3,&file_value,file_destroy) && GetLastError()==87);
}
static void find_lease_checks(void)
{
    void *handle,*lease,*payload;
    handle=mac_find_handle_create(&file_value,file_destroy);CHECK(handle!=NULL);
    CHECK(!CloseHandle(handle) && GetLastError()==6);
    CHECK(WaitForSingleObject(handle,0)==~0UL && GetLastError()==6);
    CHECK(mac_file_handle_acquire(handle,&payload)==NULL && GetLastError()==6);
    lease=mac_find_handle_acquire(handle,&payload);CHECK(lease && payload==&file_value);
    CHECK(mac_find_handle_close(handle));CHECK(file_destroyed==1);
    CHECK(mac_find_handle_acquire(handle,&payload)==NULL && GetLastError()==6);
    mac_find_handle_release(lease);CHECK(file_destroyed==2);
    handle=CreateEventA(NULL,0,0,NULL);
    CHECK(!mac_find_handle_close(handle) && GetLastError()==6);CHECK(CloseHandle(handle));
}
static void *mutex_abandon(void *value)
{
    if(WaitForSingleObject(value,1000)!=0)return (void *)1;
    return NULL; /* TLS owner teardown must make the mutex abandoned. */
}
static void *mutex_contend(void *value)
{
    if(ReleaseMutex(value) || GetLastError()!=288)return (void *)1;
    if(WaitForSingleObject(value,5)!=258)return (void *)2;
    return NULL;
}
static void mutex_checks(void)
{
    void *first,*second,*result;
    pthread_t thread;
    first=CreateMutexA(NULL,1,"opence-probe-mutex");CHECK(first && GetLastError()==0);
    second=CreateMutexA(NULL,1,"opence-probe-mutex");
    CHECK(second && second!=first && GetLastError()==183);
    CHECK(WaitForSingleObject(second,0)==0); /* Recursive across handles. */
    CHECK(!SetEvent(first) && GetLastError()==6);
    CHECK(pthread_create(&thread,NULL,mutex_contend,second)==0);
    CHECK(pthread_join(thread,&result)==0 && result==NULL);
    CHECK(ReleaseMutex(first)); /* Still owns one recursive acquisition. */
    CHECK(pthread_create(&thread,NULL,mutex_contend,second)==0);
    CHECK(pthread_join(thread,&result)==0 && result==NULL);
    CHECK(ReleaseMutex(second));
    CHECK(!ReleaseMutex(first) && GetLastError()==288);
    CHECK(CloseHandle(first));
    CHECK(pthread_create(&thread,NULL,mutex_abandon,second)==0);
    CHECK(pthread_join(thread,&result)==0 && result==NULL);
    CHECK(WaitForSingleObject(second,1000)==128);
    CHECK(ReleaseMutex(second));
    CHECK(WaitForSingleObject(second,0)==0);
    CHECK(ReleaseMutex(second));CHECK(CloseHandle(second));
    first=CreateMutexA(NULL,0,"opence-probe-mutex");CHECK(first && GetLastError()==0);
    CHECK(!ReleaseMutex(first) && GetLastError()==288);CHECK(CloseHandle(first));
    CHECK(!ReleaseMutex((void *)1) && GetLastError()==6);
}
int main(void)
{
    void *h;
    unsigned long id,code;
    CHECK(sizeof(void*)==4 && sizeof(unsigned long)==4);
    CHECK(CloseHandle((void*)1)==0 && GetLastError()==6);
    CHECK(CreateThread(NULL,0,NULL,NULL,0,NULL)==NULL && GetLastError()==87);
    CHECK(!SetThreadPriority((void *)1,0) && GetLastError()==6);
    CHECK(!SetThreadPriority((void *)-2,-16) && GetLastError()==87);
    {
        thread_precedence_policy_data_t before,after;
        mach_msg_type_number_t count=THREAD_PRECEDENCE_POLICY_COUNT;
        thread_t self=pthread_mach_thread_np(pthread_self());
        boolean_t get_default=FALSE;
        kern_return_t kr=thread_policy_get(self,THREAD_PRECEDENCE_POLICY,
            (thread_policy_t)&before,&count,&get_default);
        CHECK(kr==KERN_SUCCESS);
        if(kr==KERN_SUCCESS)
        {
            int accepted=SetThreadPriority((void *)-2,1);
            if(accepted)
            {
                count=THREAD_PRECEDENCE_POLICY_COUNT;
                get_default=FALSE;
                CHECK(thread_policy_get(self,THREAD_PRECEDENCE_POLICY,
                    (thread_policy_t)&after,&count,&get_default)==KERN_SUCCESS);
                CHECK(after.importance==1);
                CHECK(SetThreadPriority((void *)-2,before.importance));
            }
            else CHECK(GetLastError()!=0);
        }
    }
    SetLastError(123);
    h=CreateThread(NULL,0,worker,(void*)42,4,&id);
    CHECK(h && id);
    {
        int accepted=SetThreadPriority(h,2);
        if(!accepted)CHECK(GetLastError()!=0);
    }
    CHECK(GetLastError()==123);
    CHECK(GetExitCodeThread(h,&code) && code==259);
    CHECK(WaitForSingleObject(h,0)==258);
    CHECK(WaitForSingleObject(h,5)==258);
    pthread_mutex_lock(&lock);CHECK(entered==0);pthread_mutex_unlock(&lock);
    CHECK(ResumeThread(h)==1);
    CHECK(ResumeThread(h)==0);
    pthread_mutex_lock(&lock);while(!entered)pthread_cond_wait(&changed,&lock);
    released=1;pthread_cond_broadcast(&changed);pthread_mutex_unlock(&lock);
    CHECK(WaitForSingleObject(h,10000)==0);
    CHECK(GetExitCodeThread(h,&code) && code==42);
    CHECK(GetLastError()==123);
    CHECK(CloseHandle(h)==1);
    CHECK(CloseHandle(h)==0 && GetLastError()==6);
    CHECK(ResumeThread(h)==~0UL && GetLastError()==6);
    CHECK(WaitForSingleObject(h,0)==~0UL && GetLastError()==6);
    pthread_mutex_lock(&lock);released=0;entered=0;done=0;pthread_mutex_unlock(&lock);
    h=CreateThread(NULL,65536,worker,(void*)7,0,NULL);CHECK(h!=NULL);
    pthread_mutex_lock(&lock);while(!entered)pthread_cond_wait(&changed,&lock);pthread_mutex_unlock(&lock);
    CHECK(CloseHandle(h)==1);
    pthread_mutex_lock(&lock);CHECK(done==0);released=1;pthread_cond_broadcast(&changed);
    while(!done)pthread_cond_wait(&changed,&lock);pthread_mutex_unlock(&lock);
    CHECK(GetExitCodeThread(h,&code)==0 && GetLastError()==6);
    CHECK(GetExitCodeThread(NULL,NULL)==0 && GetLastError()==87);
    event_checks();file_lease_checks();find_lease_checks();service_lease_checks();mutex_checks();
    printf("native thread/event/file/mutex handle checks=%d failures=%d\n",checks,failures);
    return failures?1:0;
}
