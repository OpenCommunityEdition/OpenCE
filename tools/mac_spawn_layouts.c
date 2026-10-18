#include "cseries.h"
#include "units/biped_definitions.h"
#include "items/weapon_definitions.h"
#include <stdio.h>
static unsigned checks,failures;
#define O(t,f,v) do { ++checks; if(offsetof(struct t,f)!=(v)) { ++failures;printf("FAIL %s.%s offset=%lu expected=%lu\n",#t,#f,(unsigned long)offsetof(struct t,f),(unsigned long)(v)); } }while(0)
#define S(t,v) do { ++checks; if(sizeof(struct t)!=(v)) { ++failures;printf("FAIL sizeof %s=%lu expected=%lu\n",#t,(unsigned long)sizeof(struct t),(unsigned long)(v)); } }while(0)
int main(void)
{
    S(_object_definition,0x17c);S(_unit_definition,0x174);S(_biped_definition,0x204);S(biped_definition,0x4f4);
    S(weapon_definition,0x508);S(weapon_magazine_definition,112);S(weapon_trigger_definition,276);S(weapon_ammunition_object,28);
    O(_object_definition,type,0);O(_object_definition,flags,2);O(_object_definition,bounding_radius,4);
    O(_object_definition,bounding_offset,8);O(_object_definition,origin_offset,20);O(_object_definition,acceleration_scale,32);
    O(_object_definition,runtime_flags,36);O(_object_definition,model,0x28);O(_object_definition,animation_graph,0x38);
    O(_object_definition,collision_model,0x70);O(_object_definition,physics,0x80);O(_object_definition,modifier_shader,0x90);
    O(_object_definition,creation_effect,0xa0);O(_object_definition,render_bounding_radius,260);O(_object_definition,forced_shader_permutation_index,318);
    O(_object_definition,attachments,0x140);O(_object_definition,widgets,0x14c);O(_object_definition,functions,0x158);
    O(_object_definition,change_colors,0x164);O(_object_definition,predicted_resources,0x170);
    O(biped_definition,unit.flags,0x17c);O(biped_definition,unit.default_team,0x180);O(biped_definition,biped.flags,0x2f4);
    O(biped_definition,unit.grenade_type,0x2c4);O(biped_definition,unit.grenade_count,0x2c6);O(biped_definition,unit.initial_weapons,0x2d8);
    O(biped_definition,biped.runtime_pelvis_node_index,0x4e4);O(biped_definition,biped.runtime_head_node_index,0x4e6);
    O(biped_definition,biped.standing_camera_height,0x400);O(biped_definition,biped.crouching_camera_height,0x404);
    O(biped_definition,biped.collision_height_standing,0x424);O(biped_definition,biped.collision_height_crouching,0x428);
    O(biped_definition,biped.collision_radius,0x42c);O(biped_definition,biped.contact_points,0x4e8);
    O(weapon_definition,weapon.flags,0x308);O(weapon_definition,weapon.label,0x30c);
    O(weapon_definition,weapon.interface_definition.first_person_model,0x45c);
    O(weapon_definition,weapon.interface_definition.first_person_animations,0x46c);
    O(weapon_definition,weapon.interface_definition.hud_interface,0x480);
    O(weapon_definition,weapon.magazines,0x4f0);O(weapon_definition,weapon.triggers,0x4fc);
    O(weapon_magazine_definition,flags,0);O(weapon_magazine_definition,rounds_recharged_per_second,4);
    O(weapon_magazine_definition,rounds_total_initial,6);O(weapon_magazine_definition,rounds_total_maximum,8);
    O(weapon_magazine_definition,rounds_loaded_maximum,10);O(weapon_magazine_definition,reload_time,20);
    O(weapon_magazine_definition,rounds_reloaded,24);O(weapon_magazine_definition,chamber_time,28);
    O(weapon_magazine_definition,reloading_effect,56);O(weapon_magazine_definition,chambering_effect,72);
    O(weapon_magazine_definition,ammunition_objects,100);O(weapon_ammunition_object,rounds,0);O(weapon_ammunition_object,object,12);
    O(weapon_trigger_definition,flags,0);O(weapon_trigger_definition,initial_rate_of_fire,4);O(weapon_trigger_definition,final_rate_of_fire,8);
    O(weapon_trigger_definition,magazine_index,32);O(weapon_trigger_definition,rounds_per_shot,34);
    O(weapon_trigger_definition,minimum_rounds_loaded_per_shot,36);O(weapon_trigger_definition,rounds_between_tracers,38);
    O(weapon_trigger_definition,charging_effect,92);O(weapon_trigger_definition,projectiles_per_shot,110);O(weapon_trigger_definition,projectile,148);
    printf("actual constructor header layouts: checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
