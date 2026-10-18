/* Actual scenario placement constructors, collision/update dispatch and cleanup.
   Sound attachment records are exercised; no audio update/playback is invoked. */
#include "cseries.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "objects/objects.h"
#include "memory/data.h"
#include "units/units.h"
#include "units/unit_definitions.h"
#include "models/model_animation_definitions.h"
#include "game/players.h"
#include "sound/game_sound.h"
#ifdef MAC_RUNTIME_VIEW
#include "mac_object_render_probe.h"
#define MAC_INPUT_NO_CARBON
#include "mac_runtime_view.h"
extern struct mac_runtime_view *mac_probe_runtime_view;
#endif
#include <float.h>
#include "real_math.h"
extern struct data_array *game_looping_sound_data;
#define CHECK(x) do{++checks;if(!(x)){++failures;fprintf(stderr,"scenery FAIL %s line %d\n",#x,__LINE__);}}while(0)
static int finite_point(const real_point3d *p)
{return p->x>=-FLT_MAX&&p->x<=FLT_MAX&&p->y>=-FLT_MAX&&p->y<=FLT_MAX&&p->z>=-FLT_MAX&&p->z<=FLT_MAX;}
#ifdef MAC_RUNTIME_VIEW
static void diagnostic_camera_for_biped(long biped_index,struct mac_bsp_camera *camera,
    real_point3d *target)
{
    struct object_datum *biped=object_get(biped_index);
    real_vector3d direction;real distance;
    *target=biped->object.bounding_sphere_center;
    /* The sphere center is near the feet for this biped. Aim into the torso
       and keep the diagnostic camera close enough to clear Blood Gulch's
       terrain relief in the frame. */
    target->z+=0.25f;
    camera->position[0]=target->x-biped->object.forward.i*1.0f;
    camera->position[1]=target->y-biped->object.forward.j*1.0f;
    camera->position[2]=target->z+2.0f;
    direction.i=target->x-(real)camera->position[0];
    direction.j=target->y-(real)camera->position[1];
    direction.k=target->z-(real)camera->position[2];
    distance=square_root(direction.i*direction.i+direction.j*direction.j);
    camera->yaw=arctangent(direction.j,direction.i);
    camera->pitch=arctangent(direction.k,distance);
}

