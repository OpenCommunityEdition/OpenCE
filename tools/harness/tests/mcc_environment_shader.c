#include "cseries.h"
#include "mcc_environment_shader.h"
#include <stdio.h>
static boolean loaded;
static int enabled;
boolean mcc_cache_tags_loaded(void) {return loaded;}
void mcc_environment_shader_set(int value) {enabled=value;}
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"line %d\n",__LINE__);return 1;}} while (0)
int main(void) {
    /* Xbox and CE must ignore an identical reserved flag. */
    mcc_environment_shader_begin(8);CHECK(!enabled);
    loaded=TRUE;mcc_environment_shader_begin(8);CHECK(enabled);
    mcc_environment_shader_end();CHECK(!enabled);
    mcc_environment_shader_begin(0);CHECK(!enabled);
    mcc_environment_shader_begin(9);CHECK(enabled);
    mcc_environment_shader_begin(10);CHECK(!enabled);
    mcc_environment_shader_begin(8);CHECK(enabled);
    loaded=FALSE;mcc_environment_shader_begin(8);CHECK(!enabled);
    return 0;
}
