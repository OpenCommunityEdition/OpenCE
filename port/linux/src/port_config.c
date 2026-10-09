/*
PORT_CONFIG.C

The native ports' settings (port_config.h), parsed with tomlc17
(port/third_party/tomlc17). Every setting is in the table below with its
type, default, the HALO_* environment variable that overrides it and the
comment written into a new file. The file is read once, on the first
question; unknown keys and values of the wrong type are reported in the log
and the defaults used instead, and the file itself is never rewritten once
it exists, so that the player's edits and comments stay.
*/

#include "platform.h"
#include "port_config.h"
#include "halo_port_limits.h"
#include "tomlc17.h"

#include <SDL3/SDL.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- the settings */

enum config_type
{
	_config_boolean,
	_config_integer,
	_config_real,
	_config_string,
};

/* how the setting's environment variable sets it */
enum config_environment
{
	/* the variable's text is the value ("0", "false", "no" and "off" are
	false for a boolean) */
	_environment_value,
	/* the variable being set at all makes it true */
	_environment_set_is_true,
	/* the variable being set at all makes it false */
	_environment_set_is_false,
};

/* the builds a setting means something in, and is written for */
enum
{
	_platform_linux = 1,
	_platform_android = 2,
	_platform_windows = 4,
	_platform_desktop = _platform_linux | _platform_windows,
	_platform_all = _platform_desktop | _platform_android,
	/* the Android build for VR headsets (HALO_VR) only */
	_platform_vr = 8,
};

struct config_setting
{
	const char *name;
	enum config_type type;
	/* as it is written in the file */
	const char *default_value;
	const char *environment;
	enum config_environment environment_style;
	unsigned platforms;
	const char *comment;
};

