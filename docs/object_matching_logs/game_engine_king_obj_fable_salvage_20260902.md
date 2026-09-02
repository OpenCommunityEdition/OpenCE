# `game_engine_king.obj` Fable salvage audit (2026-09-02)

## Result

This lane audits the preserved Fable snapshot against canonical commit
`ab8e7207b61fb07b12b94d82a273c351eb1d00fd`. The snapshot was retained as
commit `da9608f1ea286c0629e54714322e09f576bdc606`; it came from Fable commit
`6445a98e4bf9a00bc09726632ee87ceab5bad068`.

The strict targeted gate advances `source/game/game_engine_king` from
`14/29` to `26/29` exact functions and from `336/5168` to `2944/5168`
target-owned padded code bytes. The admitted gain is therefore **12 exact
functions and 2608 padded bytes**, with no inherited exact function lost.

| January owner | Meaningful / padded bytes | Result |
| --- | ---: | --- |
| `_hill_points_to_2d` | `29 / 32` | exact |
| `_player_inside_hill` | `137 / 144` | exact |
| `_king_engine_player_update` | `352 / 352` | exact |
| `_king_engine_display_score` | `393 / 400` | exact |
| `_king_get_score` | `50 / 64` | exact |
| `_render_dynamic_quad_initialize` | `138 / 144` | exact |
| `_render_dynamic_quad` | `676 / 688` | exact |
| `_king_get_score_header_string` | `83 / 96` | exact |
| `_king_engine_goal_matches_player` | `24 / 32` | exact |
| `_find_next_hill` | `110 / 112` | exact |
| `_king_engine_initialize_for_new_map` | `265 / 272` | exact |
| `_king_engine_update` | `257 / 272` | exact |

Objdiff reports `2658/4865` meaningful code bytes, `26/29` exact functions,
and `79.12534%` fuzzy similarity for `.text`. The different meaningful-byte
total is navigation evidence only; the campaign gain above uses the hardened
padded-section gate.

No object-complete claim is made. Two ordinary-C bodies remain fuzzy and one
large callback remains unwritten:

| Function | Meaningful / padded bytes | Relocations (target/base) | Objdiff | Disposition |
| --- | ---: | ---: | ---: | --- |
| `_find_hill` | `847 / 848` | `35 / 35` | `97.94042%` | honestly parked, `unclassified` |
| `_king_calculate_hill_state` | `444 / 448` | `32 / 30` | `81.507355%` | honestly parked, `unclassified` |
| `_king_engine_post_rasterize` | `916 / 928` | candidate absent | n/a | unwritten |

`_find_hill` is structurally complete. Alignment-aware disassembly confines
the substantive residual to C2 choosing the z component rather than the y
component as the induction base for the six scalar bounding-box updates. The
later-Xbox scalar-if spelling was compiled as a bounded evidence-backed test
and did not improve the exact class. `_king_calculate_hill_state` retains the
complete team and free-for-all state machine, but Fable's declaration-scope,
local-state, early-return, and shared-tail experiments did not recover the
January branch and relocation schedule. The fail-closed blocker classifier
returns `UNKNOWN` for both functions, so neither is assigned a speculative
compiler recipe.

## Naming, ownership, and house-rule reconciliation

The snapshot used address placeholders because canonical had not yet named
this unit. The reconciliation maps every King function in `config/symbols.json`
to its semantic January/HaloCEA name and marks the five private helpers
`static`: `hill_points_to_2d`, `find_hill`, `player_inside_hill`,
`king_calculate_hill_state`, and `find_next_hill`. `_bss_0043e948` is renamed
`_king_globals`.

`source/game/game_engine_king.h` now owns the public callback prototypes, the
King message and hill-state enums, and the offset-asserted `king_globals`
record. Existing `game/game_engine_place.h` owns the place API. A typed
`shader_definition_get` macro is added to `shaders/shader_definitions.h`, so
the render helper no longer repeats a raw `tag_get` cast. Player and unit
access continue through `player_get` and `unit_get`.

The retained source uses cseries `real`, point/vector, boolean, datum, and
`MIN`/`MAX` vocabulary; target-proven enum constants replace ad-hoc numeric
switch labels. No-argument lists put `void` on its own line, each parameter is
on its own line, and every function has an explicit terminal return. There is
no forced inline, inline assembly, volatile/register forcing, barrier, codegen
pragma, raw address, representation pun, fake dependency, or fabricated
control flow. The January inline schedule is unchanged, and this unit neither
uses nor emits `point_from_line3d`.

