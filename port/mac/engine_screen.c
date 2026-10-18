#include "engine_screen.h"
#define MAC_INPUT_NO_CARBON
#include "mac_window.h"
#undef MAC_INPUT_NO_CARBON
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <mach/mach_time.h>

/* The game's screen is 480 lines and 640 columns, or more where the window is
   wider (mac_window.c); the render surface is that size, scaled to the window
   when presented (engine_device_state.c). */
static long native_ui_offset;

long halo_screen_commit(void)
{
    /* port: the width follows the window's shape (mac_window.c) */
    return mac_window_screen_commit();
}
void halo_screen_ui_offset(unsigned char centered)
{
    /* the menus and full-screen overlays are laid out for 640 columns: while
       they draw, everything shifts right to centre them */
    native_ui_offset=centered?(halo_screen_width()-640)/2:0;
}
long mac_engine_ui_offset(void){return native_ui_offset;}

static pthread_mutex_t blank_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t blank_condition=PTHREAD_COND_INITIALIZER;
static D3DCALLBACK blank_callback;
static unsigned long blank_count,pending_flips;
static volatile unsigned int flip_count;
static pthread_t blank_thread_id;
static int blank_thread_started,blank_thread_stopping,blank_callback_in_flight;

static void *blank_thread(void *unused)
{
    mach_timebase_info_data_t timebase;unsigned long long interval,next;
    (void)unused;
    mach_timebase_info(&timebase);
    interval=(16666667ULL*timebase.denom)/timebase.numer;
    next=mach_absolute_time();
    for(;;){
        D3DCALLBACK callback;
        next+=interval;mach_wait_until(next);
        pthread_mutex_lock(&blank_lock);
        if(blank_thread_stopping){pthread_mutex_unlock(&blank_lock);break;}
        ++blank_count;
        if(pending_flips){--pending_flips;++flip_count;}
        callback=blank_callback;
        if(callback)++blank_callback_in_flight;
        pthread_cond_broadcast(&blank_condition);
        pthread_mutex_unlock(&blank_lock);
        if(callback){
            callback(0);
            pthread_mutex_lock(&blank_lock);--blank_callback_in_flight;
            pthread_cond_broadcast(&blank_condition);pthread_mutex_unlock(&blank_lock);
        }
    }
    return NULL;
}
static int blank_start(void)
{
    int error;
    pthread_mutex_lock(&blank_lock);
    if(blank_thread_started){pthread_mutex_unlock(&blank_lock);return 1;}
    blank_thread_stopping=0;
    error=pthread_create(&blank_thread_id,NULL,blank_thread,NULL);
    if(!error)blank_thread_started=1;
    pthread_mutex_unlock(&blank_lock);
    if(error)fprintf(stderr,"cannot start native vertical blank clock (%d)\n",error);
    return error==0;
}
void WINAPI D3DDevice_SetVerticalBlankCallback(D3DCALLBACK callback)
{
    pthread_mutex_lock(&blank_lock);
    blank_callback=NULL;
    while(blank_callback_in_flight&&!pthread_equal(pthread_self(),blank_thread_id))
        pthread_cond_wait(&blank_condition,&blank_lock);
    blank_callback=callback;pthread_mutex_unlock(&blank_lock);
    if(!blank_start())abort();
}
void WINAPI D3DDevice_BlockUntilVerticalBlank(void)
{
    unsigned long count;
    if(!blank_start())abort();
    pthread_mutex_lock(&blank_lock);count=blank_count;
    while(blank_count==count)pthread_cond_wait(&blank_condition,&blank_lock);
    pthread_mutex_unlock(&blank_lock);
}
void mac_engine_vblank_presented(void)
{
    pthread_mutex_lock(&blank_lock);++pending_flips;pthread_mutex_unlock(&blank_lock);
}
void mac_engine_vblank_stop(void)
{
    pthread_t thread;
    pthread_mutex_lock(&blank_lock);
    if(!blank_thread_started){blank_callback=NULL;pending_flips=0;pthread_mutex_unlock(&blank_lock);return;}
    if(pthread_equal(pthread_self(),blank_thread_id)){pthread_mutex_unlock(&blank_lock);abort();}
    blank_callback=NULL;
    while(blank_callback_in_flight)pthread_cond_wait(&blank_condition,&blank_lock);
    blank_thread_stopping=1;thread=blank_thread_id;
    pthread_mutex_unlock(&blank_lock);
    pthread_join(thread,NULL);
    pthread_mutex_lock(&blank_lock);blank_thread_started=0;blank_thread_stopping=0;
    pending_flips=0;pthread_mutex_unlock(&blank_lock);
}
volatile unsigned int *d3d_find_flipcount(void){return &flip_count;}
