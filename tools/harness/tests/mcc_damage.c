#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
typedef float real;
typedef int boolean;
typedef union { struct { real i,j,k; }; real n[3]; } real_vector3d;
#define FALSE 0
#define TRUE 1
#define _real_epsilon 0.0001f
#define TEST_FLAG(v,b) ((v) & (1u << (b)))
#define FLAG(b) (1u << (b))
#define UNSIGNED_LONG_MAX 0xffffffffu
#include "definition.inc"
static struct unit_datum { struct { int type; } object; struct { unsigned long flags; } unit; } entity, *object=&entity;
static struct { struct { real acceleration_scale; } object; } definition, *object_definition=&definition;
static struct { struct damage_definition damage; } effect, *damage_effect=&effect;
static struct { real_vector3d direction; real scale; } hit, *damage=&hit;
static int object_index, loaded=1, calls;
static real_vector3d received;
static int mcc_cache_tags_loaded(void) { return loaded; }
static void normalize3d(real_vector3d *v) {
    real n=(real)sqrt(v->i*v->i+v->j*v->j+v->k*v->k);
    v->i/=n; v->j/=n; v->k/=n;
}
static void scale_vector3d(real_vector3d const *v, real s, real_vector3d *out) {
    out->i=v->i*s; out->j=v->j*s; out->k=v->k*s;
}
static void capture(int index, real_vector3d const *v) { received=*v; ++calls; }
#define projectile_accelerate capture
#define biped_accelerate capture
#define vehicle_accelerate capture
#define item_accelerate(i,v,d) capture(i,v)
#include "under_test.inc"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"line %d: %s\n",__LINE__,#c); return 1; } } while(0)
int main(int argc, char **argv) {
    char const *mode=argv[1];
    real values[3]={0,-0.1f,2.45f};
    int legacy=!strcmp(mode,"legacy"), plain=!strcmp(mode,"unflagged");
    entity.object.type=_object_type_biped;
    definition.object.acceleration_scale=1.f;
    hit.direction.i=1.f;
    effect.damage.flags=0x30000u;
    if (legacy) loaded=0;
    if (plain) effect.damage.flags=0;
    if (!strcmp(mode,"immune")) entity.unit.flags=FLAG(_unit_impervious_bit);
    if (!strcmp(mode,"reverse")) hit.direction.i=-1.f;
    if (!strcmp(mode,"vector_only")) effect.damage.flags=0x10000u;
    if (!strcmp(mode,"negative_blocked")) { effect.damage.flags=0x10000u; values[2]=-2.45f; }
    if (!strcmp(mode,"negative")) values[2]=-2.45f;
    if (!strcmp(mode,"scalar")) { effect.damage.flags=0x20000u; values[0]=-2.f; }
    if (!strcmp(mode,"zero")) values[1]=values[2]=0;
    if (!strcmp(mode,"nan")) { unsigned long bits=0x7fc00000u; memcpy(values+2,&bits,4); }
    if (!strcmp(mode,"overflow")) { values[2]=FLT_MAX; definition.object.acceleration_scale=FLT_MAX; }
    if (!strcmp(mode,"vehicle")) entity.object.type=_object_type_vehicle;
    if (!strcmp(mode,"projectile")) entity.object.type=_object_type_projectile;
    if (!strcmp(mode,"item")) entity.object.type=_object_type_weapon;
    memcpy(&effect.damage.instantaneous_acceleration,values,sizeof(values));
    dispatch();
    if (legacy || plain || !strcmp(mode,"immune") || !strcmp(mode,"zero") ||
        !strcmp(mode,"nan") || !strcmp(mode,"overflow") || !strcmp(mode,"negative_blocked")) CHECK(calls==0);
    else {
        CHECK(calls==1);
        if (!strcmp(mode,"scalar")) CHECK(received.i<0 && received.k<0);
        else {
            CHECK(received.i==0);
            CHECK(fabs(received.j-(-0.1f/30.f))<0.000001f);
            CHECK(fabs(received.k-values[2]/30.f)<0.000001f);
        }
    }
    /* Unloading MCC restores the native positive scalar behavior immediately. */
    if (!strcmp(mode,"transition") || legacy || plain) {
        loaded=0; calls=0; effect.damage.instantaneous_acceleration=2.f;
        dispatch(); CHECK(calls==1 && received.i>0 && received.k>0);
    }
    return 0;
}
