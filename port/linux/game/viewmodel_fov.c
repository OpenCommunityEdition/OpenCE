#include "cseries.h"
#include "viewmodel_fov.h"
#include "camera/director.h"
#include "cutscene/cinematics.h"
#include "game/players.h"
#include "render/render.h"
#include "render/render_cameras.h"
#include "render/render_cameras_internal.h"
#include "rasterizer/rasterizer.h"
#include "rasterizer/xbox/rasterizer_xbox_internal.h"
#include <math.h>

double config_real(const char *name);
unsigned long config_changes(void);

static unsigned projection_depth;
static boolean projection_applied;
static struct render_camera saved_render_camera, saved_rasterizer_camera;
static struct render_frustum saved_render_frustum, saved_rasterizer_frustum;

static real viewmodel_vertical(void)
{
	static boolean initialized;
	static unsigned long read_at;
	static real angle;
	unsigned long changes = config_changes();
	if (!initialized || read_at != changes)
	{
		double degrees = config_real("display.viewmodel_fov");
		angle = isfinite(degrees) && degrees >= 20.0 && degrees <= 150.0 ?
			(real)(2.0 * atan(tan(degrees * 3.14159265358979323846 / 360.0) * (9.0 / 16.0))) : 0.0f;
		initialized = TRUE;
		read_at = changes;
	}
	return angle;
}

void viewmodel_projection_begin(void)
{
	real angle;
	real_rectangle2d render_bounds, rasterizer_bounds;
	if (projection_depth++ != 0) return;
	projection_applied = FALSE;
	angle = viewmodel_vertical();
	if (!angle || render.local_player_index < 0 || render.local_player_index >= MAXIMUM_LOCAL_PLAYERS ||
		global_window_parameters.rasterizer_target != 0 /* primary view */ ||
		cinematic_in_progress() || (director_camera_scripted && *director_camera_scripted) ||
		director_get_perspective(render.local_player_index) != _director_perspective_first_person ||
		director_inhibited_facing(render.local_player_index)) return;

	saved_render_camera = render.camera;
	saved_render_frustum = render.frustum;
	saved_rasterizer_camera = global_window_parameters.camera;
	saved_rasterizer_frustum = global_window_parameters.frustum;
	/* Retain cropped/tiled viewport bounds. CPU visibility and GPU projection
	change together; pose, weapon markers and gameplay values stay put. */
	/* The builder takes normalized crop bounds. get_projection_bounds instead
	returns signed view-space ray slopes, which would invert/rescale the view. */
	render_bounds = saved_render_frustum.frustum_bounds;
	rasterizer_bounds = saved_rasterizer_frustum.frustum_bounds;
	render.camera.vertical_field_of_view = angle;
	global_window_parameters.camera.vertical_field_of_view = angle;
	render_camera_build_frustum(&render.camera, &render_bounds, &render.frustum, TRUE);
	render_camera_build_frustum(&global_window_parameters.camera, &rasterizer_bounds,
		&global_window_parameters.frustum, TRUE);
	projection_applied = TRUE;
	rasterizer_set_frustum_z(0.0f, 0.0f);
}

void viewmodel_projection_end(void)
{
	if (!projection_depth || --projection_depth || !projection_applied) return;
	render.camera = saved_render_camera;
	render.frustum = saved_render_frustum;
	global_window_parameters.camera = saved_rasterizer_camera;
	global_window_parameters.frustum = saved_rasterizer_frustum;
	projection_applied = FALSE;
	rasterizer_set_frustum_z(0.0f, 0.0f);
}
