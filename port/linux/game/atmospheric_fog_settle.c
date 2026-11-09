/*
ATMOSPHERIC_FOG_SETTLE.C

The atmospheric fog settling over time as well as over the camera's travel
(scenario.c's scenario_get_atmospheric_fog).

Each sky has an outdoor fog and an indoor one, and a local player's fog
moves from the one it had toward the one where the camera is by as much as
the camera has moved since the last frame: its distances by a world unit
for each the camera moves, its color, density and indoor fog screen by 5%.
A camera standing still kept the fog it had, half way between. In b40, out
of the tunnels onto the bridges (indoor fog dark blue, opaque at 120 world
units; outdoor white, at 200), a player who stopped to look up saw the far
cliffs flat and dark in the tunnels' fog until they walked on: the color
takes 20 world units of travel, the opaque distance 80, more than ten
seconds on foot.

It now also settles toward the fog where the camera is over time, by
seconds / (0.5 + seconds) of the way each frame (within 15% of it a
second on, about 2% after two seconds, at any frame rate), on top of what
the camera's travel moves it: a camera standing still has the fog of where
it is a second or two after it arrives, and one moving gets it no later
than before. The time is the game's, continuous between ticks: a paused
game's fog waits with it.
*/

#include "cseries.h"
#include "game/game.h"
#include "tag_files/tag_groups.h"
#include "scenario/scenario.h"
#include "scenario/sky_definitions.h"
#include "atmospheric_fog_settle.h"

/* seconds: the fog moves seconds / (ATMOSPHERIC_FOG_SETTLE_TIME + seconds)
of the way to where the camera is each frame */
#define ATMOSPHERIC_FOG_SETTLE_TIME 0.5f
#define ATMOSPHERIC_FOG_SETTLE_PLAYERS 4

/* how far toward where the camera is a frame of so many seconds moves the
fog (none for none, or a time gone backward) */
real atmospheric_fog_settle_fraction(
	real seconds)
{
	if (!(seconds > 0.f))
		return 0.f;
	return seconds / (ATMOSPHERIC_FOG_SETTLE_TIME + seconds);
}

static void atmospheric_fog_settle_value(
	real *value,
	real target,
	real fraction)
{
	*value += (target - *value) * fraction;
}

/* a frame's fraction of the way to the fog given, from the fog state (a
local player's, its time last settled kept here) */
void atmospheric_fog_settle_state(
	struct atmospheric_fog_state *state,
	struct sky_atmospheric_fog const *fog,
	real indoor_fog_scale,
	real fraction)
{
	atmospheric_fog_settle_value(&state->atmospheric_start_distance, fog->start_distance, fraction);
	atmospheric_fog_settle_value(&state->atmospheric_opaque_distance, fog->opaque_distance, fraction);
	atmospheric_fog_settle_value(&state->atmospheric_maximum_density, fog->maximum_density, fraction);
	atmospheric_fog_settle_value(&state->atmospheric_color.red, fog->color.red, fraction);
	atmospheric_fog_settle_value(&state->atmospheric_color.green, fog->color.green, fraction);
	atmospheric_fog_settle_value(&state->atmospheric_color.blue, fog->color.blue, fraction);
	atmospheric_fog_settle_value(&state->indoor_fog_scale, indoor_fog_scale, fraction);
}

void atmospheric_fog_settle(
	short local_player_index,
	struct atmospheric_fog_state *state,
	struct sky_atmospheric_fog const *fog,
	real indoor_fog_scale)
{
	static real settled_at[ATMOSPHERIC_FOG_SETTLE_PLAYERS];
	static boolean settled[ATMOSPHERIC_FOG_SETTLE_PLAYERS];
	real now;

	if (local_player_index < 0 || local_player_index >= ATMOSPHERIC_FOG_SETTLE_PLAYERS || !state || !fog)
		return;
	now = render_interpolation_game_time_sec(game_time_get());
	if (settled[local_player_index])
	{
		atmospheric_fog_settle_state(state, fog, indoor_fog_scale,
			atmospheric_fog_settle_fraction(now - settled_at[local_player_index]));
	}
	settled_at[local_player_index] = now;
	settled[local_player_index] = TRUE;
}
