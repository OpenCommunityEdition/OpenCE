"""Generate native object and supporting tag field descriptors from actual engine declarations.

Every declaration must be recognized; missing pointer/block metadata is fatal.
Padding and explicitly unused bytes are retained verbatim. Semantic nested
block types/limits come from tag_schema_objects.c and engine consumers.
"""
import re
from pathlib import Path
files = ['source/objects/damage_effect_definitions.h','source/objects/object_definitions.h','source/units/unit_definitions.h',
         'source/units/biped_definitions.h','source/cache/predicted_resources.h',
         'source/sound/sound_definitions.h','source/sound/sound_environment_definitions.h',
         'source/effects/effect_definitions.h','port/mac/native_effect_layout.h',
         'source/effects/particle_system_definitions.h','source/items/projectile_definitions.h','source/effects/contrail_definitions.h',
         'source/shaders/shader_definitions.h','source/shaders/shader_texture_animation.h',
         'source/items/item_definitions.h','source/items/equipment_definitions.h','source/items/weapon_definitions.h','source/game/aim_assist.h',
         'source/game/game_globals.h','source/effects/material_effect_definitions.h',
         'source/physics/breakable_surfaces.h',
         'source/devices/device_definitions.h','source/devices/device_controls.h',
         'source/interface/hud_definitions.h','source/interface/unit_hud_interface_definition.h',
         'source/interface/weapon_hud_interface_definition.h','port/mac/native_hud_layout.h',
         'port/mac/native_hud_component_layout.h','port/mac/native_grenade_hud_layout.h','port/mac/native_lens_flare_layout.h','port/mac/native_widget_layout.h',
         'port/mac/native_object_tags.c','source/units/vehicles.c','port/linux/game/tag_schema_render.c',
         # vehicles.c has a partial falling-damage view; the complete native
         # layout must win so distance floats and the primary damage reference
         # are converted and visited rather than preserved as unused bytes.
         'port/mac/native_globals_layout.h','port/mac/native_shader_layout.h',
         'source/ai/actor_definitions.h']
structs = {}
struct_sources = {}
for path in files:
    text=Path(path).read_text()
    text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
    for match in re.finditer(r'struct\s+(\w+)\s*\{([^{}]*)\};',text):
        structs[match[1]]=match[2]
        struct_sources[match[1]]=path
# actors.c's variant colours (the schema file declares them with no fields)
structs['actor_variant_change_colors']='real_rgb_color color_lower_bound; real_rgb_color color_upper_bound; unsigned long unused[2]'
# Pointer-only validators and TU-local partial views must never override the
# endian-complete native layouts silently when more source files are added.
for name, expected in {
    'game_globals_falling_damage': 'port/mac/native_globals_layout.h',
    'shader_transparent_meter_definition': 'port/mac/native_shader_layout.h',
    'shader_environment_definition': 'port/mac/native_shader_layout.h',
}.items():
    if struct_sources.get(name) != expected:
        raise ValueError(('partial declaration shadows native schema', name, struct_sources.get(name)))
# The source's anonymous union prevents the deliberately simple declaration
# reader above from recognizing this struct. Every union alternative occupies
# four 32-bit words; shield extras provide the endian-equivalent shape.
structs['metered_panel_definition'] = ('struct static_hud_element_definition background; '
    'struct meter_hud_element_definition meter; struct shield_meter_extras_definition shield_extras; '
    'long unused[4]')
