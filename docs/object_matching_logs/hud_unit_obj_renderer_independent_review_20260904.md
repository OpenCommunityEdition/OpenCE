# HUD Unit renderer independent source review, 2026-09-04

## Recommendation

Retain the exact `initialize_hud_state` helper and the corrected, source-credible
`hud_render_unit_interface` as an **honest fuzzy reconstruction**, subject to
root's final build, stable sweep, and measured-park admission. Do not claim the
renderer exact or HUD Unit whole-object Matching.

The final candidate preserves all 20 inherited exact functions and supplies the
real caller that naturally keeps the new private initializer live. The
initializer is 51 meaningful / 64 padded exact bytes. The renderer is 3,460
meaningful / 3,472 padded candidate bytes versus January's 3,493 / 3,504, with
102 relocations on each side but different normalized content. Its existing
`cseries.h` math helper also emits an extra candidate-only `_fast_ftol` COMDAT.
That extra owner receives no new January HUD Unit exact credit.

This reviewer made no source, header, config, compiler, or build-output changes,
and ran no build or gate. Only this independent ledger was written after the
author confirmed the final source/artifact freeze. This is static source/COFF
review, not a claim of runtime gameplay testing.

## Frozen evidence

| Input | Physical SHA256 |
| --- | --- |
| `source/interface/hud_unit.c` | `cc98970ee2318245d01706a1fd6941b4207aab62a3ea1e2293879a8738d260eb` |
| `scratch/hud-unit-renderer-baseline-20260904.obj` | `5dd945542fb8204395f3c1775653e89b39d21803f586ffb15bfb66ba0f8a8013` |
| `scratch/hud-unit-renderer-frozen-final-20260904.obj` | `35c94a866e13c7154f30cf23cf8701355fff7b6a6343f10f98bc54f9e570bfb2` |
| January `build/split/source/interface/hud_unit.obj` | `60c7298b55b49d1fb3c8fe8cf80ac5ec50e3d9008036c4c68150f95db8e36b02` |

The target and diagnostic candidate still use `_code_000c65c0` for the private
initializer. Production source uses semantic `initialize_hud_state`; root owns
the symbol metadata migration. The name is an honest inference supported by its
body and the later donor, not a claimed recovered January public symbol.

The precise target metadata changes supported by this review are:

```json
{ "file_offset": 812480, "flags": 32, "name": "_initialize_hud_state", "static": true }
{ "file_offset": 3034216, "flags": 0, "name": "?overcharge_count@?1??hud_render_unit_interface@@9@9", "static": true }
```

The first maps January's private initializer owner. The second maps the
function-local static described below; it does not turn adjacent data into an
invented aggregate.

| Owner | Target size / relocs | Candidate size / relocs | Normalized SHA256 |
| --- | --- | --- | --- |
| Initializer | 64 / 1 | 64 / 1 | Both `117b8363f3f64f6c4d6d19fd8264e389aa8479f00f1a7f588213d802eb430bca` |
| Renderer | 3504 / 102 | 3472 / 102 | Target `b82d575e1154cad1dd27cee874e9af5cc4b86dc302ea41685b56181ba32d93bc`; candidate `4071a4f0c7b1f808423bd5673e822e7f3c6a036e9a3a2db0fe7bd14b413b1817` |
| Extra `_fast_ftol` | Not owned by January HUD Unit | 32 / 0 | `8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248` |

The independent census uses `tools.coff_compare` plus
`tools.campaign._coff_tools.function_sections` over complete candidate objects,
not merely the target iteration printed by a gate. Baseline has 20 code owners,
January has 22, and final has 23. Every baseline owner remains strict exact.
There is no `_point_from_line3d` symbol and no COMMON allocation.

## Source/ABI and layout evidence

Read alongside the existing ledgers:

- `hud_unit_obj_play_unit_sounds_independent_review_20260904.md`;
- `hud_unit_obj_play_unit_sounds_exact_20260904.md`;
- `hud_unit_obj_get_state_tick_shield_exact_20260902.md`;
- `hud_unit_obj_jonas_state_accessor_exact_tick_boundary_20260829.md`.

Reviewed proposals are `scratch/hud-renderer-owner-proposal-20260904.md` and
`scratch/hud-unit-renderer-proposal-20260904.c`. The later behavioral donor is
the parent workspace's
`research-cache/halocea-full-blobs-20260830/src/blam/interface/hud_render_unit_interface.c`.
Its own allocation-warning caveats are important: its guessed 20-entry HUD array
and two-element auxiliary array were not copied.

