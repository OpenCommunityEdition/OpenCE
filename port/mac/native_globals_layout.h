/* Native layouts from clean tag_schema_effects.c; no platform SDK types. */
struct game_globals_falling_damage
{
	long unused0[2];
	real falling_distance_lower_bound;
	real falling_distance_upper_bound;
	struct tag_reference falling_damage;
	long terminal_velocity_unused[2];
	real maximum_distance;
	struct tag_reference maximum_distance_damage;
	struct tag_reference vehicle_hit_environment_damage_effect;
	struct tag_reference vehicle_killed_unit_damage_effect;
	struct tag_reference vehicle_collision_damage;
	struct tag_reference flaming_death_damage;
	long unused7c[4];
	real runtime_maximum_falling_velocity;
	real runtime_minimum_damage_velocity;
	real runtime_maximum_damage_velocity;
};
struct native_global_reference { struct tag_reference reference; };
struct native_look_value { real value; };
/* Playlist file layout documented by Invader globals.json (format reference
   only): two 32-byte tag strings, five LE32 values, 64 reserved bytes.
   https://github.com/SnowyMouse/invader/blob/master/src/tag/hek/definition/globals.json */
struct native_global_playlist {
 char map_name[32]; char game_variant[32];
 long minimum_experience; long maximum_experience;
 long minimum_player_count; long maximum_player_count; long rating;
 byte reserved[64];
};
typedef char native_global_playlist_size[sizeof(struct native_global_playlist)==148?1:-1];
typedef char native_global_root_size[sizeof(struct game_globals)==428?1:-1];
typedef char native_global_falling_size[sizeof(struct game_globals_falling_damage)==152?1:-1];
