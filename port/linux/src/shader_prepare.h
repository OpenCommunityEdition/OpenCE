/* Optional map-scoped preparation of previously observed shader pairs. */
#ifndef SHADER_PREPARE_H
#define SHADER_PREPARE_H

typedef unsigned long long shader_prepare_hash;
typedef unsigned int (*shader_prepare_load_fn)(shader_prepare_hash, shader_prepare_hash);
typedef void (*shader_prepare_store_fn)(unsigned int, shader_prepare_hash, shader_prepare_hash);

void shader_prepare_begin(char const *map, shader_prepare_load_fn load, shader_prepare_store_fn store);
void shader_prepare_end(void);
void shader_prepare_record(unsigned int vertex, unsigned int fragment,
    shader_prepare_hash vertex_hash, shader_prepare_hash fragment_hash);
unsigned int shader_prepare_take(shader_prepare_hash vertex, shader_prepare_hash fragment);

#endif
