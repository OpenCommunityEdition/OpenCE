#ifndef AUTHORED_EMITTER_LIGHTING_H
#define AUTHORED_EMITTER_LIGHTING_H

/* Exact object/light pairs; zero means the stock submission is untouched. */
int authored_emitter_lighting_enabled(void);
int authored_emitter_lighting_category_enabled(int category);
int authored_emitter_lighting_profile(long object, long definition, float *scale);
void authored_emitter_lighting_forget(void);

#endif
