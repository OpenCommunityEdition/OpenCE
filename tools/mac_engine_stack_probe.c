#include "engine_stack.h"
#include <string.h>
static volatile unsigned sink;
static __attribute__((noinline)) unsigned trace_leaf(FILE *stream)
{unsigned count=mac_engine_stack_trace(stream,0);sink=count;return count;}
static __attribute__((noinline)) unsigned trace_parent(FILE *stream)
{unsigned count=trace_leaf(stream);sink=count+1;return count;}
int main(void)
{
    FILE *stream=tmpfile();char line[128];unsigned count,lines=0;
    if(!stream)return 2;
    count=trace_parent(stream);
    if(count<3 || count>128){fprintf(stderr,"unexpected native frames: %u\n",count);return 1;}
    rewind(stream);while(fgets(line,sizeof(line),stream)){if(!strstr(line,"0x"))return 1;++lines;}
    if(lines!=count)return 1;
    if(mac_engine_stack_trace(NULL,0)!=0)return 1;
    if(mac_engine_stack_trace(stream,129)!=0)return 1;
    fclose(stream);printf("native stack: 5 checks, 0 failures, %u observed frames\n",count);return 0;
}
