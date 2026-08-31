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

void hud_messaging_dispose_from_old_map(
	void);
void hud_messaging_dispose(
	void);
void scripted_hud_messages_clear(void);
void hud_print_message(
	short local_player_index,
	wchar_t const *message);

/* ---------- globals */

/* ---------- public code */

#endif // __HUD_MESSAGING_H
