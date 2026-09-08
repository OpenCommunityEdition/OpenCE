# `bink_playback.obj` Fable graph reconciliation

## Result

This packet reconciles the Bink playback reconstruction from preserved donor
`C:\halo-worktrees\fable-50k-resume-20260906` at `740bbc33aeaa7c2c6ee86304b7a52279df44a930`
into canonical base `f7afb7047`. The original intake changed only
`source/bink/bink_playback.c` and this ledger; the final integrated packet also
applies the independently audited semantic symbol configuration.

The intake read the donor's `fable_50k_resume_lane_20260906.md`, all three
prior Bink object ledgers, and the preserved `scratch/facts`, `scratch/hdr`,
`scratch/reports`, and row snapshots for this unit. January split COFF remains
the byte and relocation authority; Stian's preserved PC reconstruction is
used only for control-flow and semantic corroboration.

The pre-edit canonical gate was **9 exact / 0 residual / 15 unwritten**. The
same reconciled source, measured against the donor's semantic code and BSS
symbols, is **19 exact / 5 residual / 0 unwritten**. It preserves all nine
inherited exact bodies and adds ten strict owners totaling 1,808 padded bytes.

At intake time, before the semantic symbol packet was applied, the source-only
intermediate gate read **5 exact / 5 residual / 14 unwritten**: four inherited
callers became relocation-identity residuals and semantic private definitions
were not paired with their address-named target owners. That was an
intermediate naming artifact, not a regression. The integrated symbol packet
below restores the nine inherited owners and produces the final 19/5/0 result.

## Exact owners after the symbol packet

| Owner | Padded bytes |
| --- | ---: |
| `_D3DTexture_UnlockRect@8` | 16 |
| `_IDirect3DBaseTexture8_Register@8` | 16 |
| `_IDirect3DTexture8_LockRect@20` | 32 |
| `_IDirect3DTexture8_UnlockRect@8` | 16 |
| `_bink_alloc_permanent` | 240 |
| `_bink_decompress_video_frame` | 112 |
| `_bink_free@4` | 192 |
| `_bink_get_memory_available` | 64 |
| `_is_all_bink_memory_free` | 48 |
| `_bink_playback_start` | 1,072 |
| `_bink_playback_active` | 32 |
| `_bink_playback_dispose` | 48 |
| `_bink_playback_in_progress` | 16 |
| `_bink_playback_initialize` | 48 |
| `_bink_playback_render` | 96 |
| `_bink_playback_stop` | 112 |
| `_bink_playback_ui_rendering_inhibited` | 32 |
| `_bink_playback_update` | 16 |
| `_code_001b5850` | 16 |

The last one-byte return body remains under its inherited address-derived
external name. The prior exhaustive ledger found no credible original or
semantic identity for it, and this pass does not replace evidence with a
guess. It remains exact and is not called by the reconstructed graph.

## Honest non-exact frontier

The five retained residuals are ordinary, coherent C and receive zero exact
credit:

| Owner | Target / candidate padded bytes | Difference |
| --- | ---: | --- |
| `_bink_alloc@4` | 496 / 512 | VC7 inlines the first memory-free predicate; January has 52 relocations and canonical has 51 |
| `_bink_free_texture_cache` | 96 / 144 | VC7 inlines the predicate used by the diagnostic assertion; January has 11 relocations and canonical has 12 |
| `_bink_playback_idle` | 160 / 176 | the authentic `allow_skipping` boolean spills instead of remaining in `al` |
| `_bink_query_analog_controller_buttons` | 80 / 96 | the boolean/index register allocation differs |
| `_bink_draw_frame` | 912 / 912 | 59 / 61 relocations; screen-bound loads and the stack frame schedule differ |

No manual inline/noinline directive, local clone, volatile carrier, fake
dependency, optimizer pragma, or flag experiment is used to force these rows.
The target-static `_bink_draw_frame` is now a coherent typed reconstruction of
the Xbox texture quad, debug timing text, periodic Bink summary collection,
and skipped-frame/blit diagnostics. Its candidate normalized SHA-256 is
`54104cacd763706fac4823bab62bd97ad1a20ecfb838239ad87e3c9ea3cb48d9`;
January's is
`24aa7c5023629b6b863f6028dc0f1d55396a2083472edfd6e45e48866e726910`.
It is parked at 79.409256% and receives zero exact credit.

