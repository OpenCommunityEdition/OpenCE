/* Tiger's 32-bit barrier CAS replaces the unsupported GCC 4 intrinsic only
   for debug-file publication. Aligned pointer loads are atomic on both targets. */
#include <libkern/OSAtomic.h>
#include <stdint.h>
#include "engine_atomic_file.h"
typedef char mac_atomic_pointer_size[sizeof(FILE *)==sizeof(int32_t)?1:-1];
FILE *mac_atomic_file_compare_exchange(FILE *volatile *slot,FILE *expected,FILE *replacement)
{
    FILE *observed;
    for(;;) {
        OSMemoryBarrier();
        observed=*slot;
        OSMemoryBarrier();
        if(observed!=expected)return observed;
        if(OSAtomicCompareAndSwap32Barrier((int32_t)(uintptr_t)expected,
            (int32_t)(uintptr_t)replacement,(int32_t *)slot))return expected;
    }
}
