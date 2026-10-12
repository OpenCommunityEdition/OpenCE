/* Real MCC walker and legacy helper dispatch, with a tiny schema and no assets. */
#include "cseries.h"
#include "tag_schema.h"
#include "mcc_runtime.h"
#include "mcc_tag_validate.h"
#include "mcc_syntax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

struct test_instance { unsigned long group, parent[2]; long handle; char *name; void *root; unsigned long unused[2]; };
struct test_header {
    struct test_instance *instances; long scenario; unsigned long checksum; long count;
    long vertex_count; void *vertices; long index_count; void *indices; unsigned long signature;
};
struct test_child { short enumeration; short pad; struct tag_data data; struct tag_reference reference; };
struct test_root {
    struct tag_block children; short index; short enumeration; struct tag_data file;
    struct tag_data hs_syntax_data, other_data;
    struct tag_block object_names;
    struct tag_block scenery_palette;
    short palette_entry_index;
};
struct test_bsp { void *root; long vcount; void *vertices; long icount; void *indices; unsigned long signature; };
struct test_device { short kind; unsigned short flags; struct tag_data identifier, profile; };
struct test_padded_inner { short kind; byte unused[14]; };
struct test_padded_root { struct test_padded_inner inner; };
struct test_graph { struct tag_block nodes, animations; };
struct test_graph_node {char name[32]; short next_sibling_node_index, first_child_node_index, parent_node_index;byte pad[26];};
struct test_animation { short next_animation_index; };
struct test_first_person { struct tag_block animations; };
struct test_grenades {struct tag_block grenades;};
struct test_equipment {short grenade_type;};
static int callback_calls, callback_failures;
static void *valid_bitmap;
static unsigned char external_vertices[32], external_indices[8];
static int hardware;

