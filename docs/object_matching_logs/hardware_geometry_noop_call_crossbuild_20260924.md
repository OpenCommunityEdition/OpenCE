# Hardware-geometry no-op call: cross-build limit (2026-09-24)

Research-only worktree, based on canonical `f6a35db0`. No production source,
configuration, object status, or byte credit changed. The held Lane R2
`D3DVertexBuffer_MoveResourceMemory` candidate remains untouched.

## New primary input found

The 2001-09-25 `cachebeta.xbe` was present locally at
`C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/astra-inputs/earlier-map-archives/2001-09-25/cachebeta.xbe`
(SHA-256 `7eacac85f93a4630b52142fe5f458acc017c081720b75919c9aaad41b6119dd6`).
That hash matches the first-party map-atlas record. The earlier Lane R2 ledger
said the binary was unavailable; only its linker map had been used. The paired
`cache.exe` is also present (SHA-256
`6455066470f4f823442c427cbd45956e0b6b43919057e3bb65a2bd1cf0fd5149`).

Parsing the XBE section headers and `.text` directly locates two successive
`ret 8` wrappers at XBE virtual addresses `0x167100` and `0x167110` (raw file
offsets `0x157100` and `0x157110`). The first-party map names these, in that
same object and order, `_D3DResource_MoveResourceMemory@8` and
`_D3DVertexBuffer_MoveResourceMemory@8`. The independently mapped `cache.exe`
has the same named pair at `0x504f10`/`0x504f20`, also pure `ret 8`. The
2001-08-15 map also lists the same pair in the same object, and the supplied
`1749betaP.xbe` (SHA-256
`6c3ecb127d91ee5ede8039233bc6e00ad6de9e3f761041f65f3d0856ddb272f4`)
contains a unique corresponding `ret 8` pair at `0x14cc80`/`0x14cc90`.
The August filename carries the existing patched-image caveat; September
alone suffices to authenticate the name.

## Cross-reference result

Across the full executable `.text` of each XBE and the September PE, a byte
scan of every `E8`/`E9` rel32 target found **zero** branches to either wrapper.
The little-endian absolute addresses occur nowhere in those complete image
files either. This is consistent with the XDK's
`static __forceinline`/empty `D3DResource_MoveResourceMemory` definition:
the source can mention the forwarder, causing a copy to be emitted, while its
call and both arguments leave no executable trace. The September executable
therefore does **not** reveal the caller function, call position, resource
expression, or `D3DMEM_AGP` versus `D3DMEM_VIDEO` argument. The 2001 maps
contain symbol names, not source line records; the archived original
`rasterizer_xbox_hardware_geometry.c` was not present locally.

The first-party name is no longer in doubt, but the proposed call after the
vertex-buffer Unlock with `D3DMEM_VIDEO` remains a byte-inert hypothesis.
Do not admit it on these findings. Reopen on an original source/listing, a
matching debug PDB with line records, or another first-party build whose
`MoveResourceMemory` implementation leaves a real call/argument trace.