static const struct config_setting config_settings[] =
{
	{ "browser.show_empty", _config_boolean, "true", "HALO_BROWSER_SHOW_EMPTY", _environment_value, _platform_all,
		"Include empty games in the in-game server browser." },
	{ "browser.show_full", _config_boolean, "true", "HALO_BROWSER_SHOW_FULL", _environment_value, _platform_all,
		"Include full games in the in-game server browser." },
	{ "browser.engine", _config_integer, "-1", "HALO_BROWSER_ENGINE", _environment_value, _platform_all,
		"Browser game type: -1 all, 0 co-op, 1 CTF, 2 Slayer, 3 Oddball, 4 King, 5 Race." },
	{ "browser.teams", _config_string, "\"all\"", "HALO_BROWSER_TEAMS", _environment_value, _platform_all,
		"Browser teams: all, teams, or ffa." },
	{ "browser.passwords", _config_string, "\"all\"", "HALO_BROWSER_PASSWORDS", _environment_value, _platform_all,
		"Browser password filter: all, locked, or unlocked." },
	{ "browser.maps", _config_string, "\"all\"", "HALO_BROWSER_MAPS", _environment_value, _platform_all,
		"Browser map filter: all or known (catalog only; joining still checks actual game files)." },
	{ "display.mode", _config_string, "\"\"", "HALO_DISPLAY_MODE", _environment_value, _platform_desktop,
		"\"fullscreen\" takes the display (at display.resolution's mode),\n"
		"\"borderless\" is a window over the whole desktop, \"windowed\" a window\n"
		"(display.window_size). Empty: display.fullscreen's (true: borderless).\n"
		"F11 switches to the window and back." },

	{ "display.resolution", _config_string, "\"native\"", "HALO_RESOLUTION", _environment_value, _platform_desktop,
		"What fullscreen and borderless draw at: \"native\", the display's own, or\n"
		"\"<width>x<height>\" (\"1920x1080\"), 640x480 or more. Fullscreen sets the\n"
		"display to it; borderless draws it scaled to the display." },

	{ "display.resolution_scaling", _config_string, "\"native\"", "HALO_RESOLUTION_SCALING", _environment_value,
		_platform_desktop,
		"\"native\" draws at the window's resolution (fullscreen, the display's or\n"
		"display.resolution); \"original\" draws the Xbox's 640x480 and scales it\n"
		"up." },

	{ "display.window_size", _config_string, "\"\"", "HALO_WINDOW_SIZE", _environment_value, _platform_desktop,
		"The window's size, \"<width>x<height>\" (\"1920x1080\"), 640x480 or more (it\n"
		"can be resized). Empty: display.window_scale's." },

	{ "display.max_fps", _config_integer, "0", "HALO_MAX_FPS", _environment_value, _platform_desktop,
		"With vsync off, the most frames a second: 0 for twice the display's\n"
		"refresh rate, -1 for no limit (which can hang some Intel graphics)." },

	{ "display.anti_aliasing", _config_string, "\"off\"", "HALO_ANTI_ALIASING", _environment_value, _platform_all,
		"Smoothing of jagged edges, which the Xbox did not have: \"off\"; \"fxaa\"\n"
		"or \"smaa\" smooth the 3D view once it is drawn (the HUD and menus stay\n"
		"sharp); \"ssaa2x\" draws at twice the resolution each way (four times\n"
		"the work); \"msaa2x\", \"msaa4x\" or \"msaa8x\" draw with that many samples\n"
		"a pixel. Android has \"fxaa\" for \"smaa\", and no \"ssaa2x\"." },

	{ "display.shadow_resolution", _config_integer, "128", "HALO_SHADOW_RESOLUTION", _environment_value,
		_platform_all,
		"The size the objects' shadows are drawn at, in pixels each way: 128 as\n"
		"on the Xbox, or 256, 512 or 1024 for smoother edges, as soft." },

	{ "display.menus", _config_string, "\"pc\"", "HALO_MENUS", _environment_value, _platform_all,
		"The menus: \"pc\" for the PC version's main menu (port/assets/menus,\n"
		"and a menus folder here for your own), \"xbox\" for the Xbox's." },

	{ "display.per_pixel_lighting", _config_boolean, "false", "HALO_PER_PIXEL_LIGHTING", _environment_value,
		_platform_all,
		"Light the models (characters, weapons, vehicles, scenery) for each\n"
		"pixel by the lights the game gives them, without the facets the light\n"
		"of each vertex shows across curved surfaces; false lights each vertex,\n"
		"as the Xbox does." },

	{ "audio.music_volume", _config_real, "1.0", "HALO_MUSIC_VOLUME", _environment_value, _platform_all,
		"The music's volume, 0.0 to 1.0 (of audio.volume)." },

	{ "audio.effects_volume", _config_real, "1.0", "HALO_EFFECTS_VOLUME", _environment_value, _platform_all,
		"The volume of every other sound (effects and speech), 0.0 to 1.0 (of\n"
		"audio.volume)." },

	{ "audio.reverb", _config_boolean, "false", "HALO_REVERB", _environment_value, _platform_all,
		"Reverberate the world's sounds as the place the player is in does (the\n"
		"maps' sound environments, as the Xbox's I3DL2 reverb did); false keeps\n"
		"them dry." },

	{ "input.mouse_vertical_sensitivity", _config_real, "0.0", "HALO_MOUSE_VERTICAL_SENSITIVITY", _environment_value,
		_platform_desktop,
		"How far the view turns up and down for the mouse's movement; 0 for the\n"
		"same as input.mouse_sensitivity." },

	{ "controls.move_forward", _config_string, "\"W\"", "HALO_KEY_MOVE_FORWARD", _environment_value, _platform_all,
		"The keyboard and mouse's controls, which Settings > Controls Setup\n"
		"changes: up to two keys or buttons each, separated by a comma. Keys by\n"
		"their names (\"W\", \"Space\", \"Left Ctrl\", \"F1\"), and \"Mouse Left\",\n"
		"\"Mouse Right\", \"Mouse Middle\", \"Mouse 4\", \"Mouse 5\", \"Wheel\" (either\n"
		"way), \"Wheel Up\" and \"Wheel Down\"; empty for none. Moving forward:" },

	{ "controls.move_backward", _config_string, "\"S\"", "HALO_KEY_MOVE_BACKWARD", _environment_value, _platform_all,
		"Moving backward." },

	{ "controls.strafe_left", _config_string, "\"A\"", "HALO_KEY_STRAFE_LEFT", _environment_value, _platform_all,
		"Moving left." },

	{ "controls.strafe_right", _config_string, "\"D\"", "HALO_KEY_STRAFE_RIGHT", _environment_value, _platform_all,
		"Moving right." },

	{ "controls.jump", _config_string, "\"Space\"", "HALO_KEY_JUMP", _environment_value, _platform_all,
		"Jumping (and skipping cutscenes)." },

	{ "controls.crouch", _config_string, "\"Left Ctrl, C\"", "HALO_KEY_CROUCH", _environment_value, _platform_all,
		"Crouching." },

	{ "controls.fire", _config_string, "\"Mouse Left\"", "HALO_KEY_FIRE", _environment_value, _platform_all,
		"Firing." },

	{ "controls.throw_grenade", _config_string, "\"Mouse Right, G\"", "HALO_KEY_THROW_GRENADE", _environment_value,
		_platform_all,
		"Throwing a grenade." },

	{ "controls.melee", _config_string, "\"F, Mouse 4\"", "HALO_KEY_MELEE", _environment_value, _platform_all,
		"Melee attack." },

	{ "controls.reload", _config_string, "\"R\"", "HALO_KEY_RELOAD", _environment_value, _platform_all,
		"Reloading." },

	{ "controls.zoom", _config_string, "\"Z, Mouse Middle\"", "HALO_KEY_ZOOM", _environment_value, _platform_all,
		"Zooming the scope." },

	{ "controls.switch_weapon", _config_string, "\"Wheel, 1\"", "HALO_KEY_SWITCH_WEAPON", _environment_value,
		_platform_all,
		"Switching weapons." },

	{ "controls.switch_grenade", _config_string, "\"X\"", "HALO_KEY_SWITCH_GRENADE", _environment_value, _platform_all,
		"Switching grenades." },

	{ "controls.action", _config_string, "\"E\"", "HALO_KEY_ACTION", _environment_value, _platform_all,
		"The action: picking up (held: swapping weapons), entering and leaving\n"
		"vehicles, pressing switches; never reloading (the controller's X does\n"
		"when there is nothing to act on)." },

	{ "controls.flashlight", _config_string, "\"Q\"", "HALO_KEY_FLASHLIGHT", _environment_value, _platform_all,
		"The flashlight." },

	{ "controls.scoreboard", _config_string, "\"Tab\"", "HALO_KEY_SCOREBOARD", _environment_value, _platform_all,
		"Showing the scores (the controller's Back)." },

	{ "controls.pause", _config_string, "\"Escape\"", "HALO_KEY_PAUSE", _environment_value, _platform_all,
		"The pause menu (the controller's Start)." },

	{ "debug.menu_open", _config_string, "\"\"", "HALO_MENU_OPEN", _environment_value, _platform_all,
		"Start on this screen of the menus (port/assets/menus) instead of the main\n"
		"menu, a player profile being edited; empty for the main menu." },

	{ "pvp.time_limit", _config_integer, "0", "HALO_PVP_TIME_LIMIT", _environment_value, _platform_all,
		"Match time limit in minutes: 0 (unlimited) to 1440." },

	{ "pvp.friendly_fire", _config_integer, "0", "HALO_PVP_FRIENDLY_FIRE", _environment_value, _platform_all,
		"Team damage: 0 on, 1 off, 2 shields only, 3 explosives only." },

	{ "pvp.vehicle_respawn_time", _config_integer, "0", "HALO_PVP_VEHICLE_RESPAWN_TIME", _environment_value, _platform_all,
		"Empty/destroyed vehicle respawn seconds: 0 never, up to 3600." },

	{ "pvp.auto_team_balance", _config_boolean, "false", "HALO_PVP_AUTO_TEAM_BALANCE", _environment_value, _platform_all,
		"Balance teams in the lobby and on death; host authority." },

	{ "pvp.radar_players", _config_integer, "0", "HALO_PVP_RADAR_PLAYERS", _environment_value, _platform_all,
		"Motion tracker: 0 all, 1 friends only, 2 none." },

	{ "pvp.custom_loadout", _config_boolean, "false", "HALO_PVP_CUSTOM_LOADOUT", _environment_value, _platform_all,
		"Use selected primary/secondary weapons instead of variant defaults." },

	{ "pvp.primary_weapon", _config_integer, "2", "HALO_PVP_PRIMARY_WEAPON", _environment_value, _platform_all,
		"Custom loadout: 0 none, 1 random, 2 AR, 3 pistol, 4 shotgun, 5 sniper, 6 rocket, 7 plasma pistol, 8 plasma rifle, 9 needler." },

	{ "pvp.secondary_weapon", _config_integer, "3", "HALO_PVP_SECONDARY_WEAPON", _environment_value, _platform_all,
		"Same custom-loadout weapon choices as primary_weapon." },

	{ "pvp.infinite_grenades", _config_boolean, "false", "HALO_PVP_INFINITE_GRENADES", _environment_value, _platform_all,
		"Unlimited grenades in launcher-hosted PvP, including expanded player counts." },

	{ "display.fullscreen", _config_boolean, "true", "HALO_FULLSCREEN", _environment_value, _platform_desktop,
		"Start fullscreen, drawing at the display's resolution and shape; false\n"
		"starts in a window, which draws the Xbox's 640x480. F11 switches." },
	{ "display.window_scale", _config_integer, "2", "HALO_WINDOW_SCALE", _environment_value, _platform_desktop,
		"Where display.window_size is empty: the window's size as a multiple of\n"
		"640x480." },
	{ "display.screen_width", _config_integer, "0", "HALO_SCREEN_WIDTH", _environment_value, _platform_android,
		"Columns of the 480-line picture: 0 for the display's shape, 640 for the\n"
		"Xbox's 4:3." },
	{ "display.vsync", _config_boolean, "true", "HALO_NO_VSYNC", _environment_set_is_false, _platform_all,
		"Wait for the display between frames; false draws as fast as possible." },
	{ "display.interpolation", _config_boolean, "true", "HALO_INTERPOLATION", _environment_value, _platform_all,
		"Draw a frame for every display refresh, blending between the game's 30\n"
		"ticks a second; false keeps the original 30 frames a second." },
	{ "display.direct_camera", _config_boolean, "true", "HALO_DIRECT_CAMERA", _environment_value, _platform_desktop,
		"In first person, point the view where the player aims now instead of\n"
		"where the last tick left it: the view turns the frame the mouse moves,\n"
		"not up to two ticks (66 ms) later." },
	{ "display.high_res_hud", _config_boolean, "true", "HALO_HIGH_RES_HUD", _environment_value, _platform_all,
		"Draw the HUD (meters, counters, panels, motion sensor, reticles,\n"
		"waypoints, scopes) from the high-res assets (8x the maps' bitmaps);\n"
		"false draws the maps' own bitmaps." },
	{ "display.high_res_text", _config_boolean, "true", "HALO_HIGH_RES_TEXT", _environment_value, _platform_all,
		"Draw the menus' and HUD's text with the fonts in port/assets/fonts\n"
		"(Overpass) at the resolution the game draws at, and the menus' titles\n"
		"from port/assets/titles; false draws the maps' bitmap fonts and titles." },
	{ "display.player_names", _config_string, "\"all\"", "HALO_PLAYER_NAMES", _environment_value, _platform_all,
		"In multiplayer, whose names are drawn above their heads: \"all\",\n"
		"\"allies\", \"enemies\" or \"none\". An enemy's shows only within the\n"
		"motion sensor's reach, in sight and not camouflaged; none show if the\n"
		"gametype's motion tracker shows no players, only allies' if it shows\n"
		"only friends." },
	{ "display.player_name_scale", _config_real, "1.0", "HALO_PLAYER_NAME_SCALE", _environment_value, _platform_all,
		"How large the players' names are drawn: 1.0 three quarters of the size of\n"
		"the HUD's text, 0.25 to 4." },
	{ "display.scoreboard_team_layout", _config_string, "\"teams\"", "HALO_SCOREBOARD_TEAM_LAYOUT", _environment_value,
		_platform_all,
		"How the scoreboard lists a team game's players: \"teams\" in a column for\n"
		"each team (red on the left, blue on the right), \"score\" all in order of\n"
		"score." },
	{ "display.scoreboard_background", _config_boolean, "true", "HALO_SCOREBOARD_BACKGROUND", _environment_value,
		_platform_all,
		"Draw a panel behind the multiplayer scoreboard, for clearer text." },
	{ "display.scoreboard_background_color", _config_string, "\"16, 16, 16, 150\"", "HALO_SCOREBOARD_BACKGROUND_COLOR",
		_environment_value, _platform_all,
		"The scoreboard panel's colour: \"red, green, blue, alpha\", each 0 to 255\n"
		"(alpha 0 is see-through, 255 solid)." },

	{ "audio.enabled", _config_boolean, "true", "HALO_NO_AUDIO", _environment_set_is_false, _platform_all,
		"Play sound." },
	{ "audio.volume", _config_real, "1.0", "HALO_VOLUME", _environment_value, _platform_all,
		"The volume of everything, 0.0 to 1.0." },

	{ "input.mouse_sensitivity", _config_real, "1.0", "HALO_MOUSE_SENSITIVITY", _environment_value, _platform_desktop,
		"How far the view turns for the mouse's movement." },
	{ "input.invert_mouse", _config_boolean, "false", "HALO_MOUSE_INVERT", _environment_set_is_true, _platform_desktop,
		"Moving the mouse forward looks down." },
	{ "input.mouse_aim_assist", _config_boolean, "false", "HALO_MOUSE_AIM_ASSIST", _environment_value, _platform_desktop,
		"Magnetism while aiming with the mouse, as with a controller: the view\n"
		"slowed and dragged along by a target. The last of the mouse and the\n"
		"right stick to move decides. The bullets' autoaim (bent toward the\n"
		"target) stays either way." },

	{ "game.console_log", _config_string, "\"important\"", "HALO_CONSOLE_LOG", _environment_value, _platform_all,
		"What the game's console shows on screen of what it logs: \"important\"\n"
		"(bans, players dropped for cheating, what refuses a command, and the\n"
		"asserts that stop the game), \"all\" (every line, the game's own\n"
		"chatter too), or \"none\" (the asserts that stop the game only). What\n"
		"a command prints shows whatever this is, and debug.txt has every line." },

	{ "game.language", _config_string, "\"\"", "HALO_LANGUAGE", _environment_value, _platform_all,
		"The language the game asks the Xbox for: \"ja\", \"de\", \"fr\", \"es\" or \"it\";\n"
		"empty for English. The game data decides what is translated." },
	{ "game.custom_edition", _config_boolean, "false", "HALO_CUSTOM_EDITION", _environment_set_is_true, _platform_all,
		"Load and run Halo Custom Edition and OpenSauce (.yelo) maps, which are\n"
		"otherwise refused. Experimental: docs/custom_edition_caches.md in the\n"
		"source says what works. On Android the launcher's mods turn it on while\n"
		"one is installed." },

	{ "paths.data", _config_string, "\"\"", "HALO_DATA_ROOT", _environment_value, _platform_desktop,
		"The folder holding the game data's maps folder; empty looks in the\n"
		"working directory and its assets folder. Windows paths are easiest in\n"
		"single quotes: 'C:\\Games\\Halo'." },
	{ "paths.saves", _config_string, "\"\"", "HALO_SAVE_ROOT", _environment_value, _platform_desktop,
		"Where saved games and profiles go; empty for the usual place\n"
		"(~/.local/share/halo-linux, or %APPDATA%\\halo on Windows)." },

	{ "renderer.safe_geometry", _config_boolean, "false", "HALO_SAFE_GEOMETRY", _environment_value, _platform_all,
		"Geometry compatibility mode (Android; restart required). Streams geometry\n"
		"through the fenced stream ring with no persistent buffers/static mirrors, and CPU index rebasing.\n"
		"Default on in both Quest VR and flat Android; desktop unchanged. Can reduce performance. Not a data revision selector." },
    { "renderer.vr_geometry_revision", _config_integer, "1", "HALO_VR_GEOMETRY_REVISION", _environment_value, _platform_vr,
        "Internal VR geometry-default migration revision. Keep at 1 after choosing Safe or Normal." },
    { "renderer.android_geometry_revision", _config_integer, "1", "HALO_ANDROID_GEOMETRY_REVISION", _environment_value, _platform_android,
        "Internal flat Android geometry-default migration revision. Keep at 1 after choosing Safe or Normal. VR uses vr_geometry_revision." },

	{ "network.address", _config_string, "\"\"", "HALO_NET_ADDRESS", _environment_value, _platform_all,
		"This machine's IPv4 address for system link, for a machine on several\n"
		"networks; empty chooses one." },
	{ "network.broadcast", _config_string, "\"\"", "HALO_NET_BROADCAST", _environment_value, _platform_all,
		"Comma-separated IPv4 addresses system link sends its announcements to\n"
		"instead of the local network's broadcast address (for VPNs); empty for\n"
		"the local network." },
	{ "network.online", _config_boolean, "true", "HALO_NET_ONLINE", _environment_value, _platform_all,
		"Internet play: hosting makes an invite link (logged, and put on the\n"
		"clipboard) that lets whoever has it join over the internet; opening a\n"
		"link (or copying one before switching to the game) joins. Only people\n"
		"with the invite can join. Off keeps system link to the local network." },
	{ "network.protocol_version", _config_integer, "24", "HALO_NETWORK_PROTOCOL_VERSION", _environment_value,
		_platform_all,
		"Client protocol target: 0 lists/joins all compatible PvP versions 11-24;\n"
		"11-24 selects one exact server version. This build always hosts as 24.\n"
		"Campaign and Custom Edition map joins require version 23 or newer. This\n"
		"is read once when the game starts; restart after changing it. Invalid\n"
		"values fall back to the default, 24." },
	{ "network.join_from_clipboard", _config_boolean, "true", "HALO_NET_JOIN_FROM_CLIPBOARD", _environment_value,
		_platform_all,
		"Join the game of an invite link found on the clipboard when the game\n"
		"comes to the front." },
	{ "network.tunnel_port", _config_integer, "0", "HALO_NET_TUNNEL_PORT", _environment_value, _platform_all,
		"The UDP port internet play uses; 0 picks one. A fixed one can be\n"
		"forwarded on the router, for networks whose NAT stops connections." },
	{ "network.allow_upnp", _config_boolean, "true", "HALO_NET_ALLOW_UPNP", _environment_value, _platform_all,
		"Let internet play ask the router (UPnP) to forward its port, for\n"
		"networks whose NAT stops connections: when a player joins this\n"
		"machine's game, and when joining a game takes too long. False never\n"
		"asks." },
    { "network.public_lobby", _config_boolean, "true", "HALO_NET_PUBLIC_LOBBY", _environment_value, _platform_all,
        "Discover signed public OpenCE games in the in-game System Link list." },
    { "network.host_public", _config_boolean, "false", "HALO_NET_HOST_PUBLIC", _environment_value, _platform_all,
        "List native PvP hosts publicly. Launcher PUBLIC overrides this per session. Campaign remains separate." },
	{ "network.signalling_brokers", _config_string,
		"\"opence.milenko.org:1883,broker.emqx.io:1883,broker.hivemq.com:1883,test.mosquitto.org:1883\"",
		"HALO_NET_BROKERS", _environment_value, _platform_all,
		"Public MQTT brokers through which the machines of an invite find each\n"
		"other and the server browser's listings travel (its messages are\n"
		"encrypted); comma-separated host:port, up to 4. The first is upstream's\n"
		"own broker (its brokers.txt); the earlier default list moves to this one." },
	{ "network.coop_public", _config_boolean, "false", "HALO_NET_COOP_PUBLIC", _environment_value, _platform_all,
		"Whether an online co-op game (Create Game > Internet, a SINGLEPLAYER\n"
		"map) starts as PUBLIC or, false, PRIVATE: Server Setup's LISTING in\n"
		"co-op, which writes its choice here." },
	{ "network.coop_friendly_fire", _config_string, "\"on\"", "HALO_NET_COOP_FRIENDLY_FIRE", _environment_value,
		_platform_all,
		"Whether the players of an online co-op game hurt each other: \"off\",\n"
		"\"on\", \"shields_only\" or \"explosives_only\" (Server Setup's FRIENDLY\n"
		"FIRE in co-op, which writes its choice here). Their AI allies they\n"
		"always can, as in the campaign." },
	{ "network.coop_player_collisions", _config_boolean, "true", "HALO_NET_COOP_PLAYER_COLLISIONS", _environment_value,
		_platform_all,
		"Whether the players of an online co-op game bump into each other;\n"
		"false, they walk through each other (the AI's characters they still\n"
		"bump into). Server Setup's PLAYER COLLISIONS in co-op writes its\n"
		"choice here." },
	{ "network.coop_enemies_mode", _config_string, "\"per_player\"", "HALO_NET_COOP_ENEMIES_MODE", _environment_value,
		_platform_all,
		"Online co-op's extra enemies: \"none\", \"per_player\" (each squad of\n"
		"enemies grows by coop_enemies for each player past the first) or\n"
		"\"multiplier\" (each is coop_enemies_multiplier times as large, for any\n"
		"number of players). Server Setup's EXTRA ENEMIES in co-op writes its\n"
		"choice here." },
	{ "network.coop_enemies", _config_integer, "50", "HALO_NET_COOP_ENEMIES", _environment_value, _platform_all,
		"Online co-op's extra enemies per player, a percentage: for each player\n"
		"past the first, each squad of enemies a level places gets this much of\n"
		"itself more (100: as many again; 25 to 200). Server Setup's PER PLAYER\n"
		"in co-op writes its choice here." },
	{ "network.coop_enemies_multiplier", _config_integer, "2", "HALO_NET_COOP_ENEMIES_MULTIPLIER", _environment_value,
		_platform_all,
		"Online co-op's static multiplier of its enemies: each squad of enemies\n"
		"a level places is this many times as large (2 to 32). Server Setup's\n"
		"MULTIPLIER in co-op writes its choice here." },
	{ "network.stun_servers", _config_string, "\"stun.l.google.com:19302,stun.cloudflare.com:3478\"",
		"HALO_NET_STUN", _environment_value, _platform_all,
		"Public STUN servers that tell this machine its internet address;\n"
		"comma-separated host:port." },
	{ "discord.application_id", _config_string, "\"1553978809840050229\"", "HALO_DISCORD_APPLICATION",
		_environment_value, _platform_desktop,
		"The Discord application internet play invites go through while the\n"
		"Discord desktop client runs; empty for none." },

	{ "update.auto", _config_boolean, "true", "HALO_UPDATE_AUTO", _environment_value, _platform_all,
		"Look for a new version when the game starts, and offer to update to it;\n"
		"false never looks (the game's \"Do not ask again\" writes false here)." },

	{ "debug.network_test", _config_string, "\"\"", "HALO_NETWORK_TEST", _environment_value, _platform_all,
		"Automated system link sessions for testing (port/linux/game/network_test.c):\n"
		"\"host:<map>\" hosts a game on that map, \"join\" joins the first game found;\n"
		"empty for none." },
	{ "debug.network_test_start", _config_real, "15.0", "HALO_NETWORK_TEST_START", _environment_value, _platform_all,
		"Seconds after hosting that an automated test game starts." },
	{ "debug.network_test_kill", _config_real, "0.0", "HALO_NETWORK_TEST_KILL", _environment_value, _platform_all,
		"Every this many seconds an automated test host kills its last player; 0 never." },
	{ "debug.network_test_score", _config_integer, "0", "HALO_NETWORK_TEST_SCORE", _environment_value, _platform_all,
		"The score an automated test host's game type plays to (a short game, to\n"
		"test the next); 0 the game type's own." },
	{ "debug.network_test_shoot", _config_real, "0.0", "HALO_NETWORK_TEST_SHOOT", _environment_value, _platform_all,
		"Every this many seconds each automated test player hits the next with\n"
		"their weapon, within its reach (the host brings far players near the\n"
		"first a second before); 0 never." },
	{ "debug.network_test_vehicle", _config_real, "0.0", "HALO_NETWORK_TEST_VEHICLE", _environment_value, _platform_all,
		"This many seconds into an automated test game the host seats its last\n"
		"player as a vehicle's driver (and out 15 seconds on); 0 never." },
	{ "debug.network_test_pickup", _config_real, "0.0", "HALO_NETWORK_TEST_PICKUP", _environment_value, _platform_all,
		"This many seconds into an automated test game the host stands its last\n"
		"player on a weapon, which a joining player then picks up; 0 never." },
	{ "debug.network_test_pickup_weapon", _config_string, "\"\"", "HALO_NETWORK_TEST_PICKUP_WEAPON", _environment_value,
		_platform_all,
		"The weapon network_test_pickup stands the player on: the first whose tag\n"
		"name has this in it (\"sniper\", say); empty any." },
	{ "debug.telnet_console", _config_boolean, "false", "HALO_TELNET_CONSOLE", _environment_set_is_true, _platform_all,
		"Listen on 127.0.0.1 (port telnet_console_port) for a script console that\n"
		"runs what it is sent as the game's console does, with no password; false\n"
		"none." },
	{ "debug.telnet_console_port", _config_integer, "2323", "HALO_TELNET_CONSOLE_PORT", _environment_value,
		_platform_all,
		"The port of the script console (telnet_console); the Xbox's was 23, which\n"
		"only the administrator can listen on." },
	{ "debug.network_latency", _config_real, "0.0", "HALO_NETWORK_LATENCY", _environment_value, _platform_all,
		"Milliseconds everything received is held back (a round trip between two\n"
		"machines of twice it), to test the netcode as over the internet; 0 none." },
	{ "debug.network_loss", _config_real, "0.0", "HALO_NETWORK_LOSS", _environment_value, _platform_all,
		"Percent of datagrams received that are dropped, for the same; 0 none." },
	{ "debug.network_corrupt", _config_real, "0.0", "HALO_NETWORK_CORRUPT", _environment_value, _platform_all,
		"Percent of the datagrams received that are damaged at random, to test\n"
		"that nothing a machine sends can crash the game; 0 none." },
	{ "debug.network_corrupt_stream", _config_real, "0.0", "HALO_NETWORK_CORRUPT_STREAM", _environment_value,
		_platform_all,
		"Percent of the reads of streams that are damaged at random, for the\n"
		"same (a damaged stream is closed, so a little goes a long way); 0 none." },
	{ "debug.network_corrupt_after", _config_real, "0.0", "HALO_NETWORK_CORRUPT_AFTER", _environment_value,
		_platform_all,
		"Seconds after the start before anything is damaged, so that a game can\n"
		"be set up and started first (a host's messages to its own client are\n"
		"damaged too)." },
	{ "debug.test_input", _config_string, "\"\"", "HALO_TEST_INPUT", _environment_value, _platform_all,
		"\"bot:<seed>\" plays controller 1 with a scripted pattern (automated\n"
		"network tests); \"look:<seed>\" stands still, only turning and looking\n"
		"up and down; empty for none." },
	{ "debug.update_answer", _config_string, "\"\"", "HALO_UPDATE_ANSWER", _environment_value, _platform_desktop,
		"The answer to the new version question, for automated tests: \"yes\",\n"
		"\"no\" or \"never\" (do not ask again, confirmed); empty asks." },
	{ "debug.exit_after", _config_real, "0.0", "HALO_EXIT_AFTER", _environment_value, _platform_all,
		"Quit this many seconds after the window opens; 0 never." },
	{ "debug.hidden_window", _config_boolean, "false", "HALO_HIDDEN_WINDOW", _environment_set_is_true, _platform_desktop,
		"Keep the window hidden (and never fullscreen)." },
	{ "debug.null_renderer", _config_boolean, "false", "HALO_NULL_RENDERER", _environment_set_is_true, _platform_all,
		"Run without a window, drawing nothing." },
	{ "debug.gl_debug", _config_boolean, "false", "HALO_GL_DEBUG", _environment_set_is_true, _platform_all,
		"Report OpenGL errors in the log." },
	{ "debug.gpu_stats", _config_boolean, "false", "HALO_GPU_STATS", _environment_set_is_true, _platform_all,
		"Log the renderer's draw counts once a second." },
	{ "debug.gpu_trace_frame", _config_integer, "-1", "HALO_GPU_TRACE", _environment_value, _platform_all,
		"Log every draw of this frame; -1 none." },
	{ "debug.gpu_trace_constants", _config_boolean, "false", "HALO_GPU_TRACE_CONSTANTS", _environment_set_is_true, _platform_all,
		"With gpu_trace_frame, also the vertex shader constants." },
	{ "debug.gpu_skip_vertex_shaders", _config_string, "\"\"", "HALO_GPU_SKIP_VS", _environment_value, _platform_all,
		"Comma-separated ids of vertex shaders not to draw with." },
	{ "debug.gpu_dump_shaders", _config_string, "\"\"", "HALO_GPU_DUMP_SHADERS", _environment_value, _platform_all,
		"A folder to write the generated GLSL to; empty none." },
	{ "debug.gpu_debug_expression", _config_string, "\"\"", "HALO_GPU_DEBUG_EXPR", _environment_value, _platform_all,
		"A GLSL expression every pixel shader shows instead of its result." },
	{ "debug.gpu_debug_texture0", _config_boolean, "false", "HALO_GPU_DEBUG_T0", _environment_set_is_true, _platform_all,
		"Pixel shaders show their first texture." },
	{ "debug.gpu_debug_flat", _config_boolean, "false", "HALO_GPU_DEBUG_FLAT", _environment_set_is_true, _platform_all,
		"Pixel shaders show their vertex colour." },
	{ "debug.screenshot_directory", _config_string, "\"\"", "HALO_SCREENSHOT_DIR", _environment_value, _platform_all,
		"A folder to save frames to (with screenshot_every); empty none." },
	{ "debug.screenshot_every", _config_integer, "0", "HALO_SCREENSHOT_EVERY", _environment_value, _platform_all,
		"Save every this many frames to screenshot_directory; 0 none." },
	{ "debug.texture_dump_directory", _config_string, "\"\"", "HALO_TEXTURE_DUMP", _environment_value, _platform_all,
		"A folder to write every texture to as it is uploaded; empty none." },
	{ "debug.texture_log", _config_boolean, "false", "HALO_TEXTURE_LOG", _environment_set_is_true, _platform_all,
		"Log texture uploads." },
	{ "debug.texture_no_cache", _config_boolean, "false", "HALO_TEXTURE_NO_CACHE", _environment_set_is_true, _platform_all,
		"Upload textures again every time they are used." },
	{ "debug.sample_seconds", _config_real, "0.0", "HALO_SAMPLE", _environment_value,
		_platform_android | _platform_windows,
		"Log where the game is this often, in seconds; 0 never. On Android every\n"
		"game thread (read by the app, port/android/host/host_debug.c), on\n"
		"Windows the main thread (port/windows/src/win32_memory_watch.c)." },

	{ "vr.enabled", _config_boolean, "true", "HALO_VR", _environment_value, _platform_vr,
		"Play in the headset (OpenXR); false shows the game on the flat screen\n"
		"as the phone build does." },
	{ "vr.resolution_scale", _config_real, "0.0", "HALO_VR_RESOLUTION_SCALE", _environment_value, _platform_vr,
		"The game's picture in the headset as a fraction of the runtime's\n"
		"recommended eye resolution (about 1680x1760 on the Quest 3, 1728x1728 on\n"
		"the Steam Frame), 0.5 to 2.0; 0 chooses for the headset: 1.0 on the\n"
		"Quest 3, 3S and Pro, 0.85 on the Quest 2, 0.7 on the first Quest. 1.5\n"
		"holds 72 frames a second on the Steam Frame. The eye images the\n"
		"compositor gets are this size too, so above 1.0 the extra detail\n"
		"reaches the display; 1.25 is about the Quest 3's own panels\n"
		"(2064x2208), above that supersampling." },
	{ "vr.fov_mode", _config_string, "\"full\"", "HALO_VR_FOV_MODE", _environment_value, _platform_vr,
		"How much of the view the game draws: \"full\" (all the headset shows)\n"
		"or \"glasses\" (a window of vr.glasses_fov_h by vr.glasses_fov_v\n"
		"degrees ahead of each eye, black round it), which previews the field\n"
		"of view of upcoming VR glasses. The window draws far fewer pixels\n"
		"(about 40% as many on the Quest 3) and fewer objects, which leaves\n"
		"room for a higher frame rate or vr.resolution_scale." },
	{ "vr.glasses_fov_h", _config_real, "70.0", "HALO_VR_GLASSES_FOV_H", _environment_value, _platform_vr,
		"The glasses window's width, in degrees (vr.fov_mode), 20 to 160." },
	{ "vr.glasses_fov_v", _config_real, "66.0", "HALO_VR_GLASSES_FOV_V", _environment_value, _platform_vr,
		"The glasses window's height, in degrees (vr.fov_mode), 20 to 160." },
	{ "vr.screen_distance", _config_real, "2.5", "HALO_VR_SCREEN_DISTANCE", _environment_value, _platform_vr,
		"How far ahead the flat screen (menus, cutscenes) floats, in metres." },
	{ "vr.screen_width", _config_real, "2.4", "HALO_VR_SCREEN_WIDTH", _environment_value, _platform_vr,
		"The flat screen's width, in metres (its height is three quarters)." },
	{ "vr.stereo", _config_boolean, "true", "HALO_VR_STEREO", _environment_value, _platform_vr,
		"Play in 3D around you; false keeps gameplay on the flat screen too." },
	{ "vr.world_scale", _config_real, "0.328084", "HALO_VR_WORLD_SCALE", _environment_value, _platform_vr,
		"Game units per metre of head movement and eye separation: the game's\n"
		"unit is 10 feet (0.328084 per metre). Larger makes the world feel\n"
		"smaller." },
	{ "vr.hud_distance", _config_real, "15.0", "HALO_VR_HUD_DISTANCE", _environment_value, _platform_vr,
		"How far ahead of the eyes the HUD floats, in metres: far, so the eyes\n"
		"need not refocus between it and the world." },
	{ "vr.hud_width", _config_real, "10.0", "HALO_VR_HUD_WIDTH", _environment_value, _platform_vr,
		"The HUD's width, in metres (its height is three quarters); with\n"
		"vr.hud_distance, how much of the view it spans (10 at 15: 37 degrees)." },
	{ "vr.refresh_rate", _config_real, "72.0", "HALO_VR_REFRESH_RATE", _environment_value, _platform_vr,
		"The headset's display rate to ask for, in hertz: the highest the\n"
		"runtime offers at or below it; 0 leaves the runtime's choice. The Quest 3\n"
		"offers 72, 80, 90 and 120; SteamVR on the Steam Frame offers only the\n"
		"rate its own display setting is on." },
	{ "vr.cutscenes", _config_string, "\"immersive\"", "HALO_VR_CUTSCENES", _environment_value, _platform_vr,
		"\"immersive\": cutscenes around you, seen from their camera as you look\n"
		"about (a moment of black at each cut); \"screen\": on a large screen\n"
		"ahead (in 3D, vr.cinema_3d); \"flat\": on the flat screen." },
	{ "vr.script_messages", _config_boolean, "false", "HALO_VR_SCRIPT_MESSAGES", _environment_value, _platform_vr,
		"Show script print messages in the green terminal overlay while in VR.\n"
		"False keeps them in the run log only; the flat build is unchanged." },
	{ "vr.cinema_3d", _config_boolean, "true", "HALO_VR_CINEMA_3D", _environment_value, _platform_vr,
		"With vr.cutscenes \"screen\": the cutscene screen in 3D (each eye its\n"
		"own picture); false shows them flat." },
	{ "vr.cinema_separation", _config_real, "0.064", "HALO_VR_CINEMA_SEPARATION", _environment_value, _platform_vr,
		"The cutscene screen's eye separation, in metres of the game's world:\n"
		"more for deeper 3D." },
	{ "vr.cinema_convergence", _config_real, "2.0", "HALO_VR_CINEMA_CONVERGENCE", _environment_value, _platform_vr,
		"How far from the cutscene's camera, in game units (10 feet), things\n"
		"show at the screen's depth; nearer comes out of it." },
	{ "vr.cinema_distance", _config_real, "3.0", "HALO_VR_CINEMA_DISTANCE", _environment_value, _platform_vr,
		"How far ahead the cutscene screen floats, in metres." },
	{ "vr.cinema_width", _config_real, "3.6", "HALO_VR_CINEMA_WIDTH", _environment_value, _platform_vr,
		"The cutscene screen's width, in metres." },
	{ "vr.controls", _config_string, "\"vr\"", "HALO_VR_CONTROLS", _environment_value, _platform_vr,
		"The controllers' layout. \"vr\": right trigger fire, left trigger zoom, A\n"
		"jump, B reload and action, X switch grenades, Y switch weapons, right\n"
		"bumper grenade, left bumper flashlight, right stick click melee, left\n"
		"stick click crouch, menu pause, view back (held a second, recentre).\n"
		"Quest Touch controllers take their buttons from the vr.button_* settings\n"
		"(VR Settings > BUTTONS). \"pad\": as an Xbox controller." },
	{ "vr.move_relative", _config_string, "\"head\"", "HALO_VR_MOVE_RELATIVE", _environment_value, _platform_vr,
		"What the move stick moves you relative to: \"head\", or where the\n"
		"\"left\" or \"right\" controller points." },
	{ "vr.aim", _config_string, "\"hand\"", "HALO_VR_AIM", _environment_value, _platform_vr,
		"What aims on foot: \"head\" (where you look, with a reticle ahead) or\n"
		"\"hand\" (the right controller, holding the weapon). In vehicles the\n"
		"head aims either way." },
	{ "vr.crosshair", _config_string, "\"native\"", "HALO_VR_CROSSHAIR", _environment_value, _platform_vr,
		"Gameplay crosshair: \"native\" uses the weapon's own animated artwork,\n"
		"or \"off\" hides it. The menu pointer is independent." },
	{ "vr.crosshair_size", _config_real, "1.0", "HALO_VR_CROSSHAIR_SIZE", _environment_value, _platform_vr,
		"Native crosshair size multiplier (0.25 to 3)." },
	{ "vr.crosshair_opacity", _config_real, "1.0", "HALO_VR_CROSSHAIR_OPACITY", _environment_value, _platform_vr,
		"Native crosshair opacity (0 to 1). Does not change the menu pointer." },
    { "vr.vehicle_defaults_applied", _config_boolean, "false", "HALO_VR_VEHICLE_DEFAULTS_APPLIED", _environment_value, _platform_vr,
        "Internal one-time third-person/right-controller default migration. Later saved choices are preserved." },
	{ "vr.vehicle_view", _config_string, "\"chase\"", "HALO_VR_VEHICLE_VIEW", _environment_value, _platform_vr,
		"Vehicles seen from the seat, turning with the vehicle and the horizon\n"
		"kept level (\"first_person\"), or from the game's chase camera (\"chase\")." },
	{ "vr.vehicle_tilt", _config_real, "0.0", "HALO_VR_VEHICLE_TILT", _environment_value, _platform_vr,
		"First-person vehicles, a driver's seat: how much the view tilts with the vehicle (0, the\n"
		"default: the horizon stays level and the cockpit tilts; 1: the cockpit stays put; 0.5 half)." },
	{ "vr.vehicle_steering", _config_string, "\"right\"", "HALO_VR_VEHICLE_STEERING", _environment_value, _platform_vr,
        "Driver steering: \"right\" (default), \"left\", \"head\", or \"stick\".\n"
        "Right/left use that physical controller, independent of weapon grip.\n"
#ifdef HALO_VR
        "Legacy \"hand\" means right. Gunners use vr.turret_aim. Left stick drives." },
#else
        "Legacy \"hand\" means right. Gunners retain head aim. Left stick drives." },
