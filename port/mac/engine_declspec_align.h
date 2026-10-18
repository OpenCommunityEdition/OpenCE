#ifndef HALO_MAC_ENGINE_DECLSPEC_ALIGN_H
#define HALO_MAC_ENGINE_DECLSPEC_ALIGN_H
/* Scoped to actor_type_grunt.c. Preserve its explicit four-byte alignment
   using GCC 4.0 syntax; do not erase unsupported declaration attributes. */
#define align(bytes) aligned(bytes)
#define __declspec(attribute) __attribute__((attribute))
#endif
