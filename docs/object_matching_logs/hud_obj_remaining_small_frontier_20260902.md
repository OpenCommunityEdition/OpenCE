# `hud.obj` remaining-small frontier (2026-09-02)

## Outcome

This isolated pass advances `source/interface/hud.obj` from 16 exact / zero
residual / six unwritten functions to 18 exact / three credible residual / one
unwritten. The two new strict closures add 79 meaningful and 96 padded code
bytes. Three complete evidence-backed functions replace another 2,256 padded
bytes of unwritten scope and are parked honestly at their measured compiler
boundaries. The only function still unwritten is the 880-byte
`temporary_hud_draw`.

| Newly exact function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_weapon_state_is_depleted` | 39 | 48 | 0 | `5d58458dd78a3f82e3c97ba62d170a9fdd6b92deac467c8c2592d7d9f3fcdbd7` |
| `_get_object_icon_text_index` | 40 | 48 | 2 | `0e91acf71443ef0fe8c2908a770549be45b9e9409f39079f267fac5828739299` |
| **Total** | **79** | **96** | **2** | |

Every one of the 16 inherited HUD exact functions remains strict exact.

## Authority and provenance

- Integration base: `9a56c914fb121f866a853ba4f5c2bbc56f19efc5`.
- January target `hud.obj` SHA-256:
  `fb839de981b5e363e3f3ff72c14a209253dd40f5e1b88c99c720faae8753f3ca`.
- Rebuilt candidate `hud.obj` SHA-256:
  `469729a2e3026d721fcddd05e07fd86c3aac22d75c83f6b09b5a86a6d21a4b5c`.
- Compiler: pinned Microsoft VC7/XDK 3911 through the normal Ninja edge and
  PID-isolated `tools/campaign/gate.py` checks.
- Primary semantic donor: HCEA commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, especially
  `weapon_state_is_depleted.c`, `get_object_icon_text_index.c`,
  `hud_show_action_response.c`, `hud_draw_screen.c`, and the authenticated
  HUD/player/unit/weapon enum and structure records.
- Independent corroboration: Pastudan Halo commit
  `918af885935ec470a31256ecce9a977b12b01f80` and Stian Halo commit
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf` were searched for the HUD
  family and checked against January control flow. Neither carries a more
  complete January source candidate.
- Marathon commit `ce4fdc63f20f6fa39616a86e717118f4d8bdf25c`
  was checked for contemporary C-series style and type precedent. It has no
  direct Halo action-response or temporary-HUD donor.

The January split COFF, its address-keyed symbols/relocations, and same-build
disassembly remain the byte authority. Later recovered source is semantic
evidence, not a substitute target.

## Exact closures

`weapon_state_is_depleted` uses the HCEA-authenticated `int` result and the
real `weapon_interface_state` fields. The compact boolean expression naturally
lowers to January's two depletion paths: an empty finite magazine, or an age of
exactly `1.0f`. No synthetic result carrier is required.

`get_object_icon_text_index` uses the project's typed object and object-tag
accessors. The `NONE` guard and chained definition/icon lookup naturally
reproduce the private EAX convention, both relocation identities, and all 48
padded bytes.

The semantic names come from the HCEA source/PDB atlas rather than January
address placeholders. Both functions are correctly recorded as private static
symbols in `config/symbols.json`.

## Complete fuzzy reconstructions

### `_hud_show_action_response`

The retained typed function covers every named respawn-failure and player
action-result branch, object/unit/control/weapon lookups, weapon-HUD message
icons, game-engine custom text, occupied-seat driver/gunner suppression, and
weapon-swap reminders. The target-attested unusual `while (!--weapon_count)`
topology is preserved because it is evidence, not because it helps codegen.
Correcting the later donor's provisional 1,028-element buffer to January's
1,024 elements recovers the exact `0x82c` frame.

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded bytes | 1,744 | 1,728 |
| Instructions | 631 | 626 |
| Relocations | 120 | 121 |
| Objdiff report | 100% | 71.032104% |
| Normalized SHA-256 | `bfe1022d2e8f59dc3220b33c497fed91681596b485f4cb2b9339454fe27ec317` | `853cffc36136054ae6e51f2bb4012315c6f5b991c2950c0783d7b0f4fc9cafa9` |

