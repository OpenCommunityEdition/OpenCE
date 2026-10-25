#include "harness.h"
/* network_damage.c's static_target: the map's scenery and devices whose hits
every machine shares, those with a body damage can destroy and those whose
shield is all it can break. Objects: 0 scenery with a body, 1 scenery with a
shield only (an energy shield, a tripmine), 2 scenery with neither, 3 a device
with a shield only, 4 a unit with a shield (not one: its own state is sent) */
enum { _object_mask_scenery = 1, _object_mask_device = 2, _object_mask_unit = 4 };
#define STATIC_TARGET_TYPES (_object_mask_scenery | _object_mask_device)
static struct { long mask; real body, shield; } objects[] = {
 { _object_mask_scenery, 50.0f, 0.0f }, { _object_mask_scenery, 0.0f, 80.0f }, { _object_mask_scenery, 0.0f, 0.0f },
 { _object_mask_device, 0.0f, 20.0f }, { _object_mask_unit, 75.0f, 75.0f },
};
static void *object_try_and_get_and_verify_type(long object_index,long mask)
{
 return object_index>=0&&object_index<5&&(objects[object_index].mask&mask)?&objects[object_index]:NULL;
}
static real object_get_maximum_body_vitality(long object_index,boolean ignore_difficulty)
{
 (void)ignore_difficulty;return objects[object_index].body;
}
static real object_get_maximum_shield_vitality(long object_index,boolean ignore_difficulty)
{
 (void)ignore_difficulty;return objects[object_index].shield;
}
struct damage_data { long owner_object_index; };
/* where each object is, and its size (static_target_near) */
static real_point3d origins[] = { { 0, 0, 0 }, { 10, 0, 0 }, { 10.5f, 0, 0 }, { 13, 0, 0 }, { 10.1f, 0, 0 } };
struct object_datum { struct { real bounding_sphere_radius; } object; };
static struct object_datum datums[5] = { { { 0.2f } }, { { 0.2f } }, { { 0.2f } }, { { 0.2f } }, { { 0.5f } } };
#define object_get(index) (&datums[index])
static void object_get_origin(long object_index,real_point3d *origin){*origin=origins[object_index];}
struct object_iterator { long index; long next; unsigned long mask; };
static void object_iterator_new(struct object_iterator *iterator,unsigned long mask,byte flags)
{
 (void)flags;iterator->next=0;iterator->mask=mask;iterator->index=NONE;
}
static void *object_iterator_next(struct object_iterator *iterator)
{
 while(iterator->next<5){long index=iterator->next++;if(objects[index].mask&iterator->mask){iterator->index=index;return &datums[index];}}
 return NULL;
}
#include "under_test.inc"
int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";
 CASE("body") {
  CHECK(static_target(0),"scenery with a body");
 } else CASE("shield") {
  CHECK(static_target(1),"scenery with a shield only");
  CHECK(static_target(3),"a device with a shield only");
 } else CASE("neither") {
  CHECK(!static_target(2),"scenery damage cannot break");
  CHECK(!static_target(4),"a unit, whose state the host sends");
  CHECK(!static_target(NONE),"no object");
 } else CASE("damage") {
  /* a tripmine's own explosion (its owner the tripmine), and a player's grenade's (the grenade, no static target) */
  struct damage_data tripmine={1},grenade={4},none={NONE},light={0};
  CHECK(static_target_damage(&tripmine),"a shield scenery's own damage");
  CHECK(static_target_damage(&light),"a destructible scenery's own damage");
  CHECK(!static_target_damage(&grenade),"what a unit or projectile owns");
  CHECK(!static_target_damage(&none),"damage with no owner object");
 } else CASE("near") {
  /* another machine's copy of a tripmine (object 1, a shield) where this machine has it, and not too far */
  real_point3d at={10.3f,0,0},away={11.6f,0,0},nothing={30,0,0};
  CHECK(static_target_near(&at,1.0f)==1,"the nearest static target, %ld",static_target_near(&at,1.0f));
  CHECK(static_target_near(&away,1.0f)==NONE,"no unit, nothing damage cannot break, none farther");
  CHECK(static_target_near(&nothing,1.0f)==NONE,"none within reach");
  CHECK(static_target_near(&at,0.0f)==NONE,"its size and the reach only");
  { real_point3d between={12,0,0}; CHECK(static_target_near(&between,2.0f)==3,"the nearer of two, %ld",static_target_near(&between,2.0f)); }
 } else return 2;
 return 0;
}
