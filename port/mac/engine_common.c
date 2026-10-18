/* Native storage for the engine's uninitialized common symbols. These use
   their declared C types, not the overallocated weak byte arrays of the x86
   linker bridge. Data-array pointers stay NULL until their real services
   initialize them; this file does not make any subsystem ready to use. */
#include "cseries.h"
#include "game/cheats.h"
#include "physics/collision_debug.h"
#include "effects/contrails.h"
#include "devices/devices.h"
#include "ai/ai_globals.h"
#include "ai/ai_communication.h"
#include "ai/props.h"
#include "ai/path.h"
#include "bungie_net/network/transport.h"
#include "camera/director.h"
#include "cseries/errors.h"
#include "cseries/profile.h"
#include "rasterizer/rasterizer.h"
#include "scenario/scenario.h"
#include <xtl.h>

struct cheat_globals cheat;
real_point3d collision_debug_point;
boolean collision_debug_repeat;
real_vector3d collision_debug_vector;
struct data_array *contrail_data;
struct data_array *contrail_point_data;
boolean debug_objects_biped_autoaim_pills;
boolean debug_objects_biped_physics_pills;
boolean debug_objects_devices;
boolean debug_objects_vehicle_powered_mass_points;
struct data_array *device_groups_data;
/* effects.c declares this pointer; its datum type remains private there. */
struct data_array *effect_data;
/* decals.c and particles.h declare these actual data-array pointers. */
struct data_array *global_decal_data;
struct data_array *particle_data;
struct ai_globals *ai_globals;
struct data_array *conversation_data;
boolean *director_camera_scripted;
struct error_global_data error_globals;
struct rasterizer_window_begin_parameters global_window_parameters;
boolean profile_graph;
struct data_array *prop_data;
boolean debug_ignore_broken_surfaces;
boolean debug_obstacle_path_finishing;
real_point3d debug_obstacle_path_goal_point;
long debug_obstacle_path_goal_surface_index;
real debug_obstacle_path_radius;
real_point3d debug_obstacle_path_start_point;
long debug_obstacle_path_start_surface_index;
/* hs.c owns this data-array pointer; initialization remains an HS service. */
struct data_array *hs_syntax_data;
boolean transport_initialized;

/* Engine globals with no reconstructed owner. Their declarations and use are
   in models.c, game_engine*.c, scenario.h, and rasterizer_xbox.c. Keep the
   native definitions typed; real source owners override none of these. */
boolean render_model_nodes;
boolean render_model_markers;
boolean render_model_vertex_counts;
boolean render_model_index_counts;
boolean render_model_no_geometry;
long timeout_for_endgame_sound;
struct scenario_globals *scenario_globals;

/* rasterizer_xbox.c's clean shell declaration. This is the exact consumer
   layout, including the three native handles used by Present(). */
struct window_data
{
	HINSTANCE hInstance;
	HWND hWnd;
	HWND hWndPresentTarget;
	int nCmdShow;
	void *MainWndProc;
	char class_name[0x40];
	char window_title[0x40];
};

struct window_data window_globals;

typedef char mac_common_pointer_size[sizeof(struct data_array *) == 4 ? 1 : -1];
typedef char mac_common_boolean_size[sizeof(boolean) == 1 ? 1 : -1];
typedef char mac_common_cheat_size[sizeof(struct cheat_globals) == 10 ? 1 : -1];
typedef char mac_common_point_size[sizeof(real_point3d) == 12 ? 1 : -1];
typedef char mac_common_vector_size[sizeof(real_vector3d) == 12 ? 1 : -1];
typedef char mac_common_timeout_size[sizeof(long) == 4 ? 1 : -1];
typedef char mac_common_window_globals_size[sizeof(struct window_data) == 0x94 ? 1 : -1];
typedef char mac_common_window_target_offset[
	__builtin_offsetof(struct window_data, hWndPresentTarget) == 8 ? 1 : -1];
typedef char mac_common_window_proc_offset[
	__builtin_offsetof(struct window_data, MainWndProc) == 16 ? 1 : -1];