The independent bounded PDB command, from the canonical worktree root, is:

```powershell
& '..\..\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' -sym hud_render_unit_interface '..\..\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
```

Its real local records directly establish:

- `unit_indices`: `long[0x12]`;
- `unit_hud_indices`: `long[0x12]`;
- `auxilary_values`: `float[0x1]`, represented with project `real`;
- `color`: `unsigned long[0x5]`, represented with `pixel32`;
- `overcharge_count`: **Static Local**, `long`, later data RVA `0x0146DF7C`;
- genuine `meter_hud_element_definition` copies, an absolute-placement object,
  and a `point2d` output for the motion sensor.

January independently authenticates the 18-entry arrays: each first element is
assigned and the remaining 17 dwords are zeroed with `rep stosd`; the child loop
checks `unit_count < 18`. The final ordinary initialized arrays express this
directly without guessed buffer lengths or artificial wrapper calls. The
auxiliary value is the one supported integrated-light-battery channel, consistent
with the existing authenticated `auxilary_flash_time[1]` state member.

The local static is genuinely `static long overcharge_count = 4`, not a public
facade introduced to imitate adjacent DATA. January `_data_002e4c68` contains
`04 00 00 00` and the renderer reads it at relocation offsets `0x41E` and `0x616`.
Its normalized four-byte hash is
`fb5e512425fc9449316ec95969ebe71e2d576dbab833d61e2a5b9330fd70ee02`.
Both sides use a four-byte `.data` owner with zero relocations, contribution
flags `0xc0300040`, and non-COMDAT selection 0. The final object also preserves
the target's four-byte static `_unit_hud_globals` BSS owner exactly, with no
COMMON symbol.

The typed unit, object, definition, seat, and tag-block accesses use the real
`unit_get`, `unit_try_and_get`, `object_get`, `unit_definition_get`,
`unit_hud_interface_definition_get`, and `TAG_BLOCK_GET_ELEMENT` interfaces.
January's object mask 3, `'unit'`/`'unhi'` groups, 0x11C seat stride, 0x84 overlay
stride, and 0x144 auxiliary-meter stride agree with those owner declarations.
The new HUD drawing and motion-sensor calls use owning headers, not caller-local
prototypes. January explicitly passes `player_index` to the motion-sensor
game-engine query at renderer `+0x89B`; the proper one-argument API is required.

## Stack guard is authenticated behavior

The stack-check code is not invented padding or a compiler-steering lifetime.
January calls `get_return_eip` at `+0x0B`, fills a 0x200-byte local region with
0x62 at `+0x21`, and scans its 128 long elements backward at `+0xD1A` onward.
It compares the return address again and emits the two real diagnostic assertions
at source line 0x3C9, including `"corrupt stack at %d!"`. The same conventional
guard is already present in the exact HUD update caller.

The retained code reproduces that observable validation; the review does not
claim to have recovered the literal original stack-check macro spelling. Both
new functions terminate with explicit `return;`. No forced-inline attribute,
pragma, synthetic call anchor, volatile/register steering, raw-offset view, or
new assembly block was added by this renderer implementation.

## Correctness corrections, not residual tuning

The initial draft had three independently identifiable predicate differences.
The author corrected all three before freezing:

1. Auxiliary active-meter flashing is **ordered `<= min_cutoff`**, not `<`.
   January's `fcomp` at `+0xADA`, `test ah,0x41`, and parity branch make equality
   flash, while unordered comparison clears the bit.
2. The health maximum-cutoff branch includes unordered values. January
   `+0x719..734` tests status bit C0 alone and enters the low-color arm for less
   or unordered. `!(body_vitality >= max_cutoff)` preserves that behavior; plain
   `<` does not. The subsequent minimum-cutoff check remains ordered `<=`.
3. January compares the last/reference shield value to the current value and
   chooses reference only when ordered greater. The retained
   `MAX(last_shield_vitality, shield_vitality)` preserves its NaN and signed-zero
   tie preference; swapping the macro arguments can change the selected value.

The remaining layering, shield fade, health coloring, motion-sensor placement,
team-color overlay, and active/inactive auxiliary-meter flows are coherent with
January. The apparently redundant `get_hud_state` call in the parent-seat path
is actually present at January `+0x178`; it is a real validating accessor, not a
new fake reference added to preserve helper emission.

