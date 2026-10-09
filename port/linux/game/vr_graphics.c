/*
VR_GRAPHICS.C

The VR build's graphics settings (config.toml [graphics]): which of the
game's costlier effects it draws, for headsets with less GPU to spare than
the Quest 3.

graphics.preset chooses a set ("low", "medium", "high"; "max" is "high"
with a sharper picture, port/linux/src/vr_frame.c), and "auto" chooses by
headset: low on the first Quest, medium on the Quest 2, high otherwise. Each
effect's own setting ("auto" follows the preset, or "on" or "off") overrides
it.

Each effect is a switch the game already has: the rasterizer's console
variables (rasterizer_debug_options) and render.c's effect switches. Scripts
and the console set some of them too (render_effects turns the particles
off in some cutscenes), so a switch is drawn only while both the game and
the setting want it: a change made by the game since the last frame is taken
as the game's wish, kept, and combined with the setting's.
*/

#ifdef HALO_VR

#include "cseries.h"
#include "game/cheats.h"
#include "rasterizer/rasterizer_console_vars.h"

#include "halo_vr.h"
#include "../src/vr.h"
#include "../src/port_config.h"

#include <string.h>

/* port/linux/src/platform.h (a variadic call needs its prototype in scope
on the Android guest's ABI) */
void platform_log(const char *format, ...);
/* source/networking/network_game_globals.c */
boolean network_game_distributed_client(void);

/* source/render/render.c, render_objects.c; source/effects/decals.c,
weather_particle_systems.c */
extern boolean render_shadows;
extern boolean render_particles_enabled;
extern boolean render_particle_systems_enabled;
extern boolean render_contrails_enabled;
extern boolean render_weather_particle_systems_enabled;
extern boolean decals_enabled;
extern boolean weather;

enum
{
	_preset_low,
	_preset_medium,
	_preset_high,
	NUMBER_OF_PRESETS
};

/* an effect: its setting, whether each preset draws it (low, medium,
high), and the game's switches it governs */
struct graphics_effect
{
	const char *setting;
	boolean preset[NUMBER_OF_PRESETS];
	boolean *switches[3];
};

static struct graphics_effect effects[] =
{
	{ "graphics.shadows", { FALSE, FALSE, TRUE },
		{ &render_shadows, &rasterizer_debug_options.draw_environment_shadows } },
	{ "graphics.dynamic_lights", { TRUE, TRUE, TRUE },
		{ &rasterizer_debug_options.draw_environment_diffuse_lights } },
	{ "graphics.specular", { FALSE, TRUE, TRUE },
		{ &rasterizer_debug_options.draw_environment_specular_lights,
		  &rasterizer_debug_options.draw_environment_specular_lightmaps } },
	{ "graphics.reflections", { FALSE, FALSE, TRUE },
		{ &rasterizer_debug_options.draw_environment_reflections } },
	{ "graphics.bump_mapping", { FALSE, TRUE, TRUE },
		{ &rasterizer_debug_options.bump_mapping_enabled } },
	{ "graphics.detail_objects", { FALSE, TRUE, TRUE },
		{ &rasterizer_debug_options.draw_detail_objects } },
	{ "graphics.decals", { TRUE, TRUE, TRUE },
		{ &decals_enabled, &rasterizer_debug_options.draw_environment_decals } },
	{ "graphics.particles", { TRUE, TRUE, TRUE },
		{ &render_particles_enabled, &render_particle_systems_enabled } },
	{ "graphics.contrails", { FALSE, TRUE, TRUE },
		{ &render_contrails_enabled } },
	{ "graphics.weather", { FALSE, TRUE, TRUE },
		{ &weather, &render_weather_particle_systems_enabled } },
	{ "graphics.lens_flares", { FALSE, FALSE, TRUE },
		{ &rasterizer_debug_options.draw_lens_flares } },
	{ "graphics.fog_screen", { FALSE, TRUE, TRUE },
		{ &rasterizer_debug_options.draw_environment_fog_screen } },
	{ "graphics.camouflage_multipass", { FALSE, TRUE, TRUE },
		{ &rasterizer_debug_options.active_camouflage_multipass_enabled } },
};

