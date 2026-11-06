# Lua

Lua 5.4.7, copied unchanged from <https://www.lua.org/ftp/lua-5.4.7.tar.gz> (its
`src`, without `lua.c` and `luac.c`). MIT licence (`lua.h` holds it).

SHA-256 of the archive: `9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30`

Used by `port/linux/game/mutator_scripts.c`: the scripts a mutator names
(`docs/mutators.md`). The builds compile only what those scripts have: not the
`io`, `os`, `debug`, `package` or `coroutine` libraries.