January's private caller passes `player_index` in EAX and shares a fallback
tail that VC7 duplicates in the ordinary source build. Explicit `__fastcall`
does not explain that ABI on this compiler: it emits a stack argument plus
`ret 4` and substantially worsens the candidate. Register hints, declaration
and message ordering, merged/duplicated switch arms, icon access, and seat
access variants did not close the boundary. It is parked as
`private-register-convention` rather than accepting an ABI shim or source
carrier.

### `_temporary_hud_draw_reticle`

The retained routine builds sixteen typed points with the project trigonometry
and matrix helpers and draws the closed ring through the rasterizer API. Both
artifacts have six relocation identities. January schedules the `_pi / 8`
x87 add before call-stack cleanup and retains generic signed modulo-by-16;
the candidate moves the independent add after cleanup and proves its index
nonnegative, reducing `% 16` to `and 0xf`.

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded bytes | 208 | 192 |
| Relocations | 6 | 6 |
| Objdiff report | 100% | 90.676926% |
| Normalized SHA-256 | `34a33fe789d2858840e360558ebc1d2674a329e6a14d59572a3dfb9c1bc2255c` | `676b43cbdb66de19263a32f71a6c895b867d638c75f5fcd404e031b808e47afa` |

Natural loop, pointer, increment, signed-index, and declaration-order forms do
not resolve both schedules. The function is parked as
`instruction-scheduling`; no artificial signedness carrier or gameplay
assembly is retained.

### `_hud_draw_screen`

The complete top-level draw path preserves January's rasterizer HUD bracketing,
player/team indicators, primary-local-player motion-sensor tick, perspective
and unit gates, weapon/action/sound/unit/nav/damage sequence, messaging, and
temporary-HUD tail. Target and candidate have all 33 relocations in the same
semantic sequence.

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded bytes | 320 | 304 |
| Relocations | 33 | 33 |
| Objdiff report | 100% | 94.97959% |
| Normalized SHA-256 | `4c1bd54b1e9c6c99e27b86fe5281fca35482e7f2b16be9a9d808a4110b77dffb` | `b1b1215ac7f29cbcd63a6070eb063e95473ffdde8caf4a744719cf62850343f7` |

Its envelope and register-cleanup gap is downstream of the adjacent private
action-response ABI: January moves the player index into EAX, whereas honest C
pushes it. The public HUD calls remain normal cdecl calls. This caller is
therefore parked as `private-register-convention` with its callee, not patched
with a naked wrapper.

## House-rule and declaration audit

The retained code uses `real`, `boolean`, semantic enums, C-series flag macros,
typed object/tag subsystem access, semantic private/global names, declaration
plus initialization where natural, multiline signatures, and explicit
returns. Public cross-TU declarations are in their owning or closest associated
headers (`game_engine.h`, `units.h`, the HUD subsystem headers, and
`rasterizer.h`) rather than duplicated at the use site. The provisional
weapon-interface layouts remain TU-private: a previous measured campaign pass
proved that moving those structure definitions into a widely included weapon
header perturbs the already-exact `units.obj` allocator schedule.

There is no forced inline/noinline, `volatile` steering, barrier, pragma,
optimizer-only alias, raw address field, fake dependency, undefined behavior,
or nonsensical exactness branch. No inline assembly is added. The built HUD
object emits no `point_from_line3d` symbol or COMDAT, preserving the January
inline schedule and ownership boundary.

## Validation

- Focused gate: 18 exact / three residual / one unwritten.
- Full `halobetacache_build` and `libcmt_build`: pass.
- Strict semantic report: 473 units scanned, 5,214 functions evaluated,
  5,069 semantic exact, 134 hidden exact, zero unit errors.
- Park validator: 94 active / zero stale / zero invalid.
- Protected `units.obj`: 189/189 strict exact.
- Address-keyed whole-tree regression verdict: two HUD gains and zero losses.
- Object-admission audit: zero HUD contradictions or revocations.
- Focused fake-match scan: zero findings.
- Tooling suite: 255 passed.
- `git diff --check`: pass.

The validation figures above describe the committed candidate. No whole-object
completion is claimed while `temporary_hud_draw` remains unwritten.
