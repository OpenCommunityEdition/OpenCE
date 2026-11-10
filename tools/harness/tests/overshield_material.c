/* The overshield's collision material an engineer's force field, as the active camouflage's is, not dirt
(port/linux/game/overshield_material.c) */
#include "harness.h"
#define TAG_STRING_LENGTH 31
typedef unsigned short word;
struct tag_block { long count; void *address; void *definition; };
#define TAG_BLOCK_GET_ELEMENT(block, index, type) ((type *)((char *)(block)->address + (index) * sizeof(type)))
struct tag_reference { long group_tag; char *name; long name_length; long index; };
struct damage_resistance { struct tag_block materials; };
struct collision_model { struct damage_resistance resistance; };
struct equipment_definition
{
    struct { struct tag_reference collision_model; } object;
    struct { short powerup_type; } equipment;
};
struct tag_iterator { long next; };
#define EQUIPMENT_DEFINITION_TAG 0x65716970
/* the map's equipment and collision models */
static struct equipment_definition equipment[8];
static long equipment_count;
static struct collision_model collision_models[8];

static void tag_iterator_new(struct tag_iterator *iterator, long group_tag) { iterator->next = 0; (void)group_tag; }
static long tag_iterator_next(struct tag_iterator *iterator)
{
    return iterator->next < equipment_count ? iterator->next++ : NONE;
}
static struct equipment_definition *equipment_definition_get(long index) { return &equipment[index]; }
static struct collision_model *collision_model_definition_get(long index) { return &collision_models[index]; }

#include "under_test.inc"

static void material_set(struct damage_resistance_material *material, char const *name, short type)
{
    memset(material, 0, sizeof(*material));
    strncpy(material->name, name, sizeof(material->name) - 1);
    material->material_type = type;
    material->shield_damage_multiplier = 1.f;
    material->body_damage_multiplier = 1.f;
}

static void collision_model_set(long index, struct damage_resistance_material *materials, long count)
{
    collision_models[index].resistance.materials.count = count;
    collision_models[index].resistance.materials.address = materials;
}

static void equipment_add(short powerup_type, long collision_model_index)
{
    memset(&equipment[equipment_count], 0, sizeof(equipment[equipment_count]));
    equipment[equipment_count].equipment.powerup_type = powerup_type;
    equipment[equipment_count].object.collision_model.index = collision_model_index;
    equipment_count++;
}

static boolean material_is(struct damage_resistance_material const *material, char const *name, short type)
{
    return !strcmp(material->name, name) && material->material_type == type &&
        material->shield_damage_multiplier == 1.f && material->body_damage_multiplier == 1.f;
}