static boolean check_root(struct tag_validation *validation, void *base)
{
    int mcc = mcc_validation_owns(validation);
    void *scenario = tag_validate_tag_get(validation, (long)0xe0000000UL, 'scnr');
    ++callback_calls;
    if (tag_validate_root(validation) != base || !tag_validate_contains(validation, base, sizeof(struct test_root)) ||
        tag_validate_custom_edition(validation) || !tag_validate_tag_get(validation, (long)0xe0000000UL, 'scnr') ||
        !tag_validate_file_contains(validation, 100, 10) || tag_validate_file_contains(validation, 1020, 20)) ++callback_failures;
    if (mcc && (!mcc_validation_callback_active() || !tag_validate_any_claimed(base, sizeof(struct test_root)) ||
        tag_validate_vertex_buffer_data(validation, &hardware) != external_vertices ||
        tag_validate_index_buffer_data(validation, &hardware) != (scenario == base ? external_indices : external_vertices) ||
        !tag_validate_contains(validation, external_vertices, sizeof(external_vertices)) ||
        tag_validate_any_claimed(external_indices, sizeof(external_indices)))) ++callback_failures;
    return TRUE;
}
static struct tag_schema_field const child_fields[] = {
    TAG_SCHEMA_ENUM(struct test_child, enumeration, 3, 0),
    TAG_SCHEMA_DATA(struct test_child, data, 128),
    TAG_SCHEMA_REFERENCE(struct test_child, reference, TAG_SCHEMA_GROUPS('scnr')),
    TAG_SCHEMA_END
};
static struct tag_schema_definition const child_schema = TAG_SCHEMA_DEFINITION(child, struct test_child, child_fields);
static struct tag_schema_field const name_fields[] = {TAG_SCHEMA_END};
static struct tag_schema_definition const name_schema = {"object_name",36,name_fields};
static struct tag_schema_field const root_fields[] = {
    TAG_SCHEMA_BLOCK(struct test_root, children, child_schema, 4),
    TAG_SCHEMA_BLOCK_INDEX(struct test_root, index, TAG_SCHEMA_ROOT, offsetof(struct test_root, children), 0),
    TAG_SCHEMA_ENUM(struct test_root, enumeration, 4, 0),
    TAG_SCHEMA_FILE_DATA(struct test_root, file, 256),
    TAG_SCHEMA_DATA(struct test_root, hs_syntax_data, 56L + 20L * 19001),
    TAG_SCHEMA_DATA(struct test_root, other_data, 128),
    TAG_SCHEMA_BLOCK(struct test_root, object_names, name_schema, 512),
    TAG_SCHEMA_BLOCK(struct test_root, scenery_palette, name_schema, 100),
    TAG_SCHEMA_BLOCK_INDEX(struct test_root, palette_entry_index, TAG_SCHEMA_ROOT, offsetof(struct test_root, scenery_palette), FLAG(_tag_schema_none_bit)),
    TAG_SCHEMA_CHECK(check_root),
    TAG_SCHEMA_END
};
static struct tag_schema_definition const root_schema = TAG_SCHEMA_DEFINITION(scenario, struct test_root, root_fields);
static struct tag_schema_field const alias_fields[]={TAG_SCHEMA_BLOCK(struct test_root,children,child_schema,4),TAG_SCHEMA_END};
static struct tag_schema_definition const alias_schema=TAG_SCHEMA_DEFINITION(alias,struct test_root,alias_fields);
static struct tag_schema_field const padded_inner_fields[] = {
    TAG_SCHEMA_ENUM(struct test_padded_inner, kind, 3, 0), TAG_SCHEMA_END
};
static struct tag_schema_definition const padded_inner_schema = {"padded_inner", 2, padded_inner_fields};
static struct tag_schema_field const padded_root_fields[] = {
    TAG_SCHEMA_STRUCT(struct test_padded_root, inner, padded_inner_schema), TAG_SCHEMA_END
};
static struct tag_schema_definition const padded_root_schema = {"padded_root", 2, padded_root_fields};
static boolean animation_graph_check(struct tag_validation *validation, void *base)
{ (void)validation; (void)base; ++callback_failures; return FALSE; }
static boolean bitmap_data_check(struct tag_validation *validation, void *base)
{ (void)validation; (void)base; ++callback_failures; return FALSE; }
static boolean bitmap_group_check(struct tag_validation *validation, void *base)
{ (void)validation; (void)base; ++callback_failures; return FALSE; }
static struct tag_schema_field const graph_node_fields[] = {
    TAG_SCHEMA_BLOCK_INDEX(struct test_graph_node, next_sibling_node_index, TAG_SCHEMA_ROOT, offsetof(struct test_graph,nodes), FLAG(_tag_schema_none_bit)),
    TAG_SCHEMA_BLOCK_INDEX(struct test_graph_node, first_child_node_index, TAG_SCHEMA_ROOT, offsetof(struct test_graph,nodes), FLAG(_tag_schema_none_bit)),
    TAG_SCHEMA_BLOCK_INDEX(struct test_graph_node, parent_node_index, TAG_SCHEMA_ROOT, offsetof(struct test_graph,nodes), FLAG(_tag_schema_none_bit)),
    TAG_SCHEMA_END
};
static struct tag_schema_definition const graph_node_schema = TAG_SCHEMA_DEFINITION(node, struct test_graph_node, graph_node_fields);
static struct tag_schema_field const animation_fields[] = {
    TAG_SCHEMA_BLOCK_INDEX(struct test_animation,next_animation_index,TAG_SCHEMA_ROOT,offsetof(struct test_graph,animations),FLAG(_tag_schema_none_bit)), TAG_SCHEMA_END
};
static struct tag_schema_definition const animation_schema = TAG_SCHEMA_DEFINITION(animation,struct test_animation,animation_fields);
static struct tag_schema_field const graph_fields[] = {
    TAG_SCHEMA_BLOCK(struct test_graph,nodes,graph_node_schema,64),
    TAG_SCHEMA_TOOL_BLOCK(struct test_graph,animations,animation_schema,256),
    TAG_SCHEMA_CHECK(animation_graph_check), TAG_SCHEMA_END
};
static struct tag_schema_definition const graph_schema = TAG_SCHEMA_DEFINITION(graph,struct test_graph,graph_fields);
static struct tag_schema_field const empty_fields[]={TAG_SCHEMA_END};
static struct tag_schema_definition const model_node_schema={"model_node",156,empty_fields};
static struct tag_schema_definition const damage_name_schema={"damage_permutation",32,empty_fields};
static struct tag_schema_field const damage_fields[]={TAG_SCHEMA_BLOCK(struct test_root,children,damage_name_schema,32),TAG_SCHEMA_END};
static struct tag_schema_definition const damage_schema={"damage_test",sizeof(struct test_root),damage_fields};
struct test_empty_weapon {byte unused[48];struct tag_block animations;};
static struct tag_schema_field const empty_weapon_element_fields[]={TAG_SCHEMA_BLOCK(struct test_empty_weapon,animations,name_schema,4),TAG_SCHEMA_END};
static struct tag_schema_definition const empty_weapon_schema={"animation_graph_weapon_type",60,empty_weapon_element_fields};
static struct tag_schema_field const empty_weapon_fields[]={TAG_SCHEMA_BLOCK(struct test_root,children,empty_weapon_schema,32),TAG_SCHEMA_END};
static struct tag_schema_definition const empty_weapon_root={"weapon_test",sizeof(struct test_root),empty_weapon_fields};
static struct tag_schema_field const animation_data_fields[]={TAG_SCHEMA_DATA(struct test_root,other_data,128),TAG_SCHEMA_END};
static struct tag_schema_definition const animation_data_schema={"animation",sizeof(struct test_root),animation_data_fields};
struct test_model {byte pad[184];struct tag_block nodes;byte rest[36];};
static struct tag_schema_field const model_fields[]={
    TAG_SCHEMA_BLOCK(struct test_model,nodes,model_node_schema,64), TAG_SCHEMA_END
};
static struct tag_schema_definition const model_schema={"model",232,model_fields};
static struct tag_schema_definition const object_schema={"object",72,empty_fields};
static struct tag_schema_field const bitmap_fields[] = {
    TAG_SCHEMA_CHECK(bitmap_data_check),TAG_SCHEMA_CHECK(bitmap_group_check),TAG_SCHEMA_END
};
static struct tag_schema_definition const bitmap_schema = {"bitmap",4,bitmap_fields};
static struct tag_schema_field const fp_element_fields[] = {TAG_SCHEMA_END};
static struct tag_schema_definition const fp_element_schema = {"index",2,fp_element_fields};
static struct tag_schema_field const fp_fields[] = {TAG_SCHEMA_BLOCK(struct test_first_person,animations,fp_element_schema,28),TAG_SCHEMA_END};
static struct tag_schema_definition const fp_schema = {"animation_graph_first_person_weapon_animations",sizeof(struct test_first_person),fp_fields};
static struct tag_schema_field const grenade_fields[] = {TAG_SCHEMA_BLOCK(struct test_grenades,grenades,fp_element_schema,2),TAG_SCHEMA_END};
static struct tag_schema_definition const grenade_schema = {"game_globals",sizeof(struct test_grenades),grenade_fields};
static struct tag_schema_field const equipment_fields[] = {TAG_SCHEMA_ENUM(struct test_equipment,grenade_type,2,0),TAG_SCHEMA_END};
static struct tag_schema_definition const equipment_schema = {"equipment",sizeof(struct test_equipment),equipment_fields};
struct test_anchor {short corner;};
static struct tag_schema_field const anchor_fields[]={TAG_SCHEMA_ENUM(struct test_anchor,corner,5,0),TAG_SCHEMA_END};
static struct tag_schema_definition const anchor_schema=TAG_SCHEMA_DEFINITION(hud_absolute_placement,struct test_anchor,anchor_fields);
struct test_flag {long unused;char name[31];byte padding;byte rest[56];};
static struct tag_schema_field const flag_fields[]={TAG_SCHEMA_STRING(struct test_flag,name),TAG_SCHEMA_END};
static struct tag_schema_definition const flag_schema=TAG_SCHEMA_DEFINITION(cutscene_flag,struct test_flag,flag_fields);
static struct tag_schema_group const groups[] = {
    {'dmg!',{NONE,NONE},&damage_schema},
    {'emp!',{NONE,NONE},&empty_weapon_root},
    {'adt!',{NONE,NONE},&animation_data_schema},
    {'alrt',{NONE,NONE},&alias_schema},
    {'mode',{NONE,NONE},&model_schema}, {'vehi',{'obje',NONE},&object_schema},
    {'cflg',{NONE,NONE},&flag_schema},
    {'scnr', {NONE, NONE}, &root_schema}, {'sbsp', {NONE, NONE}, &root_schema},
    {'antr', {NONE, NONE}, &graph_schema}, {'bitm', {NONE, NONE}, &bitmap_schema}, {'afp!', {NONE,NONE}, &fp_schema},
    {'matg', {NONE,NONE}, &grenade_schema}, {'eqip',{NONE,NONE}, &equipment_schema},
    {'anch', {NONE, NONE}, &anchor_schema},
    {'trim', {NONE, NONE}, &padded_root_schema}, {0, {0, 0}, NULL}
};
struct tag_schema_group const *const tag_schema_group_lists[] = {groups, NULL};
struct tag_schema_group const tag_schema_custom_edition_groups[] = {{0, {0, 0}, NULL}};

