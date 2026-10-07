/* A local display adjustment, after the observer and before either render
camera's projection and visibility frustum are built. */
#ifndef RENDER_FOV_H
#define RENDER_FOV_H

float render_fov_vertical(short local_player_index, float native_vertical_field_of_view);

#endif