#endif
#ifdef HALO_VR
	{ "vr.turret_aim", _config_string, "\"right\"", "HALO_VR_TURRET_AIM", _environment_value, _platform_vr,
		"Mounted turret/gunner aim: \"right\" (default), \"left\", \"head\", or \"stick\".\n"
		"Controller choices use that physical hand independently of weapon grip.\n"
		"The selected controller losing tracking keeps native facing; head motion does not take over." },
#endif

	{ "vr.vehicle_all_up", _config_real, "0.0", "HALO_VR_VEHICLE_ALL_UP", _environment_value, _platform_vr,
		"First-person seat up offset in metres (-0.50 to 0.50), all. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_all_forward", _config_real, "0.0", "HALO_VR_VEHICLE_ALL_FORWARD", _environment_value, _platform_vr,
		"First-person seat forward offset in metres (-0.50 to 0.50), all. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_all_right", _config_real, "0.0", "HALO_VR_VEHICLE_ALL_RIGHT", _environment_value, _platform_vr,
		"First-person seat right offset in metres (-0.50 to 0.50), all. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_warthog_hide_glass", _config_boolean, "true", "HALO_VR_VEHICLE_WARTHOG_HIDE_GLASS", _environment_value, _platform_vr,
		"Hide the occupied Warthog's glass in first-person view. Other vehicles, third-person views and collision are unchanged. On preserves test25 behavior." },
	{ "vr.vehicle_warthog_up", _config_real, "0.0", "HALO_VR_VEHICLE_WARTHOG_UP", _environment_value, _platform_vr,
		"First-person seat up offset in metres (-0.50 to 0.50), warthog. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_warthog_forward", _config_real, "0.0", "HALO_VR_VEHICLE_WARTHOG_FORWARD", _environment_value, _platform_vr,
		"First-person seat forward offset in metres (-0.50 to 0.50), warthog. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_warthog_right", _config_real, "0.0", "HALO_VR_VEHICLE_WARTHOG_RIGHT", _environment_value, _platform_vr,
		"First-person seat right offset in metres (-0.50 to 0.50), warthog. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_ghost_up", _config_real, "0.0", "HALO_VR_VEHICLE_GHOST_UP", _environment_value, _platform_vr,
		"First-person seat up offset in metres (-0.50 to 0.50), ghost. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_ghost_forward", _config_real, "0.0", "HALO_VR_VEHICLE_GHOST_FORWARD", _environment_value, _platform_vr,
		"First-person seat forward offset in metres (-0.50 to 0.50), ghost. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_ghost_right", _config_real, "0.0", "HALO_VR_VEHICLE_GHOST_RIGHT", _environment_value, _platform_vr,
		"First-person seat right offset in metres (-0.50 to 0.50), ghost. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_banshee_up", _config_real, "0.0", "HALO_VR_VEHICLE_BANSHEE_UP", _environment_value, _platform_vr,
		"First-person seat up offset in metres (-0.50 to 0.50), banshee. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_banshee_forward", _config_real, "0.0", "HALO_VR_VEHICLE_BANSHEE_FORWARD", _environment_value, _platform_vr,
		"First-person seat forward offset in metres (-0.50 to 0.50), banshee. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_banshee_right", _config_real, "0.0", "HALO_VR_VEHICLE_BANSHEE_RIGHT", _environment_value, _platform_vr,
		"First-person seat right offset in metres (-0.50 to 0.50), banshee. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_scorpion_up", _config_real, "0.0", "HALO_VR_VEHICLE_SCORPION_UP", _environment_value, _platform_vr,
		"First-person seat up offset in metres (-0.50 to 0.50), scorpion. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_scorpion_forward", _config_real, "0.0", "HALO_VR_VEHICLE_SCORPION_FORWARD", _environment_value, _platform_vr,
		"First-person seat forward offset in metres (-0.50 to 0.50), scorpion. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_scorpion_right", _config_real, "0.0", "HALO_VR_VEHICLE_SCORPION_RIGHT", _environment_value, _platform_vr,
		"First-person seat right offset in metres (-0.50 to 0.50), scorpion. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_pelican_up", _config_real, "0.0", "HALO_VR_VEHICLE_PELICAN_UP", _environment_value, _platform_vr,
		"First-person seat up offset in metres (-0.50 to 0.50), pelican. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_pelican_forward", _config_real, "0.0", "HALO_VR_VEHICLE_PELICAN_FORWARD", _environment_value, _platform_vr,
		"First-person seat forward offset in metres (-0.50 to 0.50), pelican. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },
	{ "vr.vehicle_pelican_right", _config_real, "0.0", "HALO_VR_VEHICLE_PELICAN_RIGHT", _environment_value, _platform_vr,
		"First-person seat right offset in metres (-0.50 to 0.50), pelican. Global and matching vehicle offsets add; final axis is bounded to 0.50 m." },

	{ "vr.arms", _config_string, "\"ik\"", "HALO_VR_ARMS", _environment_value, _platform_vr,
		"With vr.aim \"hand\": the first-person arms. \"ik\" reaches them from the\n"
		"shoulders to the hands (the left to the left controller, or to the gun\n"
		"when held near it); \"hidden\" shows the gun alone; \"animated\" moves\n"
		"them with the gun as the game animates them." },
	{ "vr.two_handed", _config_string, "\"auto\"", "HALO_VR_TWO_HANDED", _environment_value, _platform_vr,
		"With vr.aim \"hand\": holding the gun in both hands steadies it, pointing\n"
		"from the weapon hand to the other. \"auto\" (default): the other hand resting at the\n"
		"gun's support grip locks there, until pulled away; \"grip\": it locks only while its\n"
		"grip is squeezed there; \"off\": no attachment." },
	{ "vr.two_hand_auto_applied", _config_boolean, "false", "HALO_VR_TWO_HAND_AUTO_APPLIED", _environment_value, _platform_vr,
		"Internal one-time migration (test21): a config on the old \"grip\" default moves to\n"
		"\"auto\" (two-hand grip locks automatically). A later choice is kept." },
	{ "vr.aim_reset_applied", _config_boolean, "false", "HALO_VR_AIM_RESET_APPLIED", _environment_value, _platform_vr,
		"Internal one-time reset (test21): per-gun aim values (vr.aim_*) set while testing the first\n"
		"test21 build, whose reticle was off, return to 0 once." },
	{ "vr.left_handed", _config_boolean, "false", "HALO_VR_LEFT_HANDED", _environment_value, _platform_vr,
		"Left-handed play: the gun starts in the left hand (fire, grenade and zoom swap triggers\n"
		"and bumpers) and, with vr.mirror_controls \"auto\", the sticks and face buttons mirror.\n"
		"Changing it in the menu also mirrors vehicle steering and Move With when they used a\n"
		"hand. Palms together and the other hand's grip still pass the gun across." },
	{ "vr.mirror_controls", _config_string, "\"auto\"", "HALO_VR_MIRROR_CONTROLS", _environment_value, _platform_vr,
		"\"auto\": with vr.left_handed, move on the right stick and turn on the left, and the\n"
		"face buttons swap hands (jump and reload on the left controller). \"off\": the standard\n"
		"layout (move on the left stick) whichever hand holds the gun." },
	{ "vr.handedness_applied", _config_boolean, "false", "HALO_VR_HANDEDNESS_APPLIED", _environment_value, _platform_vr,
		"Internal one-time migration (test20d): a config already left-handed keeps standard\n"
		"sticks and buttons (vr.mirror_controls \"off\")." },
	{ "vr.melee", _config_string, "\"impact\"", "HALO_VR_MELEE", _environment_value, _platform_vr,
		"\"impact\": a hand (the weapon hand with the gun) moving at vr.melee_speed\n"
		"strikes what it sweeps through - enemies, vehicles, glass - harder the\n"
		"faster; \"swing\": a hand swung up or down that fast makes the game's own\n"
		"melee ahead. A network game's client always swings." },
	{ "vr.melee_speed", _config_real, "2.0", "HALO_VR_MELEE_SPEED", _environment_value, _platform_vr,
		"How fast a hand must move to strike (metres a second); 0 turns physical\n"
		"melee off (the right stick click still melees)." },
	{ "vr.melee_multiplayer", _config_boolean, "false", "HALO_VR_MELEE_MULTIPLAYER", _environment_value, _platform_vr,
		"Physical melee (impact or swing) in network games too. Off by default: online, a\n"
		"quick hand movement does not melee; the melee button always does." },
	{ "vr.flashlight_distance", _config_real, "0.2", "HALO_VR_FLASHLIGHT_DISTANCE", _environment_value, _platform_vr,
		"The off hand brought this close (metres) to the middle of the head\n"
		"turns the flashlight on or off; 0 turns the gesture off (0.10 to 0.30 in the menu)." },
	{ "vr.hud_tap_distance", _config_real, "0.1", "HALO_VR_HUD_TAP_DISTANCE", _environment_value, _platform_vr,
		"The weapon hand held for a moment this close (metres) beside its own side of the head (the\n"
		"right temple for a right hand) shows or hides the HUD for the session; menus, prompts,\n"
		"messages and the reticle stay. 0 turns the gesture off. VR SETTINGS > HUD also shows or\n"
		"hides it, and every start shows it." },
	{ "vr.wrist_hud", _config_boolean, "false", "HALO_VR_WRIST_HUD", _environment_value, _platform_vr,
		"Shield and health, ammunition and grenades, and the motion tracker on a panel on the off\n"
		"hand's wrist (shown while you look at it), and left out of the HUD ahead. Off by default." },
	{ "vr.wrist_hud_along", _config_real, "0", "HALO_VR_WRIST_HUD_ALONG", _environment_value, _platform_vr,
		"Moves the wrist HUD along the forearm from its place on the wrist (metres, -0.2 to 0.2;\n"
		"positive toward the elbow)." },
	{ "vr.wrist_hud_across", _config_real, "0", "HALO_VR_WRIST_HUD_ACROSS", _environment_value, _platform_vr,
		"Moves the wrist HUD across the wrist (metres, -0.2 to 0.2; positive toward the thumb)." },
	{ "vr.wrist_hud_out", _config_real, "0", "HALO_VR_WRIST_HUD_OUT", _environment_value, _platform_vr,
		"Raises the wrist HUD off the wrist (metres, -0.2 to 0.2; negative lowers it)." },
	{ "vr.wrist_hud_size", _config_real, "1", "HALO_VR_WRIST_HUD_SIZE", _environment_value, _platform_vr,
		"The wrist HUD's size (0.5 to 2; 1 is 11 cm across)." },
	{ "vr.controls_reticle_applied", _config_boolean, "false", "HALO_VR_CONTROLS_RETICLE_APPLIED", _environment_value, _platform_vr,
		"Internal one-time move of crouch off the left stick's click (now the reticle toggle's)." },
	{ "vr.crouch_height", _config_real, "0.35", "HALO_VR_CROUCH_HEIGHT", _environment_value, _platform_vr,
		"Ducking this far (metres) below your height at the last recentre\n"
		"crouches; 0 turns it off." },
	{ "vr.haptics", _config_real, "1.0", "HALO_VR_HAPTICS", _environment_value, _platform_vr,
		"The strength of the controllers' buzz (shots, gestures), 0 to 1; 0 none." },
	{ "vr.menu_3d", _config_boolean, "true", "HALO_VR_MENU_3D", _environment_value, _platform_vr,
		"The main menu in 3D: its scene around you, its menus on a panel ahead;\n"
		"false shows it all on the flat screen." },
	{ "vr.body", _config_string, "\"legs\"", "HALO_VR_BODY", _environment_value, _platform_vr,
		"\"arms\": first-person arms/hands; \"hands\": hands only; \"full\": torso\n"
		"and legs too; \"legs\": legs plus arms/hands with the torso hidden.\n"
		"Local visibility only; supported peers see the complete VR avatar." },
	{ "vr.fingers", _config_boolean, "true", "HALO_VR_FINGERS", _environment_value, _platform_vr,
		"A free hand turns with your wrist and its fingers follow yours on the\n"
		"controller (Touch senses the thumb and index finger resting): point,\n"
		"thumbs up, a fist; pointing with the thumb up held a second shows the\n"
		"middle finger. It stops at what it meets - walls, vehicles, people - its\n"
		"palm laid flat to it and each finger resting on it, felt as a buzz." },
	{ "vr.diag_finger_sign", _config_real, "1.0", "HALO_VR_DIAG_FINGER_SIGN", _environment_value, _platform_vr,
		"Legacy test8 setting, retained for old configs; absolute finger posing\n"
		"in test9 determines bend direction from the hand frame instead." },
	{ "vr.arm_run", _config_boolean, "false", "HALO_VR_ARM_RUN", _environment_value, _platform_vr,
		"Run by swinging the arms as in running, ahead where the head faces, as\n"
		"fast as the swing (both arms; holding the gun in both hands, the gun's\n"
		"bob). Offline, strong forward pumping raises movement speed up to 1.5x.\n"
		"Network games retain stock speed. Deliberate stick movement takes priority." },
	{ "vr.arm_run_speed", _config_real, "0.6", "HALO_VR_ARM_RUN_SPEED", _environment_value, _platform_vr,
		"With vr.arm_run: how fast the hands must move (metres a second) to start\n"
		"running; full speed at 2.5 times it. Lower is easier." },
	{ "vr.close_contact", _config_boolean, "true", "HALO_VR_CLOSE_CONTACT", _environment_value, _platform_vr,
		"Offline local VR player only: reduce capsule radius by up to 15 percent\n"
		"(at most 5 cm, minimum radius 18 cm) for closer hand contact. Solid\n"
		"collision and total height remain; false restores the stock radius." },
	{ "vr.weapons", _config_string, "\"physical\"", "HALO_VR_WEAPONS", _environment_value, _platform_vr,
		"With vr.aim \"hand\": \"locked\" keeps the gun in the weapon hand always;\n"
		"\"physical\" keeps it there while that hand's grip holds it (the trigger\n"
		"fires whatever gun is in the hand): let go, it falls - at a holster it\n"
		"is put away, the hand empty, and with the other hand on it, it passes to\n"
		"that hand. A gun the game puts in the hand (a level's start, a pickup)\n"
		"stays supported until the first grip. An empty hand gripping at a\n"
		"holster draws the weapon there, and by a weapon lying about takes it.\n"
		"Grenades then go on the left's lower button (a tap throws, a hold\n"
		"switches). In network games only with vr.physical_multiplayer." },
	{ "vr.physical_multiplayer", _config_boolean, "false", "HALO_VR_PHYSICAL_MULTIPLAYER", _environment_value,
		_platform_vr,
		"Physical weapons (vr.weapons) in network games too, where the other\n"
		"machines may not see the guns dropped and taken; false keeps network\n"
		"games to the locked behaviour." },
	{ "vr.holsters", _config_boolean, "true", "HALO_VR_HOLSTERS", _environment_value, _platform_vr,
		"Holsters over each shoulder and at each hip (the weapon hand buzzes\n"
		"coming into one): its grip there switches weapons, or with physical\n"
		"weapons (vr.weapons) a gun let go there is put away and an empty hand\n"
		"gripping there draws one." },
	{ "vr.holster_size", _config_real, "0.2", "HALO_VR_HOLSTER_SIZE", _environment_value, _platform_vr,
		"How far from its place each holster reaches, in metres (0.05 to 0.6)." },
	{ "vr.roomscale", _config_boolean, "true", "HALO_VR_ROOMSCALE", _environment_value, _platform_vr,
		"Walking about your room walks your character (on foot, not in\n"
		"vehicles or cutscenes). Walls stop it where they stop the character, so\n"
		"your room and the game drift apart; recentre (hold View) to line them up." },
	{ "vr.scope", _config_boolean, "true", "HALO_VR_SCOPE", _environment_value, _platform_vr,
		"With vr.aim \"hand\": a zoomed weapon shows its zoom in a scope held at\n"
		"the gun (an extra view rendered while zoomed); your eyes stay unzoomed." },
	{ "vr.scope_size", _config_real, "0.06", "HALO_VR_SCOPE_SIZE", _environment_value, _platform_vr,
		"How wide the scope is, in metres." },
	{ "vr.scope_pistol_forward", _config_real, "0.0", "HALO_VR_SCOPE_PISTOL_FORWARD", _environment_value, _platform_vr,
		"The pistol (round sight) scope moved forward (toward the muzzle) from its usual place, metres (-0.30..0.30)." },
	{ "vr.scope_pistol_up", _config_real, "0.0", "HALO_VR_SCOPE_PISTOL_UP", _environment_value, _platform_vr,
		"The pistol (round sight) scope moved up from its usual place, metres (-0.30..0.30)." },
	{ "vr.scope_pistol_right", _config_real, "0.0", "HALO_VR_SCOPE_PISTOL_RIGHT", _environment_value, _platform_vr,
		"The pistol (round sight) scope moved right (in either hand) from its usual place, metres (-0.30..0.30)." },
	{ "vr.scope_pistol_scale", _config_real, "1.0", "HALO_VR_SCOPE_PISTOL_SCALE", _environment_value, _platform_vr,
		"The pistol (round sight) scope's size as a share of vr.scope_size (1 = as usual)." },
	{ "vr.scope_sniper_forward", _config_real, "0.0", "HALO_VR_SCOPE_SNIPER_FORWARD", _environment_value, _platform_vr,
		"The sniper rifle scope moved forward (toward the muzzle) from its usual place, metres (-0.30..0.30)." },
	{ "vr.scope_sniper_up", _config_real, "0.0", "HALO_VR_SCOPE_SNIPER_UP", _environment_value, _platform_vr,
		"The sniper rifle scope moved up from its usual place, metres (-0.30..0.30)." },
	{ "vr.scope_sniper_right", _config_real, "0.0", "HALO_VR_SCOPE_SNIPER_RIGHT", _environment_value, _platform_vr,
		"The sniper rifle scope moved right (in either hand) from its usual place, metres (-0.30..0.30)." },
	{ "vr.scope_sniper_scale", _config_real, "1.0", "HALO_VR_SCOPE_SNIPER_SCALE", _environment_value, _platform_vr,
		"The sniper rifle scope's size as a share of vr.scope_size (1 = as usual)." },
	{ "vr.button_jump", _config_string, "\"a\"", "HALO_VR_BUTTON_JUMP", _environment_value, _platform_vr,
		"Quest: the button that jumps: a, b (the gun hand's lower and upper), x, y (the other hand's),\n"
		"right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_action", _config_string, "\"b\"", "HALO_VR_BUTTON_ACTION", _environment_value, _platform_vr,
		"Quest: the button for action and reload: a, b (the gun hand's lower and upper), x, y (the other hand's),\n"
		"right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_melee", _config_string, "\"right_stick\"", "HALO_VR_BUTTON_MELEE", _environment_value, _platform_vr,
		"Quest: the button that melees: a, b (the gun hand's lower and upper), x, y (the other hand's),\n"
		"right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_crouch", _config_string, "\"right_stick_down\"", "HALO_VR_BUTTON_CROUCH", _environment_value, _platform_vr,
		"Quest: the button that crouches (default since 1.0.8: the turning stick held down; ducking\n"
		"also crouches): a, b (the gun hand's lower and upper), x, y (the other hand's),\n"
		"right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_reticle", _config_string, "\"left_stick\"", "HALO_VR_BUTTON_RETICLE", _environment_value, _platform_vr,
		"Quest: the button that shows or hides the reticle (it starts shown; not in a seat, and not\n"
		"with both sticks clicked, which recentres): a, b, x, y, right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_switch_weapon", _config_string, "\"y\"", "HALO_VR_BUTTON_SWITCH_WEAPON", _environment_value, _platform_vr,
		"Quest: the button that switches weapons: a, b (the gun hand's lower and upper), x, y (the other hand's),\n"
		"right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_grenade", _config_string, "\"x\"", "HALO_VR_BUTTON_GRENADE", _environment_value, _platform_vr,
		"Quest: the button that throws a grenade (\"grip\": the gun hand's grip, locked weapons only, throws while held): a, b (the gun hand's lower and upper), x, y (the other hand's),\n"
		"right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.button_switch_grenade", _config_string, "\"hold\"", "HALO_VR_BUTTON_SWITCH_GRENADE", _environment_value, _platform_vr,
		"Quest: the button that switches grenades, or \"hold\" (default): holding the grenade\n"
		"button switches and tapping it throws. Also a, b, x, y, right_stick, left_stick, right_stick_down (the turning stick held down), grip or none." },
	{ "vr.align_left_pitch", _config_real, "0.0", "HALO_VR_ALIGN_LEFT_PITCH", _environment_value, _platform_vr,
		"Left controller tracking correction, local pitch: degrees (-180..180). Moves hand AND gun;\n"
		"for comfort use vr.hand_* (hand) or vr.weapon_* (gun). Zero preserves runtime tracking." },
	{ "vr.align_left_yaw", _config_real, "0.0", "HALO_VR_ALIGN_LEFT_YAW", _environment_value, _platform_vr,
		"Left controller tracking correction, local yaw: degrees (-180..180). Moves hand AND gun;\n"
		"for comfort use vr.hand_* (hand) or vr.weapon_* (gun). Zero preserves runtime tracking." },
	{ "vr.align_left_roll", _config_real, "0.0", "HALO_VR_ALIGN_LEFT_ROLL", _environment_value, _platform_vr,
		"Left controller tracking correction, local roll: degrees (-180..180). Moves hand AND gun;\n"
		"for comfort use vr.hand_* (hand) or vr.weapon_* (gun). Zero preserves runtime tracking." },
	{ "vr.align_left_right", _config_real, "0.0", "HALO_VR_ALIGN_LEFT_RIGHT", _environment_value, _platform_vr,
		"Left controller local right: metres (-0.20..0.20), in the original controller frame. Zero preserves runtime tracking." },
	{ "vr.align_left_up", _config_real, "0.0", "HALO_VR_ALIGN_LEFT_UP", _environment_value, _platform_vr,
		"Left controller local up: metres (-0.20..0.20), in the original controller frame. Zero preserves runtime tracking." },
	{ "vr.align_left_back", _config_real, "0.0", "HALO_VR_ALIGN_LEFT_BACK", _environment_value, _platform_vr,
		"Left controller local back: metres (-0.20..0.20), in the original controller frame. Zero preserves runtime tracking." },
	{ "vr.align_left_grip_aim", _config_boolean, "false", "HALO_VR_ALIGN_LEFT_GRIP_AIM", _environment_value, _platform_vr,
		"Use left grip pose for aim too. Optional controller compatibility mode; false preserves native aim." },
	{ "vr.align_right_pitch", _config_real, "0.0", "HALO_VR_ALIGN_RIGHT_PITCH", _environment_value, _platform_vr,
		"Right controller tracking correction, local pitch: degrees (-180..180). Moves hand AND gun;\n"
		"for comfort use vr.hand_* (hand) or vr.weapon_* (gun). Zero preserves runtime tracking." },
	{ "vr.align_right_yaw", _config_real, "0.0", "HALO_VR_ALIGN_RIGHT_YAW", _environment_value, _platform_vr,
		"Right controller tracking correction, local yaw: degrees (-180..180). Moves hand AND gun;\n"
		"for comfort use vr.hand_* (hand) or vr.weapon_* (gun). Zero preserves runtime tracking." },
	{ "vr.align_right_roll", _config_real, "0.0", "HALO_VR_ALIGN_RIGHT_ROLL", _environment_value, _platform_vr,
		"Right controller tracking correction, local roll: degrees (-180..180). Moves hand AND gun;\n"
		"for comfort use vr.hand_* (hand) or vr.weapon_* (gun). Zero preserves runtime tracking." },
	{ "vr.align_right_right", _config_real, "0.0", "HALO_VR_ALIGN_RIGHT_RIGHT", _environment_value, _platform_vr,
		"Right controller local right: metres (-0.20..0.20), in the original controller frame. Zero preserves runtime tracking." },
	{ "vr.align_right_up", _config_real, "0.0", "HALO_VR_ALIGN_RIGHT_UP", _environment_value, _platform_vr,
		"Right controller local up: metres (-0.20..0.20), in the original controller frame. Zero preserves runtime tracking." },
	{ "vr.align_right_back", _config_real, "0.0", "HALO_VR_ALIGN_RIGHT_BACK", _environment_value, _platform_vr,
		"Right controller local back: metres (-0.20..0.20), in the original controller frame. Zero preserves runtime tracking." },
	{ "vr.align_right_grip_aim", _config_boolean, "false", "HALO_VR_ALIGN_RIGHT_GRIP_AIM", _environment_value, _platform_vr,
		"Use right grip pose for aim too. Optional controller compatibility mode; false preserves native aim." },
	{ "vr.hand_tracking", _config_string, "\"ik\"", "HALO_VR_HAND_TRACKING", _environment_value, _platform_vr,
		"How tracked hands follow the controllers (with vr.arms \"ik\"): \"ik\" body-IK arms reach\n"
		"from the shoulders; \"floating\" hands go exactly where the controllers are, with no\n"
		"arms; \"floating_arms\" hands go exactly to the controllers and arms hang from a\n"
		"shoulder that follows them." },
	{ "vr.hand_left_pitch", _config_real, "-70.0", "HALO_VR_HAND_LEFT_PITCH", _environment_value, _platform_vr,
		"Left visible hand only: degrees (-180..180) around the controller's X axis. Default -70\n"
		"lines the empty hand up with a hand holding a Touch controller. Does not move the gun." },
	{ "vr.hand_left_yaw", _config_real, "0.0", "HALO_VR_HAND_LEFT_YAW", _environment_value, _platform_vr,
		"Left visible hand only: degrees around the controller's Y axis. Does not move the gun." },
	{ "vr.hand_left_roll", _config_real, "0.0", "HALO_VR_HAND_LEFT_ROLL", _environment_value, _platform_vr,
		"Left visible hand only: degrees around the controller's Z axis. Does not move the gun." },
	{ "vr.hand_right_pitch", _config_real, "-70.0", "HALO_VR_HAND_RIGHT_PITCH", _environment_value, _platform_vr,
		"Right visible hand only: as vr.hand_left_pitch. Does not move the gun." },
	{ "vr.hand_right_yaw", _config_real, "0.0", "HALO_VR_HAND_RIGHT_YAW", _environment_value, _platform_vr,
		"Right visible hand only: as vr.hand_left_yaw. Does not move the gun." },
	{ "vr.hand_right_roll", _config_real, "0.0", "HALO_VR_HAND_RIGHT_ROLL", _environment_value, _platform_vr,
		"Right visible hand only: as vr.hand_left_roll. Does not move the gun." },
	{ "vr.weapon_pitch", _config_real, "0.0", "HALO_VR_WEAPON_PITCH", _environment_value, _platform_vr,
		"One-handed gun angle relative to the controller's aim: degrees (-180..180). Turns the\n"
		"gun, its shots and reticle together; mirrored for the left hand. Hands are unaffected." },
	{ "vr.weapon_yaw", _config_real, "0.0", "HALO_VR_WEAPON_YAW", _environment_value, _platform_vr,
		"As vr.weapon_pitch, around the controller's up axis." },
	{ "vr.weapon_roll", _config_real, "0.0", "HALO_VR_WEAPON_ROLL", _environment_value, _platform_vr,
		"As vr.weapon_pitch, around the aim axis." },
	{ "vr.aim_pistol_up", _config_real, "0.0", "HALO_VR_AIM_PISTOL_UP", _environment_value, _platform_vr,
		"Per-gun aim adjustment (Hands + Gun > Aim Up while holding the gun): degrees (-10..10)\n"
		"the pistol's shots, reticle and scope turn up from where the gun points. The gun itself\n"
		"does not move. 0 = no adjustment." },
	{ "vr.aim_pistol_right", _config_real, "0.0", "HALO_VR_AIM_PISTOL_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the pistol." },
	{ "vr.aim_plasma_pistol_up", _config_real, "0.0", "HALO_VR_AIM_PLASMA_PISTOL_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the plasma pistol." },
	{ "vr.aim_plasma_pistol_right", _config_real, "0.0", "HALO_VR_AIM_PLASMA_PISTOL_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the plasma pistol." },
	{ "vr.aim_assault_rifle_up", _config_real, "0.0", "HALO_VR_AIM_ASSAULT_RIFLE_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the assault rifle." },
	{ "vr.aim_assault_rifle_right", _config_real, "0.0", "HALO_VR_AIM_ASSAULT_RIFLE_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the assault rifle." },
	{ "vr.aim_plasma_rifle_up", _config_real, "0.0", "HALO_VR_AIM_PLASMA_RIFLE_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the plasma rifle." },
	{ "vr.aim_plasma_rifle_right", _config_real, "0.0", "HALO_VR_AIM_PLASMA_RIFLE_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the plasma rifle." },
	{ "vr.aim_shotgun_up", _config_real, "0.0", "HALO_VR_AIM_SHOTGUN_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the shotgun." },
	{ "vr.aim_shotgun_right", _config_real, "0.0", "HALO_VR_AIM_SHOTGUN_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the shotgun." },
	{ "vr.aim_sniper_rifle_up", _config_real, "0.0", "HALO_VR_AIM_SNIPER_RIFLE_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the sniper rifle." },
	{ "vr.aim_sniper_rifle_right", _config_real, "0.0", "HALO_VR_AIM_SNIPER_RIFLE_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the sniper rifle." },
	{ "vr.aim_rocket_launcher_up", _config_real, "0.0", "HALO_VR_AIM_ROCKET_LAUNCHER_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the rocket launcher." },
	{ "vr.aim_rocket_launcher_right", _config_real, "0.0", "HALO_VR_AIM_ROCKET_LAUNCHER_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the rocket launcher." },
	{ "vr.aim_needler_up", _config_real, "0.0", "HALO_VR_AIM_NEEDLER_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the needler." },
	{ "vr.aim_needler_right", _config_real, "0.0", "HALO_VR_AIM_NEEDLER_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the needler." },
	{ "vr.aim_fuel_rod_up", _config_real, "0.0", "HALO_VR_AIM_FUEL_ROD_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the fuel rod." },
	{ "vr.aim_fuel_rod_right", _config_real, "0.0", "HALO_VR_AIM_FUEL_ROD_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the fuel rod." },
	{ "vr.aim_flamethrower_up", _config_real, "0.0", "HALO_VR_AIM_FLAMETHROWER_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the flamethrower." },
	{ "vr.aim_flamethrower_right", _config_real, "0.0", "HALO_VR_AIM_FLAMETHROWER_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the flamethrower." },
	{ "vr.aim_other_up", _config_real, "0.0", "HALO_VR_AIM_OTHER_UP", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, for the any other gun (custom maps)." },
	{ "vr.aim_other_right", _config_real, "0.0", "HALO_VR_AIM_OTHER_RIGHT", _environment_value, _platform_vr,
		"As vr.aim_pistol_up, to the right (negative: left), for the any other gun (custom maps)." },
	{ "vr.calibration_split_applied", _config_boolean, "false", "HALO_VR_CALIBRATION_SPLIT", _environment_value, _platform_vr,
		"Internal one-time migration (test20c): hand-comfort rotations saved in vr.align_* move to\n"
		"vr.hand_*, so the gun returns to the controller's aim. Roll flips (+-180) stay." },
	{ "vr.gun_anchor", _config_boolean, "true", "HALO_VR_GUN_ANCHOR", _environment_value, _platform_vr,
		"Anchors the held gun to the controller: the gun hand's wrist sits where the empty hand's\n"
		"wrist would, for every weapon, and the gun turns about the hand. false restores the\n"
		"older camera placement (each weapon's animation decides where the gun sits)." },
	{ "vr.gun_forward", _config_real, "0.0", "HALO_VR_GUN_FORWARD", _environment_value, _platform_vr,
		"With vr.gun_anchor: moves the held gun along its barrel, in metres (-0.2..0.2;\n"
		"negative pulls it back toward you)." },
	{ "vr.gun_up", _config_real, "0.0", "HALO_VR_GUN_UP", _environment_value, _platform_vr,
		"As vr.gun_forward, up (negative is down)." },
	{ "vr.gun_out", _config_real, "0.0", "HALO_VR_GUN_OUT", _environment_value, _platform_vr,
		"As vr.gun_forward, away from your body's middle (right for the right hand, left for\n"
		"the left hand; negative is inward)." },
	{ "vr.weapon_offset_right", _config_real, "0.10", "HALO_VR_WEAPON_RIGHT", _environment_value, _platform_vr,
		"Advanced. How far right of the weapon camera the gun hand's grip sits, in metres\n"
		"(placing that camera). With vr.gun_anchor the gun stays in the hand whatever this is; it\n"
		"moves only the point the wall check measures from. Adjust the gun with vr.gun_* instead." },
	{ "vr.weapon_offset_up", _config_real, "-0.12", "HALO_VR_WEAPON_UP", _environment_value, _platform_vr,
		"Advanced; as vr.weapon_offset_right, above the camera (negative is below)." },
	{ "vr.weapon_offset_back", _config_real, "-0.20", "HALO_VR_WEAPON_BACK", _environment_value, _platform_vr,
		"Advanced; as vr.weapon_offset_right, behind the camera (negative is ahead)." },
	{ "vr.snap_turn", _config_real, "0.0", "HALO_VR_SNAP_TURN", _environment_value, _platform_vr,
		"Degrees the right stick turns you at a flick; 0 turns smoothly instead." },
	{ "vr.snap_turn_amount", _config_real, "45.0", "HALO_VR_SNAP_TURN_AMOUNT", _environment_value, _platform_vr,
		"The snap turn angle the VR menu's Comfort page keeps while turning smoothly, and\n"
		"takes up again for Turning: Snap (degrees). vr.snap_turn is what the game turns by." },
	{ "vr.vignette", _config_real, "0.0", "HALO_VR_VIGNETTE", _environment_value, _platform_vr,
		"Comfort vignette: how much the edges of the view darken while you move or turn\n"
		"(0 off, the default; 0.35 low, 0.65 medium, 1 high)." },
	{ "vr.vignette_when", _config_string, "\"move_turn\"", "HALO_VR_VIGNETTE_WHEN", _environment_value, _platform_vr,
		"When the comfort vignette shows: \"move_turn\" (moving or turning by stick, the default),\n"
		"\"turn\" (turning only) or \"always\"." },
	{ "vr.smooth_turn_speed", _config_real, "120.0", "HALO_VR_SMOOTH_TURN_SPEED", _environment_value, _platform_vr,
		"With vr.snap_turn 0: degrees a second the right stick turns you at full\n"
		"push." },
	{ "vr.force_render", _config_boolean, "false", "HALO_VR_FORCE_RENDER", _environment_value, _platform_vr,
		"Draw stereo frames with the headset off (in standby), from a head\n"
		"looking ahead: for testing." },
	{ "vr.diag_yaw", _config_real, "0.0", "HALO_VR_DIAG_YAW", _environment_value, _platform_vr,
		"With vr.force_render: turn that head left by this many degrees." },
	{ "vr.diag_hand_yaw", _config_real, "0.0", "HALO_VR_DIAG_HAND_YAW", _environment_value, _platform_vr,
		"With vr.force_render: turn the right hand left of the head by this many\n"
		"degrees." },
	{ "vr.diag_two_handed", _config_boolean, "false", "HALO_VR_DIAG_TWO_HANDED", _environment_value, _platform_vr,
		"With vr.force_render: hold the left hand on the gun, ahead and a little\n"
		"left of the right." },
	{ "vr.diag_drive_seconds", _config_real, "0.0", "HALO_VR_DIAG_DRIVE_SECONDS", _environment_value, _platform_vr,
		"This many seconds into play, seat the player as the nearest vehicle's\n"
		"driver: for testing; 0 never." },
	{ "vr.diag_walk_speed", _config_real, "0.0", "HALO_VR_DIAG_WALK_SPEED", _environment_value, _platform_vr,
		"With vr.force_render: the synthetic head walks ahead at this speed\n"
		"(metres a second), for testing vr.roomscale; 0 still." },
	{ "vr.diag_zoom_seconds", _config_real, "0.0", "HALO_VR_DIAG_ZOOM_SECONDS", _environment_value, _platform_vr,
		"This many seconds into play, zoom in (switching first to a weapon that\n"
		"zooms): for testing the scope; 0 never." },
	{ "vr.dump_frame", _config_integer, "0", "HALO_VR_DUMP_FRAME", _environment_value, _platform_vr,
		"Write the eyes and HUD of this stereo frame as vr-eye0.bmp, vr-eye1.bmp\n"
		"and vr-hud.bmp in the data folder; 0 none." },
	{ "vr.timing", _config_boolean, "true", "HALO_VR_TIMING", _environment_value, _platform_vr,
		"Log where each frame's time goes every 300 frames ([vr-frame])." },
	{ "vr.timing_gpu", _config_boolean, "false", "HALO_VR_TIMING_GPU", _environment_value, _platform_vr,
		"With vr.timing: wait for the GPU before showing each frame, to time it\n"
		"apart (slows the game)." },
	{ "vr.dump_cinema_frame", _config_integer, "0", "HALO_VR_DUMP_CINEMA_FRAME", _environment_value, _platform_vr,
		"As vr.dump_frame, for this frame of cutscenes on the 3D screen." },
	{ "vr.probe_seconds", _config_real, "0.0", "HALO_VR_PROBE", _environment_value, _platform_vr,
		"Before the game starts, show dim test colours in each eye for this many\n"
		"seconds and log the OpenXR frame rate; 0 skips it." },

	{ "graphics.preset", _config_string, "\"auto\"", "HALO_GRAPHICS_PRESET", _environment_value, _platform_vr,
		"Which effects the headset draws (port/linux/game/vr_graphics.c): \"low\",\n"
		"\"medium\", \"high\", or \"max\" (high, and with vr.resolution_scale 0 a\n"
		"sharper picture than the headset recommends). \"auto\": low on the first\n"
		"Quest, medium on the Quest 2, high otherwise. The settings below override\n"
		"it one effect at a time." },
	{ "graphics.shadows", _config_string, "\"auto\"", "HALO_GRAPHICS_SHADOWS", _environment_value, _platform_vr,
		"Objects' shadows: \"auto\" (as graphics.preset: high), \"on\" or \"off\"." },
	{ "graphics.dynamic_lights", _config_string, "\"auto\"", "HALO_GRAPHICS_DYNAMIC_LIGHTS", _environment_value, _platform_vr,
		"Moving lights on the level (the flashlight, plasma, explosions):\n"
		"\"auto\" (every preset), \"on\" or \"off\"." },
	{ "graphics.specular", _config_string, "\"auto\"", "HALO_GRAPHICS_SPECULAR", _environment_value, _platform_vr,
		"Shine on the level's surfaces: \"auto\" (medium and high), \"on\" or\n"
		"\"off\"." },
	{ "graphics.reflections", _config_string, "\"auto\"", "HALO_GRAPHICS_REFLECTIONS", _environment_value, _platform_vr,
		"The level's reflective surfaces: \"auto\" (high), \"on\" or \"off\"." },
	{ "graphics.bump_mapping", _config_string, "\"auto\"", "HALO_GRAPHICS_BUMP_MAPPING", _environment_value, _platform_vr,
		"Surface relief: \"auto\" (medium and high), \"on\" or \"off\"." },
	{ "graphics.detail_objects", _config_string, "\"auto\"", "HALO_GRAPHICS_DETAIL_OBJECTS", _environment_value, _platform_vr,
		"Grass and small plants: \"auto\" (medium and high), \"on\" or \"off\"." },
	{ "graphics.decals", _config_string, "\"auto\"", "HALO_GRAPHICS_DECALS", _environment_value, _platform_vr,
		"Bullet holes, scorch marks and blood: \"auto\" (every preset), \"on\" or\n"
		"\"off\"." },
	{ "graphics.particles", _config_string, "\"auto\"", "HALO_GRAPHICS_PARTICLES", _environment_value, _platform_vr,
		"Sparks, smoke and other particles: \"auto\" (every preset), \"on\" or\n"
		"\"off\"." },
	{ "graphics.contrails", _config_string, "\"auto\"", "HALO_GRAPHICS_CONTRAILS", _environment_value, _platform_vr,
		"Projectiles' trails: \"auto\" (medium and high), \"on\" or \"off\"." },
	{ "graphics.weather", _config_string, "\"auto\"", "HALO_GRAPHICS_WEATHER", _environment_value, _platform_vr,
		"Rain and snow: \"auto\" (medium and high), \"on\" or \"off\"." },
	{ "graphics.lens_flares", _config_string, "\"auto\"", "HALO_GRAPHICS_LENS_FLARES", _environment_value, _platform_vr,
		"Lens flares and the sun's glow: \"auto\" (high), \"on\" or \"off\"." },
	{ "graphics.fog_screen", _config_string, "\"auto\"", "HALO_GRAPHICS_FOG_SCREEN", _environment_value, _platform_vr,
		"The layered fog drawn across the view: \"auto\" (medium and high),\n"
		"\"on\" or \"off\"." },
	{ "graphics.camouflage_multipass", _config_string, "\"auto\"", "HALO_GRAPHICS_CAMOUFLAGE_MULTIPASS",
		_environment_value, _platform_vr,
		"Active camouflage's finer, costlier look: \"auto\" (medium and high),\n"
		"\"on\" or \"off\"." },
};

#define NUMBER_OF_CONFIG_SETTINGS (sizeof(config_settings) / sizeof(config_settings[0]))

#if defined(HALO_ANDROID) && defined(HALO_VR)
#define CONFIG_PLATFORM (_platform_android | _platform_vr)
#elif defined(HALO_ANDROID)
#define CONFIG_PLATFORM _platform_android
#elif defined(_WIN32)
#define CONFIG_PLATFORM _platform_windows
#else
#define CONFIG_PLATFORM _platform_linux
#endif

struct config_value
{
	int boolean;
	long integer;
	double real;
	char *string;
};

static struct config_value config_values[NUMBER_OF_CONFIG_SETTINGS];
static int config_loaded = 0;
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t active_network_version_lock = PTHREAD_MUTEX_INITIALIZER;
static int active_network_version = HALO_PORT_NETWORK_VERSION;
static int active_network_version_initialized = 0;

/* The active selection is a remote client target, not the local host wire
 * version. Zero means all versions in the upstream declared PvP range. */
static int network_protocol_version_resolve(long configured)
{
	if (configured == 0 || (configured >= HALO_PORT_NETWORK_VERSION_MINIMUM &&
		configured <= HALO_PORT_NETWORK_VERSION_MAXIMUM))
		return (int)configured;
	platform_log("network.protocol_version=%ld is unsupported; using default %d (supported: 0 or %d..%d)",
		configured, HALO_PORT_NETWORK_VERSION, HALO_PORT_NETWORK_VERSION_MINIMUM,
		HALO_PORT_NETWORK_VERSION_MAXIMUM);
	return HALO_PORT_NETWORK_VERSION;
}

static void active_network_version_initialize(void)
{
	long configured = config_integer("network.protocol_version");
	active_network_version = network_protocol_version_resolve(configured);
	if (active_network_version == 0)
		platform_log("network client target: all compatible (%d..%d); local host/listing protocol remains %d; fixed until restart",
			HALO_PORT_NETWORK_VERSION_MINIMUM, HALO_PORT_NETWORK_VERSION_MAXIMUM,
			HALO_PORT_NETWORK_VERSION);
	else
		platform_log("network client target: exact %d; local host/listing protocol remains %d; fixed until restart",
			active_network_version, HALO_PORT_NETWORK_VERSION);
}

int halo_port_active_network_version(void)
{
	int result;
	pthread_mutex_lock(&active_network_version_lock);
	if (!active_network_version_initialized)
	{
		active_network_version_initialize();
		active_network_version_initialized = 1;
	}
	result = active_network_version;
	pthread_mutex_unlock(&active_network_version_lock);
	return result;
}

/* ---------- the file */

static void config_path(char *path, size_t size)
{
#ifdef HALO_ANDROID
	/* the data folder, which the app names (port/android/host/host_main.c) */
	const char *root = getenv("HALO_DATA_ROOT");

	snprintf(path, size, "%s/config.toml", root && *root ? root : ".");
#else
	/* the executable's folder, with its separator */
	const char *base = SDL_GetBasePath();

	snprintf(path, size, "%sconfig.toml", base ? base : "");
#endif
}

/* the whole file, NUL terminated, or NULL; free() it */
static char *config_read_file(const char *path, size_t *size)
{
#ifdef HALO_ANDROID
	FILE *file = fopen(path, "rb");
	char *text = NULL;
	long length;

	if (!file)
		return NULL;
	if (fseek(file, 0, SEEK_END) == 0 && (length = ftell(file)) >= 0 && fseek(file, 0, SEEK_SET) == 0)
	{
		text = malloc((size_t)length + 1);
		if (text && fread(text, 1, (size_t)length, file) == (size_t)length)
		{
			text[length] = 0;
			*size = (size_t)length;
		}
		else
		{
			free(text);
			text = NULL;
		}
	}
	fclose(file);
	return text;
#else
	/* SDL's, for UTF-8 paths on Windows */
	void *data = SDL_LoadFile(path, size);
	char *text;

	if (!data)
		return NULL;
	text = malloc(*size + 1);
	if (text)
	{
		memcpy(text, data, *size);
		text[*size] = 0;
	}
	SDL_free(data);
	return text;
#endif
}

static int config_write_file(const char *path, const char *text)
{
#ifdef HALO_ANDROID
	FILE *file = fopen(path, "wb");
	int written;

	if (!file)
		return 0;
	written = fwrite(text, 1, strlen(text), file) == strlen(text);
	return fclose(file) == 0 && written;
#else
	return SDL_SaveFile(path, text, strlen(text));
#endif
}

struct config_text
{
	char *buffer;
	size_t length, capacity;
};

static void config_append(struct config_text *text, const char *string)
{
	size_t length = strlen(string);

	if (text->length + length + 1 > text->capacity)
	{
		size_t capacity = (text->capacity ? text->capacity : 4096) * 2 + length;
		char *buffer = realloc(text->buffer, capacity);

		if (!buffer)
			return;
		text->buffer = buffer;
		text->capacity = capacity;
	}
	memcpy(text->buffer + text->length, string, length + 1);
	text->length += length;
}

/* the first length characters of text, as a string of their own */
static char *config_copy(const char *text, size_t length)
{
	char *copy = malloc(length + 1);

	if (copy)
	{
		memcpy(copy, text, length);
		copy[length] = 0;
	}
	return copy;
}

static const char *config_default_value(const struct config_setting *setting)
{
#if defined(HALO_ANDROID) || defined(HALO_VR)
    if (!strcmp(setting->name, "renderer.safe_geometry")) return "true";
#endif
    return setting->default_value;
}

/* one setting as the file holds it: its comment, and its key at the
default */
static void config_append_setting(struct config_text *text, const struct config_setting *setting)
{
	const char *dot = strchr(setting->name, '.');
	const char *line;
	char buffer[256];

	config_append(text, "\n");
	for (line = setting->comment; *line;)
	{
		size_t length = strcspn(line, "\n");

		snprintf(buffer, sizeof(buffer), "# %.*s\n", (int)length, line);
		config_append(text, buffer);
		line += length;
		if (*line)
			line++;
	}
#ifndef HALO_ANDROID
	/* (Android apps have no environment to set) */
	switch (setting->environment_style)
	{
	case _environment_value:
		snprintf(buffer, sizeof(buffer), "# (for one run: %s=<value>)\n", setting->environment);
		break;
	case _environment_set_is_true:
		snprintf(buffer, sizeof(buffer), "# (for one run: %s=1 makes it true)\n", setting->environment);
		break;
	case _environment_set_is_false:
		snprintf(buffer, sizeof(buffer), "# (for one run: %s=1 makes it false)\n", setting->environment);
		break;
	}
	config_append(text, buffer);
#endif
	snprintf(buffer, sizeof(buffer), "%s = %s\n", dot + 1, config_default_value(setting));
	config_append(text, buffer);
}

/* the file with every setting of this build at its default */
static char *config_default_text(void)
{
	struct config_text text = { NULL, 0, 0 };
	char section[32] = "";
	size_t index;

#ifdef HALO_ANDROID
	config_append(&text,
		"# Halo settings\n"
		"#\n"
		"# The game writes this file with the defaults when it is missing: delete\n"
		"# it to go back to them.\n");
#else
	config_append(&text,
		"# Halo settings\n"
		"#\n"
		"# The game writes this file with the defaults when it is missing: delete\n"
		"# it to go back to them. Each setting can also be set for one run with\n"
		"# the environment variable named with it, which wins over this file.\n");
#endif
	for (index = 0; index < NUMBER_OF_CONFIG_SETTINGS; index++)
	{
		const struct config_setting *setting = &config_settings[index];
		const char *dot = strchr(setting->name, '.');
		size_t prior, item, length;
		char buffer[64];
		if (!(setting->platforms & CONFIG_PLATFORM) || !dot) continue;
		length = (size_t)(dot - setting->name);
		for (prior = 0; prior < index; prior++)
		{
			const struct config_setting *other = &config_settings[prior];
			if ((other->platforms & CONFIG_PLATFORM) &&
				!strncmp(other->name, setting->name, length) && other->name[length] == '.') break;
		}
		if (prior < index) continue;
		snprintf(section, sizeof(section), "%.*s", (int)length, setting->name);
		snprintf(buffer, sizeof(buffer), "\n[%s]\n", section);
		config_append(&text, buffer);
		for (item = index; item < NUMBER_OF_CONFIG_SETTINGS; item++)
		{
			const struct config_setting *other = &config_settings[item];
			if ((other->platforms & CONFIG_PLATFORM) &&
				!strncmp(other->name, section, length) && other->name[length] == '.')
				config_append_setting(&text, other);
		}
	}
	return text.buffer;
}

/* the settings of this build that text (the file, parsed as table) lacks,
added to it in their sections, keeping the rest as it is: a newer version's
settings appear in an older file. Returns the new text, or NULL if nothing
was missing */
static char *config_add_missing(const char *text, toml_datum_t table)
{
	char *result = NULL;
	size_t index;

	for (index = 0; index < NUMBER_OF_CONFIG_SETTINGS; index++)
	{
		const struct config_setting *setting = &config_settings[index];
		const char *dot = strchr(setting->name, '.');
		const char *current = result ? result : text;
		struct config_text block = { NULL, 0, 0 };
		struct config_text updated = { NULL, 0, 0 };
		char header[40];
		const char *line;
		const char *insert = NULL;

		if (!(setting->platforms & CONFIG_PLATFORM) || !dot || toml_seek(table, setting->name).type != TOML_UNKNOWN)
			continue;
		snprintf(header, sizeof(header), "[%.*s]", (int)(dot - setting->name), setting->name);
		/* the end of the section's last line that is not blank */
		for (line = current; *line; )
		{
			const char *start = line;
			size_t length = strcspn(line, "\n");

			while (*start == ' ' || *start == '\t')
				start++;
			if (insert && *start == '[')
				break;
			if (!insert && !strncmp(start, header, strlen(header)))
				insert = line + length;
			else if (insert && start < line + length && *start != '\r')
				insert = line + length;
			line += length;
			if (*line)
				line++;
		}
		if (insert)
		{
			if (*insert)
				insert++;
			config_append_setting(&block, setting);
		}
		else
		{
			/* no such section: a new one at the end */
			insert = current + strlen(current);
			config_append(&block, current[0] && insert[-1] != '\n' ? "\n\n" : "\n");
			config_append(&block, header);
			config_append(&block, "\n");
			config_append_setting(&block, setting);
		}
		if (!block.buffer)
			continue;
		{
			char *before = config_copy(current, (size_t)(insert - current));

			if (before)
				config_append(&updated, before);
			free(before);
		}
		if (insert > current && insert[-1] != '\n')
			config_append(&updated, "\n");
		config_append(&updated, block.buffer);
		config_append(&updated, insert);
		free(block.buffer);
		if (updated.buffer)
		{
			free(result);
			result = updated.buffer;
			platform_log("settings: added %s (new in this version) at its default", setting->name);
		}
	}
	return result;
}

/* ---------- values */

static int config_text_is_false(const char *text)
{
	char lower[8];
	size_t index;

	for (index = 0; index + 1 < sizeof(lower) && text[index]; index++)
		lower[index] = (char)tolower((unsigned char)text[index]);
	lower[index] = 0;
	return !strcmp(lower, "0") || !strcmp(lower, "false") || !strcmp(lower, "no") || !strcmp(lower, "off");
}

static void config_set_from_text(struct config_value *value, enum config_type type, const char *text)
{
	switch (type)
	{
	case _config_boolean:
		value->boolean = !config_text_is_false(text);
		break;
	case _config_integer:
		value->integer = strtol(text, NULL, 10);
		break;
	case _config_real:
		value->real = strtod(text, NULL);
		break;
	case _config_string:
		free(value->string);
		value->string = strdup(text);
		break;
	}
}

/* the value in the file, if it is there and of the setting's type */
static void config_set_from_file(struct config_value *value, const struct config_setting *setting,
	toml_datum_t table)
{
	toml_datum_t datum = toml_seek(table, setting->name);
	int wrong_type = 0;

	if (datum.type == TOML_UNKNOWN)
		return;
	switch (setting->type)
	{
	case _config_boolean:
		if (datum.type == TOML_BOOLEAN)
			value->boolean = datum.u.boolean;
		else
			wrong_type = 1;
		break;
	case _config_integer:
		if (datum.type == TOML_INT64)
			value->integer = (long)datum.u.int64;
		else
			wrong_type = 1;
		break;
	case _config_real:
		if (datum.type == TOML_FP64)
			value->real = datum.u.fp64;
		else if (datum.type == TOML_INT64)
			value->real = (double)datum.u.int64;
		else
			wrong_type = 1;
		break;
	case _config_string:
		if (datum.type == TOML_STRING)
		{
			free(value->string);
			value->string = strdup(datum.u.s);
		}
		else
		{
			wrong_type = 1;
		}
		break;
	}
	if (wrong_type)
	{
		static const char *const expected[] = { "true or false", "a whole number", "a number", "a quoted string" };

		platform_log("config.toml line %d: %s should be %s; using %s", datum.lineno, setting->name,
			expected[setting->type], config_default_value(setting));
	}
}

static long config_setting_index(const char *name)
{
	size_t index;

	for (index = 0; index < NUMBER_OF_CONFIG_SETTINGS; index++)
	{
		if (!strcmp(config_settings[index].name, name))
			return (long)index;
	}
	return -1;
}

/* keys in the file that are no setting, likely misspelt */
static void config_report_unknown_keys(toml_datum_t table)
{
	int section_index;

	for (section_index = 0; section_index < table.u.tab.size; section_index++)
	{
		toml_datum_t section = table.u.tab.value[section_index];
		int key_index;

		if (section.type != TOML_TABLE)
		{
			platform_log("config.toml line %d: unknown setting %s", section.lineno, table.u.tab.key[section_index]);
			continue;
		}
		for (key_index = 0; key_index < section.u.tab.size; key_index++)
		{
			char name[128];

			snprintf(name, sizeof(name), "%s.%s", table.u.tab.key[section_index], section.u.tab.key[key_index]);
			if (config_setting_index(name) < 0)
				platform_log("config.toml line %d: unknown setting %s", section.u.tab.value[key_index].lineno, name);
		}
	}
}

#if defined(HALO_ANDROID) || defined(HALO_VR)
/* Work only on parsed, ordinary section/key lines; preserve all other text.
 * An unusual inline/dotted form is never rewritten by guessing a TOML span. */
#ifdef HALO_VR
#define GEOMETRY_REVISION_KEY "vr_geometry_revision"
#define GEOMETRY_EDITION "VR"
#else
#define GEOMETRY_REVISION_KEY "android_geometry_revision"
#define GEOMETRY_EDITION "Android"
#endif
static int config_line_key(const char *, const char *, const char *);
static int config_line_section(const char *, const char *, char *, size_t);
/* Preserve the previous config and replace atomically within its directory.
 * If storage fails, this run is still safe and the migration retries next launch. */
static int config_write_geometry_migration(const char *path, const char *completed, const char *original)
{
    char backup[1100], temporary[1100];
    FILE *existing;
    toml_result_t verified = toml_parse(completed, (int)strlen(completed));
    int valid = verified.ok;
    toml_free(verified);
    if (!valid || !original) return 0;
    snprintf(backup, sizeof(backup), "%s.pre-safe-geometry", path);
    snprintf(temporary, sizeof(temporary), "%s.safe-geometry.tmp", path);
    existing = fopen(backup, "rb");
    if (existing) fclose(existing);
    else {
        /* Exclusive creation proves ownership: an unreadable existing backup
         * must never be truncated or removed after a failed open. */
        FILE *created = fopen(backup, "wbx");
        int written, closed;
        if (!created) return 0;
        written = fwrite(original, 1, strlen(original), created) == strlen(original);
        closed = fclose(created) == 0;
        if (!written || !closed) {
            /* Do not leave a partial new backup that retries would trust. */
            remove(backup);
            return 0;
        }
    }
    if (!config_write_file(temporary, completed)) { remove(temporary); return 0; }
    if (rename(temporary, path) != 0) { remove(temporary); return 0; }
    return 1;
}

static char *config_replace_section_line(const char *text, const char *wanted, const char *key, const char *value, int *found)
{
    struct config_text out = {0};
    char section[64] = "", replacement[128];
    const char *line = text;
    *found = 0;
    snprintf(replacement, sizeof(replacement), "%s = %s\n", key, value);
    while (*line) {
        const char *end = line + strcspn(line, "\n"), *next = *end ? end + 1 : end;
        config_line_section(line, end, section, sizeof(section));
        if (!strcmp(section, wanted) && config_line_key(line, end, key)) {
            config_append(&out, replacement); *found = 1;
        } else {
            char *copy = config_copy(line, next - line);
            if (copy) { config_append(&out, copy); free(copy); }
        }
        line = next;
    }
    return out.buffer;
}
static char *config_replace_geometry_line(const char *text, const char *key, const char *value, int *found)
{
    return config_replace_section_line(text, "renderer", key, value, found);
}

#endif

static void config_load(void)
{
	char path[1024];
	size_t size = 0;
	char *text;
	size_t index;

	for (index = 0; index < NUMBER_OF_CONFIG_SETTINGS; index++)
	{
		const char *default_value = config_default_value(&config_settings[index]);

		if (config_settings[index].type == _config_string)
		{
			/* written as a TOML basic string without escapes */
			size_t length = strlen(default_value);

			config_values[index].string = length >= 2 ? config_copy(default_value + 1, length - 2) : strdup("");
		}
		else
		{
			config_set_from_text(&config_values[index], config_settings[index].type, default_value);
		}
	}

	config_path(path, sizeof(path));
	text = config_read_file(path, &size);
	if (text)
	{
		toml_result_t result = toml_parse(text, (int)size);

		if (result.ok)
		{
			char *completed;
#if defined(HALO_ANDROID) || defined(HALO_VR)
            toml_datum_t revision = toml_seek(result.toptab, "renderer." GEOMETRY_REVISION_KEY);
            int migrate = revision.type != TOML_INT64 || revision.u.int64 < 1;
            int can_write = 1;
            char *original = migrate ? strdup(text) : NULL;
            if (migrate) {
                toml_datum_t safe = toml_seek(result.toptab, "renderer.safe_geometry");
                int found;
                char *changed = config_replace_geometry_line(text, "safe_geometry", "true", &found);
                can_write = changed && (found || safe.type == TOML_UNKNOWN);
                if (can_write) {
                    char *with_revision = config_replace_geometry_line(changed, GEOMETRY_REVISION_KEY, "1", &found);
                    free(changed);
                    if (with_revision) {
                        toml_result_t verified = toml_parse(with_revision, (int)strlen(with_revision));
                        if (verified.ok) {
                            free(text); text = with_revision;
                            toml_free(result); result = verified;
                        } else {
                            toml_free(verified); free(with_revision); can_write = 0;
                        }
                    } else can_write = 0;
                } else free(changed);
                platform_log("settings: " GEOMETRY_EDITION " Safe geometry default migration%s; later Safe/Normal choices preserved",
                    can_write ? " applied" : " in memory only (custom config syntax preserved)");
            }
#endif

			for (index = 0; index < NUMBER_OF_CONFIG_SETTINGS; index++)
				config_set_from_file(&config_values[index], &config_settings[index], result.toptab);
			config_report_unknown_keys(result.toptab);
			platform_log("settings: %s", path);
#if defined(HALO_ANDROID) || defined(HALO_VR)
            if (migrate) config_values[config_setting_index("renderer.safe_geometry")].boolean = 1;
            completed = can_write ? config_add_missing(text, result.toptab) : NULL;
            if (migrate && can_write && !completed) completed = strdup(text);
#else
            completed = config_add_missing(text, result.toptab);
#endif
#if defined(HALO_ANDROID) || defined(HALO_VR)
            if (completed && !(migrate ? config_write_geometry_migration(path, completed, original) : config_write_file(path, completed)))
                platform_log("settings: cannot write %s; Safe geometry migration will retry", path);
            free(original);
#else
            if (completed && !config_write_file(path, completed))
                platform_log("settings: cannot write %s", path);
#endif
            free(completed);
		}
		else
		{
			platform_log("config.toml: %s; using the defaults", result.errmsg);
		}
		toml_free(result);
		free(text);
	}
	else
	{
		char *defaults = config_default_text();

		if (defaults && config_write_file(path, defaults))
			platform_log("settings: wrote the defaults to %s", path);
		else
			platform_log("settings: cannot write %s; using the defaults", path);
		free(defaults);
	}

	for (index = 0; index < NUMBER_OF_CONFIG_SETTINGS; index++)
	{
		const struct config_setting *setting = &config_settings[index];
		const char *environment = getenv(setting->environment);

		if (!environment)
			continue;
		switch (setting->environment_style)
		{
		case _environment_value:
			config_set_from_text(&config_values[index], setting->type, environment);
			break;
		case _environment_set_is_true:
			config_values[index].boolean = 1;
			break;
		case _environment_set_is_false:
			config_values[index].boolean = 0;
			break;
		}
	}
}

static const struct config_value *config_value(const char *name, enum config_type type)
{
	static const struct config_value none = { 0, 0, 0.0, "" };
	long index;

	pthread_mutex_lock(&config_lock);
	if (!config_loaded)
	{
		config_load();
		config_loaded = 1;
	}
	pthread_mutex_unlock(&config_lock);
	index = config_setting_index(name);
	if (index < 0 || config_settings[index].type != type)
	{
		platform_log("settings: no %s setting %s", type == _config_string ? "string" : "such", name);
		return &none;
	}
	return &config_values[index];
}

/* ---------- writing a setting */

/* the line's key, if it is "key = ..." (after spaces), in key */
static int config_line_key(const char *line, const char *end, const char *key)
{
	size_t length = strlen(key);

	while (line < end && (*line == ' ' || *line == '\t'))
		line++;
	if ((size_t)(end - line) <= length || strncmp(line, key, length) != 0)
		return 0;
	line += length;
	while (line < end && (*line == ' ' || *line == '\t'))
		line++;
	return line < end && *line == '=';
}

/* the section the line opens, if it is "[section]" (after spaces) */
static int config_line_section(const char *line, const char *end, char *section, size_t size)
{
	const char *close;

	while (line < end && (*line == ' ' || *line == '\t'))
		line++;
	if (line >= end || *line != '[')
		return 0;
	close = memchr(line, ']', (size_t)(end - line));
	if (!close || (size_t)(close - line - 1) >= size)
		return 0;
	memcpy(section, line + 1, (size_t)(close - line - 1));
	section[close - line - 1] = 0;
	return 1;
}

/* sets a setting of the type given, for now and in config.toml: its line
there is changed (or added), the rest of the file kept as it is. `value`
is the setting as text, `written` as the file holds it (a string quoted) */
/* test27: settings written since the start (config_changes; upstream's) */
static volatile unsigned long config_change_count;

static int config_write_typed(const char *name, enum config_type type, const char *value, const char *written_value)
{
	const char *dot = strchr(name, '.');
	long index = config_setting_index(name);
	char section[64], key[64], wanted[80], current[64] = "", line_text[256], path[1024];
	struct config_text out = { 0 };
	size_t size = 0;
	char *text;
	const char *line;
	int written = 0, in_section = 0, succeeded;

	if (index < 0 || config_settings[index].type != type || !dot || (size_t)(dot - name) >= sizeof(section))
		return 0;
	/* (the file read first, as the other settings are) */
	config_value(name, type);
	pthread_mutex_lock(&config_lock);
	snprintf(section, sizeof(section), "%.*s", (int)(dot - name), name);
	snprintf(key, sizeof(key), "%s", dot + 1);
	snprintf(line_text, sizeof(line_text), "%s = %s\n", key, written_value);
	snprintf(wanted, sizeof(wanted), "%s", section);
	config_path(path, sizeof(path));
	text = config_read_file(path, &size);
	for (line = text ? text : ""; *line;)
	{
		const char *end = line + strcspn(line, "\n");
		const char *next = *end ? end + 1 : end;

		if (config_line_section(line, end, current, sizeof(current)))
		{
			/* (leaving the section without the key: it goes at its end) */
			if (in_section && !written)
			{
				config_append(&out, line_text);
				written = 1;
			}
			in_section = !strcmp(current, wanted);
		}
		else if (in_section && !written && config_line_key(line, end, key))
		{
			config_append(&out, line_text);
			written = 1;
			line = next;
			continue;
		}
		{
			char *copy = config_copy(line, (size_t)(next - line));

			if (copy)
			{
				config_append(&out, copy);
				free(copy);
			}
		}
		line = next;
	}
	if (!written)
	{
		if (out.length && out.buffer[out.length - 1] != '\n')
			config_append(&out, "\n");
		if (!in_section)
		{
			char header[80];

			snprintf(header, sizeof(header), "\n[%s]\n", section);
			config_append(&out, header);
		}
		config_append(&out, line_text);
	}
	succeeded = out.buffer && config_write_file(path, out.buffer);
	if (succeeded)
	{
		/* Publish the value and its generation together only after persistence
		succeeds. Failed menu saves must leave all live consumers on the old value. */
		config_set_from_text(&config_values[index], type, value);
		config_change_count++;
	}
	pthread_mutex_unlock(&config_lock);
	free(out.buffer);
	free(text);
	return succeeded;
}

int config_write_boolean(const char *name, int value)
{
	return config_write_typed(name, _config_boolean, value ? "true" : "false", value ? "true" : "false");
}

int config_write_real(const char *name, double value)
{
	char text[64];

	snprintf(text, sizeof(text), "%.6g", value);
	/* (TOML wants a real's point) */
	if (!strpbrk(text, ".eEn"))
		strcat(text, ".0");
	return config_write_typed(name, _config_real, text, text);
}

int config_write_string(const char *name, const char *value)
{
	char quoted[256];

	if (strpbrk(value, "\"\\\n") || strlen(value) + 3 > sizeof(quoted))
		return 0;
	snprintf(quoted, sizeof(quoted), "\"%s\"", value);
	return config_write_typed(name, _config_string, value, quoted);
}

/* ---------- public code */

unsigned long config_changes(void)
{
	return config_change_count;
}

int config_boolean(const char *name)
{
	return config_value(name, _config_boolean)->boolean;
}

long config_integer(const char *name)
{
	return config_value(name, _config_integer)->integer;
}

double config_real(const char *name)
{
	return config_value(name, _config_real)->real;
}

/* the table's default for a real setting (reset buttons); 0 when unknown */
double config_default_real(const char *name)
{
	long index = config_setting_index(name);

	return index >= 0 && config_settings[index].type == _config_real ? atof(config_settings[index].default_value) : 0.0;
}

/* test20d: the menu's combined rows. Whether a setting holds a value given
as text, compared as the setting's own type (0 for an unknown name) */
int config_matches(const char *name, const char *text)
{
	long index = config_setting_index(name);
	double difference;

	if (index < 0)
		return 0;
	switch (config_settings[index].type)
	{
	case _config_boolean:
		return config_boolean(name) == !strcmp(text, "true");
	case _config_integer:
		return config_integer(name) == atol(text);
	case _config_real:
		difference = config_real(name) - atof(text);
		return difference > -0.001 && difference < 0.001;
	default:
		return !strcmp(config_string(name), text);
	}
}

/* writes a setting from its value as text, as the setting's own type; 1 on
success (integers are not written) */
int config_write_text(const char *name, const char *text)
{
	long index = config_setting_index(name);

	if (index < 0)
		return 0;
	switch (config_settings[index].type)
	{
	case _config_boolean:
		return config_write_boolean(name, !strcmp(text, "true"));
	case _config_real:
		return config_write_real(name, atof(text));
	case _config_string:
		return config_write_string(name, text);
	default:
		return 0;
	}
}

const char *config_string(const char *name)
{
	const char *string = config_value(name, _config_string)->string;

	return string ? string : "";
}

#ifdef HALO_VR
void config_vr_vehicle_defaults(void)
{
    char path[1024], backup[1100], temporary[1100];
    char *original, *changed;
    size_t size;
    int ok = 1, found, i;
    const char *keys[] = {"vehicle_view", "vehicle_steering", "vehicle_defaults_applied"};
    const char *values[] = {"\"chase\"", "\"right\"", "true"};
    if (config_boolean("vr.vehicle_defaults_applied")) return;
    pthread_mutex_lock(&config_lock);
    config_path(path, sizeof(path));
    original = config_read_file(path, &size);
    changed = original ? strdup(original) : NULL;
    for (i = 0; changed && i < 3; i++) {
        char *next = config_replace_section_line(changed, "vr", keys[i], values[i], &found);
        free(changed); changed = next;
        if (!found) ok = 0; /* Preserve nonstandard TOML syntax, no guessed edits. */
    }
    if (changed) {
        toml_result_t parsed = toml_parse(changed, (int)strlen(changed));
        ok = ok && parsed.ok; toml_free(parsed);
    } else ok = 0;
    snprintf(backup, sizeof(backup), "%s.pre-vehicle-defaults", path);
    snprintf(temporary, sizeof(temporary), "%s.vehicle-defaults.tmp", path);
    if (ok) {
        FILE *prior = fopen(backup, "rb");
        if (prior) fclose(prior); else ok = config_write_file(backup, original);
    }
    if (ok) {
        ok = config_write_file(temporary, changed) && rename(temporary, path) == 0;
        if (!ok) remove(temporary);
    }
    config_set_from_text(&config_values[config_setting_index("vr.vehicle_view")], _config_string, "chase");
    config_set_from_text(&config_values[config_setting_index("vr.vehicle_steering")], _config_string, "right");
    config_values[config_setting_index("vr.vehicle_defaults_applied")].boolean = ok;
    pthread_mutex_unlock(&config_lock);
    free(changed); free(original);
    platform_log("vr: third-person/right-controller defaults%s; later saved vehicle choices preserved", ok ? " saved" : " in memory only (migration will retry)");
}
#endif


int config_write(const char *name, const char *text)
{
	long index = config_setting_index(name);
	if (index < 0 || !text) return 0;
	if (config_settings[index].type == _config_integer)
	{
		char *end;
		long value;
		errno = 0;
		value = strtol(text, &end, 10);
		char canonical[32];
		if (end == text || *end || errno == ERANGE) return 0;
		snprintf(canonical, sizeof(canonical), "%ld", value);
		return config_write_typed(name, _config_integer, canonical, canonical);
	}
	if (config_settings[index].type == _config_real)
	{
		char *end;
		double value;
		errno = 0;
		value = strtod(text, &end);
		if (end == text || *end || errno == ERANGE || !isfinite(value)) return 0;
		return config_write_real(name, value);
	}
	if (config_settings[index].type == _config_boolean && strcmp(text, "true") && strcmp(text, "false"))
		return 0;
	return config_write_text(name, text);
}

int config_default(const char *name, char *text, size_t size)
{
	long index = config_setting_index(name);
	const char *value;
	size_t length;

	if (index < 0)
		return 0;
	value = config_default_value(&config_settings[index]);
	length = strlen(value);
	/* (a string's without its quotes: the defaults have no escapes) */
	if (config_settings[index].type == _config_string && length >= 2 && value[0] == '"')
	{
		value++;
		length -= 2;
	}
	snprintf(text, size, "%.*s", (int)length, value);
	return 1;
}

int config_text(const char *name, char *text, size_t size)
{
	long index = config_setting_index(name);
	const struct config_value *value;

	if (index < 0)
		return 0;
	value = config_value(name, config_settings[index].type);
	switch (config_settings[index].type)
	{
	case _config_boolean:
		snprintf(text, size, "%s", value->boolean ? "true" : "false");
		break;
	case _config_integer:
		snprintf(text, size, "%ld", value->integer);
		break;
	case _config_real:
		snprintf(text, size, "%.15g", value->real);
		break;
	case _config_string:
		snprintf(text, size, "%s", value->string ? value->string : "");
		break;
	}
	return 1;
}

void config_folder(char *path, size_t size)
{
	char file[1024];
	char *separator;

	config_path(file, sizeof(file));
	separator = strrchr(file, '/');
#ifndef HALO_ANDROID
	if (!separator || (strrchr(file, '\\') && strrchr(file, '\\') > separator))
		separator = strrchr(file, '\\');
#endif
	if (separator)
		separator[1] = 0;
	else
		file[0] = 0;
	snprintf(path, size, "%s", file);
}

char *config_file_read(const char *path, size_t *size)
{
	return config_read_file(path, size);
}
