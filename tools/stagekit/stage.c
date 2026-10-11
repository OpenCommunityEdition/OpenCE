/*
STAGE.C

What every StageKit map has: the scenario, the chamber (its structure BSP),
the globals and the interface's font, HUD and strings, the player's biped, no
scripts, and a player start. A scenario's content goes in between (its build,
README.md). Each value says where it comes from (README.md, "Values").
*/

/* ---------- headers */

#include "stagekit.h"

/* ---------- constants */

enum
{
	/* no scripts, as a map that has them holds none running: an empty array of
	script nodes, and room for the console's: as many as hs.c's
	MAXIMUM_HS_SYNTAX_NODES_PER_SCENARIO, which it keeps to itself (hs_scenario_syntax_data_valid) */
	SCRIPT_NODES = 19001,
	SCRIPT_NODE_BYTES = 0x14,
	SCRIPT_STRINGS = 0x400,
	/* (bipeds.c reads the first falling damage every tick: that file's layout) */
	FALLING_DAMAGE_BYTES = 0x98,
};

/* ---------- structures */

struct structure_bsp_header
{
	struct structure_bsp *bsp;
	unsigned long unused[4];
	unsigned long signature;
};
typedef char structure_bsp_header_size_check[sizeof(struct structure_bsp_header) == 0x18 ? 1 : -1];

/* ---------- private code */

/* the chamber: its collision, one leaf in one cluster that sees itself */
static void chamber_build(
	struct structure_bsp *bsp)
{
	struct collision_bsp *collision = cache_bsp_block(&bsp->collision_bsp, 1, sizeof(struct collision_bsp));
	struct structure_leaf *leaf = cache_bsp_block(&bsp->leaves, 1, sizeof(struct structure_leaf));
	struct structure_cluster *cluster = cache_bsp_block(&bsp->clusters, 1, sizeof(struct structure_cluster));
	unsigned long *pvs = cache_bsp_allocate(sizeof(unsigned long));
	real_rectangle3d room = { { -CHAMBER_HALF_WIDTH, CHAMBER_HALF_WIDTH, -CHAMBER_HALF_WIDTH, CHAMBER_HALF_WIDTH,
		0.f, CHAMBER_HEIGHT } };

	box_build(collision, cache_bsp_block, &room, FALSE);
	leaf->cluster_index = 0;
	cluster->sky_index = NONE;
	cluster->fog_reference = NONE;
	cluster->background_sound_palette_index = NONE;
	cluster->sound_environment_palette_index = NONE;
	cluster->weather_palette_index = NONE;
	*pvs = 1;
	bsp->cluster_data.size = sizeof(*pvs);
	bsp->cluster_data.address = pvs;
	bsp->world_bounds.x0 = bsp->world_bounds.y0 = -CHAMBER_HALF_WIDTH;
	bsp->world_bounds.x1 = bsp->world_bounds.y1 = CHAMBER_HALF_WIDTH;
	bsp->world_bounds.z0 = 0.f;
	bsp->world_bounds.z1 = CHAMBER_HEIGHT;
	bsp->vehicle_floor = 0.f;
	bsp->vehicle_ceiling = CHAMBER_HEIGHT;
}

