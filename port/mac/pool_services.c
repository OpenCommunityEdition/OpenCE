/* Native overlapping-copy boundary used by the actual relocating object pool. */
#include "cseries.h"
#undef memmove
void *csmemmove(void *out,const void *in,unsigned long size)
{ return memmove(out,in,size); }
