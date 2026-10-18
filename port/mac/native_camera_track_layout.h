#ifndef HALO_MAC_NATIVE_CAMERA_TRACK_LAYOUT_H
#define HALO_MAC_NATIVE_CAMERA_TRACK_LAYOUT_H
#include "math/real_math.h"
#include "tag_files/tag_groups.h"
/* The camera track (trak) as source/camera/following_camera.c declares it,
   which keeps it private to that file. */
struct native_camera_track_control_point {
    real_vector3d position; real_quaternion orientation; long unused[8];
};
struct native_camera_track_definition {
    unsigned long flags; struct tag_block control_points; long unused[8];
};
typedef char native_camera_track_control_point_size[sizeof(struct native_camera_track_control_point)==0x3C?1:-1];
typedef char native_camera_track_control_points_offset[offsetof(struct native_camera_track_definition,control_points)==0x4?1:-1];
typedef char native_camera_track_size[sizeof(struct native_camera_track_definition)==0x30?1:-1];
#endif
