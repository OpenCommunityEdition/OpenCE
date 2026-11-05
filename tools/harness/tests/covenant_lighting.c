#include "harness.h"
#define NUMBEROF(a) (sizeof(a)/sizeof((a)[0]))
#include <string.h>
#define MAX(a,b) ((a) > (b) ? (a) : (b))
typedef struct { real red, green, blue; } real_rgb_color;
typedef struct { real elements[13]; } real_matrix4x3;
enum { _object_type_projectile = 5, _object_mask_projectile = 1, _object_invisible_bit = 0,
 MAXIMUM_NUMBER_OF_ATTACHMENTS_PER_OBJECT = 8 };
struct object_datum { long definition_index; struct { short type; long flags; struct { int size; } node_matrices; real_point3d bounding_sphere_center; } object; };
struct object_attachment_definition { struct { long group_tag, index; } type; };
struct block { int count; };
struct object_definition { struct { struct block attachments; struct { long index; } model; } object; };
static struct object_attachment_definition authored_attachment = {{'ligh',99}};
#define TAG_BLOCK_GET_ELEMENT(b,i,t) (&authored_attachment)
static struct object_definition definition;
struct observer_result { real_point3d position; };
static struct observer_result camera;
static real_vector3d zero, forward = {1,0,0};
static const real_vector3d *global_zero_vector3d = &zero, *global_forward3d = &forward;
static struct object_datum objects[10];
static int count = 10, enabled = 1, allocs, deletes, disconnects, submits, fail, missing, tag_queries;
static const char *name = "weapons\\needler\\needle";
static struct { int active, connected; long owner; real red; } storage[64];
struct object_iterator { long index; };
static void object_iterator_new(struct object_iterator *i, int mask, int flags) { i->index = -1; }
static void *object_iterator_next(struct object_iterator *i) { return ++i->index < count ? &objects[i->index] : NULL; }
static struct object_datum *object_try_and_get(long i) { return i >= 0 && i < count ? &objects[i] : NULL; }
static const char *tag_get_name(long i) { tag_queries++; return name; }
static long exhaust_tag=77;
static int exhaust_queries;
static long tag_loaded(long group,const char *tag)
{
 CHECK(group=='ligh'&&!strcmp(tag,"weapons\\fuel rod gun\\fuel rod exhaust"),"only the known flight light is resolved");
 exhaust_queries++;return exhaust_tag;
}
static struct object_definition *object_definition_get(long i) { return &definition; }
static short local_player_get_next(short i) { return i == NONE ? 0 : NONE; }
static const struct observer_result *observer_get_camera(short i) { return &camera; }
static real distance3d(const real_point3d *a, const real_point3d *b) { return a->x - b->x; }
int lighting_config_boolean(const char *n) { return enabled; }
long light_port_glow_definition_get(void) { return missing ? NONE : 99; }
static long light_new_unattached(long d,long o,short n,const real_point3d *p,const real_vector3d *v,real s)
{
 if (fail) return NONE;
 storage[allocs].active=storage[allocs].connected=1; storage[allocs].owner=o;
 return 100+allocs++;
}
boolean light_port_glow_set(long l,long o,const real_rgb_color *c,real r)
{
 int i=(int)l-100;
 if (i<0 || i>=allocs || !storage[i].active || storage[i].owner!=o) return FALSE;
 storage[i].red=c->red; return TRUE;
}
static void light_delete(long l) { storage[l-100].active=0; deletes++; }
static void light_disconnect_from_map(long l) { CHECK(storage[l-100].connected,"disconnect requires connected light"); storage[l-100].connected=0; disconnects++; }
static void light_reconnect_to_map(long l) { CHECK(!storage[l-100].connected,"connect requires disconnected light"); storage[l-100].connected=1; }
void plasma_occlusion_submit(long l,long r) { submits++; }
#include "under_test.inc"
static void color_checks(void)
{
 real_rgb_color authored={.6f,.8f,.5f},color;
 name="weapons\\fuel rod gun\\fuel rod";
 color=authored;enabled=0;covenant_lighting_color(0,77,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color))&&exhaust_queries==0,"disabled colour stays authored without resolving tags");
 enabled=1;color=authored;covenant_lighting_color(NONE,77,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color)),"ownerless effect untouched");
 objects[0].object.type=0;covenant_lighting_color(0,77,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color)),"non-projectile untouched");objects[0].object.type=5;
 name="vehicles\\banshee\\banshee fuel rod";covenant_lighting_color(0,77,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color)),"other fuel-rod profiles keep their colour");
 name="weapons\\fuel rod gun\\fuel rod";covenant_lighting_color(0,78,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color)),"unrelated attached light untouched");
 covenant_lighting_color(0,77,&color);
 CHECK(color.red==authored.red*.30f&&color.green==authored.green&&color.blue==authored.blue*.20f,"flight light is greener with unchanged green intensity");
 CHECK(authored.red==.6f&&authored.green==.8f&&authored.blue==.5f,"authored source remains unchanged");
 color=authored;covenant_lighting_color(0,77,&color);
 CHECK(color.red==authored.red*.30f&&exhaust_queries==1,"fresh submissions do not compound and tag lookup is cached");
 color.red=color.green=color.blue=0;covenant_lighting_color(0,77,&color);
 CHECK(color.red==0&&color.green==0&&color.blue==0,"authored dark intervals stay dark");
 covenant_lighting_forget();exhaust_tag=NONE;color=authored;covenant_lighting_color(0,77,&color);
 covenant_lighting_color(0,77,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color))&&exhaust_queries==2,"missing map tag stays stock and is cached");
 covenant_lighting_forget();exhaust_tag=78;covenant_lighting_color(0,77,&color);
 CHECK(!memcmp(&color,&authored,sizeof(color)),"old map handle is rejected after transition");
 covenant_lighting_color(0,78,&color);
 CHECK(color.red==authored.red*.30f&&exhaust_queries==3,"new map handle is resolved after transition");
}
int main(int argc,char **argv)
{
 int i, before;
 long held;
 for(i=0;i<10;i++) { objects[i].object.node_matrices.size=sizeof(real_matrix4x3); objects[i].object.type=5; objects[i].object.bounding_sphere_center.x=1; }
 if(argc>1&&!strcmp(argv[1],"color")) { color_checks();return 0; }
 CHECK(covenant_lighting_scale(NULL)==0 && covenant_lighting_scale("unknown")==0,"exact profile admission");
 for(i=0;i<NUMBEROF(covenant_profiles);i++) CHECK(covenant_lighting_scale(covenant_profiles[i].tag)==covenant_profiles[i].scale,"all authored profiles");
 covenant_lighting_update(); CHECK(allocs==8,"bounded eight owned lights");
 CHECK(tag_queries==count,"one tag query per projectile during admission, including profile zero");
 CHECK(covenant_lighting_radius(100,0,0.25f)==1.0f,"owned radius covers template");
 CHECK(covenant_lighting_radius(100,1,0.25f)==0.25f,"wrong owner keeps bounds");
 covenant_lighting_window_begin();
 for(i=0;i<8;i++) { covenant_lighting_submitted(100+i,i); CHECK(covenant_lighting_selected(i),"submitted covenant selected"); }
 CHECK(covenant_lighting_owns(100,0) && !covenant_lighting_owns(100,1),"owned identity admission");
 CHECK(submits==8 && !covenant_lighting_selected(NONE),"only valid submissions");
 covenant_lighting_frame_end(); CHECK(disconnects==8,"owned lights disconnected before simulation");
 for(i=0;i<8;i++) CHECK(storage[i].red==0 && !covenant_lighting_selected(i),"dark and unselected between frames");
 covenant_lighting_frame_end(); CHECK(disconnects==8,"repeated frame cleanup is safe");
 tag_queries=0;
 covenant_lighting_update(); CHECK(storage[0].connected,"reconnect before engine scene pass");
 CHECK(tag_queries==count+8,"only ownership validation and admission resolve each retained projectile");
 covenant_lighting_update(); CHECK(allocs==8,"stable identities across views/frames");
 enabled=0; covenant_lighting_update(); CHECK(deletes==8,"disable releases owned lights");
 covenant_lighting_update(); CHECK(allocs==8,"disabled never allocates");
 enabled=1; count=1; covenant_lighting_update(); CHECK(allocs==9,"re-enable allocates");
 count=0; covenant_lighting_update(); CHECK(deletes==9,"expired covenant cleaned up");
 count=1; covenant_lighting_update(); held=covenant_lights[0].light_index;
 storage[held-100].owner=42; before=deletes;
 covenant_lighting_update(); CHECK(deletes==before && allocs==11,"stale owner never deleted or reused");
 covenant_lighting_submitted(covenant_lights[0].light_index,2);
 covenant_lighting_forget(); CHECK(!covenant_lighting_selected(2),"map/recovery forgets all handles");
 before=allocs; missing=1; covenant_lighting_update(); CHECK(allocs==before,"missing template fallback");
 missing=0; fail=1; covenant_lighting_update(); CHECK(allocs==before,"allocation failure fallback");
 fail=0; definition.object.attachments.count=1; covenant_lighting_update(); CHECK(allocs==before,"authored light prevents duplicate");
 definition.object.attachments.count=0; name="weapons\\covenant launcher\\explosion";
 covenant_lighting_update(); CHECK(allocs==before,"explosion cannot acquire flight light");
 name="weapons\\needler\\needle"; objects[0].object.bounding_sphere_center.x=30;
 covenant_lighting_update(); CHECK(allocs==before,"distant covenants leave capacity free");
 objects[0].object.bounding_sphere_center.x=1; covenant_lighting_update();
 before=deletes; enabled=0; covenant_lighting_frame_end(); CHECK(deletes==before+1,"disable at frame end releases latest owned light");
	enabled=1; objects[0].object.flags=1; before=allocs;
	covenant_lighting_update(); CHECK(allocs==before,"hidden modeled needle stays dark");
	name="vehicles\\wraith\\wraith mortar"; definition.object.model.index=NONE;
	covenant_lighting_update(); CHECK(allocs==before+1,"particle-only Wraith remains eligible");
 CHECK(covenant_lighting_radius(covenant_lights[0].light_index,0,.2f)==3.5f,"wraith bounds");
 return 0;
}
