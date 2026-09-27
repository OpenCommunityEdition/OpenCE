# tomlc17

A TOML v1.1 parser in C99, by CK Tan, MIT licensed (see `LICENSE`).

Upstream: https://github.com/cktan/tomlc17, release `R260821`
(commit e0e8868546b4611a86fcdb284819e60866a3480b). `tomlc17.c` and
`tomlc17.h` are copied unchanged from its `src/` directory.

The native ports read their settings file, `config.toml`, with it
(`port/linux/src/port_config.c`); it builds into the Linux, Windows and
Android platform layers and the Android host.
