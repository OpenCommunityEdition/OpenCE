#include "engine_atomic_file.h"
#include <pthread.h>
#include <stdlib.h>
#define THREADS 8
static FILE *volatile winner;
static FILE *files[THREADS],*observed[THREADS];
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition=PTHREAD_COND_INITIALIZER;
static unsigned ready;
static int release_threads;
static void *publish(void *opaque)
{
    unsigned index=*(unsigned *)opaque;
    pthread_mutex_lock(&lock);++ready;pthread_cond_broadcast(&condition);
    while(!release_threads)pthread_cond_wait(&condition,&lock);
    pthread_mutex_unlock(&lock);
    observed[index]=mac_atomic_file_compare_exchange(&winner,NULL,files[index]);
    return NULL;
}
int main(void)
{
    unsigned i,cycle,checks=0,wins;pthread_t threads[THREADS];unsigned indices[THREADS];
    for(i=0;i<THREADS;++i){files[i]=tmpfile();if(!files[i])return 2;indices[i]=i;}
    for(cycle=0;cycle<100;++cycle){
        winner=NULL;ready=0;release_threads=0;wins=0;
        for(i=0;i<THREADS;++i)if(pthread_create(&threads[i],NULL,publish,&indices[i]))return 2;
        pthread_mutex_lock(&lock);
        while(ready!=THREADS)pthread_cond_wait(&condition,&lock);
        release_threads=1;pthread_cond_broadcast(&condition);pthread_mutex_unlock(&lock);
        for(i=0;i<THREADS;++i){pthread_join(threads[i],NULL);++checks;if(!observed[i]){++wins;if(winner!=files[i])return 1;}else if(observed[i]!=winner)return 1;}
        ++checks;if(wins!=1)return 1;
        ++checks;if(mac_atomic_file_compare_exchange(&winner,winner,NULL)==NULL || winner!=NULL)return 1;
    }
    for(i=0;i<THREADS;++i)fclose(files[i]);
    printf("native atomic FILE publication: %u checks, 0 failures\n",checks);return 0;
}
