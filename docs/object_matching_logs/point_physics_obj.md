# `source/physics/point_physics.obj` strategy ledger

This retroactive ledger records preserved strict measurements and explicitly
marks the detail lost before per-object experiment matrices were required.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/physics/point_physics.obj`.
- Historical baseline: 5/6 functions exact; `_point_physics_update` was the
  only residual.
- Current verified inventory: 6/6 functions exact, 1,822/1,822 meaningful
  code bytes, and 220/220 data bytes. `.text`, `.rdata`, `.data`, and `.bss`
  all report 100% after a clean XDK 3911 rebuild.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | Measured residual | Class |
|---|---:|---:|---|---|---|
| `_point_physics_update` | `0x570/0x570` | `68/68` identities | `5aca2728fa2d89a716057b07ccbcc1e00cbf126644391dcf1a534707f5cba88c` / `93750650472a8e7cfc7dff5fd05b16a5d0b61d4ba611b324a6d1612ed82dbeb4` | First divergence is target `MOV ECX,EAX` versus candidate three-byte `MOVZX` from CL, shifting later relocations by one; additional commutative x87 operand-order differences remain. | TU-context optimization |

## Preserved experiment history

Five grounded low-byte/current-flags source shapes were built. The untouched
source remained best at ordinary 99.39632%. Attempts to force the byte form
introduced extra loads/spills and regressed the function. Individual
transient hashes and source diffs were not retained by the historical lane.

Do not repeat generic casts, low-byte snapshots, current-flags aliases, or
declaration-order variants without new evidence. Do not use `volatile`,
assembly, byte patches, undefined behavior, or flag changes.

## Reopen and disposition

Reopen only with a legal-C use/lifetime shape proven to select the target
two-byte move while retaining the 68-relocation graph, or with original
local/source provenance. Recheck the 8-byte `.bss` under strict ownership
rules before any object admission.

## Upstream resolution (2026-08-07)

The completed object in `punpckhdq/halo` commit `76a2766930a98f946d669c6db3565c95301d11c4`
was rebuilt in a pristine detached worktree before any integration. All six
functions passed the hardened comparator, including relocation identity.
The upstream `.rdata` (200 bytes), `.data` (12 bytes), and `.bss` (8 bytes)
also matched the January object.

The commit was then applied to the current campaign in an isolated integration
branch. Conflicts were resolved without discarding unrelated current work:

- the January-proven 0x40-byte `point_physics_definition` layout replaced the
  incomplete 0x34-byte HCEX-derived layout;
- the anonymous 8-byte `point_physics_globals` block was replaced by the two
  target-owned four-byte globals;
- the collision-user enum was restored so the source names user 13 instead of
  retaining a magic number;
- a typed `point_physics_definition_get()` macro and size assertion were added
  to retain the project house rules.

The consolidated Halo build completed successfully. The final strict function
measurements are 6/6 exact, with `_point_physics_update` at 0x570 bytes and
68/68 relocations. The old `MOVZX`/x87 residual was therefore a type/source-
provenance problem, not a permanent compiler tie.

Disposition: strict object complete; eligible for canonical admission after
the whole-tree regression manifest check.

## House-rule compliance re-audit (2026-08-07)

The admitted source and header were re-read against `docs/house_rules.md` and
Berthalamew's formatting/access guidance.  Parameters and no-argument `void`
lists are vertically formatted, every `void` definition ends with an explicit
`return;`, tag access goes through the typed
`point_physics_definition_get()` macro, and there are no raw object accesses,
alignment coercions, assembly, `volatile`, `__forceinline`, or optimization
pragmas.  Files use CRLF consistently.  No compliance edit was required.

## Independent atomic salvage revalidation (2026-08-13)

The old completion was treated as untrusted and first transplanted into a fresh
worktree at campaign parent `e30b15682e8745d5aa8ceb5cb4e455f3bedfcc83`.
After it passed there, the result was transplanted again onto authoritative
code tip `19236aca35b1a319bc4ed050bf628b2f4f9e5bb0` and every code, data,
shared-header, build, and tooling gate was repeated.  That explicitly includes
the newly landed `collision_features.obj` changes to `real_math.h`; they caused
no point-physics or sibling regression.  The current XDK 3911 compiler and the
freshly re-split January target were used for every measurement.  This audit
found two ownership details that the old completion report had missed:

- `debug_point_physics` is defined by January's `source/linker_common.obj`, not
  by `point_physics.obj`.  Keeping the upstream definition here produced a
  12-byte rebuilt `.bss` against the target's 8 bytes.  Removing that duplicate
  definition leaves an ordinary external reference and restores the exact
  target-owned 8-byte block.
- The two anonymous BSS fields have distinct meanings and offsets.  January
  stores the water mass factor at offset 0 and the air mass factor at offset 4.
  With the declarations in the old order, VC7 emitted the two statics in the
  reverse layout.  Declaring water before air produces the proven target layout:
  `_global_water_mass_over_radius_cubed` at 0 and
  `_global_air_mass_over_radius_cubed` at 4.

The relevant `symbols.json` entries now name the private renderer and both BSS
fields.  Re-splitting the target with those aliases makes the hardened
`section_infos_equal` proof direct; no fuzzy or byte-only exception is used.

### Strict function evidence

| Function | Padded bytes | Relocations | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_point_physics_initialize_for_new_map` | `0x30` | 6 | `9010f31148fb9f42a69e7b6dc423092e9c8d4f6e639697c11ef26ad900c91cd4` | exact |
| `_point_physics_dispose_from_old_map` | `0x10` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | exact |
| `_point_physics_definition_get_mass` | `0x20` | 0 | `8792859ba0282c4bb0a61e5c5a43f9a5bbde5b578125c991b2a2a2b8dcae99ee` | exact |
| `_point_physics_definition_interpolate` | `0x160` | 19 | `06fcdedf097eb65d7a48b29d30bdf0364aebfd23a79f8aa3bf6abc67e19571a3` | exact |
| `_render_debug_point_physics` | `0x30` | 3 | `f8dfcdb85893a7f5ff751bcfc788d259cd7f1d13ddfa752c6312c547932eccaa` | exact |
| `_point_physics_update` | `0x570` | 68 | `5aca2728fa2d89a716057b07ccbcc1e00cbf126644391dcf1a534707f5cba88c` | exact |