int main(int argc, char **argv)
{
    char const *case_name = argc > 1 ? argv[1] : "";
    CASE("maps") {
        /* bloodgulch's: the overshield's collision model (powerups\active camoflage\active camoflage), the active
        camouflage's (powerups\over shield\over shield), the health pack's and the grenades' */
        struct damage_resistance_material overshield, camouflage, health, grenade;
        material_set(&overshield, "force field", _material_dirt);
        material_set(&camouflage, "force field", _material_engineer_force_field);
        material_set(&health, "default", _material_plastic);
        material_set(&grenade, "default", _material_dirt);
        collision_model_set(0, &overshield, 1);
        collision_model_set(1, &camouflage, 1);
        collision_model_set(2, &health, 1);
        collision_model_set(3, &grenade, 1);
        equipment_count = 0;
        equipment_add(_equipment_powerup_grenade, 3);
        equipment_add(_equipment_powerup_active_camouflage, 1);
        equipment_add(_equipment_powerup_overshield, 0);
        equipment_add(_equipment_powerup_health, 2);
        overshield_material_tags_loaded();
        CHECK(material_is(&overshield, "force field", _material_engineer_force_field),
            "the overshield's force field an engineer's (%d)", overshield.material_type);
        CHECK(material_is(&camouflage, "force field", _material_engineer_force_field), "the active camouflage's as it was");
        CHECK(material_is(&health, "default", _material_plastic), "the health pack's as it was");
        CHECK(material_is(&grenade, "default", _material_dirt), "the grenade's as it was");
        overshield_material_tags_loaded();
        CHECK(material_is(&overshield, "force field", _material_engineer_force_field), "loaded again, as it is");
        CHECK(material_is(&camouflage, "force field", _material_engineer_force_field), "the active camouflage's again");
    }
    CASE("overshield only") {
        /* a10, c20 and d40: an overshield, and no active camouflage */
        struct damage_resistance_material overshield;
        material_set(&overshield, "force field", _material_dirt);
        collision_model_set(0, &overshield, 1);
        equipment_count = 0;
        equipment_add(_equipment_powerup_overshield, 0);
        overshield_material_tags_loaded();
        CHECK(material_is(&overshield, "force field", _material_engineer_force_field), "with no active camouflage");
    }
    CASE("other materials") {
        struct damage_resistance_material materials[5];
        material_set(&materials[0], "force field", _material_glass);
        material_set(&materials[1], "ground", _material_dirt);
        material_set(&materials[2], "force fields", _material_dirt);
        material_set(&materials[3], "force field", _material_dirt);
        material_set(&materials[4], "force field", _material_engineer_force_field);
        collision_model_set(0, materials, 5);
        equipment_count = 0;
        equipment_add(_equipment_powerup_overshield, 0);
        overshield_material_tags_loaded();
        CHECK(material_is(&materials[0], "force field", _material_glass), "a force field of another type");
        CHECK(material_is(&materials[1], "ground", _material_dirt), "dirt of another name");
        CHECK(material_is(&materials[2], "force fields", _material_dirt), "a name that only begins so");
        CHECK(material_is(&materials[3], "force field", _material_engineer_force_field), "the force field left dirt");
        CHECK(material_is(&materials[4], "force field", _material_engineer_force_field), "one already an engineer's");
        CHECK(overshield_material_fixed(&materials[1]) == FALSE && overshield_material_fixed(&materials[3]) == FALSE,
            "nothing to fix is not fixed");
        material_set(&materials[3], "force field", _material_dirt);
        CHECK(overshield_material_fixed(&materials[3]) == TRUE &&
            materials[3].material_type == _material_engineer_force_field, "fixed, and so");
    }
    CASE("not an overshield") {
        struct damage_resistance_material camouflage, health, speed;
        material_set(&camouflage, "force field", _material_dirt);
        material_set(&health, "force field", _material_dirt);
        material_set(&speed, "force field", _material_dirt);
        collision_model_set(0, &camouflage, 1);
        collision_model_set(1, &health, 1);
        collision_model_set(2, &speed, 1);
        equipment_count = 0;
        equipment_add(_equipment_powerup_active_camouflage, 0);
        equipment_add(_equipment_powerup_health, 1);
        equipment_add(_equipment_powerup_double_speed, 2);
        overshield_material_tags_loaded();
        CHECK(material_is(&camouflage, "force field", _material_dirt), "an active camouflage's is the map's");
        CHECK(material_is(&health, "force field", _material_dirt), "a health pack's is the map's");
        CHECK(material_is(&speed, "force field", _material_dirt), "another powerup's is the map's");
    }
    CASE("no collision") {
        struct damage_resistance_material overshield;
        material_set(&overshield, "force field", _material_dirt);
        collision_model_set(0, NULL, 0);
        collision_model_set(1, NULL, 3);
        collision_model_set(2, &overshield, 1);
        equipment_count = 0;
        equipment_add(_equipment_powerup_overshield, NONE);
        equipment_add(_equipment_powerup_overshield, 0);
        equipment_add(_equipment_powerup_overshield, 1);
        equipment_add(_equipment_powerup_overshield, 2);
        overshield_material_tags_loaded();
        CHECK(material_is(&overshield, "force field", _material_engineer_force_field),
            "the next overshield's past one with no collision model, no materials, or none to read");
    }
    return 0;
}
