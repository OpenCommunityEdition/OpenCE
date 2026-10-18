/* Compile against the actual engine declarations: this is the ABI gate for
   the native scenario arrays, not a duplicate set of numeric-only asserts. */
#include "cseries/cseries.h"
#define random halo_source_random
#define __inline static __inline
#include "math/real_math.h"
#undef __inline
#undef random
#include "scenario/scenario_definitions.h"
#include "native_scenario_blocks.h"

typedef char witness_reference_size[(sizeof(struct mac_native_scenario_reference)==sizeof(struct tag_reference))?1:-1];
typedef char witness_palette_size[(sizeof(struct mac_native_scenario_palette_entry)==sizeof(struct scenario_object_palette_entry))?1:-1];
typedef char witness_palette_unused_offset[(offsetof(struct mac_native_scenario_palette_entry,unused)==offsetof(struct scenario_object_palette_entry,unused))?1:-1];
typedef char witness_object_size[(sizeof(struct mac_native_scenario_object_core)==sizeof(struct scenario_object_datum))?1:-1];
typedef char witness_object_palette_offset[(offsetof(struct mac_native_scenario_object_core,palette_entry_index)==offsetof(struct scenario_object_datum,palette_entry_index))?1:-1];
typedef char witness_object_position_offset[(offsetof(struct mac_native_scenario_object_core,position)==offsetof(struct scenario_object_datum,position))?1:-1];
typedef char witness_object_rotation_offset[(offsetof(struct mac_native_scenario_object_core,rotation)==offsetof(struct scenario_object_datum,rotation))?1:-1];
typedef char witness_permutation_size[(sizeof(struct mac_native_scenario_permutation)==sizeof(struct scenario_object_permutation))?1:-1];
typedef char witness_permutation_region_offset[(offsetof(struct mac_native_scenario_permutation,region_permutations)==offsetof(struct scenario_object_permutation,region_permutations))?1:-1];
typedef char witness_name_size[(sizeof(struct mac_native_scenario_name)==sizeof(struct scenario_object_name))?1:-1];
typedef char witness_name_runtime_offset[(offsetof(struct mac_native_scenario_name,runtime_scenario_datum_index)==offsetof(struct scenario_object_name,runtime_scenario_datum_index))?1:-1];
typedef char witness_vehicle_size[(sizeof(struct mac_native_scenario_vehicle_record)==0x78)?1:-1];
typedef char witness_vehicle_flags_offset[(offsetof(struct mac_native_scenario_vehicle_record,multiplayer_spawn_flags)==0x5a)?1:-1];

/* Exercise the engine's TAG_BLOCK_GET_ELEMENT macro itself at middle and last
   indices. The source macro computes byte addressing from sizeof(type). */
int mac_scenario_blocks_source_index_witness(
    const struct mac_native_scenario_blocks *native)
{
    struct scenario scenario;
    struct tag_block_definition palette_definition;
    struct scenario_object_palette_entry *middle,*last,*vehicle_middle,*vehicle_last;
    const struct mac_native_scenario_palette_entry *source,*vehicle_source;
    unsigned char *clear;
    for(clear=(unsigned char *)&scenario;clear<(unsigned char *)(&scenario+1);++clear)*clear=0;
    for(clear=(unsigned char *)&palette_definition;clear<(unsigned char *)(&palette_definition+1);++clear)*clear=0;
    palette_definition.element_size=sizeof(struct scenario_object_palette_entry);
    scenario.scenery_palette.count=native->families[MAC_SCENARIO_SCENERY].palette_count;
    scenario.scenery_palette.address=native->families[MAC_SCENARIO_SCENERY].palette;
    scenario.scenery_palette.definition=&palette_definition;
    middle=TAG_BLOCK_GET_ELEMENT(&scenario.scenery_palette,1,struct scenario_object_palette_entry);
    last=TAG_BLOCK_GET_ELEMENT(&scenario.scenery_palette,scenario.scenery_palette.count-1,struct scenario_object_palette_entry);
    source=native->families[MAC_SCENARIO_SCENERY].palette;
    scenario.vehicle_palette.count=native->families[MAC_SCENARIO_VEHICLE].palette_count;
    scenario.vehicle_palette.address=native->families[MAC_SCENARIO_VEHICLE].palette;
    scenario.vehicle_palette.definition=&palette_definition;
    vehicle_middle=TAG_BLOCK_GET_ELEMENT(&scenario.vehicle_palette,1,struct scenario_object_palette_entry);
    vehicle_last=TAG_BLOCK_GET_ELEMENT(&scenario.vehicle_palette,scenario.vehicle_palette.count-1,struct scenario_object_palette_entry);
    vehicle_source=native->families[MAC_SCENARIO_VEHICLE].palette;
    return middle && last && vehicle_middle && vehicle_last &&
        middle->reference.group_tag==source[1].reference.group_tag &&
        middle->reference.index==source[1].reference.index &&
        last->reference.group_tag==source[scenario.scenery_palette.count-1].reference.group_tag &&
        last->reference.index==source[scenario.scenery_palette.count-1].reference.index &&
        vehicle_middle->reference.index==vehicle_source[1].reference.index &&
        vehicle_last->reference.index==vehicle_source[scenario.vehicle_palette.count-1].reference.index;
}

/* TAG_BLOCK_GET_ELEMENT falls back here only for invalid bounds or a bad
   element-size witness. The test cases above are valid fast-path indices. */
void *tag_block_get_element_with_size(const struct tag_block *block,long index,long size)
{ (void)block;(void)index;(void)size;return NULL; }
