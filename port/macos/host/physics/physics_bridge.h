/*
 * physics_bridge.h - the guest's physics batches (port/macos/include/
 * host_physics.h), checked and carried out on a physics world (physics.h).
 *
 * The bridge reads guest memory only through the pointer function it is
 * given, which turns a guest range into a host pointer or refuses it, so
 * that the tests can give it memory of their own. Everything the guest
 * sends is copied before it is checked, and checked before it is used.
 */

#ifndef OPENCE_PHYSICS_BRIDGE_H
#define OPENCE_PHYSICS_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct physics_bridge physics_bridge;

typedef struct physics_bridge_memory {
    /* [address, address + size) as a host pointer; NULL if any of it is
       not guest memory */
    void *(*pointer)(void *context, uint32_t address, uint32_t size);
    void *context;
} physics_bridge_memory;

/* a command refused: its byte in the batch, its op, and why */
typedef void (*physics_bridge_refused)(void *context, uint32_t offset, uint32_t op, const char *reason);

physics_bridge *physics_bridge_create(void);
void physics_bridge_destroy(physics_bridge *bridge);

/* Carries out the size bytes of batch at guest address batch, steps the
   world, and writes the results into the results_size bytes at guest
   address results. The number of commands refused, or -1 for a batch
   refused whole (its header, or a range that is not guest memory), with the
   reason in reason. */
int physics_bridge_step(physics_bridge *bridge, const physics_bridge_memory *memory, uint32_t batch, uint32_t size,
                        uint32_t results, uint32_t results_size, physics_bridge_refused refused, void *context,
                        char *reason, uint32_t reason_size);

/* what the bridge holds */
uint32_t physics_bridge_ragdolls(const physics_bridge *bridge);
uint32_t physics_bridge_bodies(const physics_bridge *bridge);
int physics_bridge_has_level(const physics_bridge *bridge);

#ifdef __cplusplus
}
#endif

#endif