All six functions pass strict size, normalized-byte, relocation-address,
relocation-type, destination, and addend comparison.  This is 1,888/1,888
padded code bytes and the ordinary report's 1,822/1,822 meaningful code bytes.

### Owned data and ownership evidence

All 17 target-owned COFF sections pass `section_infos_equal`: the six function
sections above, nine `.rdata` COMDATs (182 raw bytes, reported as 200 bytes with
target alignment), the 12-byte `_global_point_physics_reference` `.data`, and
the 8-byte mass-factor `.bss`.

| Owned data | Bytes | Relocations | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| nine `.rdata` COMDATs | 200 reported | 0 | each owner checked independently | exact |
| `_global_point_physics_reference` | 12 | 0 | `ca5622977c575e2fcacb390addd8f0dc1cd2b513916cf76e16b192067b183b53` | exact |
| mass-factor `.bss` | 8 | 0 | `af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc` | exact |

The split target necessarily labels extracted private sections as external,
while VC7 emits the private helper and mass fields with static storage.  The
aliases, section bytes, exact field offsets, and every relocation destination
are unambiguous; the external/static class difference is the known csplit
ownership representation, not a different linked address.

### Rejected and accepted source shapes

| Shape | Measurement | Disposition |
|---|---|---|
| Historical incomplete reconstruction | `_point_physics_update` `0x570`, 68 relocs, hash `93750650...`; 99.39632% | rejected |
| Upstream completion copied literally | code exact, but rebuilt `.bss` `12/8` because it also defined `debug_point_physics` | rejected |
| External `debug_point_physics`; old air-then-water declaration order | `.bss` size corrected to 8, but symbols measured air@0/water@4 against January water@0/air@4 | rejected |
| External debug flag; water declared before air | `.bss` 8/8, water@0, air@4; all 17 owners strict | accepted |

The five earlier low-byte/current-flags experiments remain recorded above and
were not repeated.  The decisive improvement came from recovered structure and
ownership provenance, not from coercing the compiler.

### Blast radius and style audit

The five changed shared headers were followed transitively through the source
include graph.  Thirty-three translation units were rebuilt in both the clean
parent baseline and the candidate.  All 1,633 previously strict target-owned
sections remained strict; there were zero regressions.  External-symbol
metadata changed only in `physics.obj` (the January-owned physics globals are
now declared/defined) and `point_physics.obj` (the intended completion).

The final source follows the house and Berthalamew rules: one parameter per
line, vertically formatted `(void)`, explicit final `return;` in void
definitions, a single return where feasible, typed `tag_get` access through
`point_physics_definition_get()`, named collision-user and result enums, and no
raw object access.  It contains no assembly, `volatile`, `__forceinline`,
alignment coercion, optimization pragma, undefined behavior, or byte patch.
No original January gameplay bug was identified in this object, so no fake bug
comment was added; the duplicate debug-global definition was a reconstruction
ownership bug and was corrected outright.

### Final gates

- `ninja halobetacache_build`: passed from a clean full rebuild at the current
  campaign code state.
- `ninja progress`: passed; the semantic audit reported zero unit errors, and
  `point_physics.obj` reports 6/6 functions, 1,822/1,822 meaningful code bytes,
  and 220/220 data bytes.
- `tools.regression_gate snapshot/check --unit source/physics/point_physics`:
  passed with six `still_exact`, no failures, no warnings, and no changed
  nonexact function.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 173/173 passed.