#define NUMBER_OF_EFFECTS (sizeof(effects) / sizeof(effects[0]))
#define SWITCHES_PER_EFFECT (sizeof(effects[0].switches) / sizeof(effects[0].switches[0]))

static struct
{
	int generation;
	boolean read;
	/* whether the settings allow each effect */
	boolean allowed[NUMBER_OF_EFFECTS];
	/* what the game wants of each switch, and what was written last frame */
	boolean wanted[NUMBER_OF_EFFECTS][SWITCHES_PER_EFFECT];
	boolean written[NUMBER_OF_EFFECTS][SWITCHES_PER_EFFECT];
} vr_graphics;

static int preset_for_headset(void)
{
	const char *system = vr_system_name();

	if (strstr(system, "Quest 2") || strstr(system, "Quest2"))
		return _preset_medium;
	if (strstr(system, "Quest") &&
		!strstr(system, "Quest 3") && !strstr(system, "Quest3") && !strstr(system, "Quest Pro"))
	{
		return _preset_low;
	}
	return _preset_high;
}

static int preset_setting(void)
{
	const char *preset = config_string("graphics.preset");

	if (!strcmp(preset, "low"))
		return _preset_low;
	if (!strcmp(preset, "medium"))
		return _preset_medium;
	if (!strcmp(preset, "high") || !strcmp(preset, "max"))
		return _preset_high;
	return preset_for_headset();
}

static void read_settings(void)
{
	int preset = preset_setting();
	unsigned int index;

	for (index = 0; index < NUMBER_OF_EFFECTS; index++)
	{
		const char *value = config_string(effects[index].setting);

		vr_graphics.allowed[index] =
			!strcmp(value, "on") ? TRUE :
			!strcmp(value, "off") ? FALSE :
			effects[index].preset[preset];
	}
	platform_log("vr: graphics preset %s (%s)", preset == _preset_low ? "low" : preset == _preset_medium ? "medium" : "high",
		config_string("graphics.preset"));
	for (index = 0; index < NUMBER_OF_EFFECTS; index++)
		platform_log("vr:   %s %s", effects[index].setting, vr_graphics.allowed[index] ? "on" : "off");
}

void vr_graphics_apply(void)
{
	unsigned int index, which;

	if (!vr_graphics.read)
	{
		/* the game's switches as it starts are its wishes */
		for (index = 0; index < NUMBER_OF_EFFECTS; index++)
		{
			for (which = 0; which < SWITCHES_PER_EFFECT && effects[index].switches[which]; which++)
			{
				vr_graphics.wanted[index][which] = *effects[index].switches[which];
				vr_graphics.written[index][which] = *effects[index].switches[which];
			}
		}
		vr_graphics.generation = vr_settings_generation();
		read_settings();
		vr_graphics.read = TRUE;
	}
	else if (vr_graphics.generation != vr_settings_generation())
	{
		vr_graphics.generation = vr_settings_generation();
		read_settings();
	}

	for (index = 0; index < NUMBER_OF_EFFECTS; index++)
	{
		for (which = 0; which < SWITCHES_PER_EFFECT && effects[index].switches[which]; which++)
		{
			boolean *game_switch = effects[index].switches[which];
			boolean value;

			/* the game (a script, the console) changed it since: its wish */
			if (*game_switch != vr_graphics.written[index][which])
				vr_graphics.wanted[index][which] = *game_switch;
			/* (in another's game, what everyone must draw is drawn: its
			host's rules, cheats.c, which would put it back every frame) */
			value = vr_graphics.wanted[index][which] && (vr_graphics.allowed[index] ||
				(network_game_distributed_client() && cheats_network_client_switch_enforced(game_switch)));
			*game_switch = value;
			vr_graphics.written[index][which] = value;
		}
	}
}

#endif
