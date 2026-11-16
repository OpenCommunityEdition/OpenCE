/*
FORGE_NET_OVERRIDE.H

Declarations for Forge multiplayer session overrides and menu hooks.
*/

#ifndef FORGE_NET_OVERRIDE_H
#define FORGE_NET_OVERRIDE_H

#include "halo_port_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

struct game_variant;
struct game_variant_options;
extern long forge_get_network_version(void);
extern unsigned char forge_game_modes_kind(struct game_variant const *variant, struct game_variant_options const *options);

#ifdef __cplusplus
}
#endif

enum { HALO_PORT_NETWORK_VERSION_STOCK = HALO_PORT_NETWORK_VERSION };
#undef HALO_PORT_NETWORK_VERSION
#define FORGE_NETWORK_VERSION 0x4634
#define FORGE_GAME_KIND 0x46
#define HALO_PORT_NETWORK_VERSION (forge_get_network_version())
#define game_modes_kind forge_game_modes_kind

#endif /* FORGE_NET_OVERRIDE_H */
