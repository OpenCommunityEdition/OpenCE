#ifndef AMMO_COUNTER_FALLOFF_H
#define AMMO_COUNTER_FALLOFF_H

float ammo_counter_falloff_factor(const float axis[3], float x, float y, float z);
float needler_falloff_factor(float height);
float plasma_prong_falloff_factor(const float axis[3], float x, float y, float z);
void *ammo_counter_falloff_texture(long rasterizer, int radial, float transform[4]);
void ammo_counter_falloff_window(void);
void ammo_counter_falloff_dispose(void);

#endif
