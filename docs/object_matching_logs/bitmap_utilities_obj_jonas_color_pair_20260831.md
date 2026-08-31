# `bitmap_utilities.obj` authenticated color pair (2026-08-31)

Two small, typed color helpers from Claude's unwritten-function wave were
manually admitted:

| Owner | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_valid_real_rgb_color` | 192 | 6 | `1c7421e6f8f014a28511488265c5bc03678fd4c240adf67d44b8dbf5d5e1acbc` |
| `_rgb_colors_interpolate_and_scale` | 352 | 21 | `881e3268ec4ccc243e399e0db02619771739f9f6bac7a03ec5391453194a1707` |

Both functions are strict exact, including relocation identities, and advance
the object from 15 to 17 exact owners.  The scale helper uses real color types,
the existing interpolation API, a semantic alpha blend, and the authentic
assertion strings.  It introduces no address-named helper, rdata-address
global, assembly, or codegen-only local.

`valid_real_rgb_color` is now declared in the owning `bitmaps.h`.
`particles.c` includes that header instead of carrying a foreign prototype.
All nine direct bitmap-header consumers and the complete tree rebuilt with no
exact-function regression.  The fake-match scanner's four `particles.c` leads
are pre-existing naked assembly bodies and are unrelated to this include-only
cleanup.

The broader bitmap clone/fade/interpolation candidates remain excluded under
their frozen first-shot ledgers: Claude's closures are lifetime/x87 retuning
without new independent January-PC source evidence.
