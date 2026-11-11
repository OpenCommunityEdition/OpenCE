#ifndef THIRD_PERSON_H
#define THIRD_PERSON_H

enum third_person_match_policy
{
	_third_person_player_choice,
	_third_person_forced_on,
	_third_person_forced_off
};
/* Bits in network_game.cooperative_flags, shared by all match types. */
#define THIRD_PERSON_POLICY_SHIFT 4
#define THIRD_PERSON_POLICY_MASK 48
/* Replicated with each input tick, not latched like a button press. */
#define THIRD_PERSON_INPUT_BIT 16
short third_person_host_policy(void);
short third_person_policy(void);
boolean third_person_enabled(void);
boolean third_person_match_flags_valid(unsigned long flags);
void third_person_input_update(void);

struct render_camera;
union point2d;
union real_point3d;
union real_vector3d;
boolean third_person_muzzle_shot(long weapon_index, long unit_index, short trigger_index,
	union real_point3d *origin, union real_vector3d *forward);
boolean third_person_muzzle_history(long unit_index, union real_point3d *position);
void third_person_muzzle_target(short local_player_index, union real_point3d *target);
boolean third_person_free_look_input(short local_player_index, real yaw, real pitch);
void third_person_reset(void);
boolean third_person_active(short local_player_index);
boolean third_person_available(short local_player_index);
boolean third_person_director_active(short local_player_index);
void third_person_camera_update(void *camera, void *controls, void *result);
int third_person_camera_scroll(float notches, unsigned long actions);
void third_person_camera(short local_player_index, struct render_camera *camera);
boolean third_person_reticle(union point2d *point, real reticle_scale);

#endif
