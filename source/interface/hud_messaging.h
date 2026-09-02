/*
HUD_MESSAGING.H

header included in hcex build.
*/

#ifndef __HUD_MESSAGING_H
#define __HUD_MESSAGING_H
#pragma once

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes/HUD_MESSAGING.C */

void hud_messaging_initialize(
	void);
void hud_messaging_initialize_for_new_map(
	void);
void hud_messaging_dispose_from_old_map(
	void);
void hud_messaging_dispose(
	void);
void hud_messaging_globals_update(
	void);
void scripted_hud_set_flashing_state(
	boolean flash);
void scripted_hud_set_timer_position(
	short x,
	short y,
	short corner);
void scripted_hud_messages_clear(
	void);
void hud_print_message(
	short local_player_index,
	wchar_t const *message);
void hud_add_item_message(
	short local_player_index,
	long item_definition_index,
	short quantity,
	char message_offset);

/* ---------- globals */

/* ---------- public code */

#endif // __HUD_MESSAGING_H