void tag_validate_report(char const *message) { (void)message; }
uint32_t mcc_audio_stream_end(struct mcc_runtime *runtime) { (void)runtime; return 1024; }
uint32_t mcc_bitmaps_stream_end(struct mcc_runtime *runtime) { (void)runtime; return 1024; }
int mcc_audio_contains(struct mcc_runtime *runtime, uint32_t offset, uint32_t size)
{ (void)runtime; (void)offset; (void)size; return 0; }
int mcc_bitmaps_contains(struct mcc_runtime *runtime, uint32_t offset, uint32_t size)
{ (void)runtime; (void)offset; (void)size; return 0; }
int mcc_bitmaps_valid(struct mcc_runtime *runtime, struct bitmap_data *bitmap)
{ (void)runtime; return bitmap == valid_bitmap; }
int mcc_bitmaps_group_valid(struct mcc_runtime *runtime, void *group)
{ (void)runtime; return group == valid_bitmap; }
void *mcc_geometry_buffer_data(struct mcc_runtime *runtime, void const *buffer, int indices, unsigned long *bytes)
{
    (void)runtime; *bytes = indices ? sizeof(external_indices) : sizeof(external_vertices);
    return buffer == &hardware ? indices ? (void *)external_indices : external_vertices : NULL;
}
int mcc_geometry_contains(struct mcc_runtime *runtime, void const *address, unsigned long bytes)
{
    uintptr_t a = (uintptr_t)address, v = (uintptr_t)external_vertices, i = (uintptr_t)external_indices;
    (void)runtime;
    return (a >= v && a - v <= sizeof(external_vertices) && bytes <= sizeof(external_vertices) - (a - v)) ||
        (a >= i && a - i <= sizeof(external_indices) && bytes <= sizeof(external_indices) - (a - i));
}

