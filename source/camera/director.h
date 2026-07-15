/*
DIRECTOR.H

header included in hcex build.
*/

#ifndef __DIRECTOR_H
#define __DIRECTOR_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "networking/network_connection.h"

/* ---------- constants */

enum
{
	NUMBER_OF_DIRECTOR_GAME_MODES = 5
};

/* ---------- macros */

/* ---------- structures */

typedef short director_perspective;

typedef void (*director_camera_update_proc)(
	void *camera,
	void *command,
	void *result);

struct director_scripting_globals
{
	boolean camera_scripted;
	byte pad[3];
};

struct director_camera_state
{
	byte pad0[4];
	real transition;
	director_camera_update_proc update;
	byte padC[0x45];
	boolean inhibit_facing;
	boolean inhibit_input;
	byte pad53[3];
	director_perspective perspective;
	byte pad58[0x68];
	boolean unknown_c0;
	byte padC1[3];
	real unknown_c4;
	byte padC8[0x28];
};

struct director_player_globals
{
	byte pad0[4];
	short mode;
	boolean mode_changed;
	byte pad7;
	struct director_camera_state camera;
};

/* ---------- prototypes/DIRECTOR.C */

void director_initialize(
	void);
void director_dispose(
	void);
void director_dispose_from_old_map(
	void);
void director_inhibit_facing(
	short local_player_index);
void director_inhibit_input(
	short local_player_index);
boolean director_inhibited_facing(
	short local_player_index);
boolean director_inhibited_input(
	short local_player_index);
director_perspective director_get_perspective(
	short local_player_index);
short director_desired_perspective(
	long unit_index,
	director_perspective *perspective);
short director_camera_deterministic(
	long unit_index,
	real_point3d *position,
	real_vector3d *forward);
void director_set_mode(
	short mode);
void director_initialize_for_new_map(
	void);
void director_script_camera(
	boolean scripted);
void director_initialize_for_saved_game(
	void);

/* ---------- globals */

extern struct director_scripting_globals *director_camera_scripted;
extern struct director_player_globals director_globals[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS];

/* ---------- public code */

#endif // __DIRECTOR_H
