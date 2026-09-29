# miniupnpc

The MiniUPnP project's UPnP IGD client library in C, by Thomas Bernard,
licensed under the BSD 3-Clause License (see `LICENSE`).

Upstream: https://github.com/miniupnp/miniupnp (https://miniupnp.tuxfamily.org),
release `miniupnpc-2.3.3`, `miniupnpc-2.3.3.tar.gz`, SHA-256
`d52a0afa614ad6c088cc9ddff1ae7d29c8c595ac5fdd321170a05f41e634bd1a`
(its sources match the repository's `miniupnpc_2_3_3` tag but for their
`$Id$` lines).

Only the library's sources in `src/` (not its test and command-line
programs), `include/` and `LICENSE` are kept, without their build files,
and nothing in them is changed. `src/miniupnpcstrings.h`, which the
library's build generates from `miniupnpcstrings.h.in`, is written here.

Internet play (`port/linux/src/p2p.c`, through `port/linux/src/posix_upnp.c`)
uses it to ask the router to forward the tunnel's UDP port
(`network.allow_upnp` in config.toml), for players whose NATs are too
strict for hole punching.
