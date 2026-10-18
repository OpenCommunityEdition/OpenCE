/* Map the engine's control-word reset to native rounding and, on Intel, the
   real x87 precision/exception fields. PowerPC has no x87 precision field;
   its unsupported control fields are kept per thread, while RC is read back
   from the real fenv state. */
#include <fenv.h>
#include <float.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_key_t mac_float_control_key;
static pthread_once_t mac_float_control_once=PTHREAD_ONCE_INIT;
static int mac_float_control_key_ready;

static void mac_float_control_key_create(void)
{
    mac_float_control_key_ready=pthread_key_create(&mac_float_control_key,free)==0;
}

static unsigned int *mac_float_control_state(void)
{
    unsigned int *state;
    if(pthread_once(&mac_float_control_once,mac_float_control_key_create)!=0 ||
        !mac_float_control_key_ready)return NULL;
    state=(unsigned int *)pthread_getspecific(mac_float_control_key);
    if(!state)
    {
        state=(unsigned int *)malloc(sizeof(*state));
        if(!state)return NULL;
        *state=CW_DEFAULT;
        if(pthread_setspecific(mac_float_control_key,state)!=0)
        {
            free(state);
            return NULL;
        }
    }
    return state;
}

#if defined(__i386__)
static unsigned short mac_x87_get_control(void)
{
    unsigned short word;
    __asm__ __volatile__("fnstcw %0" : "=m"(word));
    return word;
}

static unsigned int mac_x87_control_to_msvc(unsigned short word)
{
    unsigned int result=0;
    if(word&0x0001)result|=_EM_INVALID;
    if(word&0x0002)result|=_EM_DENORMAL;
    if(word&0x0004)result|=_EM_ZERODIVIDE;
    if(word&0x0008)result|=_EM_OVERFLOW;
    if(word&0x0010)result|=_EM_UNDERFLOW;
    if(word&0x0020)result|=_EM_INEXACT;
    if((word&0x0300)==(2<<8))result|=_PC_53;
    else if((word&0x0300)==(3<<8))result|=_PC_64;
    else result|=_PC_24;
    result|=(unsigned int)((word&0x0c00)>>2);
    return result;
}

static unsigned short mac_x87_set_control(unsigned int value,unsigned short word)
{
    word&=(unsigned short)~0x0f3f;
    if(value&_EM_INVALID)word|=0x0001;
    if(value&_EM_DENORMAL)word|=0x0002;
    if(value&_EM_ZERODIVIDE)word|=0x0004;
    if(value&_EM_OVERFLOW)word|=0x0008;
    if(value&_EM_UNDERFLOW)word|=0x0010;
    if(value&_EM_INEXACT)word|=0x0020;
    if((value&_MCW_PC)==_PC_53)word|=2<<8;
    else if((value&_MCW_PC)==_PC_64)word|=3<<8;
    word|=(unsigned short)(((value&_MCW_RC)>>8)<<10);
    return word;
}
#endif

static unsigned int mac_fenv_rounding(void)
{
    switch(fegetround())
    {
    case FE_DOWNWARD:return _RC_DOWN;
    case FE_UPWARD:return _RC_UP;
    case FE_TOWARDZERO:return _RC_CHOP;
    default:return _RC_NEAR;
    }
}

unsigned int _control87(unsigned int new_value,unsigned int mask)
{
    unsigned int *state=mac_float_control_state();
    unsigned int previous,updated;
    int rounding;
#if defined(__i386__)
    unsigned short word;
    previous=mac_x87_control_to_msvc(mac_x87_get_control());
#else
    previous=state ? *state : CW_DEFAULT;
    previous=(previous&~_MCW_RC)|mac_fenv_rounding();
#endif
    if(!mask)return previous;
    updated=(previous&~mask)|(new_value&mask);
    switch(updated&_MCW_RC)
    {
    case _RC_DOWN:rounding=FE_DOWNWARD;break;
    case _RC_UP:rounding=FE_UPWARD;break;
    case _RC_CHOP:rounding=FE_TOWARDZERO;break;
    default:rounding=FE_TONEAREST;break;
    }
    if(fesetround(rounding)!=0)return previous;
#if defined(__i386__)
    word=mac_x87_set_control(updated,mac_x87_get_control());
    __asm__ __volatile__("fldcw %0" : : "m"(word));
    return mac_x87_control_to_msvc(mac_x87_get_control());
#else
    if(state)*state=updated;
    return (updated&~_MCW_RC)|mac_fenv_rounding();
#endif
}