blocks = {
 ('material_effects_definition','effects'):('material_effect',13),
 ('material_effect','materials'):('material_effect_material',33),
 ('shader_transparent','extra_layers'):('shader_layer',4),
 ('shader_transparent','maps'):('shader_transparent_chicago_map',4),
 ('shader_transparent_chicago_extended_definition','two_stage_maps'):('shader_transparent_chicago_map',2),
 ('effect_definition','locations'):('effect_location_definition',32),
 ('shader_transparent_water_definition','ripples'):('water_ripple',4),
 ('effect_definition','events'):('effect_event_definition',32),
 ('effect_event_definition','parts'):('effect_part_definition',32),
 ('effect_event_definition','particles'):('effect_particles_definition',32),
 ('particle_system_definition','physics_constants'):('particle_system_physics_constant',16),
 ('particle_system_definition','types'):('particle_system_type',4),
 ('particle_system_type','physics_constants'):('particle_system_physics_constant',16),
 ('particle_system_type','type_states'):('particle_system_type_state',8),
 ('particle_system_type','particle_states'):('particle_system_type_particle_state',8),
 ('particle_system_type_state','physics_constants'):('particle_system_physics_constant',16),
 ('item_collection_definition','permutations'):('item_permutation_definition',32767),
 ('_projectile_definition','material_responses'):('projectile_material_response_definition',33),
 ('contrail_definition','states'):('contrail_point_state',16),
 ('_object_definition','attachments'):('object_attachment_definition',8),
 ('_object_definition','widgets'):('object_definition_widget',4),
 ('_object_definition','functions'):('object_function_definition',4),
 ('_object_definition','change_colors'):('object_change_color_definition',4),
 ('_object_definition','predicted_resources'):('predicted_resource',1024),
 ('object_change_color_definition','permutations'):('object_change_color_permutation',32),
 ('unit_camera','unit_camera_tracks'):('native_unit_camera_track',2),
 ('_unit_definition','huds'):('unit_hud_reference',2),
 ('_unit_definition','dialogue_variants'):('native_unit_dialogue',16),
 ('_unit_definition','powered_seats'):('powered_seat_definition',2),
 ('_unit_definition','initial_weapons'):('native_unit_initial_weapon',4),
 ('_unit_definition','seats'):('unit_seat',16),
 ('unit_seat','seat_huds'):('unit_hud_reference',2),
 ('_biped_definition','contact_points'):('native_biped_contact',2),
 ('_weapon_definition','magazines'):('weapon_magazine_definition',2),
 ('_weapon_definition','triggers'):('weapon_trigger_definition',2),
 ('_weapon_definition','predicted_resources'):('predicted_resource',1024),
 ('weapon_magazine_definition','ammunition_objects'):('weapon_ammunition_object',8),
 ('weapon_trigger_definition','firing_effects'):('native_weapon_firing_effect',8),
 ('static_hud_element_definition','multitexture_overlays'):('multitexture_overlay_hud_element_definition',30),
 ('meter_hud_element_definition','multitexture_overlays'):('multitexture_overlay_hud_element_definition',30),
 ('multitexture_overlay_hud_element_definition','functions'):('multitexture_overlay_hud_element_effector_definition',30),
 ('auxilary_panel_definition','auxilary_overlays'):('auxilary_overlay_definition',16),
 ('unit_hud_interface_definition','warning_sounds'):('hud_sound_definition',12),
 ('unit_hud_interface_definition','auxilary_meters'):('auxilary_meter_definition',16),
 ('weapon_hud_overlay_definition','items'):('weapon_hud_overlay_item',16),
 ('weapon_hud_interface_definition','statics'):('weapon_hud_static_element',16),
 ('weapon_hud_interface_definition','meters'):('weapon_hud_meter_element',16),
 ('weapon_hud_interface_definition','numbers'):('weapon_hud_number_element',16),
 ('weapon_hud_interface_definition','crosshairs'):('weapon_hud_crosshairs_element',19),
 ('weapon_hud_interface_definition','overlays'):('weapon_hud_overlays_element',16),
 ('weapon_hud_interface_definition','warning_sounds'):('hud_sound_definition',12),
 ('weapon_hud_interface_definition','screen_effects'):('hud_screen_effect_definition',1),
 ('weapon_hud_crosshair_definition','items'):('weapon_hud_crosshair_item',64),
 ('lens_flare_definition','reflections'):('lens_flare_reflection',32),
}
blocks.update({('lightning_definition','markers'):('lightning_marker_definition',16),
 ('lightning_definition','shaders'):('lightning_shader',1),
 ('flag_definition','attachment_points'):('flag_attachment_point',5)})
