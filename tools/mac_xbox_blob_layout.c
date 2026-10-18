/* Offsets of the tag data fields the Mac port converts (xbox_blob.c), on the target ABI. */
#include "tools/mac_compile_probe_prefix.h"
#define __inline static __inline__
#include "cseries.h"
#include "real_math.h"
#include "tag_files/tag_groups.h"
#include "memory/data.h"
#include "hs/hs_scenario_definitions.h"
struct location;
#include "scenario/scenario_definitions.h"
#include "cutscene/recorded_animation_definitions.h"
#include "models/model_animation_definitions.h"
#include "structures/structure_bsp_definitions.h"
#include "text/text_group.h"
#undef printf
#define P(t, f) printf("%s.%s %lu\n", #t, #f, (unsigned long)offsetof(struct t, f))
#define S(t) printf("sizeof %s %lu\n", #t, (unsigned long)sizeof(struct t))
int main(void)
{
    P(scenario, hs_syntax_data); P(scenario, hs_string_constants); P(scenario, recorded_animations);
    S(recorded_animation_definition); P(recorded_animation_definition, unit_control_data_version); P(recorded_animation_definition, event_stream);
    P(structure_bsp, cluster_data); P(structure_bsp, sound_cluster_data);
    S(animation); P(animation, frame_info); P(animation, default_data); P(animation, data); P(animation, flags);
    P(animation, nodes_with_translation_flags); P(animation, nodes_with_rotation_flags); P(animation, nodes_with_scale_flags);
    P(animation, node_count); P(animation, frame_count); P(animation, frame_size); P(animation, frame_info_type);
    S(string_list_entry); P(string_list, strings);
    return 0;
}