## Reconstructed graph and source quality

The packet restores the complete start/allocation/decode/idle path available
from the donor: memory-pool selection and guarding, DirectSound setup, Bink
open, frame-buffer allocation and protection, the hand-built Xbox texture and
bitmap, event suppression, initial decode, state restoration, per-frame wait,
skip input, frame presentation and diagnostics, end-of-movie stop, and
texture-cache release.

The Bink SDK owner header is not vendored. The TU-local `HBINK`, summary and
realtime ABI layouts plus RAD SDK prototypes are therefore retained as the
explicitly permitted fallback.
Every other dependency comes from its real subsystem header. In particular,
`attract_mode_reset_timer` now comes from `interface/attract_mode.h`; the
donor's unrelated local extern was removed.

Function declarations use one parameter per line, no-argument lists put
`void` on its own line, and every reconstructed definition has an explicit
return. The code uses project `boolean`, `byte`, `pixel32`, bitmap, rasterizer,
input and Xbox types. Original file/line diagnostics use `match_assert`,
`match_dassert`, and `match_vassert` with invariant-true polarity and lazy
formatted diagnostics. The file contains no raw-address access, new BSS/data
definition, address-derived private/global identifier, representation pun,
assembly, opcode replay, or synthetic object-byte forcing.

## Integrated `config/symbols.json` packet

Preserve record order and regenerate the split target after these changes.
All code rows have `flags: 32`; all renamed private/XDK rows below add
`"static": true`.

| File offset | Old name | New name |
| ---: | --- | --- |
| 1790960 | `_code_001b53f0` | `_bink_get_memory_available` |
| 1791104 | `_code_001b5480` | `_bink_alloc_permanent` |
| 1791344 | `_code_001b5570` | `_is_all_bink_memory_free` |
| 1791392 | `_code_001b55a0` | `_bink_alloc@4` |
| 1791888 | `_code_001b5790` | `_bink_free@4` |
| 1792096 | `_code_001b5860` | `_bink_draw_frame` |
| 1793008 | `_code_001b5bf0` | `_IDirect3DBaseTexture8_Register@8` |
| 1793024 | `_code_001b5c00` | `_bink_query_analog_controller_buttons` |
| 1793104 | `_code_001b5c50` | `_D3DTexture_UnlockRect@8` |
| 1793120 | `_code_001b5c60` | `_IDirect3DTexture8_LockRect@20` |
| 1793152 | `_code_001b5c80` | `_IDirect3DTexture8_UnlockRect@8` |
| 1793216 | `_code_001b5cc0` | `_bink_free_texture_cache` |
| 1793312 | `_code_001b5d20` | `_bink_decompress_video_frame` |
| 1793584 | `_code_001b5e30` | `_bink_playback_idle` |

Rename the data row at file offset 3238204 from `_data_0031693c` to
`_bink_saved_state` (flags remain 0).

Split the BSS owner as follows, all with `flags: 0`:

| File offset | Action | Name and typed source view |
| ---: | --- | --- |
| 5057264 | rename `_bss_004d2af0` | `_bink_pointer_blocks`: `void *[16]` |
| 5057328 | insert | `_bink_bitmap`: `struct bitmap_data` |
| 5057376 | insert | `_bink_texture`: `D3DBaseTexture` |
| 5057616 | insert after `_bink_globals` | `_bink_pointer_block_count`: `long` |

The BSS split is relocation-backed: the table occupies +0x00..+0x3f, the
bitmap starts at +0x40, the texture starts at +0x70, `_debug_bink` is at
+0x84, and `_bink_globals` starts at +0x88. Its independently exact clear size
is 0xD8, ending at +0x160; the allocation count starts there and therefore is
not part of that structure. The following sound BSS begins at +0x168.

## Verification

- Canonical pre-edit gate: 9 exact / 0 residual / 15 unwritten.
- Intake-time source-only gate before the symbol packet: 5 exact / 5 residual /
  14 unwritten.
- Final canonical gate: 19 exact / 5 residual / 0 unwritten; emitted-symbol
  guard passed. `_bink_draw_frame` remains a measured zero-credit residual.
- `tools/fake_match_scan.py --fail-on-findings`: zero review leads.
- `git diff --check` for the touched source: clean.
- The orchestrator's full Ninja build, global stable sweep, park validation,
  and tracked-tools suite cover the integrated packet before publication.
