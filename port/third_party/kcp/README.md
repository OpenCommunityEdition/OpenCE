# KCP

A reliable ARQ protocol over unreliable datagrams, in C, by Lin Wei
(skywind3000), MIT licensed (see `LICENSE`).

Upstream: https://github.com/skywind3000/kcp, commit
b1a7a2101dcbb96017681a500d6b82bbe5a88766 (after release 2.1.1).
`ikcp.c`, `ikcp.h` and `LICENSE` are copied unchanged.

Internet play carries the game's TCP connections over its UDP tunnel with
it (`port/linux/src/p2p.c`); it builds into the Linux, Windows and Android
platform layers.
