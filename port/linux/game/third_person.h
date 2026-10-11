#ifndef THIRD_PERSON_H
#define THIRD_PERSON_H

struct render_camera;
union point2d;
boolean third_person_active(short local_player_index);
void third_person_camera(short local_player_index, struct render_camera *camera);
boolean third_person_reticle(union point2d *point, real reticle_scale);

#endif
