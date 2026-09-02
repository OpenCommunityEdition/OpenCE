# `game_engine_race.obj` Fable salvage (2026-09-02)

## Result

The Fable small-family snapshot was replayed on the canonical
`fca0dd5d7` base and reconciled to the current source and naming rules.  The
frozen pre-salvage gate was **15/36 exact**, with 21 functions unwritten.  The
reconciled source has 34 target-backed exact bodies, one credible ordinary-C
residual, and one unwritten formatter after semantic target-name regeneration.

The strict function gain is therefore **+19 functions / +3,760 padded target
bytes**.  The reconstruction also supplies the target-shaped 136-byte
`race_engine` table and the 208-byte `race_globals` BSS owner.  Final board
credit remains conditional on the orchestrator's clean configure, full build,
and address-normalized regression sweep.

## Provenance and naming

- Snapshot commit replayed in isolation: `209232df5` (local replay
  `17e244bc2`).
- January target object SHA-256:
  `5CB2E65CF598535C9FDFB2C1FF687D8E12CC7C91FA77B09041735B3BFB363002`.
- Reconciled probe object SHA-256:
  `81DA7E6578DB15BA0769551B871C3FC1514617C0C0FE1A2CBBBBE99D17BC787E`.
- The later-Xbox HaloCEA race engine table authenticates the public callback
  names and ordering.  Its individual race functions authenticate the private
  names `can_touch_team`, `find_closest_vehicle`, `new_rally_flag`,
  `race_complete_lap`, `race_team_can_win_game`, `race_touch_flag`, and
  `count_bits_32`.
- The January table topology, not the later 156-byte table, controls the PC
  136-byte layout and null slots.
- `_bss_0043ecb0` is named `race_globals`; target relocation addends prove its
  208-byte PC layout.  The otherwise unused tail scalar is named
  `persistent_team_score` from the corresponding later source family rather
  than retaining an address-derived or `unknown` global member name.
- The former player fields `unknown88` and `unknown_d1` use the canonical
  semantic names `multiplayer_special` and `quit_out_of_game`.

The public declarations and globals live in the owner header
`source/game/game_engine_race.h`.  `game_engine_list.c` includes that header
instead of redeclaring `race_engine` in a consumer `.c` file.

## Exactness evidence

The PID-isolated XDK 3911 gate, with aliases used only to compare against the
pre-regeneration placeholder target atlas, reports 33 exact, two residual,
and one unwritten function.  One reported relocation-only residual,
`race_touch_flag`, has identical 336-byte code, all 30 relocations, and the
same normalized SHA-256 on both sides.  The only artificial difference is the
assertion string produced when the diagnostic alias rewrites `race_globals`
to `bss_0043ecb0`; the January assertion itself contains `race_globals`.
Regenerating the target split with the authenticated global name removes that
diagnostic-only mismatch, yielding the expected 34/36 strict census.

The exact data shapes were checked independently:

| Owner | Size | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `race_engine` | 136 | 25 | `97a4c632a978acafd20933da4504d8ec3fdf3c5afd2f78934bced57dc8f746f4` |
| `race_globals` | 208 | 0 | `46f531b7ea0428fbf2c3ca2b60e8dc33d6bbfa000e0fd1b489c5e39140a47006` |

The table has the exact January byte hash and relocation topology.  Before
semantic regeneration, its only symbolic difference is the deliberately
renamed unwritten `race_engine_display_score` target.

## Honest residual and unwritten work

`race_engine_get_score` is retained as ordinary typed C and parked, pending
the final generated report measurement.  Target and candidate are both 224
padded bytes with four relocations and have the same control flow and
calculation.  January keeps the lap bit vector in EDX, the lap count in EDI,
and the inlined population count in ESI; the available VC7 build selects an
equivalent register permutation.  Natural assignment-order, direct-expression,
and scoped-local probes either reproduce that permutation or stop unrolling
the exact `count_bits_32` helper and shrink the body to 96 bytes.  The
fail-closed blocker classifier returns `UNKNOWN`; no compiler recipe is
claimed.

`race_engine_display_score` remains unwritten at 1,152 padded bytes.  No stub,
raw lift, or later-build body is admitted merely to make the object appear
complete.

## House-rule audit

The retained source uses cseries scalar and geometry types, typed player/unit
and tag-block accessors, semantic enum constants, owner-header prototypes,
one parameter per line, `(void)` formatting, and explicit terminal returns.
It adds no inlining control, `volatile`, register forcing, pragma, barrier,
raw address, manual byte emission, inline assembly, undefined behavior, or
nonsensical code-generation carrier.  The January inline schedule is
unchanged and no `point_from_line3d` reference or COMDAT is emitted.

