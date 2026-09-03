/*
SAVED_GAME_FILES.H

header included in hcex build.
*/

#ifndef __SAVED_GAME_FILES_H
#define __SAVED_GAME_FILES_H
#pragma once

#include "cseries/cseries.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/SAVED_GAME_FILES.C */

long saved_game_file_find_profile_index_for_directory_path(
	char *directory_path,
	long file_type);
boolean saved_game_file_get_path_to_enclosing_directory(
	long profile_index,
	char *directory_path);
word saved_game_file_get_type(
	long profile_index);
void saved_game_file_remember_last_used_multiplayer_variant_directory(
	char *directory_path);
void saved_game_file_remember_player1_last_used_profile_directory(
	char *directory_path);
boolean saved_game_file_retrieve_player1_last_used_profile_directory(
	char *directory_path);
boolean saved_game_files_take_mutex(
	void);
void saved_game_files_release_mutex(
	void);
boolean saved_game_file_name_unique(
	wchar_t const *name);
short saved_game_perform_file_system_checks(
	void);
void enumerate_memory_units_test(
	void);

/* ---------- globals */

/* ---------- public code */

#endif // __SAVED_GAME_FILES_H
