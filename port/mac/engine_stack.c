#include "engine_stack.h"
#include <stdint.h>
#include <unwind.h>
struct native_trace { FILE *stream;unsigned skip,visited,written; };
static _Unwind_Reason_Code frame(struct _Unwind_Context *context,void *opaque)
{
    struct native_trace *trace=opaque;
    uintptr_t pc=(uintptr_t)_Unwind_GetIP(context);
    if(trace->visited++>=128)return _URC_END_OF_STACK;
    if(trace->skip){--trace->skip;return _URC_NO_REASON;}
    if(pc){fprintf(trace->stream,"  #%u 0x%08lx\n",trace->written,(unsigned long)pc);++trace->written;}
    return _URC_NO_REASON;
}
unsigned mac_engine_stack_trace(FILE *stream,unsigned skip)
{
    struct native_trace trace;
    if(!stream)return 0;
    trace.stream=stream;trace.skip=skip;trace.visited=trace.written=0;
    _Unwind_Backtrace(frame,&trace);
    if(!trace.written)fputs("  native unwind provided no frames\n",stream);
    return trace.written;
}
void stack_walk(short levels_to_ignore)
{
    fputs("native engine stack:\n",stderr);
    mac_engine_stack_trace(stderr,(unsigned)(levels_to_ignore<0?0:levels_to_ignore)+1);
}