blocks.update({('sound_definition','pitch_ranges'):('sound_pitch_range',8),
 ('sound_pitch_range','permutations'):('sound_permutation',256)})
blocks.update({('looping_sound_definition','tracks'):('looping_sound_track',4),
 ('looping_sound_definition','details'):('looping_sound_detail',32)})
blocks.update({
 ('game_globals', name):(child,limit) for name,child,limit in [
 ('playlist','native_global_playlist',20),('sounds','native_global_reference',2),('camera','native_global_reference',1),
 ('player_control','game_globals_player_control',1),('difficulty_information','game_globals_difficulty_information',1),
 ('grenades','game_globals_grenade',2),('rasterizer_data','game_globals_rasterizer_data',1),
 ('interface_tag_references','game_globals_interface_tag_references',1),('weapon_list','native_global_reference',20),
 ('cheat_powerups','native_global_reference',20),('multiplayer_information','game_globals_multiplayer_information',1),
 ('player_information','game_globals_player_information',1),('first_person_interface','game_globals_first_person_interface',1),
 ('falling_damage','game_globals_falling_damage',1),('materials','material_definition',33)]})
blocks.update({('game_globals_player_control','look_function'):('native_look_value',16),
 ('game_globals_multiplayer_information','vehicles'):('game_globals_vehicle',20),
 ('game_globals_multiplayer_information','sounds'):('native_global_reference',60),
 ('breakable_surface','particle_effects'):('breakable_surface_particle_effect',32)})
blocks.update({('actor_variant_definition','change_colors'):('actor_variant_change_colors',4)})
blocks.update({('hud_messaging_parameters_definition','button_icons'):('icon_hud_element_definition',18),
 ('hud_waypoint_definition','arrows'):('hud_waypoint_arrow',16)})
reference_groups={}
metadata=Path('port/linux/game/tag_schema_objects.c').read_text()+Path('port/linux/game/tag_schema_render.c').read_text()
for m in re.finditer(r"TAG_SCHEMA_REFERENCE\(struct (\w+),\s*([\w.]+),\s*TAG_SCHEMA_GROUPS\(([^)]*)\)\)",metadata):
    reference_groups[m[1],m[2]]=re.findall(r"'([^']+)'",m[3])
# Import only the new family: other effect schemas describe Xbox group names
# (e.g. mode), while this decoder reads retail PC mod2 references.
for m in re.finditer(r"TAG_SCHEMA_REFERENCE\(struct (\w+),\s*([\w.]+),\s*TAG_SCHEMA_GROUPS\(([^)]*)\)\)",
                     Path('port/linux/game/tag_schema_effects.c').read_text()):
    if m[1] in {'looping_sound_definition','looping_sound_track','looping_sound_detail','sound_definition','effect_part_definition','effect_particles_definition','damage_effect_definition','material_effect_material',
                 'hud_number_definition','unit_hud_interface_definition','weapon_hud_interface_definition','static_hud_element_definition','meter_hud_element_definition','multitexture_overlay_hud_element_definition','weapon_hud_overlay_definition','weapon_hud_crosshairs_element','hud_sound_definition','hud_screen_effect_definition','lens_flare_definition'}:
        reference_groups[m[1],m[2]]=re.findall(r"'([^']+)'",m[3])
reference_groups['_object_definition','model']=['mod2']
for f in ('first_person_model','first_person_animations','hud_interface'):
    reference_groups['weapon_interface_definition',f]=reference_groups['_weapon_definition','interface_definition.'+f]
