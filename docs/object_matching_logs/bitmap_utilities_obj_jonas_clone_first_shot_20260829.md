# `bitmap_utilities.obj` `bitmap_clone` first-shot boundary (2026-08-29)

## Scope and authority

This ledger records the one frozen production candidate for
`source/bitmaps/bitmap_utilities.obj::_bitmap_clone`. The candidate was a
strict miss and was removed in full. Production source is restored to Git blob
`c2cac170f88adfaeaf55b7f04eebd4628c96f6e5`; no source or header change is
retained, and the object's nine inherited exact functions remain exact.

The lane began at canonical campaign commit
`34b5fc91b74538f0b28ea3d8b6b6a877dc5d760a`. Before editing, the complete
campaign playbook, matching methodology, object-ledger README, applicable tree
notes, authenticated Stian `CLAUDE.md`/`AGENTS.md`, and both existing
`bitmap_utilities.obj` ledgers were read. History, ledgers, and ref-reachable
source contained no earlier emitted candidate for `_bitmap_clone`.

January PC COFF is authoritative. Target object
`build/split/source/bitmaps/bitmap_utilities.obj` has raw SHA-256
`efb68953ba917a1a9e758d175b9778fefb638b43f31bdfece342dcbb02a0579e`.
The compiler was the unchanged XDK 3911 Microsoft C/C++ compiler invoked by
the normal production Ninja edge.

## Authenticated source evidence

HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` provides
`src/blam/bitmaps/bitmap_clone.c`, Git blob
`e872f358574631d455b7e749b6c30a5b5d5cbdd6`. It authenticates the natural
semantic shape: select the 2D, 3D, or cube-map allocator from the bitmap type;
if the allocation owns pixel storage, copy the complete mip payload and flags;
return the allocated bitmap. January disassembly independently fixes the PC
assertions, silent error path, exact named fields, call order, and 26-relocation
schedule.

The frozen candidate used readable C89, one parameter per line, typed named
fields, and a normal explicit return. It used no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, barrier, raw address or byte offset,
pointer/integer reconstruction, representation pun, undefined behavior, byte
forcing, compiler change, tool exception, or admission exception.

## Frozen one-shot result

Exactly one normal production object edge compiled the candidate. The raw
artifact was preserved as
`build/audit/bitmap_utilities_bitmap_clone_first_shot_20260829.obj`, SHA-256
`1aa335bc8bf13e4c6d966a45ba2eb4d2bb7c30016a3995c46de805601c335e6e`.

| Measure | January target | Frozen candidate |
| --- | ---: | ---: |
| Instruction extent | 397 | 384 |
| Padded COMDAT extent | 400 | 384 |
| Relocations | 26 | 26 |
| Normalized SHA-256 | `b3a3a787ae05d64c12e27dbdfbe6ec1031ecdbfb0e7c150b3d62f1ec2908d981` | `f89eb2c1e0cc91849a7c99a0c4728bdd13276d8c16bb82395d4d03e27ed1f7fc` |

The first byte difference is already at `+0x08`, before the first assertion.
January pushes `EDI` and clears it before testing `source_bitmap`; the natural
HCEA-derived spelling does not establish a persistent null clone at entry and
tests the argument before pushing `EDI`. That lifetime difference also changes
the unsupported-type epilogue: January returns the zeroed clone register after
the assert/error path, while the candidate forms a direct `NULL` return.

The allocation arms themselves have the same order and public calls, and the
candidate reaches the correct two mipmap-address calls, two pixel-size calls,
copy, and flags assignment. Nevertheless the entry lifetime shifts every
relocation by two bytes initially and the default-path cleanup makes the gap
five bytes by the allocator cluster. The equality assertion literal also does
not reproduce January's shorter source expression, so one of the 26 relocation
destinations differs even though the call schedule is otherwise link-closed.

Closing either boundary would require a second source spelling or local-lifetime
experiment. The one-shot rule forbids that without new independently
authenticated January PC source evidence. `_bitmap_clone` therefore receives
no credit and must not be retried from this evidence alone.

## Restoration and focused gate

The candidate body, declarations, type constants, and include were
inverse-removed. The source hashes to the committed blob named above. A normal
single-object restoration rebuild removed `_bitmap_clone`, and hardened COFF
comparison reconfirmed all nine inherited exact functions:

- `_bitmap_fill`;
- `_bitmap_alpha_to_rgb`;
- `_real_rgb_color_brightness`;
- `_rgb_color_to_hsv_color`;
- `_real_rgb_color_to_real_hsv_color`;
- `_argb_color_to_real_argb_color`;
- `_rgb_color_to_real_rgb_color`;
- `_pixel32_to_real_argb_color`;
- `_pixel32_to_real_rgb_color`.

The final campaign-wide gates are run after this ledger is integrated with the
other concurrent isolated lanes. Until new PC source evidence appears, this
function is a documented fixed point rather than an active candidate.
