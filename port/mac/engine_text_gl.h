#ifndef HALO_MAC_ENGINE_TEXT_GL_H
#define HALO_MAC_ENGINE_TEXT_GL_H
#include "engine_text.h"
/* Optional GL variant, compiled instead of engine_text.c. Caller owns a
   current legacy CGL context. Supply invalidation for a cached renderer;
   NULL is valid for a renderer that saves/restores actual GL state each draw.
   Keep that context current through atlas calls and mac_text_dispose().
   No window creation, draw submission or renderer integration is implied. */
int mac_text_gl_start(void (*invalidate)(void));
/* 0: not the atlas; 1: borrowed texture ready; -1: atlas upload failed. */
int mac_text_gl_resolve(unsigned long data,unsigned int *name,float scale[4]);
#endif
