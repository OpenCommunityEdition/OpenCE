#include "cseries.h"
#include "engine_services.h"
#undef malloc
#undef free
#undef realloc
#include <stdlib.h>
#include <string.h>
extern void *system_malloc(long);
extern void system_free(void *);
static unsigned checks,failures;
static unsigned char arena[16];
static int owns(const void *p){return p==arena;}
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"heap FAIL %s\n",#x);}}while(0)
int main(void)
{
    unsigned char *p,*grown;unsigned i;
    p=debug_malloc(32,TRUE,__FILE__,__LINE__);CHECK(p!=NULL);
    if(!p)return 1;
    for(i=0;i<32;++i){CHECK(p[i]==0);p[i]=(unsigned char)(i+1);}
    grown=debug_realloc(p,4096,__FILE__,__LINE__);CHECK(grown!=NULL);
    if(!grown){debug_free(p,__FILE__,__LINE__);return 1;}p=grown;
    for(i=0;i<32;++i)CHECK(p[i]==i+1);
    grown=debug_realloc(p,8,__FILE__,__LINE__);CHECK(grown!=NULL);
    if(!grown){debug_free(p,__FILE__,__LINE__);return 1;}p=grown;
    for(i=0;i<8;++i)CHECK(p[i]==i+1);
    CHECK(debug_realloc(p,0,__FILE__,__LINE__)==NULL);
    CHECK(debug_realloc(NULL,0,__FILE__,__LINE__)==NULL);
    p=debug_realloc(NULL,16,__FILE__,__LINE__);CHECK(p!=NULL);debug_free(p,__FILE__,__LINE__);
    mac_engine_services_set_arena_ownership(owns);arena[0]=73;
    CHECK(debug_realloc(arena,64,__FILE__,__LINE__)==NULL && arena[0]==73);
    debug_free(arena,__FILE__,__LINE__);CHECK(arena[0]==73);
    mac_engine_services_set_arena_ownership(NULL);
    CHECK(system_malloc(-1)==NULL);CHECK(system_malloc(0)==NULL);
    p=system_malloc(128);CHECK(p!=NULL);if(p){memset(p,0x5a,128);CHECK(p[127]==0x5a);system_free(p);}
    system_free(NULL);
    printf("native heap: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
