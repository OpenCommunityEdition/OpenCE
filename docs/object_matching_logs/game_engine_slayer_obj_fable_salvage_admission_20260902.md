# `game_engine_slayer.obj` Fable salvage and admission (2026-09-02)

## Scope and provenance

- Canonical base: `ab8e7207b61fb07b12b94d82a273c351eb1d00fd`.
- Preserved Fable snapshot: `e84ff5e8f` from
  `fable-small-families-salvage-20260902`; its original experiments and logs
  remain under
  `C:\halo-worktrees\fable-small-families-20260901\scratch`.
- Isolated branch/worktree:
  `agent/fable-game-engine-slayer-salvage-20260902` at
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\fable-game-engine-slayer-salvage-20260902`.
- Target authority: the January 2002 split object at
  `build/split/source/game/game_engine_slayer.obj`, compared with PID-local
  compiler output from `tools/campaign/gate.py` and the hardened full-tree
  exact-set comparator.

The canonical baseline was already complete in source terms: 25 exact, two
credible residuals, and no unwritten functions. This salvage therefore keeps
all 25 inherited exact functions and admits only the two Fable source shapes
that independently reproduce the January bytes.

## Result

| Function | Padded bytes | Result | Normalized SHA-256 |
| --- | ---: | --- | --- |
| `_slayer_get_score_header_string` | 96 | strict exact | `5634f4fc63ca196bd01bfedc309a0b89d402b324ab777ee1adbe7fcb8e37a19a` |
| `_slayer_engine_display_score` | 640 | strict exact | `0c232f29dab93c8f31dcea0fae6d71a6024bef79360a36634aedbeb8c8e97b08` |

Final unit gate: **27 exact / 0 residual / 0 unwritten of 27**. The strict
campaign gain is two functions and 736 padded code bytes. The whole-project
progress report gains 710 meaningful code bytes (688,799 to 689,509), and the
object admission adds the exact 136-byte engine table (1,875,267 to 1,875,403
matched data bytes). Linked units rise from 384 to 385.

The header-string implementation uses an ordinary ternary text-access macro
matching the exact `GET_GAME_ENGINE_HUD_FORMAT` idiom in
`source/game/game_engine.c`. This naturally recovers January's branch-local
ESI schedule. The display formatter uses Fable experiment E2: an unconditional
typed `player_get` validation followed by a separately initialized target
pointer, the authenticated pre-switch new-target block, and the original
switch topology. It is source-equivalent to Fable E1 and compiles to the same
640 exact bytes without retaining a misleading unused local.

## Engine table and linkage reconciliation

The previous Fable ledger deliberately left its exact `_slayer_engine` table
in scratch because the then-current `game_engine.h` declared the
`player_killed_player` slot with the wrong callback type. Canonical now has the
correct four-argument type, so the table is admitted with no cast. HCEX PDB
linkage identifies every engine callback as file-private; all 23 callbacks are
therefore `static`, their declarations are removed from the public owner
header, and `config/symbols.json` records their real January linkage.
`update_speed_for_score` remains the one authenticated external helper.

The complete `_slayer_engine` table is 136 bytes with 24 relocations and
normalized SHA-256
`cd47b29508fea4e4c11451792c17f629a9ad240b3c3df5c889c0d416c378bc78`.
All normalized bytes, relocation addresses, relocation types, destinations,
and addends match. The only COFF presentation difference is the first string
relocation: csplit names `??_C@_06CBFFIGEC@slayer?$AA@` as an undefined image
symbol, while MSVC owns the identically named select-any string COMDAT in the
candidate object. This benign ownership difference is recorded fail-closed in
`config/semantic_data_matches.json`. `_slayer_globals` remains independently
exact at 128 bytes, zero relocations, SHA-256
`38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca`.

With both code residuals closed and data proved, the two stale parks are
removed and `source/game/game_engine_slayer.c` is promoted from `NonMatching`
to `Matching`.

## Evidence reviewed

- Preserved Fable artifacts:
  `scratch/workers/game_engine_slayer_vE1.c`,
  `game_engine_slayer_vE2.c`, `game_engine_slayer_vTABLE.c`,
  `game_engine_slayer.log`, and
  `game_engine_slayer_table_experiment.py` under the original Fable scratch
  tree above.
- Fable's complete
  `game_engine_slayer_obj_fable_small_families_20260901.md` ledger and the
  canonical Slayer public-leaf, score-quartet, and target/update ledgers.
- HCEA reconstruction donors
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\halocea\src\slayer_engine_display_score.c`,
  `slayer_get_score_header_string.c`, and `find_next_target.c`, used for names
  and semantic topology rather than as byte authority.
- January target disassembly, relocation graphs, callback-table slots, exact
  sibling source shapes, and HCEX PDB names/signatures/linkage.

## House-rule audit

- All private functions have semantic authenticated names and real `static`
  linkage; no `code_<address>` or `bss_<address>` placeholder remains in the
  Slayer source or symbol map for this unit.
- The only public function prototype remains in the dedicated closest owner
  header. No broad header or unrelated translation unit is changed.
- Existing typed `player_get` / `unit_get` accessors, `cseries` scalar types,
  named enum constants, and the typed Unicode tag group are retained.
- No handwritten offset access, fake dependency, duplicate condition,
  volatile/register optimizer control, pragma trick, inline assembly,
  undefined-behavior carrier, or nonsensical matching construct is present.
- Parameter formatting, `(void)` no-argument formatting, and explicit terminal
  returns follow the campaign rules. No function is forced inline and no
  `point_from_line3d` COMDAT is emitted.

## Verification

- `python tools/campaign/gate.py source/game/game_engine_slayer --all`:
  27 exact, 0 residual, 0 unwritten.
- Frozen-base versus candidate full-tree exact-set sweep: zero losses; only
  `_slayer_get_score_header_string` and `_slayer_engine_display_score` gained.
- Full 572-unit Ninja compilation, semantic report, progress report, and link
  accounting pass. Semantic accepted exact count is 5,223 with zero unit
  errors.
- `python -m pytest -q -p no:cacheprovider`: 258 passed.
- `python tools/fake_match_scan.py source/game/game_engine_slayer.c --fail-on-findings`:
  zero findings.
- Park audit: 106 active, zero stale, zero invalid; no Slayer parks remain.
- Object admission audit after promotion: zero candidates, zero contradictions,
  and zero Slayer rejection/revocation.
- `git diff --check` passes; both config JSON files parse; no candidate-only or
  target-only text symbol exists.

## Integration order

Cherry-pick the source-only salvage commit first, then this reconciliation and
admission commit. The second commit intentionally depends on the corrected
four-argument callback slot already present in canonical `game_engine.h`.
No shared build configuration or unrelated source file is part of this lane.
