#ifndef __NETWORK_VR_POSE_H
#define __NETWORK_VR_POSE_H
#include "math/real_math.h"
void network_vr_pose_reset(void);
void network_vr_pose_tick(void);
void network_vr_pose_receive(long machine, byte type, void const *data, short count, word size);
void network_vr_pose_capture(long unit, real_matrix4x3 const *matrices, short count);
boolean network_vr_pose_wanted(void);
real_matrix4x3 *network_vr_pose_render(long unit, real_matrix4x3 *stock);
#endif
