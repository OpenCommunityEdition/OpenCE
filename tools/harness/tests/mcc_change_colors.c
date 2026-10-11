#include <stdio.h>
#include <stdlib.h>
typedef float real;
typedef int boolean;
#define TRUE 1
#define FALSE 0
#define MIN(a,b) ((a)<(b)?(a):(b))
#define PIN(x,a,b) ((x)<(a)?(a):(x)>(b)?(b):(x))
#define TEST_FLAG(v,b) ((v)&(1u<<(b)))
#define _object_runtime_scaled_change_colors_bit 0
#define NUMBER_OF_OBJECT_CHANGE_COLORS 4
#define TAG_BLOCK_GET_ELEMENT(b,i,t) ((t *)(b)->address+(i))
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
union real_rgb_color {struct {real red,green,blue;};};
typedef union real_rgb_color real_rgb_color;
struct object_change_color_definition {short darken_by,scaled_by;unsigned long scale_flags;real_rgb_color color_lower_bound,color_upper_bound;};
struct object_definition {struct {unsigned long runtime_flags;struct {long count;void *address;} change_colors;} object;};
struct object_datum {long definition_index;struct {real_rgb_color outgoing_change_colors[4];real values[2];} object;};
static struct object_definition definition;
static struct object_datum obj;
static int loaded=1,calls;
static int mcc_cache_tags_loaded(void){return loaded;}
static struct object_datum *object_get(long i){(void)i;return &obj;}
static struct object_definition *object_definition_get(long i){(void)i;return &definition;}
static void object_definition_data_error(long i,char const *msg){(void)i;(void)msg;CHECK(0);}
#define OBJECT_INCOMING_FUNCTION_GET_VALUE(o,i) ((o)->object.values[(i)-1])
static void rgb_colors_interpolate(real_rgb_color *out,unsigned long flags,real_rgb_color const *lo,real_rgb_color const *hi,real f){
    (void)flags;calls++;
    out->red=(1-f)*lo->red+f*hi->red;out->green=(1-f)*lo->green+f*hi->green;out->blue=(1-f)*lo->blue+f*hi->blue;
}
/* IMPLEMENTATION */
int main(void){
    struct object_change_color_definition color={0,1,0,{ { .101961002f,.101961002f,1 } },{ { .309803993f,.309803993f,1 } }};
    int pass;
    definition.object.runtime_flags=1;definition.object.change_colors.count=1;definition.object.change_colors.address=&color;
    for(pass=0;pass<2;pass++){
        loaded=pass;calls=0;obj.object.values[0]=-.528170288f;
        object_compute_change_colors(0);
        CHECK(obj.object.outgoing_change_colors[0].red==0&&obj.object.outgoing_change_colors[0].blue==1);
        CHECK(calls==!loaded);
    }
    /* Preserve extrapolation before darkening, rather than clamping the factor. */
    color.color_lower_bound.red=.2f;color.color_upper_bound.red=.8f;color.darken_by=2;
    obj.object.values[0]=2;obj.object.values[1]=.5f;calls=0;
    object_compute_change_colors(0);
    CHECK(obj.object.outgoing_change_colors[0].red>.6999f&&obj.object.outgoing_change_colors[0].red<.7001f&&calls==0);
    obj.object.values[0]=.5f;object_compute_change_colors(0);CHECK(calls==1);
    color.scale_flags=1;obj.object.values[0]=2;object_compute_change_colors(0);CHECK(calls==2);
    return 0;
}
