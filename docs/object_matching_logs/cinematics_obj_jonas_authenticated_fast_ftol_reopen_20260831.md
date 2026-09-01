# `cinematics.obj` authenticated `fast_ftol` reopen

Date: 2026-08-31

## Result

`source/cutscene/cinematics.obj` advances from 15 exact / zero residual / two
unwritten target functions to 16 exact / one residual / zero unwritten.

- `_cinematic_set_title_delayed` is strict exact at 144 padded bytes, seven
  relocations, and normalized SHA-256
  `2efe5dea0cc04b8a14adb1524c901f2709dc4dafb36e8a5c4f4518fef287a252`.
- `_cinematic_render` is retained as a measured credible fuzzy reconstruction:
  target and candidate are both 1,280 padded bytes with the same 57 relocation
  identities and addresses. The candidate SHA-256 is
  `1eb7cfd27d9ce0693adee50153f2c344766443274dcf486917756d03eb2bf7ac`
  versus January
  `a89dcee38e6a239615cde9f2d1f3b2fc577ced717e620de08a6397f74a0e6288`.
  Objdiff reports 99.32809%.
- All 15 inherited exact functions remain strict exact. The four target-owned
  string sections and `0.125f` remain strict exact. The inherited source form
  of `_cinematic_globals` remains an external four-byte COMMON symbol while
  csplit represents January's linked owner as a four-byte `.bss` section; this
  wave neither changes that boundary nor claims strict BSS ownership.

