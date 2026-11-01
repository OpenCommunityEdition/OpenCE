#include <stdio.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define NONE -1
#define FLAG(n) (1UL << (n))
#define TEST_FLAG(v,n) (((v) & FLAG(n)) != 0)
#define VALID_INDEX(i,n) ((i) >= 0 && (i) < (n))
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&((t *)(b)->address)[i])
typedef int boolean;
enum {_director_perspective_first_person, third_person};
struct object_marker {float x, y, z;};
struct tag_block {int count;void *address;};
struct object_attachment_definition {char marker_name[32];};
struct weapon_datum {long definition_index;};
struct object_definition {struct {struct tag_block attachments;} object;};
static struct weapon_datum weapon = {42};
static struct object_attachment_definition attachment = {"spawn fire"};
static struct object_definition definition = {{{1, &attachment}}};
static int active = 1, exists = 1, local = 0, perspective, reads, queries, marker_exists = 1;
static int mcc_cache_tags_loaded(void) {return active;}
static struct weapon_datum *weapon_try_and_get(long index) {
    ++reads;return exists && index == 123 ? &weapon : NULL;
}
static long first_person_weapon_get_local_index(long index) {(void)index;return local;}
static int director_get_perspective(short index) {(void)index;return perspective;}
static struct object_definition *object_definition_get(long index) {(void)index;return &definition;}
static short first_person_weapon_get_marker_by_name(long index, char const *name,
    struct object_marker *markers, short maximum) {
    ++queries;
    if (index != 123 || strcmp(name,"spawn fire") || maximum <= 0 || !marker_exists) return 0;
    markers[0].x=10;markers[0].y=20;markers[0].z=30;
    return 1;
}
/* IMPLEMENTATION */
#define CHECK(v) do {if(!(v)){fprintf(stderr,"line %d\n",__LINE__);return 1;}} while(0)
int main(int argc,char **argv) {
    unsigned long fp=FLAG(_particle_system_type_nodraw_third_person_bit);
    unsigned long world=FLAG(_particle_system_type_nodraw_first_person_bit);
    struct object_marker markers[2]={{1,2,3},{4,5,6}};
    int viewer, flags;
    if(argc!=2)return 2;
    if(!strcmp(argv[1],"markers")) {
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,0)==1);
        CHECK(markers[0].z==30 && markers[1].z==6 && queries==1);
        markers[0].z=3;
        CHECK(mcc_particle_system_markers(123,0,world,markers,1,1)==1);
        CHECK(mcc_particle_system_markers(123,0,0,markers,1,2)==2);
        CHECK(markers[0].z==3 && queries==1);
        marker_exists=0;CHECK(mcc_particle_system_markers(123,0,fp,markers,1,1)==0);
        CHECK(markers[0].z==3);
        CHECK(mcc_particle_system_markers(123,0,fp|world,markers,1,1)==0);
    } else if(!strcmp(argv[1],"views")) {
        local=2;
        for(viewer=-1;viewer<4;viewer++)for(flags=0;flags<4;flags++) {
            unsigned long mask=(flags&1 ? world:0)|(flags&2 ? fp:0);
            CHECK(mcc_particle_system_visible(123,mask,(short)viewer)==
                !(flags & (viewer==2 ? 1:2)));
        }
        perspective=third_person;
        CHECK(!mcc_particle_system_visible(123,fp,2));
        CHECK(mcc_particle_system_visible(123,world,2));
        CHECK(!mcc_particle_system_visible(NONE,fp,2));
        CHECK(mcc_particle_system_visible(NONE,world,2));
    } else if(!strcmp(argv[1],"invalid")) {
        CHECK(mcc_particle_system_markers(NONE,0,fp,markers,1,1)==0);
        CHECK(mcc_particle_system_markers(456,0,fp,markers,1,1)==0);
        CHECK(mcc_particle_system_markers(123,-1,fp,markers,1,1)==0);
        CHECK(mcc_particle_system_markers(123,1,fp,markers,1,1)==0);
        CHECK(mcc_particle_system_markers(123,0,fp,markers,0,1)==0);
        CHECK(queries==0 && markers[0].z==3);
    } else if(!strcmp(argv[1],"lifecycle")) {
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,0)==1);
        local=NONE; /* Holstered, zoomed, remote, or not yet visible. */
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,1)==0);
        CHECK(!mcc_particle_system_visible(123,fp,0));
        local=0;perspective=third_person;
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,1)==0);
        perspective=0;exists=0; /* Deleted weapon must not expose a stale FP cache. */
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,1)==0);
        CHECK(!mcc_particle_system_visible(123,fp,0));
        exists=1;active=0;
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,0)==0);
        CHECK(mcc_particle_system_visible(123,fp,3));
        active=1;
        CHECK(mcc_particle_system_markers(123,0,fp,markers,1,0)==1);
        CHECK(mcc_particle_system_visible(123,fp,0));
    } else if(!strcmp(argv[1],"legacy")) {
        active=0; /* Xbox and CE retain all native results without reading objects. */
        for(flags=0;flags<4;flags++) {
            unsigned long mask=(flags&1 ? world:0)|(flags&2 ? fp:0);
            CHECK(mcc_particle_system_markers(456,-1,mask,markers,1,2)==2);
            CHECK(mcc_particle_system_visible(456,mask,0));
        }
        CHECK(reads==0 && queries==0 && markers[0].z==3);
    } else return 3;
    return 0;
}
