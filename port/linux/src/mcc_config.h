/* MCC discovery defaults for configs written before discovery was enabled. */
#ifndef MCC_CONFIG_H
#define MCC_CONFIG_H

#include "tomlc17.h"

/* Upgrade the three saved booleans in place, preserving length and comments.
The caller supplies the successfully parsed text and records completion by
adding game.mcc_discovery_defaults with the other missing settings. */
int mcc_config_upgrade(char *text, toml_datum_t table);

#endif
