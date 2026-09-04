# HUD Unit damage indicators exact reconstruction — 2026-09-04

## Scope and result

Canonical baseline: `4795da4089557f919fd9877b3c0a3b5f1e648931`.
This packet reconstructs the previously unwritten
`hud_render_damage_indicators` in `source/interface/hud_unit.c`. The focused
PID-isolated gate advances `hud_unit.obj` from 18 exact / 0 residual / 4
unwritten functions to **19 exact / 0 residual / 3 unwritten**. All 18 inherited
exact functions remain strict exact. This is partial-function progress, not a
whole-object completion or data-credit claim.

The January Xbox COFF remains the sole byte and ABI authority. The existing HUD
Unit ledgers, `docs/matching_methodology.md`, and
`docs/campaign_throughput_lessons.md` were read before implementation. No
Ninja/configure run, target regeneration, configuration edit, commit, or push
was made in this worker lane; the orchestrator owns the full-build and stable
regression admission.

## Authenticated source model

The later-build HCEA reconstruction used only as behavioral corroboration is:

`research-cache/halocea-full-blobs-20260830/src/blam/interface/hud_render_damage_indicators.c`

Its PC-only `hcex_damage_indicators_scale`, `hcex_damage_indicators_ofsx`, and
`hcex_damage_indicators_ofsy` adjustments are absent from January's relocation
stream and are deliberately absent from the retained Xbox source.

The type authority is:

`research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX.pdb`

read with:

`research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`

The PDB authenticates the complete semantic prefix required to reach
`hud_globals_definition.damage_indicators`; no opaque byte prefix was invented:

| Member | Offset | Size |
| --- | ---: | ---: |
| `messaging` | `0x000` | `0x120` |
| `waypoint` | `0x120` | `0x09C` |
| `multiplayer` | `0x1BC` | `0x104` |
| `defaults` | `0x2C0` | `0x050` |
| `damage_indicators` | `0x310` | `0x050` |

The nested structures use the PDB's real field names and types, including its
named `unused` arrays. Compile-time size and offset assertions cover every
prefix component, the `0x310` damage-indicator offset, the `0x28` bitmap
reference, and the `0x50` damage-definition size. The relevant damage fields
are `top_offset` at `+0`, `bottom_offset` at `+2`, `left_offset` at `+4`,
`right_offset` at `+6`, `indicator_bitmap.index` at `+0x34`,
`sequence_index` at `+0x38`, `multiplayer_sequence_index` at `+0x3A`, and
`color` at `+0x3C`.

`hud_globals` remains an external typed pointer; this translation unit does not
claim its storage. The concrete prefix view stops after the only block consumed
here rather than fabricating unneeded later members. The public function
prototype already lives in the owning `interface/hud_unit.h`. The scale helper
prototype now lives in its owning `interface/hud_draw.h` as
`real hud_globals_get_scale(boolean in_multiplayer)`; no consumer-local
prototype was added.

No PDB enum name was available for the four directions. The retained local
semantic enum is therefore an explicitly bounded inference from January's jump
table: top, left, bottom, and right are values zero through three. Every switch
arm uses those constants rather than integer case labels.

## Retained behavior

The natural C implementation follows the January control flow and ordered
relocations:

- a `NONE` local-player index exits immediately;
- the existing player accessor supplies the unit datum index, and the typed
  `unit_try_and_get` macro validates that the object is a unit;
- a missing unit clears the player's damage indicators;
- split-screen state is passed as a `boolean` to `hud_globals_get_scale`;
- the four byte timers are rendered only in the open interval `(0, 30)` using
  the project `TICKS_PER_SECOND` constant;
- a `real_point2d` holds the viewport-relative position, following the same
  established project idiom used by HUD navigation rendering;
- edge offsets come from the authenticated definition; midpoint expressions
  use the natural low-bound-plus-high-bound order; rotations use `_pi`,
  `_pi / 2`, zero, and `3 * _pi / 2`;
- the bitmap and clip are retrieved through the existing HUD interface. The
  bitmap output is intentionally mutable because the texture cache may create
  its hardware format;
- the hardware format check uses the real cache declaration, and the bitmap is
  drawn through `hud_draw_bitmap_direct` with `_hud_anchor_center`, the selected
  SP/MP sequence, scale, rotation, color, and `FALSE` interface-bitmap flag.

The initially unwritten reconstruction was corrected only through ordinary,
source-credible forms. The target's first player-to-unit control-flow shape is
expressed with a semantic `unit_index`; the PDB-backed `real_point2d` restored
the genuine contiguous coordinate layout; and commutative midpoint operands
were written in conventional lower/upper order. No register, volatile,
noinline/force-inline, pragma, assembly, optimizer barrier, dummy branch,
self-assignment, fake dependency, lifetime trick, raw address, representation
pun, or undefined behavior was used.

## Strict COFF evidence

| Measure | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful extent | 685 | 685 |
| Padded COMDAT extent | 688 | 688 |
| Ordered relocations | 39 | 39 |
| Normalized SHA-256 | `cf7cb8af78f4f4f9fb2a11c7bb67c2e5fb0e31cedcbe8b062ef7aa7fad4cacd0` | `cf7cb8af78f4f4f9fb2a11c7bb67c2e5fb0e31cedcbe8b062ef7aa7fad4cacd0` |

All relocation addresses, types, symbolic identities, addends, and internal
switch destinations match. The retained focused object is
`scratch/hud-unit-damage-final-candidate-20260904.obj`, raw SHA-256
`7d694e17f8b5fa0390661d6dc6bef3c1b03e93ff8ad2c9424a4c0298edc9d072`.
The untouched baseline object is
`scratch/hud-unit-damage-baseline-20260904.obj`, raw SHA-256
`bbc8ca07e1ae3502380028ab87a4705fa47ed80e1d7f7db92bbdc5754902448c`.
The frozen source SHA-256 is
`8566b10e06a57443e7887e30ab6a4db00f40f7f716cb025d87dc383191d92496`.

The complete gate command was:

```text
python tools/campaign/gate.py source/interface/hud_unit --all --out scratch/hud-unit-damage-final-candidate-20260904.obj --forbid-emitted-symbol _point_from_line3d
```

It reports the emitted-symbol guard passed, 19 exact, zero residual, and three
unwritten functions. `tools/fake_match_scan.py` reports zero review leads for
`hud_unit.c` and `hud_unit.h`, and `git diff --check` is clean.

## Runtime ownership boundary

The new function emits no candidate-only code function, COMMON, `.data`, or
`.bss` owner. Its internal switch labels are ordinary offsets inside the exact
688-byte function. The only newly materialized non-code COMDAT is the natural
15-byte compiler-pooled assertion literal `!"unreachable"\0`, raw SHA-256
`31427496dd4b18d19bc04a0368edef7c9e4e18680ad09373c9ecc9baaa3e6a31`.
January's exact function relocates to the identical decorated literal symbol;
csplit leaves that definition outside this split object. It is required source
output and receives no separate data credit.

All new external calls are obtained from their owning headers:
`effects/player_effects.h`, `cache/texture_cache.h`, `render/render.h`,
`interface/hud_definitions.h`, and the existing HUD/player/unit interfaces.
No point/line helper or other incidental math COMDAT is emitted. The remaining
three unwritten HUD Unit functions and the target's unmatched non-code records
remain outside this packet.

## Integration requirements

The orchestrator must regenerate the canonical object, run the full Ninja
build, compare the stable per-function snapshot, validate all active parks,
run the tool tests and source-policy scans, and inspect the rebuilt runtime
owner set before admission. Until those gates pass, this ledger records an
isolated strict candidate rather than published canonical progress.
