/* Controller-local calibration shared by grip, aim, IK, contacts and avatars.
 * Rotation order: local yaw(Y), pitch(X), roll(Z); position is in the ORIGINAL
 * controller frame (+X right, +Y up, +Z back), metres. No reflected matrices.
 * Zero correction is a byte-preserving no-op for accepted controller poses. */
#ifndef HALO_VR_ALIGNMENT_H
#define HALO_VR_ALIGNMENT_H
#include <math.h>
#include <string.h>
static float vr_alignment_bound(double value, float limit)
{
    return isfinite(value) ? (float)fmax(-limit, fmin(limit, value)) : 0.f;
}
static void vr_alignment_multiply(const float a[4], const float b[4], float out[4])
{
    float t[4] = {
        a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
        a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
        a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
        a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2] };
    memcpy(out,t,sizeof(t));
}
static void vr_alignment_rotation(const float degrees[3], float out[4])
{
    const float radians = 0.00872664626f;
    float p=degrees[0]*radians,y=degrees[1]*radians,r=degrees[2]*radians;
    float pitch[4]={sinf(p),0,0,cosf(p)},yaw[4]={0,sinf(y),0,cosf(y)},roll[4]={0,0,sinf(r),cosf(r)};
    vr_alignment_multiply(yaw,pitch,out); vr_alignment_multiply(out,roll,out);
}
static int vr_alignment_apply(float position[3], float orientation[4], const float rotation[4], const float offset[3])
{
    float q[4], translated[3], n=0.f; int i;
    for(i=0;i<4;i++) { if(!isfinite(orientation[i])) return 0; n+=orientation[i]*orientation[i]; }
    for(i=0;i<3;i++) if(!isfinite(position[i])) return 0;
    if(n<0.25f || n>4.f) return 0;
    if(rotation[0]==0.f && rotation[1]==0.f && rotation[2]==0.f && rotation[3]==1.f &&
        offset[0]==0.f && offset[1]==0.f && offset[2]==0.f) return 1;
    n=sqrtf(n); for(i=0;i<4;i++) q[i]=orientation[i]/n;
    /* q * (offset,0) * conjugate(q). */
    {
        float v[4]={offset[0],offset[1],offset[2],0}, inverse[4]={-q[0],-q[1],-q[2],q[3]}, t[4];
        vr_alignment_multiply(q,v,t); vr_alignment_multiply(t,inverse,t);
        for(i=0;i<3;i++) translated[i]=position[i]+t[i];
    }
    vr_alignment_multiply(q,rotation,orientation);
    memcpy(position,translated,sizeof(translated)); return 1;
}
static inline int vr_alignment_pose_valid(const float position[3], const float orientation[4])
{
    float n=0.f; int i;
    for(i=0;i<4;i++) { if(!isfinite(orientation[i])) return 0; n+=orientation[i]*orientation[i]; }
    for(i=0;i<3;i++) if(!isfinite(position[i])) return 0;
    return n>=0.25f && n<=4.f;
}
/* One calibration for one physical controller, applied to both of its poses
 * as a single rigid correction (test20). A held weapon takes its orientation
 * from the aim pose and an empty hand from the grip pose; both take the grip's
 * position. Applying the same local angles to each pose separately turned them
 * about different axes (Touch grip and aim frames are tilted apart), so a
 * yaw/roll tuned for the gun misaligned the empty hand, or the reverse.
 * Rotation keeps its aim-local meaning (the armed weapon is unchanged for
 * saved calibrations); the grip turns by the same world rotation. The offset
 * stays in the grip (controller) frame and moves both poses by one vector.
 * Without a valid aim pose the grip is corrected alone, as before.
 * valid: bit 0 grip, bit 1 aim; returns the bits still valid. */
static inline unsigned vr_alignment_apply_controller(float grip_position[3], float grip_orientation[4],
    float aim_position[3], float aim_orientation[4], unsigned valid, const float rotation[4], const float offset[3])
{
    float g[4], a[4], world[4], conjugate[4], v[4], t[4], inverse[4]; float n; int i;
    if((valid&1) && !vr_alignment_pose_valid(grip_position,grip_orientation)) valid&=~1u;
    if((valid&2) && !vr_alignment_pose_valid(aim_position,aim_orientation)) valid&=~2u;
    if(rotation[0]==0.f && rotation[1]==0.f && rotation[2]==0.f && rotation[3]==1.f &&
        offset[0]==0.f && offset[1]==0.f && offset[2]==0.f) return valid;
    if(!(valid&2)) {
        if((valid&1) && !vr_alignment_apply(grip_position,grip_orientation,rotation,offset)) valid&=~1u;
        return valid;
    }
    if(!(valid&1)) {
        if(!vr_alignment_apply(aim_position,aim_orientation,rotation,offset)) valid&=~2u;
        return valid;
    }
    for(n=0.f,i=0;i<4;i++) n+=aim_orientation[i]*aim_orientation[i];
    n=sqrtf(n); for(i=0;i<4;i++) a[i]=aim_orientation[i]/n;
    for(n=0.f,i=0;i<4;i++) n+=grip_orientation[i]*grip_orientation[i];
    n=sqrtf(n); for(i=0;i<4;i++) g[i]=grip_orientation[i]/n;
    /* world = (a * rotation) * conjugate(a) */
    vr_alignment_multiply(a,rotation,aim_orientation);
    conjugate[0]=-a[0]; conjugate[1]=-a[1]; conjugate[2]=-a[2]; conjugate[3]=a[3];
    vr_alignment_multiply(aim_orientation,conjugate,world);
    /* one translation, g * (offset,0) * conjugate(g) */
    v[0]=offset[0]; v[1]=offset[1]; v[2]=offset[2]; v[3]=0.f;
    inverse[0]=-g[0]; inverse[1]=-g[1]; inverse[2]=-g[2]; inverse[3]=g[3];
    vr_alignment_multiply(g,v,t); vr_alignment_multiply(t,inverse,t);
    vr_alignment_multiply(world,g,grip_orientation);
    for(i=0;i<3;i++) { grip_position[i]+=t[i]; aim_position[i]+=t[i]; }
    return valid;
}
#endif