reference_groups['weapon_interface_definition','first_person_model']=['mod2']
reference_groups['native_decal_definition','next_decal_in_chain']=['deca']
reference_groups['native_decal_shader_definition','map']=['bitm']
reference_groups['native_dialogue_definition','vocalizations']=['snd!']
reference_types={'native_unit_camera_track':'unit_camera_track',
                 'native_unit_initial_weapon':'unit_initial_weapon',
                 'native_unit_dialogue':'unit_dialogue_variant_definition',
                 'native_weapon_firing_effect':'weapon_firing_effect'}
hud_groups = {
 'hud_messaging_parameters_definition': {'single_player_font':'font','multi_player_font':'font','hud_item_messages':'ustr','messaging_icons':'bitm','alternate_icon_text':'ustr','hud_messages':'hmt '},
 'hud_waypoint_definition': {'arrow_bitmap':'bitm'},
 'hud_defaults_definition': {'default_weapon_hud':'wphi'},
 'hud_damage_indicators_definition': {'indicator_bitmap':'bitm'},
 'hud_globals_definition': {'carnage_report_bitmap':'bitm','checkpoint_sound':'snd!'}}
for ht,fields in hud_groups.items():
    for hf,group in fields.items():reference_groups[ht,hf]=[group]
reference_groups['actor_ranged_combat_properties','reference']=['weap']
reference_groups['actor_item_properties','equipment_reference']=['eqip']
reference_groups['multitexture_overlay_hud_element_definition','map']=['bitm']
reference_groups['weapon_hud_overlay_definition','bitmap']=['bitm']
reference_groups['weapon_hud_crosshair_definition','bitmap']=['bitm']
reference_groups['hud_number_definition','number_bitmap']=['bitm']
reference_groups['hud_sound_definition','sound']=['snd!','lsnd']
reference_groups['hud_screen_effect_definition','mask_fullscreen']=['bitm']
reference_groups['hud_screen_effect_definition','mask_splitscreen']=['bitm']
reference_groups['lens_flare_definition','primary_map']=['bitm']
reference_groups['fog_screen_definition','map']=['bitm']
reference_groups['fog_definition_full','background_sound']=['lsnd']
reference_groups['shader_transparent_plasma_definition','primary_noise_map']=['bitm']
reference_groups['shader_transparent_plasma_definition','secondary_noise_map']=['bitm']
reference_groups['fog_definition_full','sound_environment']=['snde']
output=['/* Generated by tools/mac_native_object_schema.py; do not hand edit. */']
seen=set()
fields_total=0
padding_total=0
def emit(t):
    global fields_total,padding_total
    if t in seen:return
    seen.add(t)
    fields=[]
    for declaration in structs[t].split(';'):
        declaration=' '.join(declaration.split())
        if not declaration:continue
        m=re.fullmatch(r'(.+?)\s+(\w+)((?:\[[^]]+\])*)?',declaration)
        if not m:raise ValueError((t,declaration))
        typ,name,array=m.groups()
        # hud_weapon.c never reads the grenade HUD's trailing warning/icon
        # records (also documented by tag_schema_effects.c). Preserve them as
        # opaque unused storage rather than following their disk pointers.
        if t=='grenade_hud_interface_definition' and name in {'warning_sounds','messaging_icon_bitmap'}:
            padding_total+=1
            continue
        if typ=='struct tag_block' and name=='unused_block':
            fields.append(f'    FIELD({t},{name},NEMPTYBLOCK),')
            continue
        if 'unused' in name or name.startswith('pad') or name.startswith('reserved') or name=='tag_padding':
            padding_total+=1
            continue
        if typ=='struct tag_block':
            child,limit=blocks[(t,name)];emit(child)
            fields.append(f'    CHILD({t},{name},NBLOCK,{child},{limit}),')
        elif typ=='struct tag_data':
            kind='NFILEDATA' if name=='samples' else 'NDATA'
            limit={'samples':0x400000,'mouth_data':8192,'subtitle_data':512}[name]
            fields.append(f'    {{offsetof(struct {t},{name}),sizeof(struct tag_data),{kind},NULL,{limit},NULL}},')
        elif typ=='struct tag_reference':
            key=(reference_types.get(t,t),name)
            if key not in reference_groups and t not in {'native_global_reference','game_globals_grenade','game_globals_rasterizer_data','game_globals_interface_tag_references','game_globals_vehicle','game_globals_multiplayer_information','game_globals_player_information','game_globals_first_person_interface','game_globals_falling_damage','breakable_surface','breakable_surface_particle_effect','material_definition'}:raise ValueError(('missing reference metadata',key))
            groups=reference_groups.get(key, [])
            group_name=f'groups_{t}_{name}'
            words=[f'UINT32_C(0x{int.from_bytes(g.encode(), "big"):08x})' for g in groups]
            output.append(f'static const uint32_t {group_name}[]={{'+','.join(words+['0'])+'};')
            fields.append(f'    REF({t},{name},{group_name}),')
        elif typ.startswith('struct '):
            child=typ[7:]
            if child=='hud_color_definition':child='native_hud_color'
            # Published contrail shader is a partial view; render schema specifies
            # the complete effect shader including its secondary bitmap.
            if t=='contrail_definition' and name=='shader':child='shader_effect_definition'
            emit(child)
            fields.append(f'    CHILD({t},{name},{"NSTRUCTARRAY" if array else "NSTRUCT"},{child},0),')
        else:
            if t=='predicted_resource' and name=='tag_index':kind='NTAGINDEX'
            elif typ=='char' and array:kind='NSTRING'
            elif typ in ('char','byte'):kind='N8'
            elif typ in ('short','word','unsigned short','point2d','rectangle2d'):kind='N16'
            elif typ in ('long','unsigned long','int','unsigned int','pixel32'):kind='N32'
            elif typ=='real' or typ.startswith('real_'):kind='NREAL'
            else:raise ValueError((t,name,typ))
            fields.append(f'    FIELD({t},{name},{kind}),')
    fields_total+=len(fields)
    if fields:
        output.extend([f'static const struct native_field fields_{t}[]={{',*fields,'};',f'SCHEMA({t});'])
    else:
        output.append(f'static const struct native_schema schema_{t}={{sizeof(struct {t}),NULL,0}};')
