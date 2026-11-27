#ifndef MCC_ENVIRONMENT_SHADER_H
#define MCC_ENVIRONMENT_SHADER_H

/* Explicit draw scope, separate from the emulated Xbox render states. */
void mcc_environment_shader_begin(unsigned short flags);
void mcc_environment_shader_end(void);
void mcc_environment_shader_set(int enabled);
int mcc_environment_shader_active(void);
struct xgpu_text;
void mcc_environment_shader_emit(struct xgpu_text *text);
#endif
