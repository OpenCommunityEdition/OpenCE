#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
typedef float real;
typedef unsigned char boolean;
typedef unsigned short word;
typedef struct {real x,y,z;} real_point3d;
typedef struct {real i,j,k;} real_vector3d;
typedef struct {real a,r,g,b;} real_argb_color;
struct shader_effect_definition {int sentinel;};
struct shader {int sentinel;};
struct bitmap_data {int id;};
/* HEADER */
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define FLAG(b) (1u<<(b))
#define TEST_FLAG(v,b) ((v)&FLAG(b))
#define SET_FLAG(v,b,t) ((v)=(t)?((v)|FLAG(b)):((v)&~FLAG(b)))
#define NUMBER_OF_VERTICES_PER_QUADRILATERAL 4
#define MAXIMUM_BUILD_SPRITE_GROUPS 8
#define _build_sprites_valid_bit NUMBER_OF_BUILD_SPRITES_FLAGS
#define _rasterizer_geometry_viewspace_bit 5
#define _rasterizer_geometry_first_person_bit 7
#define _rasterizer_lock_sprite 1
#define _rasterizer_lock_none 0
#define _error_silent 0
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static int assertions;
#define match_assert(f,l,x) do {if(!(x))assertions++;}while(0)
#define match_vassert(f,l,x,msg) match_assert(f,l,x)
static int loaded,ce,depth,draws,unlocks,deletes,allocations,fail_alloc;
static int expected_first_person;
static unsigned drawn[80];
static char vertices[80];
static struct {int current_lock_operation;} rasterizer_globals;
static struct {struct {int view_to_world;} frustum;} render;
static real_point3d origin;
static real_point3d const *global_origin3d=&origin;
static int mcc_cache_tags_loaded(void){return loaded;}
static int custom_edition_cache_tags_loaded(void){return ce;}
static void viewmodel_projection_begin(void){depth++;}
static void viewmodel_projection_end(void){CHECK(depth>0);depth--;}
static void matrix4x3_transform_point(int const *m,real_point3d const *in,real_point3d *out){(void)m;*out=*in;}
static int _texture_cache_bitmap_get_hardware_format(struct bitmap_data *b,int a,int c){(void)b;(void)a;(void)c;return 1;}
static long rasterizer_dynamic_vertices_new(int type,int count){CHECK(type==6);CHECK(count>0);return fail_alloc?NONE:allocations++;}
static void *rasterizer_dynamic_vertices_lock(long i){CHECK(i>=0&&i<80);return vertices+i;}
static void rasterizer_dynamic_vertices_unlock(long i){CHECK(i>=0&&i<allocations);unlocks++;}
static void rasterizer_dynamic_vertices_delete(long i){CHECK(i>=0&&i<allocations);deletes++;}
static void rasterizer_dynamic_screen_geometry_draw(int a,int b,long c,int d){(void)a;(void)b;(void)c;(void)d;CHECK(0);}
static void rasterizer_dynamic_unlit_geometry_draw(struct shader const *s,struct bitmap_data const *b,void *a,int q,long v,int n,real_point3d const *c,unsigned long flags){
    CHECK(s->sentinel==123&&a==NULL&&q==-4&&v>=0&&n>0);
    CHECK(c->x==2&&c->y==3&&c->z==4);
    CHECK(!!TEST_FLAG(flags,_rasterizer_geometry_first_person_bit)==expected_first_person);
    CHECK(b->id>=0&&b->id<80);drawn[b->id]+=(unsigned)n/2;draws++;
}
static char const *tag_get_name(long i){(void)i;return "test";}
static void error(int level,char const *fmt,...){(void)level;(void)fmt;}
/* IMPLEMENTATION */
int main(void){
    struct build_sprite_data data;
    struct shader_effect_definition shader={123};
    struct bitmap_data bitmaps[80];
    int i,pass;
    short g;
    for(i=0;i<80;i++)bitmaps[i].id=i;
    for(pass=0;pass<2;pass++){
        memset(&data,0,sizeof(data));memset(drawn,0,sizeof(drawn));
        loaded=1;expected_first_person=pass;allocations=draws=unlocks=deletes=0;
        build_sprites_begin(&data,65,77,&shader,pass?FLAG(_build_sprites_first_person_bit):0);
        for(i=0;i<65;i++){
            CHECK(data.sprite_count<data.maximum_sprite_count);
            g=build_sprite_get_group(&data,bitmaps+i);CHECK(g>=0&&g<8);
            CHECK(data.bitmap_group_index==77&&data.shader==&shader);
            CHECK(depth==pass&&data.group_count<=8);
            if(i==7)CHECK(build_sprite_get_group(&data,bitmaps)==0&&draws==0);
            data.groups[g].sprite_count++;data.sprite_count++;
            data.centroid.x+=2;data.centroid.y+=3;data.centroid.z+=4;
        }
        CHECK(data.sprite_count==data.maximum_sprite_count);
        build_sprites_end(&data);
        CHECK(draws==65&&unlocks==65&&deletes==65&&depth==0&&assertions==0);
        for(i=0;i<65;i++)CHECK(drawn[i]==1);
    }
    /* Original Xbox assertion and CE refusal still apply without MCC. */
    loaded=0;expected_first_person=0;allocations=0;
    for(pass=0;pass<2;pass++){
        ce=pass;build_sprites_begin(&data,10,77,&shader,0);
        for(i=0;i<8;i++)CHECK(build_sprite_get_group(&data,bitmaps+i)==i);
        CHECK(build_sprite_get_group(&data,bitmaps+8)==NONE);
        CHECK(assertions==1&&data.group_count==8&&data.maximum_sprite_count==10);
    }
    /* A failed allocation is not drawn and cannot leak a locked buffer. */
    loaded=1;ce=0;fail_alloc=1;draws=unlocks=deletes=0;
    build_sprites_begin(&data,10,77,&shader,0);
    for(i=0;i<17;i++)CHECK(build_sprite_get_group(&data,bitmaps+i)==NONE);
    CHECK(assertions==1&&draws==0&&unlocks==0&&deletes==0);
    return 0;
}
