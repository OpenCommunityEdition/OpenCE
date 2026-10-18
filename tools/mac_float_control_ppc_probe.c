#include <fenv.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "float.h"

void real_math_reset_precision(void);

#if !defined(__ppc__)
#error This probe verifies native PowerPC fenv behavior.
#endif

struct ppc_float_thread_result { int rounding; unsigned int control; };

static void *ppc_float_thread(void *argument)
{
    struct ppc_float_thread_result *result=(struct ppc_float_thread_result *)argument;
    if(fesetround(FE_UPWARD)!=0)return (void *)1;
    result->control=_control87(0,0);
    result->rounding=fegetround();
    if((_control87(_RC_DOWN,_MCW_RC)&_MCW_RC)!=_RC_DOWN)return (void *)1;
    if(fegetround()!=FE_DOWNWARD)return (void *)1;
    return NULL;
}

int main(void)
{
    pthread_t thread;
    struct ppc_float_thread_result result;
    void *thread_result=NULL;
    unsigned int control;

    if(fesetround(FE_DOWNWARD)!=0 ||
        (_control87(0,0)&_MCW_RC)!=_RC_DOWN) {
        fprintf(stderr,"PPC query did not report native downward fenv\n");
        return 1;
    }
    real_math_reset_precision();
    control=_control87(0,0);
    if(fegetround()!=FE_TONEAREST || (control&_MCW_RC)!=_RC_NEAR ||
        (control&_MCW_PC)!=_PC_53) {
        fprintf(stderr,"PPC reset mismatch: fenv=%d control=%08x\n",fegetround(),control);
        return 1;
    }
    if(pthread_create(&thread,NULL,ppc_float_thread,&result)!=0 ||
        pthread_join(thread,&thread_result)!=0 || thread_result ||
        result.rounding!=FE_UPWARD || (result.control&_MCW_RC)!=_RC_UP ||
        fegetround()!=FE_TONEAREST || (_control87(0,0)&_MCW_RC)!=_RC_NEAR) {
        fprintf(stderr,"PPC float state leaked across threads\n");
        return 1;
    }
    printf("PPC fenv reset/thread isolation passed: control=%08x thread=%08x\n",
        control,result.control);
    return 0;
}
