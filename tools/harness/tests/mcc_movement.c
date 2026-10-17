#include "cseries.h"
#include "units/units.h"
#include "items/weapons.h"
#include "items/weapon_definitions.h"
#include "mcc_movement.h"
#include <stdio.h>
static boolean active;
static struct unit_datum unit;
static struct weapon_datum weapon;
static struct weapon_definition definition;
boolean mcc_cache_tags_loaded(void) {return active;}
void *object_try_and_get_and_verify_type(long index,unsigned long mask) {
    if(index==1 && mask==_object_mask_unit)return &unit;
    if(index==2 && mask==_object_mask_weapon)return &weapon;
    return NULL;
}
void *tag_get(long group,long index) {return group==WEAPON_DEFINITION_TAG && index==3 ? &definition : NULL;}
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    real_vector3d v;
    union {unsigned long bits;real number;} invalid;
    unit.object.parent_object_index=NONE;
    unit.unit.player_index=0;unit.unit.current_weapon_index=0;
    unit.unit.weapon_object_indices[0]=2;unit.unit.current_zoom_level=NONE;
    weapon.definition_index=3;
    definition.weapon.forward_movement_penalty=.333f;
    definition.weapon.sideways_movement_penalty=.5f;
    v.i=1;v.j=-1;v.k=5;mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==-1 && v.k==5);
    active=TRUE;mcc_movement_apply(1,&v);CHECK(v.i==1-.333f && v.j==-.5f && v.k==5);
    definition.weapon.movement_penalty_mode=1;
    v.i=-1;v.j=1;mcc_movement_apply(1,&v);CHECK(v.i==-1 && v.j==1);
    unit.unit.current_zoom_level=0;mcc_movement_apply(1,&v);CHECK(v.i==-(1-.333f) && v.j==.5f);
    definition.weapon.movement_penalty_mode=2;unit.unit.current_zoom_level=NONE;
    weapon.weapon.magazines[1].state=_magazine_reloading;
    v.i=1;v.j=1;mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    unit.unit.current_zoom_level=0;mcc_movement_apply(1,&v);CHECK(v.i==1-.333f && v.j==.5f);
    unit.unit.current_zoom_level=NONE;weapon.weapon.magazines[1].state=_magazine_idle;
    v.i=1;v.j=1;mcc_movement_apply(1,&v);CHECK(v.i==1-.333f && v.j==.5f);
    definition.weapon.movement_penalty_mode=0;
    definition.weapon.forward_movement_penalty=0;definition.weapon.sideways_movement_penalty=.5f;
    v.i=1;v.j=1;mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==.5f);
    definition.weapon.forward_movement_penalty=2;definition.weapon.sideways_movement_penalty=1;
    v.i=1;v.j=1;mcc_movement_apply(1,&v);CHECK(v.i==-1 && v.j==0);
    invalid.bits=0x7fc00000;definition.weapon.forward_movement_penalty=invalid.number;
    definition.weapon.sideways_movement_penalty=-10;
    v.i=1;v.j=1;mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    definition.weapon.forward_movement_penalty=1;definition.weapon.sideways_movement_penalty=1;
    unit.unit.current_weapon_index=MAXIMUM_WEAPONS_PER_UNIT;
    mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    unit.unit.current_weapon_index=0;unit.unit.weapon_object_indices[0]=NONE;
    mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    unit.unit.weapon_object_indices[0]=2;unit.object.parent_object_index=12;
    mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    unit.object.parent_object_index=NONE;unit.unit.player_index=NONE;
    mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    unit.unit.player_index=0;active=FALSE;
    mcc_movement_apply(1,&v);CHECK(v.i==1 && v.j==1);
    return 0;
}