static void render_actual_biped_and_weapon(unsigned *checks,unsigned *failures)
{
    long biped_index,weapon_index;short slot;struct unit_datum *unit;struct object_datum *weapon;
    struct unit_definition *unit_definition;struct animation_graph *animation_graph;
    struct animation_graph_unit_seat *unit_seat;struct animation_graph_weapon_class *weapon_class;
    struct object_marker hand_marker,grip_marker;
    short hand_marker_count,grip_marker_count;
    struct mac_object_render_probe actor_probe;struct mac_runtime_view_frame bsp_frame,actor_frame;
    struct mac_bsp_camera camera;real_point3d target;const char *view_error=NULL;long objects[2];int attached;
#define ACTOR_CHECK(test) do{++*checks;if(!(test)){++*failures;fprintf(stderr,"actor FAIL %s line %d\n",#test,__LINE__);}}while(0)
    biped_index=player_control_get_unit_index(0);
    ACTOR_CHECK(biped_index!=NONE);
    if(biped_index==NONE||!mac_probe_runtime_view)return;
    unit=unit_get(biped_index);
    ACTOR_CHECK(unit->object.type==_object_type_biped);
    slot=unit->unit.current_weapon_index;
    ACTOR_CHECK(slot>=0&&slot<MAXIMUM_WEAPONS_PER_UNIT);
    if(slot<0||slot>=MAXIMUM_WEAPONS_PER_UNIT)return;
    weapon_index=unit->unit.weapon_object_indices[slot];
    ACTOR_CHECK(weapon_index!=NONE);
    if(weapon_index==NONE)return;
    weapon=object_get(weapon_index);
    ACTOR_CHECK(weapon->object.type==_object_type_weapon);
    ACTOR_CHECK(weapon->object.parent_object_index==biped_index);
    ACTOR_CHECK(weapon->object.parent_node_index>=0);
    ACTOR_CHECK(!TEST_FLAG(weapon->object.flags,_object_invisible_bit));
    if(weapon->object.parent_object_index!=biped_index||weapon->object.parent_node_index<0||
       TEST_FLAG(weapon->object.flags,_object_invisible_bit))return;
    /* object_attach_to_node stores a child origin in its parent's local space.
       The normal unit path recursively refreshes descendant matrices after
       changing a pose; this standalone probe advances objects_update directly,
       so perform that same real cache computation before reading the render
       matrices. It does not change the actor or weapon pose. */
    object_compute_node_matrices_recursive(biped_index);
    unit_definition=unit_definition_get(unit->definition_index);
    ACTOR_CHECK(unit_definition!=NULL&&unit_definition->object.animation_graph.index!=NONE);
    if(!unit_definition||unit_definition->object.animation_graph.index==NONE)return;
    animation_graph=animation_graph_definition_get(unit_definition->object.animation_graph.index);
    ACTOR_CHECK(animation_graph!=NULL&&animation_graph->unit_seats.count>unit->unit.animation.seat_index&&
        unit->unit.animation.seat_index>=0&&animation_graph->unit_seats.address!=NULL);
    if(!animation_graph||unit->unit.animation.seat_index<0||
       unit->unit.animation.seat_index>=animation_graph->unit_seats.count||!animation_graph->unit_seats.address)return;
    unit_seat=(struct animation_graph_unit_seat *)animation_graph->unit_seats.address+
        unit->unit.animation.seat_index;
    ACTOR_CHECK(unit->unit.animation.weapon_index>=0&&
        unit->unit.animation.weapon_index<unit_seat->weapon_classes.count&&unit_seat->weapon_classes.address!=NULL);
    if(unit->unit.animation.weapon_index<0||unit->unit.animation.weapon_index>=unit_seat->weapon_classes.count||
       !unit_seat->weapon_classes.address)return;
    weapon_class=(struct animation_graph_weapon_class *)unit_seat->weapon_classes.address+
        unit->unit.animation.weapon_index;
    hand_marker.node_index=-1;grip_marker.node_index=-1;
    hand_marker_count=object_get_marker_by_name(biped_index,weapon_class->hand_marker_name,&hand_marker,1);
    grip_marker_count=object_get_marker_by_name(weapon_index,weapon_class->grip_marker_name,&grip_marker,1);
    ACTOR_CHECK(hand_marker_count==1&&grip_marker_count==1);
    /* In the retail source an empty marker name resolves to the model root;
       object_attach_to_marker then aligns that root marker and attaches the
       child to its node. Do not report this as an explicit hand grip. */
    if(!weapon_class->hand_marker_name[0])ACTOR_CHECK(hand_marker.node_index==0);
    if(!weapon_class->grip_marker_name[0])ACTOR_CHECK(grip_marker.node_index==0);
    ACTOR_CHECK(hand_marker.node_index==weapon->object.parent_node_index);
    objects[0]=biped_index;objects[1]=weapon_index;
    diagnostic_camera_for_biped(biped_index,&camera,&target);
    ACTOR_CHECK(finite_point(&target));
    {
        struct object_datum *biped=object_get(biped_index);
        real_matrix4x3 *biped_nodes=object_get_node_matrices(biped_index);
        real_matrix4x3 *weapon_nodes=object_get_node_matrices(weapon_index);
        ACTOR_CHECK(biped_nodes!=NULL&&weapon_nodes!=NULL);
        if(biped_nodes&&weapon_nodes) {
            real_point3d weapon_origin;
            object_get_origin(weapon_index,&weapon_origin);
            fprintf(stderr,
            "actor diagnostic pose: biped=(%.6g,%.6g,%.6g) bounds=(%.6g,%.6g,%.6g) radius=%.6g forward=(%.6g,%.6g,%.6g) root=(%.6g,%.6g,%.6g) weapon-local=(%.6g,%.6g,%.6g) weapon-world=(%.6g,%.6g,%.6g) cam=(%.6g,%.6g,%.6g) yaw/pitch=(%.6g,%.6g)\n",
            biped->object.position.x,biped->object.position.y,biped->object.position.z,
            target.x,target.y,target.z,biped->object.bounding_sphere_radius,biped->object.forward.i,biped->object.forward.j,biped->object.forward.k,
            biped_nodes[0].position.x,biped_nodes[0].position.y,biped_nodes[0].position.z,
            weapon->object.position.x,weapon->object.position.y,weapon->object.position.z,
            weapon_origin.x,weapon_origin.y,weapon_origin.z,
            camera.position[0],camera.position[1],camera.position[2],camera.yaw,camera.pitch);
        }
    }
    if(!mac_runtime_view_draw(mac_probe_runtime_view,&camera,&bsp_frame,&view_error)) {
        fprintf(stderr,"actor baseline view: %s\n",view_error?view_error:"frame failed");ACTOR_CHECK(0);return;
    }
    attached=mac_object_render_probe_attach_many(mac_probe_runtime_view,&actor_probe,objects,2);
    ACTOR_CHECK(attached);
    if(!attached)return;
    if(mac_runtime_view_draw(mac_probe_runtime_view,&camera,&actor_frame,&view_error)) {
        ACTOR_CHECK(mac_object_render_probe_object_textured(&actor_probe,0));
        ACTOR_CHECK(mac_object_render_probe_object_textured(&actor_probe,1));
        ACTOR_CHECK(actor_frame.pixel_digest!=bsp_frame.pixel_digest);
        printf("actual third-person actors: biped=%08lx weapon=%08lx parent=%08lx attach_nodes=%d/%d explicit_markers=%d/%d marker_bytes=%02x/%02x animation_indices=%d/%d ppm_capture=%d biped_parts=%u textured=%u/%lu weapon_parts=%u textured=%u/%lu unsupported_base_textures=%u/%u unsupported_shader_semantics=%u/%u unsupported_first=%08lx/%08lx semantic_first=%08lx/%08lx base_groups=%08lx/%08lx semantic_groups=%08lx/%08lx pixels=%08lx->%08lx; opaque base maps only, shader alpha/light/animation unsupported\n",
            (unsigned long)biped_index,(unsigned long)weapon_index,(unsigned long)weapon->object.parent_object_index,
            hand_marker.node_index,grip_marker.node_index,weapon_class->hand_marker_name[0]!=0,
            weapon_class->grip_marker_name[0]!=0,(unsigned char)weapon_class->hand_marker_name[0],
            (unsigned char)weapon_class->grip_marker_name[0],unit->unit.animation.seat_index,
            unit->unit.animation.weapon_index,getenv("HALO_MAC_ACTOR_RENDER_PPM")&&*getenv("HALO_MAC_ACTOR_RENDER_PPM"),
            actor_probe.stats.drawn_parts,actor_probe.stats.textured_parts,actor_probe.stats.submitted_vertices,
            actor_probe.additional_stats[0].drawn_parts,actor_probe.additional_stats[0].textured_parts,actor_probe.additional_stats[0].submitted_vertices,
            actor_probe.stats.unsupported_shaders,actor_probe.additional_stats[0].unsupported_shaders,
            actor_probe.stats.unsupported_shader_semantics,actor_probe.additional_stats[0].unsupported_shader_semantics,
            (unsigned long)actor_probe.stats.first_unsupported_shader,(unsigned long)actor_probe.additional_stats[0].first_unsupported_shader,
            (unsigned long)actor_probe.stats.first_semantic_shader,
            (unsigned long)actor_probe.additional_stats[0].first_semantic_shader,
            (unsigned long)actor_probe.stats.first_unsupported_group,
            (unsigned long)actor_probe.additional_stats[0].first_unsupported_group,
            (unsigned long)actor_probe.stats.first_semantic_group,
            (unsigned long)actor_probe.additional_stats[0].first_semantic_group,
            (unsigned long)bsp_frame.pixel_digest,(unsigned long)actor_frame.pixel_digest);
        {
            const char *ppm=getenv("HALO_MAC_ACTOR_RENDER_PPM");
            if(ppm&&*ppm)ACTOR_CHECK(mac_runtime_view_write_ppm(mac_probe_runtime_view,ppm));
        }
    } else {fprintf(stderr,"actor overlay draw: %s\n",view_error?view_error:"frame failed");ACTOR_CHECK(0);}
    ACTOR_CHECK(mac_object_render_probe_detach(mac_probe_runtime_view,&actor_probe));
#undef ACTOR_CHECK
}
#endif
int mac_scenery_runtime_probe(void)
{
    struct scenario *scenario=global_scenario_get();
    struct tag_block *blocks[2],*palettes[2];
    long created[64],before=object_header_data->actual_count;
    int looping_before;
    unsigned count=0,family,checks=0,failures=0,i;
    blocks[0]=&scenario->scenery;blocks[1]=&scenario->sound_scenery;
    palettes[0]=&scenario->scenery_palette;palettes[1]=&scenario->sound_scenery_palette;
    CHECK(blocks[0]->count==47);CHECK(blocks[1]->count==2);
    CHECK(game_looping_sound_data && game_looping_sound_data->valid);
    if(failures)return 0;
    /* The real map may already own looping sounds before this diagnostic
       population is created. Verify attachment cleanup restores that state. */
    looping_before=game_looping_sound_data->actual_count;
    for(family=0;family<2;++family) {
        long n;unsigned stride=family?0x28:0x48;
        for(n=0;n<blocks[family]->count;++n) {
            struct scenario_object_datum *record=(struct scenario_object_datum *)((byte *)blocks[family]->address+n*stride);
            struct object_datum *object;long index;
            if(count>=64){CHECK(count<64);break;}
            index=object_new_from_scenario(record,palettes[family]);CHECK(index!=NONE);
            if(index==NONE)continue;
            created[count++]=index;object=object_get(index);
            CHECK(object->object.type==(family?11:6));
            CHECK(object->object.location.cluster_index!=NONE);
            CHECK(finite_point(&object->object.position));
            CHECK(object->object.position.x==record->position.x &&
                object->object.position.y==record->position.y && object->object.position.z==record->position.z);
        }
    }
    CHECK(count==49);CHECK(object_header_data->actual_count==before+(long)count);
    printf("actual scenario population: scenery=%ld sound_scenery=%ld live=%u looping_records=%d\n",
        blocks[0]->count,blocks[1]->count,count,game_looping_sound_data->actual_count);
    if(!failures) {
        objects_update();
        for(i=0;i<count;++i) {
            CHECK(object_header_try_and_get(created[i])!=NULL);
            if(object_header_try_and_get(created[i]))CHECK(finite_point(&object_get(created[i])->object.position));
        }
    }
#ifdef MAC_RUNTIME_VIEW
    if(!failures&&count>0&&mac_probe_runtime_view) {
        struct mac_object_render_probe object_probe;struct mac_runtime_view_frame bsp_frame,object_frame;
        struct mac_bsp_camera camera;const char *view_error=NULL;struct object_datum *boulder=object_get(created[0]);int attached;
        CHECK(mac_object_render_probe_uv_check());
        CHECK(boulder!=NULL);
        if(boulder) {
            /* A repeatable diagnostic view of the real first scenery object,
               independent of the player's actual camera. */
            camera.position[0]=boulder->object.position.x-10.0;
            camera.position[1]=boulder->object.position.y-10.0;
            camera.position[2]=boulder->object.position.z+2.0;
            camera.yaw=0.7853981633974483;camera.pitch=-0.1404897;
            if(mac_runtime_view_draw(mac_probe_runtime_view,&camera,&bsp_frame,&view_error)) {
                attached=mac_object_render_probe_attach(mac_probe_runtime_view,&object_probe,created[0]);
                CHECK(attached);
                if(attached) {
                    if(mac_runtime_view_draw(mac_probe_runtime_view,&camera,&object_frame,&view_error)) {
                        CHECK(mac_object_render_probe_valid(&object_probe));
                        CHECK(object_frame.pixel_digest!=bsp_frame.pixel_digest);
                        printf("actual scenery draw: object=%08lx parts=%u vertices=%lu unsupported_shaders=%u pixels=%08lx->%08lx\n",
                            (unsigned long)created[0],object_probe.stats.drawn_parts,object_probe.stats.submitted_vertices,
                            object_probe.stats.unsupported_shaders,(unsigned long)bsp_frame.pixel_digest,
                            (unsigned long)object_frame.pixel_digest);
                        {
                            const char *ppm=getenv("HALO_MAC_OBJECT_RENDER_PPM");
                            if(ppm&&*ppm)CHECK(mac_runtime_view_write_ppm(mac_probe_runtime_view,ppm));
                        }
                    } else {
                        fprintf(stderr,"scenery overlay draw: %s\n",view_error?view_error:"frame failed");
                        CHECK(0);
                    }
                    CHECK(mac_object_render_probe_detach(mac_probe_runtime_view,&object_probe));
                }
            } else CHECK(0);
        }
    }
    if(!failures&&mac_probe_runtime_view)render_actual_biped_and_weapon(&checks,&failures);
#endif
    for(i=0;i<count;++i) {
        object_delete_immediately(created[i]);CHECK(object_header_try_and_get(created[i])==NULL);
    }
    CHECK(object_header_data->actual_count==before);
    CHECK(game_looping_sound_data->actual_count==looping_before);
    printf("actual scenario population: %u checks %u failures; no audio playback or full game tick\n",checks,failures);
    return failures==0;
}