## Float conversion and repeated PIN evaluation

January distinguishes two genuine conversion APIs:

- Shield and auxiliary meter byte clamping uses direct `fistp`, preserving the
  current x87 rounding behavior. The existing `cseries.h::fast_ftol` helper
  expresses that behavior. A plain C cast in the first draft used the wrong
  conversion route; replacing it was a semantic correction, not byte forcing.
- Health uses truncation-correcting `fast_ftol_C`. The canonical `PIN` macro
  evaluates its first argument up to three times. January contains exactly six
  ordered health call sites at `+0x77D`, `+0x79A`, `+0x7BD`, `+0x7D4`, `+0x7F1`,
  and `+0x814`, matching the two nested clamping expressions. Do not cache these
  calls or replace them with the differently rounding helper merely to shorten
  the source. Final candidate retains the same six calls, shifted by its residual
  instruction schedule.

The sparing assembly is inside the already-established authentic math helper,
not embedded as new handwritten renderer assembly. Final code inlines the
renderer call sites but still emits a standalone 32-byte `_fast_ftol` copy.
The reviewer directly verified it strict-equal to January's selected
`source/ai/actor_combat::_fast_ftol`. The HUD candidate copy has flags
`0x60501020` and COMDAT selection 2 (`IMAGE_COMDAT_SELECT_ANY`); January's
selected actor-combat owner has the same normalized body and flags but COMDAT
selection 1 (`IMAGE_COMDAT_SELECT_NODUPLICATES`). That is provenance for a real
shared helper, not proof of compatible ownership.

The compatible-link result is already established by the independently
preserved XDK 3911 experiment in
`player_rumble_obj_jonas_symbol_backed_fast_ftol_closeout_20260831.md`:
`Link.Exe /dll /noentry /nodefaultlib /force:unresolved` was run on the rebuilt
select-any `_fast_ftol` copy and the split actor-combat no-duplicates owner in
both input orders. Both fail with `_fast_ftol` `LNK2005`, followed by `LNK1169`.
`bitmap_utilities_obj_jonas_vector_map_quartet_20260831.md` independently records
the same ownership failure in its six-helper link test. The HUD copy therefore
cannot be called an ordinary discardable duplicate without new authenticated
linkage evidence.

## Remaining accounting boundary

All inherited DATA/BSS owners remain unchanged. New candidate DATA relative to
the baseline consists of the real four-byte local static, an ordinary 0.2f
literal, and the renderer assertion literal. The author corrected the draft's
spaced `==` spelling to January's authenticated
`player->local_player_index==render.local_player_index` diagnostic text before
the final freeze. That removes the wrong 56-byte literal and uses the authentic
54-byte shared literal. The final normalized renderer byte hash and strict
counts are unchanged: relocated payload bytes are normalized before hashing,
while ordered relocation identities and raw data/string ownership are separate
required checks. Neither ordinary literal receives extra
January-owned exact-byte credit here.

The complete final candidate-only read-only-data census is pooling, not a
HUD-owned data gain: local-player range assertion (`camera/director.obj`),
`0.1875f` (`ai/action_alert.obj`), `0.0f` (`ai/action_charge.obj`), corrupt-stack
and corrupt-return diagnostics (`interface/hud_draw.obj`), the health-cutoff
constant `0.033333335f` (`ai/actions.obj`), `1.0f` (`ai/action_charge.obj`), the
unreachable diagnostic (`ai/actor_moving.obj`), `0.2f`
(`ai/actor_combat.obj`), and the corrected no-space player/render assertion
(`interface/hud_draw.obj`). Each has a compatible January selected select-any
owner with the same payload and zero relocations. There is no other
candidate-only CODE, DATA, BSS, RDATA, or COMMON owner beyond the incompatible
`_fast_ftol` copy already disclosed.

Do not count the extra `_fast_ftol` owner, the 3,472-byte renderer, or ordinary
deduplicable literals as fresh exact HUD Unit progress. The supported new strict
function gain is **51 meaningful / 64 padded bytes** for the initializer;
renderer semantic/fuzzy progress is separate. Root still owns the symbol/data
mapping, measured residual entry, full rebuild, stable sweep, admission policy,
and any subsequent publication.