void *mcc_runtime_allocate(struct mcc_runtime *runtime,uint32_t bytes)
{
    uint32_t start=(runtime->used+15)&~15u;
    if (start>runtime->capacity || bytes>runtime->capacity-start) return NULL;
    runtime->used=start+bytes;
    memset(runtime->tags+start,0,bytes);
    return runtime->tags+start;
}

int main(int argc, char **argv)
{
    struct mcc_runtime runtime;
    struct test_header *header;
    struct test_instance *instance;
    struct test_root *root;
    struct test_child *child;
    int expected = 1;
    CHECK(argc == 2);
    memset(&runtime, 0, sizeof(runtime));
    runtime.capacity = 65536; runtime.used = 4096; runtime.source.size = 1024;
    if (!strncmp(argv[1], "syntax_", 7)) runtime.capacity = runtime.used = 1048576;
    if (!strncmp(argv[1], "names_", 6)) runtime.used = runtime.capacity = 2097152;
    if (!strncmp(argv[1], "palette_", 8)) runtime.used = runtime.capacity = 2097152;
    runtime.tags = malloc(runtime.capacity); CHECK(runtime.tags != NULL);
    memset(runtime.tags, 0, runtime.capacity);
    header = (struct test_header *)runtime.tags;
    instance = (struct test_instance *)(runtime.tags + 0x28);
    root = (struct test_root *)(runtime.tags + 0x100);
    child = (struct test_child *)(runtime.tags + 0x200);
    header->instances = instance; header->count = 1; header->scenario = (long)0xe0000000UL; header->signature = 'tags';
    instance->group = 'scnr'; instance->parent[0] = instance->parent[1] = NONE;
    instance->handle = header->scenario; instance->root = root; instance->name = (char *)runtime.tags + 0x1c0;
    strcpy(instance->name, "scenario");
    root->palette_entry_index = NONE;
    root->children.count = 1; root->children.address = child; root->index = 7; root->enumeration = 9;
    root->file.file_offset = 100; root->file.size = 8;
    child->data.address = runtime.tags + 0x300; child->data.size = 16;
    child->reference.group_tag = 'scnr'; child->reference.index = header->scenario; child->reference.name = NULL;
    if (!strcmp(argv[1], "overlap")) { child->data.address = root; expected = 0; }
    if (!strcmp(argv[1], "outside")) { root->children.address = runtime.tags + runtime.used - 1; expected = 0; }
    if (!strcmp(argv[1], "misaligned")) { root->children.address = (byte *)child + 1; expected = 0; }
    if (!strcmp(argv[1], "negative")) { root->children.count = -1; expected = 0; }
    if (!strcmp(argv[1], "cycle")) { root->children.address = root; expected = 0; }
    if (!strcmp(argv[1], "file_range")) { root->file.file_offset = 1023; expected = 0; }
    if (!strcmp(argv[1], "stream_hole")) { root->file.file_offset = 0x4ffffff0; expected = 0; }
    if (!strcmp(argv[1], "handle")) { instance->handle++; expected = 0; }
    if (!strcmp(argv[1], "unknown_group")) { instance->group = 'nope'; expected = 0; }
    if (!strcmp(argv[1], "bsp")) {
        header->count = 2; instance[1] = instance[0]; instance[1].group = 'sbsp'; instance[1].handle++;
    }
    if (!strncmp(argv[1], "alias", 5)) {
        header->count=2;instance[1]=instance[0];instance[1].handle++;
        if (!strcmp(argv[1],"alias_partial")) {instance[1].root=(byte *)root+4;expected=0;}
        if (!strcmp(argv[1],"alias_group")) {instance[1].group='trim';expected=0;}
        if (!strcmp(argv[1],"alias_bad_child")) {child->data.address=root;expected=0;}
        if (!strcmp(argv[1],"alias_block") || !strcmp(argv[1],"alias_block_partial")) {
            struct test_root *other=(struct test_root *)(runtime.tags+0x500);
            *other=*root;instance[1].root=other;instance[1].group='alrt';
            if (!strcmp(argv[1],"alias_block_partial")) {other->children.address=(byte *)child+4;expected=0;}
        }
    }
    if (!strncmp(argv[1],"prefix_",7)) {
        strcpy((char *)runtime.tags+0x800,"prefix");
        root->object_names.address=runtime.tags+0x800;root->object_names.count=2;
        root->scenery_palette.address=runtime.tags+0x800;root->scenery_palette.count=1;
        if (!strcmp(argv[1],"prefix_expand")) {root->object_names.count=1;root->scenery_palette.count=2;}
        if (!strcmp(argv[1],"prefix_tail_overlap")) {
            root->object_names.count=1;root->scenery_palette.count=2;
            child->data.address=runtime.tags+0x800+36;child->data.size=4;
        }
    }
    if (!strncmp(argv[1],"empty_weapon",12) || !strncmp(argv[1],"animation_data",14)) {
        struct test_root *other=(void *)(runtime.tags+0x500);
        header->count=2;instance[1]=instance[0];instance[1].handle++;instance[1].root=other;
        root->other_data.address=runtime.tags+0x800;root->other_data.size=64;
        if (!strncmp(argv[1],"empty_weapon",12)) {
            instance[1].group='emp!';other->children.address=root->other_data.address;other->children.count=1;
            if (!strcmp(argv[1],"empty_weapon_nonzero")) {runtime.tags[0x800]=1;expected=0;}
        } else {
            instance[1].group='adt!';other->other_data.address=runtime.tags+0x810;other->other_data.size=32;
            memset(runtime.tags+0x800,0x7b,64);
            if (!strcmp(argv[1],"animation_data_outside")) {other->other_data.address=runtime.tags+runtime.used-1;expected=0;}
        }
    }
    if (!strncmp(argv[1], "collection", 10)) {
        struct tag_block *collection = (struct tag_block *)(runtime.tags + 0x500);
        struct tag_reference *reference = (struct tag_reference *)(runtime.tags + 0x600);
        header->count = 2; instance[1] = instance[0]; instance[1].group = 'tagc'; instance[1].handle++;
        instance[1].root = collection; collection->count = 1; collection->address = reference;
        *reference = child->reference;
        if (!strcmp(argv[1], "collection_outside")) { collection->address = runtime.tags + runtime.used - 1; expected = 0; }
    }
    if (!strcmp(argv[1],"damage_names")) {
        struct test_root *damage=(struct test_root *)(runtime.tags+0x500);
        header->count=2;instance[1]=instance[0];instance[1].group='dmg!';instance[1].handle++;instance[1].root=damage;
        damage->children.count=1;damage->children.address=child;
    }
    if (!strncmp(argv[1], "device", 6)) {
        struct test_device *device = (struct test_device *)(runtime.tags + 0x500);
        header->count = 2; instance[1] = instance[0]; instance[1].group = 'devc'; instance[1].handle++;
        instance[1].root = device; device->kind = 1;
        device->identifier.address = runtime.tags + 0x600; device->identifier.size = 16;
        device->profile.address = runtime.tags + 0x700; device->profile.size = 128;
        if (!strcmp(argv[1], "device_outside")) { device->profile.size = 4096; expected = 0; }
    }
    if (!strcmp(argv[1], "trimmed_structure")) {
        header->count = 2; instance[1] = instance[0]; instance[1].group = 'trim'; instance[1].handle++;
        instance[1].root = runtime.tags + runtime.used - 2;
    }
    if (!strncmp(argv[1],"graph",5)) {
        struct test_graph *graph=(struct test_graph *)(runtime.tags+0x700);
        struct test_graph_node *nodes=(struct test_graph_node *)(runtime.tags+0x800);
        struct test_animation *animations=(struct test_animation *)(runtime.tags+0x900);
        int i;
        header->count=2; instance[1]=instance[0]; instance[1].group='antr'; instance[1].handle++; instance[1].root=graph;
        graph->nodes.count=2; graph->nodes.address=nodes; graph->animations.count=257; graph->animations.address=animations;
        nodes[0].next_sibling_node_index=NONE; nodes[0].first_child_node_index=1; nodes[0].parent_node_index=0;
        nodes[1].next_sibling_node_index=NONE; nodes[1].first_child_node_index=NONE; nodes[1].parent_node_index=0;
        for(i=0;i<257;i++) animations[i].next_animation_index=i==256?NONE:(short)(i+1);
        if (!strcmp(argv[1],"graph_cycle")) {animations[256].next_animation_index=0; expected=0;}
        if (!strcmp(argv[1],"graph_node_cycle")) {nodes[1].first_child_node_index=0; expected=0;}
        if (!strcmp(argv[1],"graph_orphan")) {nodes[0].first_child_node_index=NONE; expected=0;}
        if (!strcmp(argv[1],"graph_missing_parent")) nodes[1].parent_node_index=NONE;
        if (!strcmp(argv[1],"graph_conflicting_parent")) {nodes[1].parent_node_index=1;expected=0;}
        if (!strcmp(argv[1],"graph_missing_parent_cycle")) {
            nodes[1].parent_node_index=NONE;nodes[1].first_child_node_index=0;expected=0;
        }
        if (!strcmp(argv[1],"graph_root_sibling")) nodes[0].next_sibling_node_index=1;
        if (!strcmp(argv[1],"graph_root_outside")) nodes[0].next_sibling_node_index=3;
        if (!strcmp(argv[1],"graph_root_orphan")) {nodes[0].next_sibling_node_index=1;nodes[0].first_child_node_index=NONE;expected=0;}
        if (!strncmp(argv[1],"graph_model",11)) {
            byte *model=runtime.tags+0xc00,*model_nodes=runtime.tags+0xd00,*object=runtime.tags+0xf00;
            struct tag_block *block=(struct tag_block *)(model+184);
            short links[6]={NONE,1,NONE,NONE,NONE,0};
            header->count=4;
            instance[2]=instance[0];instance[2].group='mode';instance[2].handle+=2;instance[2].root=model;
            instance[3]=instance[0];instance[3].group='vehi';instance[3].handle+=3;instance[3].root=object;
            memcpy(object+52,&instance[2].handle,4);memcpy(object+68,&instance[1].handle,4);
            block->count=2;block->address=model_nodes;
            strcpy(nodes[0].name,"root");strcpy(nodes[1].name,"child");
            strcpy((char *)model_nodes,"root");strcpy((char *)model_nodes+156,"child");
            memcpy(model_nodes+32,links,6);memcpy(model_nodes+156+32,links+3,6);
            nodes[0].first_child_node_index=NONE;
            if (!strncmp(argv[1],"graph_model_empty",17)) {
                nodes[0].parent_node_index=nodes[1].parent_node_index=NONE;
                if (!strcmp(argv[1],"graph_model_empty_partial")) {
                    nodes[1].next_sibling_node_index=0;expected=0;
                }
            }
            if (!strncmp(argv[1],"graph_model_tail",16)) {
                short tail_links[3]={NONE,NONE,0},sibling=2;
                block->count=graph->nodes.count=3;
                strcpy(nodes[2].name,"tail");strcpy((char *)model_nodes+312,"tail");
                memcpy(model_nodes+312+32,tail_links,6);
                memcpy(model_nodes+156+32,&sibling,2);
                nodes[0].first_child_node_index=2;nodes[1].next_sibling_node_index=2;
                nodes[2].next_sibling_node_index=nodes[2].first_child_node_index=NONE;
                nodes[2].parent_node_index=0;
                if (!strcmp(argv[1],"graph_model_tail_conflict")) {nodes[1].first_child_node_index=2;expected=0;}
                if (!strcmp(argv[1],"graph_model_tail_cycle")) {
                    short cycle=1;memcpy(model_nodes+312+32,&cycle,2);
                    nodes[2].next_sibling_node_index=1;expected=0;
                }
            }
            if (!strcmp(argv[1],"graph_model_mismatch")) {model_nodes[156]='x';expected=0;}
            if (!strcmp(argv[1],"graph_model_outside")) {block->address=runtime.tags+runtime.used-1;expected=0;}
            if (!strcmp(argv[1],"graph_model_cycle")) {short cycle=0;memcpy(model_nodes+156+34,&cycle,2);expected=0;}
        }
    }
    if (!strncmp(argv[1],"bitmap",6)) {
        header->count=2; instance[1]=instance[0]; instance[1].group='bitm'; instance[1].handle++;
        instance[1].root=runtime.tags+0x700; valid_bitmap=instance[1].root;
        if(!strcmp(argv[1],"bitmap_unowned")){valid_bitmap=NULL;expected=0;}
    }
    if (!strcmp(argv[1],"first_person_slots")) {
        struct test_first_person *fp=(struct test_first_person *)(runtime.tags+0x700);
        header->count=2; instance[1]=instance[0]; instance[1].group='afp!'; instance[1].handle++; instance[1].root=fp;
        fp->animations.count=30; fp->animations.address=runtime.tags+0x800;
    }
    if (!strncmp(argv[1],"grenades",8)) {
        struct test_grenades *globals=(struct test_grenades *)(runtime.tags+0x700);
        struct test_equipment *equipment=(struct test_equipment *)(runtime.tags+0x900);
        header->count=3; instance[1]=instance[0]; instance[1].group='matg'; instance[1].handle++; instance[1].root=globals;
        instance[2]=instance[0];instance[2].group='eqip';instance[2].handle+=2;instance[2].root=equipment;equipment->grenade_type=3;
        globals->grenades.count=4;globals->grenades.address=runtime.tags+0x800;
        if(!strcmp(argv[1],"grenades_two")){globals->grenades.count=2;equipment->grenade_type=1;}
        if(!strcmp(argv[1],"grenades_one")){globals->grenades.count=1;expected=0;}
        if(!strcmp(argv[1],"grenades_zero")){globals->grenades.count=0;expected=0;}
        if(!strcmp(argv[1],"grenades_excess")){globals->grenades.count=5;expected=0;}
    }
    if (!strcmp(argv[1], "legacy_isolation")) {
        CHECK(tag_validate_tags(header, runtime.used, runtime.source.size, "legacy"));
        CHECK(tag_validate_corrections() == 3);
    }
    if (!strncmp(argv[1], "syntax_", 7)) {
        root->hs_syntax_data.address = runtime.tags + 0x1000;
        root->hs_syntax_data.size = MCC_SYNTAX_MAXIMUM_DATA_BYTES;
        ((byte *)root->hs_syntax_data.address)[MCC_SYNTAX_MAXIMUM_DATA_BYTES - 1] = 0xA5;
        root->other_data.address = runtime.tags + 0xB0000;
        root->other_data.size = 256;
        if (!strcmp(argv[1], "syntax_overflow")) root->hs_syntax_data.size += 20;
        if (!strcmp(argv[1], "syntax_outside")) {
            root->hs_syntax_data.address = runtime.tags + runtime.used - 1; expected = 0;
        }
        if (!strcmp(argv[1], "syntax_overlap")) {
            root->other_data.address = (byte *)root->hs_syntax_data.address + 32; expected = 0;
        }
        if (!strcmp(argv[1], "syntax_legacy")) {
            CHECK(tag_validate_tags(header, runtime.used, runtime.source.size, "legacy syntax"));
            CHECK(root->hs_syntax_data.size == 56L + 20L * 19001);
            root->hs_syntax_data.size = MCC_SYNTAX_MAXIMUM_DATA_BYTES;
        }
    }
    if (!strncmp(argv[1],"anchor_",7)) {
        struct test_anchor *anchor=(void *)(runtime.tags+0x500);
        header->count=2;instance[1].group='anch';instance[1].handle=(long)0xe0010001UL;
        instance[1].root=anchor;instance[1].name=instance->name;
        anchor->corner=(short)atoi(argv[1]+7);
    }
    if (!strncmp(argv[1],"names_",6)) {
        root->object_names.count=atoi(argv[1]+6);
        root->object_names.address=runtime.tags+0x1000;
        expected=root->object_names.count<=32767;
    }
    if(!strcmp(argv[1],"flag_name")) {
        struct test_flag *flag=(void*)(runtime.tags+0x500);
        header->count=2;instance[1].group='cflg';instance[1].handle=(long)0xe0010001UL;
        instance[1].root=flag;instance[1].name=instance->name;
        memcpy(flag->name,"return_cart_cliffs_dropoff_flag",31);
    }
    if (!strncmp(argv[1], "palette_", 8)) {
        root->scenery_palette.count = atoi(argv[1]+8);
        if (!root->scenery_palette.count) root->scenery_palette.count = 101;
        root->scenery_palette.address = runtime.tags+0x1000;
        root->palette_entry_index = (short)(root->scenery_palette.count-1);
        expected = root->scenery_palette.count<=32767;
        if (!strcmp(argv[1], "palette_bad_index")) root->palette_entry_index = 101;
        if (!strcmp(argv[1], "palette_outside")) {
            root->scenery_palette.address = runtime.tags+runtime.used-36; expected = 0;
        }
        if (!strcmp(argv[1], "palette_overlap")) {
            root->scenery_palette.address = child; expected = 0;
        }
    }
    CHECK(mcc_tags_validate(&runtime) == expected);
    if (!strncmp(argv[1], "palette_", 8)) {
        int count = atoi(argv[1]+8);
        if (!count) count = 101;
        CHECK(root->scenery_palette.count == count);
        if (expected) {
            CHECK(root->palette_entry_index == (!strcmp(argv[1], "palette_bad_index") ? NONE : count-1));
            CHECK(tag_validate_tags(header,runtime.used,runtime.source.size,"legacy scenery"));
            CHECK(root->scenery_palette.count == MIN(count,100));
            CHECK(root->palette_entry_index == (count>100 ? NONE : count-1));
        }
        mcc_validation_dispose(&runtime); free(runtime.tags); return 0;
    }
    if(!strcmp(argv[1],"flag_name")) {
        struct test_flag *flag=(void*)(runtime.tags+0x500);
        CHECK(!strcmp(flag->name,"return_cart_cliffs_dropoff_flag"));
        CHECK(tag_validate_tags(header,runtime.used,runtime.source.size,"legacy flag"));
        CHECK(flag->name[30]==0);
    }
    if (!strncmp(argv[1],"names_",6)) {
        CHECK(root->object_names.count==atoi(argv[1]+6));
        CHECK(tag_validate_tags(header,runtime.used,runtime.source.size,"legacy names"));
        CHECK(root->object_names.count==512);
        mcc_validation_dispose(&runtime);free(runtime.tags);return 0;
    }
    if (!strncmp(argv[1],"anchor_",7)) {
        short input=(short)atoi(argv[1]+7);
        struct test_anchor *anchor=instance[1].root;
        CHECK(anchor->corner==(input>=0 && input<9 ? input : 0));
        CHECK(tag_validate_tags(header,runtime.used,runtime.source.size,"legacy anchors"));
        CHECK(anchor->corner==(input>=0 && input<5 ? input : 0));
        mcc_validation_dispose(&runtime);free(runtime.tags);return 0;
    }
    CHECK(!mcc_validation_callback_active());
    if (!strncmp(argv[1], "syntax_", 7) && expected) {
        CHECK(root->hs_syntax_data.size == MCC_SYNTAX_MAXIMUM_DATA_BYTES);
        CHECK(root->hs_syntax_data.address == runtime.tags + 0x1000);
        CHECK(((byte *)root->hs_syntax_data.address)[MCC_SYNTAX_MAXIMUM_DATA_BYTES - 1] == 0xA5);
        CHECK(root->other_data.size == 128);
        if (!strcmp(argv[1], "syntax_legacy")) {
            CHECK(tag_validate_tags(header, runtime.used, runtime.source.size, "legacy syntax again"));
            CHECK(root->hs_syntax_data.size == 56L + 20L * 19001);
        }
    }
    if (!strcmp(argv[1],"first_person_slots")) CHECK(((struct test_first_person *)instance[1].root)->animations.count==30);
    if (!strcmp(argv[1],"graph_model_tail"))
        CHECK(((struct test_graph_node *)((struct test_graph *)instance[1].root)->nodes.address)->first_child_node_index==1);
    if (!strcmp(argv[1],"graph_root_sibling") || !strcmp(argv[1],"graph_root_outside"))
        CHECK(((struct test_graph_node *)((struct test_graph *)instance[1].root)->nodes.address)->next_sibling_node_index==NONE);
    if (!strcmp(argv[1],"grenades_four")) {
        CHECK(((struct test_grenades *)instance[1].root)->grenades.count==4);
        CHECK(((struct test_equipment *)instance[2].root)->grenade_type==3);
    }
    if (expected) {
        if (!strcmp(argv[1],"prefix_tail_overlap")) {
            CHECK(root->scenery_palette.address!=runtime.tags+0x800);
            CHECK(!memcmp(root->scenery_palette.address,runtime.tags+0x800,72));
        }
        if (!strcmp(argv[1],"empty_weapon")) CHECK(((struct test_root *)instance[1].root)->children.address!=root->other_data.address);
        if (!strcmp(argv[1],"animation_data")) {
            void *copy=((struct test_root *)instance[1].root)->other_data.address;
            CHECK(copy!=runtime.tags+0x810 && !memcmp(copy,runtime.tags+0x810,32));
        }
        if (!strcmp(argv[1],"damage_names")) {
            struct test_root *damage=instance[1].root;
            CHECK(damage->children.address!=child);
            CHECK(!memcmp(damage->children.address,child,4));
        }
        CHECK(root->index == 0 && root->enumeration == 0);
        CHECK(child->reference.name == instance->name && child->reference.name_length == 8);
        CHECK(callback_calls > 0 && callback_failures == 0);
    }
    if (!strcmp(argv[1], "legacy_isolation")) {
        CHECK(tag_validate_corrections() == 3);
        CHECK(tag_validate_claimed(root));
        CHECK(tag_validate_tags(header, runtime.used, runtime.source.size, "legacy again"));
        CHECK(tag_validate_corrections() == 0 && callback_failures == 0);
    }
    if (!strcmp(argv[1], "bsp")) {
        struct test_bsp *bsp = (struct test_bsp *)(runtime.tags + 8192);
        struct test_root *broot = (struct test_root *)((byte *)bsp + 64);
        bsp->root = broot; bsp->signature = 'sbsp'; broot->file.file_offset = 100;
        CHECK(mcc_bsp_validate(&runtime, instance[1].handle, bsp, 512));
        CHECK(callback_failures == 0);
        broot->children.count = 1; broot->children.address = root;
        CHECK(!mcc_bsp_validate(&runtime, instance[1].handle, bsp, 512));
    }
    mcc_validation_dispose(&runtime); free(runtime.tags);
    return 0;
}
