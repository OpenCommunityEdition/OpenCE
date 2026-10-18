#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void posix_random_bytes(void *,unsigned int);
void OutputDebugStringA(const char *);
int main(void)
{
    unsigned char *data=(unsigned char *)malloc(131074U);
    unsigned char small[34];
    int failures=0;
    if(!data)return 2;
    memset(data,0xa5,131074U);memset(small,0x5a,sizeof(small));
    posix_random_bytes(NULL,0);
    posix_random_bytes(small+1,32);
    posix_random_bytes(data+1,131072U);
    if(small[0]!=0x5a||small[33]!=0x5a)++failures;
    if(data[0]!=0xa5||data[131073]!=0xa5)++failures;
    /* Check actual writes without publishing random output or claiming a
       statistical entropy test. */
    if(!memcmp(data+1,data+65537,65536))++failures;
    OutputDebugStringA(NULL);
    OutputDebugStringA("native diagnostic output\n");
    free(data);
    printf("native host services: %d failures\n",failures);
    return failures?1:0;
}
