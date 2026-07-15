/*
DIRECTOR.H

header included in hcex build.
*/

#ifndef __DIRECTOR_H
#define __DIRECTOR_H
#pragma once

/* ---------- headers */

#include "cseries/cseries.h"
#include "networking/network_connection.h"

/* ---------- constants */

enum
{
	NUMBER_OF_DIRECTOR_GAME_MODES = 5
};

/* ---------- macros */

/* ---------- structures */

struct director_scripting_globals
{
	boolean camera_scripted;
	byte pad[3];
};

struct director_player_globals
{
	byte pad0[4];
	short mode;
	boolean mode_changed;
	byte pad7[0x52];
	boolean inhibit_facing;
	boolean inhibit_input;
	byte pad5B[0x9D];
};

/* ---------- prototypes/DIRECTOR.C */

void director_initialize(
	void);
void director_dispose(
	void);
void director_inhibit_facing(
	short local_player_index);
void director_inhibit_input(
	short local_player_index);
boolean director_inhibited_facing(
	short local_player_index);
boolean director_inhibited_input(
	short local_player_index);
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
