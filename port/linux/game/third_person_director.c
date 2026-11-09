/* The director owns the camera mode; the shoulder solver still runs after
   interpolation and camera effects, keeping its collision and aiming pivot. */
#include "cseries.h"
#include "third_person.h"
#include "camera/director.h"
#include "camera/first_person_camera.h"

boolean third_person_director_active(short player)
{
	return director_camera_is(player, third_person_camera_update);
}

void third_person_camera_update(void *camera, void *controls, void *result)
{
	/* Preserve the stock eye, aim assistance pivot and scope interpolation.
	   The wrapper has the director's exact callback type on wasm too. */
	first_person_camera_update((struct first_person_camera *)camera,
		(struct first_person_camera_action const *)controls,
		(struct first_person_camera_result *)result);
}
