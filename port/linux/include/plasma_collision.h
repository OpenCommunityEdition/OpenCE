/* Synchronous render-only ray batches; no state survives end(). */
#ifndef PLASMA_COLLISION_H
#define PLASMA_COLLISION_H
union real_point3d;
union real_vector3d;
struct collision_model_instance;
enum { PLASMA_COLLISION_RAYS = 1536, PLASMA_COLLISION_WORDS = PLASMA_COLLISION_RAYS / 32 };
void plasma_collision_begin(unsigned long flags, union real_point3d const *point, long ignore);
float plasma_collision_depth(unsigned long flags, union real_point3d const *point,
	union real_vector3d const *vector, long ignore);
void plasma_collision_end(void);
void plasma_collision_depths(unsigned long flags, union real_point3d const *point,
	union real_vector3d const *vectors, int count, long ignore, float *depths);
/* Internal model-major phase; all state and borrowed pointers are synchronous. */
void plasma_collision_model_depths(struct collision_model_instance const *instance,
	union real_point3d const *point, union real_vector3d const *vectors, int count,
	unsigned int const *pending, float *depths);
/* Value-only snapshots are bounded and reset with the occlusion fields. */
enum { PLASMA_COLLISION_SCENE_BYTES = 64 * 1024 };
struct plasma_collision_scene
{
	unsigned char bytes[PLASMA_COLLISION_SCENE_BYTES];
	unsigned long revision;
	int size;
};
unsigned long plasma_collision_scene_revision(struct plasma_collision_scene *scene,
	union real_point3d const *point, float extent, int vehicles);
#endif