emit('material_effects_definition')
emit('damage_effect_definition')
emit('continuous_damage_effect_definition')
emit('shader_transparent_chicago_definition')
emit('shader_transparent_chicago_extended_definition')
emit('shader_transparent_glass_definition')
emit('shader_transparent_meter_definition')
emit('shader_environment_definition')
emit('shader_model_definition')
emit('particle_definition')
emit('effect_definition')
emit('particle_system_definition')
emit('equipment_definition')
emit('item_definition')
emit('item_collection_definition')
emit('sound_environment_definition')
emit('sound_definition')
emit('looping_sound_definition')
emit('object_definition')
emit('biped_definition')
emit('vehicle_definition')
emit('weapon_definition')
emit('projectile_definition')
emit('contrail_definition')
emit('game_globals')
emit('hud_globals_definition')
emit('hud_number_definition')
emit('unit_hud_interface_definition')
emit('weapon_hud_interface_definition')
emit('grenade_hud_interface_definition')
emit('lens_flare_definition')
emit('native_decal_definition')
emit('native_dialogue_definition')
emit('device_definition')
emit('machine_definition')
emit('control_definition')
emit('shader_transparent_water_definition')
emit('fog_definition_full')
emit('shader_transparent_plasma_definition')
emit('actor_definition')
emit('actor_variant_definition')
emit('glow_definition')
emit('lightning_definition')
emit('flag_definition')
output.append(f'/* {len(seen)} structures, {fields_total} typed fields, {padding_total} preserved padding/unused fields. */')
Path(__import__('os').environ.get('SCHEMA_OUT','port/mac/native_object_schema.inc')).write_text('\n'.join(output)+'\n')
print(output[-1])