/* the players' biped: a capsule a unit tall, a quarter round, that no fall hurts */
static long biped_build(
	void)
{
	struct animation_graph *animations;
	struct animation_graph_unit_seat *seat;
	struct animation_graph_weapon_class *weapon_class;
	struct animation_graph_weapon_type *weapon_type;
	struct biped_definition *biped;
	long animations_handle;
	long biped_handle;
	void *data;

	animations_handle = cache_tag_new(FOURCC('a', 'n', 't', 'r'), NONE, NONE, "stagekit\\biped",
		sizeof(struct animation_graph), &data);
	animations = data;
	seat = cache_block(&animations->unit_seats, 1, sizeof(*seat));
	strcpy(seat->label, "stand");
	weapon_class = cache_block(&seat->weapon_classes, 1, sizeof(*weapon_class));
	strcpy(weapon_class->label, "unarmed");
	weapon_type = cache_block(&weapon_class->weapon_types, 1, sizeof(*weapon_type));
	strcpy(weapon_type->label, "unarmed");
	biped_handle = cache_tag_new(FOURCC('b', 'i', 'p', 'd'), FOURCC('u', 'n', 'i', 't'), FOURCC('o', 'b', 'j', 'e'),
		"stagekit\\biped", sizeof(struct biped_definition), &data);
	biped = data;
	biped->object.type = _object_type_biped;
	cache_reference(&biped->object.model, prop_empty_model());
	cache_reference(&biped->object.animation_graph, animations_handle);
	/* (moved by physics from its input, as players are, not by its animations,
	at the engine's own walking and running speeds: bipeds.c, old player physics) */
	biped->biped.flags = FLAG(_biped_immune_to_falling_damage_bit) | FLAG(_biped_uses_player_physics_bit) |
		FLAG(_biped_uses_old_player_physics_bit);
	/* (the engine's own field of view, player_control.c's for no unit) */
	biped->unit.camera_field_of_view = DEGREES_TO_RADIANS(70.f);
	/* (StageKit's own round numbers: a body a unit tall, half crouched, a
	quarter round, its eyes a tenth below its top) */
	biped->biped.collision_radius = 0.25f;
	biped->biped.collision_height_standing = 1.f;
	biped->biped.collision_height_crouching = 0.5f;
	biped->biped.standing_camera_height = 0.9f;
	biped->biped.crouching_camera_height = 0.45f;
	/* (StageKit's own, in world units a tick: under the engine's gravity, a
	jump about 0.9 high) */
	biped->biped.jump_velocity = 0.08f;
	/* (StageKit's own angles, of exact sines and cosines: ground up to 45 degrees,
	slopes from 30 to 60 degrees that leave its speed as it is, a body that turns
	a full circle a second, and turns when standing 30 degrees off its facing,
	crouching in a quarter second) */
	biped->biped.maximum_slope_angle = DEGREES_TO_RADIANS(45.f);
	biped->biped.downhill_falloff_angle = biped->biped.uphill_falloff_angle = DEGREES_TO_RADIANS(30.f);
	biped->biped.downhill_cutoff_angle = biped->biped.uphill_cutoff_angle = DEGREES_TO_RADIANS(60.f);
	biped->biped.downhill_velocity_scale = biped->biped.uphill_velocity_scale = 1.f;
	biped->biped.moving_turning_speed = DEGREES_TO_RADIANS(360.f);
	biped->biped.stationary_turning_threshold = DEGREES_TO_RADIANS(30.f);
	biped->biped.crouch_transition_time = 0.25f;
	/* (and what bipeds.c reads of them, which a map carries made from them: the
	lowest ground normal's height, the heights of a move's direction where
	slopes start and stop scaling it, and a tick's crouch; cos 45, sin 30, sin
	60, cos 30) */
	biped->biped.runtime_minimum_normal_k = square_root(0.5f);
	biped->biped.runtime_downhill_k0 = -0.5f;
	biped->biped.runtime_downhill_k1 = -square_root(0.75f);
	biped->biped.runtime_uphill_k0 = 0.5f;
	biped->biped.runtime_uphill_k1 = square_root(0.75f);
	biped->biped.runtime_cosine_stationary_turning_threshold = square_root(0.75f);
	biped->biped.runtime_crouch_transition_velocity = 1.f / (biped->biped.crouch_transition_time * TICKS_PER_SECOND);
	/* (its model has no pelvis or head) */
	biped->biped.runtime_pelvis_node_index = biped->biped.runtime_head_node_index = NONE;
	/* (round it from its feet) */
	biped->object.bounding_radius = biped->biped.collision_height_standing;
	return biped_handle;
}

/* the globals: how the player looks round, the interface's tags, the player's
biped and its speeds, and falling damage */
static void globals_build(
	long font_handle,
	long strings_handle,
	long hud_handle,
	long biped_handle)
{
	struct game_globals *globals;
	struct game_globals_player_control *control;
	struct game_globals_interface_tag_references *interface;
	struct game_globals_player_information *player;
	real *look;
	void *data;

	cache_tag_new(FOURCC('m', 'a', 't', 'g'), NONE, NONE, "globals\\globals", sizeof(struct game_globals), &data);
	globals = data;
	control = cache_block(&globals->player_control, 1, sizeof(*control));
	look = cache_block(&control->look_function, 2, sizeof(real));
	/* (StageKit's own: a stick turns the view as far as it is pushed, at most 100
	degrees a second across and 50 up and down; no acceleration, a scale of 1,
	over a time player_control.c needs above 0) */
	look[0] = 0.f;
	look[1] = 1.f;
	control->look_default_pitch_rate = 50.f;
	control->look_default_yaw_rate = 100.f;
	control->look_acceleration_time = 1.f;
	control->look_acceleration_scale = 1.f;
	cache_block(&globals->rasterizer_data, 1, sizeof(struct game_globals_rasterizer_data));
	interface = cache_block(&globals->interface_tag_references, 1, sizeof(*interface));
	cache_reference(&interface->interface_tag_references[_interface_font_system], font_handle);
	cache_reference(&interface->interface_tag_references[_interface_font_terminal], font_handle);
	cache_reference(&interface->interface_tag_references[_interface_hud_globals], hud_handle);
	cache_reference(&interface->interface_tag_references[_interface_string_list_localization], strings_handle);
	player = cache_block(&globals->player_information, 1, sizeof(*player));
	cache_reference(&player->player_unit, biped_handle);
	/* (StageKit's own, a second: speeds in world units, accelerations in world
	units a second, bipeds.c taking a tick's of them; the engine has its own
	walking and running speeds) */
	player->sneak_forward_speed = 1.f;
	player->sneak_backward_speed = 1.f;
	player->sneak_sideways_speed = 1.f;
	player->sneak_acceleration = 10.f;
	player->airborne_acceleration = 2.f;
	cache_block(&globals->falling_damage, 1, FALLING_DAMAGE_BYTES);
	/* (a camera track and a first person interface, as every map has: none,
	which the game reads as it read their absence; game_globals_check) */
	cache_block(&globals->camera, 1, sizeof(struct tag_reference));
	cache_block(&globals->first_person_interface, 1, sizeof(struct game_globals_first_person_interface));
}

