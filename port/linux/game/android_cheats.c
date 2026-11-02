/* Android cheat menu inspired by thelinkin3000/halo-ce-universal's overlay
(406f793f4c08751f854a3615ed244ea3b1f90b88), which credits hardran3's
OpenCE-Ultrahand and HaloDrones packages. The fork's general console and
renderer overlay are not imported; these requests use existing cheat APIs.
See docs/reading/bots_and_android_features.md for credits and constraints. */
#include "cseries.h"
#include "android_cheats.h"

#ifdef HALO_ANDROID
#include "game/cheats.h"
#include "networking/network_game_globals.h"

int config_boolean(char const *name);
int touch_game_playing(void);
int touch_game_cinematic_playing(void);
void host_android_cheats_state(int state);
int host_android_cheats_read(void);

/* IDs 0..8 are switches; 9..12 are one-shot actions. The Android UI and
host queue use the same IDs. Bit 8 carries the requested switch value. */
extern boolean rider_ejection;
static boolean *const switches[] =
{
	&cheat.deathless_player, &cheat.infinite_ammo, &cheat.bottomless_clip, &cheat.super_jump,
	&cheat.omnipotent, &cheat.jetpack, &cheat.medusa, &cheat.bump_possession, &rider_ejection
};
static boolean previous[NUMBEROF(switches)], applied[NUMBEROF(switches)];
static unsigned long owned;

void android_cheats_reset(void)
{
	long index;
	for (index = 0; index < (long)NUMBEROF(switches); index++)
	{
		/* Leave a subsequent console/script change alone. */
		if (TEST_FLAG(owned, index) && *switches[index] == applied[index])
			*switches[index] = previous[index];
	}
	owned = 0;
	host_android_cheats_state(0);
}

void android_cheats_update(void)
{
	int enabled = config_boolean("android.cheat_menu");
	/* Match the desktop console: a joined client never changes the game,
	including the lobby before distributed-client simulation begins. */
	int allowed = enabled && touch_game_playing() &&
		!network_game_distributed_client() &&
		(!global_network_game_client_get() || global_network_game_server_get());
	int request, count, state = enabled ? 1 : 0;
	long index;
	if (!allowed)
	{
		android_cheats_reset();
		host_android_cheats_state(state);
		return;
	}
	/* Four requests at most per input poll; the host queue is bounded too. */
	for (count = 0; !touch_game_cinematic_playing() && count < 4 && (request = host_android_cheats_read()) >= 0; count++)
	{
		index = request & 0xFF;
		if ((request & ~0x10F) != 0 || index > 12 || (index >= 9 && (request & 0x100))) continue;
		if (index < (long)NUMBEROF(switches))
		{
			if (!TEST_FLAG(owned, index)) previous[index] = *switches[index];
			SET_FLAG(owned, index, TRUE);
			/* Easy Rider is on when the engine's rider ejection is off. */
			applied[index] = ((request & 0x100) != 0) != (index == 8);
			*switches[index] = applied[index];
		}
		else if (index == 9) cheat_all_weapons();
		else if (index == 10) cheat_all_powerups();
		else if (index == 11) cheat_active_camouflage_local_player(0);
		else if (index == 12) cheat_all_vehicles();
	}
	if (!touch_game_cinematic_playing()) state |= 2;
	for (index = 0; index < (long)NUMBEROF(switches); index++)
		if ((*switches[index] != FALSE) != (index == 8)) state |= 1 << (8 + index);
	host_android_cheats_state(state);
}
#endif
