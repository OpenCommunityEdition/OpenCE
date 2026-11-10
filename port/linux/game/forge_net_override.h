/*
 * OpenCE Forge - Dynamic Network Version Override Header
 * Included via -include on network_client_manager.c, network_server_message_handler.c, and p2p_lobby.c.
 * Routes HALO_PORT_NETWORK_VERSION through forge_get_network_version():
 *   - Standard Multiplayer menu -> OpenCE's own HALO_PORT_NETWORK_VERSION (standard OpenCE servers/clients)
 *   - Forge Multiplayer menu    -> FORGE_NETWORK_VERSION (Forge-only servers/clients)
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
#define FORGE_NETWORK_VERSION 0x4629
#define FORGE_GAME_KIND 0x46
#define HALO_PORT_NETWORK_VERSION (forge_get_network_version())
#define game_modes_kind forge_game_modes_kind

#endif /* FORGE_NET_OVERRIDE_H */