/* ---------- public code */

void stage_build(
	struct stage *stage,
	struct stagekit_scenario const *scenario)
{
	struct structure_bsp_header *bsp_header;
	struct structure_bsp *bsp;
	struct scenario_structure_bsp_reference *bsp_reference;
	struct string_list *strings;
	struct string_list_entry *language;
	struct data_array *nodes;
	long scenario_handle, bsp_handle, font_handle, strings_handle, hud_handle;
	char *code;
	void *data;

	cache_open();
	/* the structure BSP, at the top of the tag cache: its header, then itself */
	bsp_header = cache_bsp_allocate(sizeof(*bsp_header));
	bsp = cache_bsp_allocate(sizeof(struct structure_bsp));
	bsp_header->bsp = bsp;
	bsp_header->signature = FOURCC('s', 'b', 's', 'p');

	scenario_handle = cache_tag_new(FOURCC('s', 'c', 'n', 'r'), NONE, NONE, "stagekit\\void", sizeof(struct scenario),
		&data);
	stage->scenario = data;
	cache_set_scenario(scenario_handle);
	bsp_handle = cache_tag_new(FOURCC('s', 'b', 's', 'p'), NONE, NONE, "stagekit\\chamber", 0, &data);
	cache_tag_no_data(bsp_handle);
	cache_no_reference(&stage->scenario->custom_object_names, FOURCC('u', 's', 't', 'r'));
	cache_no_reference(&stage->scenario->ingame_help_text, FOURCC('u', 's', 't', 'r'));
	cache_no_reference(&stage->scenario->hud_messages, FOURCC('h', 'm', 't', ' '));
	cache_no_reference(&bsp->lightmap_group, FOURCC('b', 'i', 't', 'm'));
	chamber_build(bsp);

	/* the interface's font, strings (the first: the language code) and HUD */
	font_handle = cache_tag_new(FOURCC('f', 'o', 'n', 't'), NONE, NONE, "stagekit\\font", sizeof(struct font_header),
		&data);
	strings_handle = cache_tag_new(FOURCC('s', 't', 'r', '#'), NONE, NONE, "stagekit\\localization",
		sizeof(struct string_list), &data);
	strings = data;
	language = cache_block(&strings->strings, 1, sizeof(*language));
	code = cache_allocate(2);
	code[0] = '0';
	language->string.size = 2;
	language->string.address = code;
	hud_handle = cache_tag_new(FOURCC('h', 'u', 'd', 'g'), NONE, NONE, "stagekit\\hud",
		sizeof(struct hud_globals_definition), &data);
	/* (the HUD an unarmed player's weapon HUD is: hud_weapon.c reads it unchecked) */
	{
		struct hud_globals_definition *hud = data;

		cache_reference(&hud->defaults.default_weapon_hud,
			cache_tag_new(FOURCC('w', 'p', 'h', 'i'), NONE, NONE, "stagekit\\unarmed",
				sizeof(struct weapon_hud_interface_definition), &data));
	}

	globals_build(font_handle, strings_handle, hud_handle, biped_build());

	stage->scenario->hs_syntax_data.size = sizeof(struct data_array) + SCRIPT_NODES * SCRIPT_NODE_BYTES;
	nodes = stage->scenario->hs_syntax_data.address = cache_allocate(stage->scenario->hs_syntax_data.size);
	strcpy(nodes->name, "script node");
	nodes->maximum_count = SCRIPT_NODES;
	nodes->size = SCRIPT_NODE_BYTES;
	nodes->valid = TRUE;
	nodes->signature = FOURCC('d', '@', 't', '@');
	/* (seeded as data_delete_all seeds it: the name's first two characters and
	the high bit; an array with none gives out no node, and the console needs some) */
	nodes->next_identifier = (short)(((unsigned char)nodes->name[1] << 8 | (unsigned char)nodes->name[0]) | 0x8000);
	nodes->data = nodes + 1;
	stage->scenario->hs_string_constants.size = SCRIPT_STRINGS;
	stage->scenario->hs_string_constants.address = cache_allocate(SCRIPT_STRINGS);

	scenario->build(stage);

	/* (a scenario that places no player start has one at the middle of the floor) */
	if (!stage->scenario->players.count)
	{
		real_point3d middle = { { 0.f, 0.f, 0.f } };

		player_start(stage, &middle);
	}
	stage->scenario->type = _scenario_type_solo;

	bsp_reference = cache_block(&stage->scenario->structure_bsp_references, 1, sizeof(*bsp_reference));
	cache_bsp_place(bsp_reference);
	cache_reference(&bsp_reference->structure_bsp, bsp_handle);
}