The translation unit is deliberately not marked complete. Rebuilding it also
emits a candidate-only `_fast_ftol` COMDAT: 32 padded bytes, zero relocations,
SHA-256
`8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.
January selects that helper from `actor_combat.obj`, not `cinematics.obj`.
Target-owned function progress is real, but the extra owner must be reconciled
before whole-object admission.

The complete runtime owner census is 17 target code owners versus 18 candidate
code owners. All 17 target owners are present: 16 are strict exact and only
`_cinematic_render` is residual. The sole candidate-only code owner is
`_fast_ftol` above. Five target RDATA owners are strict exact (the four strings
and `__real@3e000000`). Six additional candidate literal COMDATs total 28 bytes:

| Candidate literal | Bytes | January natural owner |
| --- | ---: | --- |
| `__real@41f00000` | 4 | `source/ai/action_alert.obj` |
| `__real@3f4ccccd` | 4 | `source/ai/action_charge.obj` |
| `__real@3f1a36e2e0000000` | 8 | `source/ai/action_charge.obj` |
| `__real@3f800000` | 4 | `source/ai/action_charge.obj` |
| `__real@3d088889` | 4 | `source/ai/actions.obj` |
| `__real@00000000` | 4 | `source/ai/action_charge.obj` |

Each literal has the same payload, alignment flags, zero-relocation shape, and
selection 2 (`IMAGE_COMDAT_SELECT_ANY`) as that independently selected January
owner. They are normal linker-discardable support for the reconstructed bodies
and receive no cinematics data credit. `_fast_ftol` is not folded into that
exception: although its 32 payload bytes equal the actor-combat owner, the
candidate emits selection 2 while January's actor-combat owner is selection 1
(`IMAGE_COMDAT_SELECT_NODUPLICATES`). That code-owner mismatch remains the
explicit whole-object blocker.

## Source and name evidence

The January map/PDB already authenticates the two public function names and
signatures. The local HCEA reconstruction independently supplies the coherent
control flow:

- scan the four title slots for the first free entry;
- convert the requested delay to a negative tick count;
- report the scenario chapter-title name when the queue is full;
- interpolate the letterbox fraction from elapsed game ticks;
- draw two black bars at one eighth of the viewport height;
- validate the scenario text index, choose authored or default bounds, fade the
  title, set draw and shadow colors, draw the localized string, and retire it.

The January-specific reconciliation deliberately keeps the authored title
bounds unscaled, uses the 0x60-byte chapter-title layout, initializes fade before
the fallback bounds check, derives shadow alpha directly from the title field,
and adds zero to title time while paused. Those details agree with the target
instruction and relocation topology rather than blindly copying the later HCEA
viewport-scaling path.

The recovered `cinematic_global_data` fields are named `letterbox_amount` and
`letterbox_last_game_time`, replacing the former layout-only placeholders.
Public declarations live in `cutscene/cinematics.h`. The bitmap conversion and
rasterizer text declarations live in their genuine owner headers. The chapter
title and narrow HUD views remain TU-private because no shared complete HUD type
is yet reconciled and moving those definitions into widely included headers is
known to perturb VC7 allocation in unrelated objects.

The Bungie Marathon 2/Infinity source release and current Aleph One source were
checked as style/provenance references. No directly reusable chapter-title or
Xbox x87 implementation surfaced; Halo's January binary and HCEA topology remain
the controlling evidence.

## Why `fast_ftol` is admissible here

The shared `cseries.h` helper is the authenticated original-era math primitive:
an explicitly inline `fld` / dword `fistp` conversion. January's standalone
`_fast_ftol` body, the HCEX metadata, historical Microsoft source, and the
campaign's earlier positive-control experiments all agree on that mechanism.
The current house rule permits sparse inline assembly in helper/math code. No
assembly is copied into either cinematic function, no opcode bytes are emitted,
and no unrelated compiler-control construct is used.

The helper naturally closes `_cinematic_set_title_delayed`. Ordinary casts call
`__ftol2`; `/QIfist` emits a qword `fistp`, so neither is behaviorally or
structurally equivalent.

## Render boundary

The retained render body is the previously measured F01 source boundary. It
spells the tick-to-seconds expression at the two branch uses, allowing VC7 to
emit January's compact false-path `fsubr; fistp; reload` sequence. That removes
the former size and relocation drift without changing runtime semantics.

After relocation normalization, 46 of 1,280 padded bytes differ. The surviving
differences are allocator/scheduler choices:

- January stages elapsed ticks at `[ebp-4]`; the candidate uses `[ebp-8]`.
- January assigns the bar height, viewport height, helper float, and helper
  result to `-4`, `-8`, `-10`, and `-0C`; VC7 rotates the same live values.
- the final shadow alpha has the same three `PIN` evaluations but a different
  color-pack register schedule.

The preserved historical-helper, slot-topology, false-subtraction, shadow-pack,
declaration-order, scope, aggregate, and HCEA-layout matrices exhaust the obvious
natural source controls. Exact-only barriers, volatile lifetimes, fake aliases,
representation punning, direct gameplay assembly, and nonsensical branch
shapes were rejected. The residual is parked until authoritative local-variable
records or a natural same-compiler donor explains the remaining allocation.

The retained packed-color shift/mask is the defined unsigned spelling from the
preserved F01 boundary. The XDK 3911 `RGBA_GETALPHA` / `RGBA_SETALPHA` topology
independently authenticates the operation, but those legacy DX7 macros are not
exposed by this Xbox translation unit's current include closure: an unresolved
macro spelling becomes a false external call, while directly including
`D3DTypes.h` is rejected by the DX8/Xbox headers. A duplicate TU-local macro or
broad SDK-header change was therefore not retained merely to alter scheduling.

## Header and ownership regression audit

Every direct C includer of the changed `bitmaps.h`, `cinematics.h`, and
`rasterizer.h` was compiled twice under the campaign XDK flags, once against the
canonical headers and once against this candidate. All target-function scores
and all common strict section identities were unchanged outside cinematics.
Notable protected results include:

- `source/units/units`: 189 exact / zero residual / zero unwritten in both runs;
- `source/bitmaps/bitmap_utilities`: 17 / 0 / 31 in both runs;
- `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps`: 18 / 1 / 0 in both;
- `source/objects/objects`: 121 / 0 / 0 in both;
- `source/ai/ai_debug`: 56 / 4 / 0 in both.

The remaining direct consumers (`bitmaps`, `bitmaps_quantitize`, `tiff_file`,
`particles`, `breakable_surfaces`, `object_types`, `decals`, `light_volumes`,
`main`, `render_contrails`, `rasterizer_xbox_active_camouflage`,
`rasterizer_xbox_models`, and the unchanged HUD consumers) likewise retained
their baseline counts and common function bytes.

## Reopen criteria

Reopen `_cinematic_render` only for authoritative January source/local-variable
records or a natural same-compiler donor that explains the stack-slot and shadow
pack schedule. Do not repeat the exhausted declaration/scope/barrier families.

Reopen whole-object admission separately when the ordinary linker/COMDAT schedule
can be proven to select the actor-combat helper without leaving a candidate-only
cinematics owner. Do not hide, rename, or suppress the surplus merely to make an
object-level count green.