`find_next_hill` intentionally retains an uninitialized no-candidate result.
This is not an optimizer trick: both the January object and independent later
Xbox reconstruction have that behavior, and the caller's map contract expects
at least two distinct hill ids.

The candidate emits the target's full `560`-byte zero-filled state region as
three semantically named owners: `king_globals` at `0`, private
`king_engine_hill_count` at `428`, and private `king_engine_hills` at `432`.
The split target collapses that region under one `_king_globals` owner, so the
ordinary report shows `.bss` at `50%`; this lane does not add a false ownership
exception. The target also owns a `136`-byte `_king_engine` callback table,
which remains absent. HaloCEA's later table is `156` bytes and is not a valid
January donor, so it was deliberately not copied.

## Evidence

The January split object remains the byte and ownership authority. This audit
uses the stock Microsoft C/C++ `13.00.9254.1` edge and hardened
`tools/campaign/gate.py` / `tools/coff_compare.py` comparison. Applicable
project guidance and prior ledgers were read, especially:

- `docs/object_matching_logs/game_engine_king_obj_jonas_public_leaves_20260828.md`;
- `docs/object_matching_logs/game_engine_player_added_family_jonas_20260826.md`;
- `docs/campaign_throughput_lessons.md`;
- the authenticated campaign `CLAUDE.md` and `AGENTS.md` guides.

HaloCEA is used only as independent semantic and naming evidence, pinned at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Relevant blobs include:

| HaloCEA path | Git blob |
| --- | --- |
| `src/blam/game/find_hill.c` | `8e86c04fab70ed52b02c631aec6a43c2c9d780d7` |
| `src/blam/game/king_calculate_hill_state.c` | `e41d3e0894c96f053bdf538d2cc5bc2c7776e9e3` |
| `src/blam/game/king_engine_post_rasterize.c` | `5ee668b4803d3665b097cac021628e63092c97b2` |
| `src/headers/king_message.h` | `9ff163fab6c6774e8cf96f1dd2d3cf1c46ccf73b` |
| `src/headers/king_hill_state.h` | `3f7db4dbeb76357086583bf20fdf34d64c7c334c` |
| `src/headers/king_globals.h` | `9c92f4e9bbdb1faaf4f08fec2f209edd04af02cc` |
| `src/data/king_engine.c` | `d143f1e8375ac611a38b54b262b90a092540214a` |

The open Marathon 2 source was consulted for source-era King-of-the-Hill
state/score style, pinned at
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c`. The relevant
`marathon2/network_games.c` and `marathon2/player.c` blobs are respectively
`0c9dcdbc7965884b4ed9db5ba34e7d34fa86d303` and
`c46b19a87d1888b4b9df20472685e67d2e57cd2a`. Marathon and HaloCEA never
override January bytes.

## Verification

The reconciled targeted gate reports:

```text
== exact 26  residual 2  unwritten 1  (of 29 listed)
```

Additional checks:

- targeted `game_engine_king.obj` compile: pass (only C4700 for the
  independently authenticated `find_next_hill` quirk);
- parked-function validation against the generated report: `110 active`,
  `0 stale`, `0 invalid`;
- address-keyed full-report exact-set comparison against the pre-lane
  snapshot: `5084 -> 5096` exact functions, `0` losses, exactly the twelve
  King gains listed above;
- fake-match scan over the King source/header and shader owner header:
  `0 review leads`;
- Python verification suite: `258 passed`;
- fail-closed blocker classification: `UNKNOWN` for both parked residuals;
- candidate `.rdata`: `180/180` exact;
- source/config hazard scan: no address-derived code/global names, assembly,
  inline/codegen directive, volatile forcing, or `point_from_line3d` use.

The orchestrator explicitly prohibited a full Ninja run for this resumed
lane, so final verification is translation-unit scoped. The pinned baseline
had already passed its isolated full build before the resumed constraint; the
integrator must still run the canonical full build and full exact-set sweep
after admission.

## Integration notes

Cherry-pick the snapshot commit first, then the reconciliation commit that
contains this ledger. If `config/symbols.json` conflicts with newer canonical
work, preserve the semantic King names at file offsets `655408` through
`660304` and `_king_globals` at `4450632`; do not reintroduce `_code_*` or
`_bss_*` placeholders. Regenerate configuration/split artifacts before the
targeted gate, then run canonical full-build, full exact-set, semantic
ownership, parked-ledger, fake-match, and pytest admission gates. Do not claim
the object complete until the `928`-byte post-rasterize callback and the
`136`-byte engine table are reconstructed and both fuzzy parks are resolved or
accepted under the campaign's documented fuzzy policy.
