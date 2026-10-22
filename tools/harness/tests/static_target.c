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
 } else return 2;
 return 0;
}
