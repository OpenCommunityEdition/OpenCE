# `game_engine_oddball.obj` Fable salvage and admission audit (2026-09-02)

## Verdict

The preserved Fable snapshot was reconstructed against canonical commit
`fca0dd5d7103a7cb331106c61a86dc26e8c84356`, then reconciled to the current
house rules. The isolated result is **38/38 strict-exact functions**, with no
residual or unwritten functions. It owns **4464/4464 padded code bytes** and
**4109 meaningful code bytes**. An address-normalized audit found no loss
from the canonical baseline and measured gains of **25 functions, 4192
padded bytes, and 4021 meaningful bytes**.

All target-owned data now compares exact: **11 sections / 736 bytes**. The
canonical baseline already owned two sections / 302 bytes; this wave adds
nine sections / 434 bytes, including the 136-byte `_oddball_engine` table.
The object is an admission candidate, but this isolated worker intentionally
did not set `Matching` because the requested consolidated full build/link is
reserved for the integrating owner.

## Scope and provenance

- Unit: `source/game/game_engine_oddball.c`.
- January target: `build/split/source/game/game_engine_oddball.obj`, raw
  SHA-256
  `70aa145fd07f88ee32489a708e7b01f521555bb2751621bc19d4919373a856e1`.
- Canonical base: `fca0dd5d7103a7cb331106c61a86dc26e8c84356`.
- Preserved Fable input: `ca732fa1d2116be09f76714e898c3b07db3ada94`.
- Snapshot import commit: `e0359ebe8504b86604fd30676890179a38f9b4ce`.
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1 with the configured January
  flags, measured through PID-local `tools/campaign/gate.py` objects only.
- The repository contains no `CLAUDE.md` or `AGENTS.md`. The audit read the
  three campaign/matching documents, both prior Oddball ledgers, and the
  preserved Fable ledger in full before changing source.
- Semantic evidence came from January bytes, relocations and string anchors;
  January `_oddball_engine` table slots; HCEX PDB symbol/linkage records; the
  HCEA Oddball source family; and exact sibling idioms in CTF, Slayer,
  `game_engine.c`, `objects.c`, and `stack_memory_pool.c`. HCEA was used as a
  semantic donor only; January remained the byte authority.

## Address-normalized result

The audit maps target and candidate functions by the January final-image
`file_offset`, rather than by symbol spelling. Therefore the semantic symbol
renames below cannot appear as false gains or regressions.

| Metric | Canonical baseline | Reconciled | Gain |
|---|---:|---:|---:|
| Exact functions | 13 | 38 | 25 |
| Padded code bytes | 272 | 4464 | 4192 |
| Meaningful code bytes | 88 | 4109 | 4021 |
| Exact target-owned data sections | 2 | 11 | 9 |
| Exact target-owned data bytes | 302 | 736 | 434 |

Lost function addresses: none. Lost target-owned data addresses: none.

The 25 gained functions are:

| Address | Function | Padded | Meaningful |
|---:|---|---:|---:|
| `0xA16E0` | `oddball_add_score` | 208 | 200 |
| `0xA17B0` | `oddball_add_time_with_ball` | 64 | 57 |
| `0xA17F0` | `player_ball_count` | 48 | 36 |
| `0xA1830` | `player_has_ball` | 48 | 45 |
| `0xA1860` | `ball_available` | 64 | 49 |
| `0xA18A0` | `oddball_engine_display_score` | 736 | 724 |
| `0xA1BD0` | `oddball_get_score` | 64 | 59 |
| `0xA1C10` | `oddball_ball_transfer_by_killing` | 32 | 23 |
| `0xA1C30` | `accumulate_score_by_time` | 32 | 19 |
| `0xA1C50` | `terminator_scoring_rules` | 32 | 19 |
| `0xA1C70` | `oddball_test_flag` | 32 | 32 |
| `0xA1C90` | `oddball_test_trait` | 80 | 74 |
| `0xA1CE0` | `oddball_get_score_string` | 80 | 80 |
| `0xA1D30` | `oddball_get_score_header_string` | 112 | 101 |
| `0xA1DA0` | `oddball_get_team_score_string` | 80 | 75 |
| `0xA1DF0` | `find_position_for_ball` | 320 | 312 |
| `0xA1F30` | `create_the_ball` | 144 | 140 |
| `0xA1FC0` | `oddball_engine_initialize_for_new_map` | 288 | 282 |
| `0xA20E0` | `reset_ball` | 112 | 105 |
| `0xA2150` | `update_ball_ownership` | 144 | 144 |
| `0xA21E0` | `oddball_engine_player_update` | 448 | 448 |
| `0xA23A0` | `oddball_engine_weapon_update` | 192 | 191 |
| `0xA2460` | `oddball_engine_update` | 208 | 204 |
| `0xA2530` | `oddball_engine_player_killed_player` | 448 | 435 |
| `0xA26F0` | `oddball_weapon_pickup` | 176 | 167 |

## Data result

- `_oddball_globals`: exact 260-byte BSS, no relocations; normalized
  SHA-256
  `9c0095c04ef53d9df41602f3783c90ef3c3e27cc9d0b38262d23930db6313f5a`.
- `_oddball_engine`: exact 136-byte initialized table, 27 relocations;
  normalized SHA-256
  `ffe1ef0aa827017276299f3a497655019d91f94374129c31634061500e5ae4b9`.
- Eight target-owned assertion/UI string sections add 298 exact bytes. The
  eight-byte `"oddball"` literal is pooled as a discardable COMDAT and is not
  target-owned by this split unit, so it is correctly excluded from the
  address-owned 736-byte total.

## Reconciliation and naming

- Every address placeholder in this unit was replaced with a semantic name.
  Table callback names and signatures come from the January engine-table
  slots and the Oddball family; helper/global names use HCEX/HCEA provenance
  recorded in
  `game_engine_oddball_obj_fable_small_families_20260901.md`.
- `_oddball_globals` and every callback/helper reached only inside this
  object are file-static. `player_has_ball` and `ball_available` remain
  external because both are HCEX public symbols.
- The object now emits the complete `struct game_engine oddball_engine`
  table rather than relying on an absent data owner.
- Variant fields have descriptive TU access macros; switches use named
  Oddball/game-engine enums; source uses project `boolean`, `real`, datum,
  flag and typed-access conventions. There are no raw `tag_get` or
  `object_get` casts, raw bit arithmetic, address names, or primitive
  `float`/`double` declarations.
- The TU uses the established `game_engine_place.h` owner. Two runtime
  functions defined by `game_engine.c` are declared in the narrow
  `game_engine_runtime.h`; `game_engine.c` includes that header so the owner
  sees its declarations.
- No-argument functions spell `void` on its own line, parameters are one per
  line, and every function has an explicit terminal return. No function was
  forced inline and no assembly was added.
- The January inline schedule is unchanged. Candidate-only code owners are
  empty and no `point_from_line3d` symbol or COMDAT is emitted.

## Authentic target quirks retained

Two odd-looking shapes were kept only because January directly proves them
and HCEA independently supports their semantics:

- `find_position_for_ball` returns its untouched result after the fatal
  missing-spawn assertion. This is an original defect, not a manufactured
  match.
- `oddball_engine_player_killed_player` re-queries transfer-by-killing inside
  an arm already gated by the same mode. January and HCEA both retain it.
- `oddball_engine_initialize_for_new_map` begins with a discarded
  `global_scenario_get()` result because January contains that call and
  relocation.

The connected-to-map test in `oddball_engine_weapon_update` remains a named
`boolean` local. That ordinary C shape matches January's `shr/test` sequence
and has exact precedents in `objects.c` and `stack_memory_pool.c`; flattening
it into the surrounding `&&` changes VC7 instruction selection.

## Header blast-radius experiment

An initial attempt placed the two runtime prototypes in shared
`game_engine.h`. A targeted compile of all 21 direct/transitive consumers
showed that this semantically harmless declaration-position change reopened
`units.obj::_unit_preprocess_node_orientations` (189 to 188 exact functions).
That experiment was rejected completely.

With `game_engine.h` restored byte-for-byte and the declarations isolated in
`game_engine_runtime.h`, the same 21-unit sweep stayed at **879 exact
functions before and after**, with no changed unit. A targeted
`game_engine.obj` gate likewise stayed at **169 exact / 11 residual / 0
unwritten**. This is the relevant cross-unit regression proof for the only
shared-source addition.

## Verification

- Final Oddball gate: **38 exact / 0 residual / 0 unwritten**.
- Address-normalized baseline comparison: no lost code or target-owned data
  identities.
- Aligned normalized disassembly of the three largest/stateful checks:
  `oddball_engine_display_score` 258/258 instructions with no diff block;
  `oddball_engine_player_killed_player` 158/158 with no diff block;
  `oddball_engine_weapon_update` 65/65 with no diff block.
- Targeted header blast-radius sweep: 21 units, 879 exact before and after,
  no changed unit.
- Tool tests: **258 passed**.
- Fake-match scan over the Oddball source, runtime header, and owner include:
  **0 review leads**.
- Diff policy check: pass.
- Canonical parked audit (read-only): 113 active, 0 stale, 0 invalid.
- Canonical admission audit (read-only): 0 candidates, 0 contradicted, one
  pre-existing unrelated rejection, 0 revoked.
- No full `ninja`, canonical edit, or push was performed by this isolated
  worker.

## Residual and admission state

There are no fuzzy residuals to park and no unwritten functions. The
integrating owner should cherry-pick both commits, run the consolidated full
build/link and exact-set audit, then set the unit `Matching` if that combined
verification remains clean.

Do not move the runtime prototypes back into shared `game_engine.h`; the
January VC7 allocator is declaration-position-sensitive and the measured
Units regression is real. Also preserve the exact natural-C shapes cataloged
in the prior Fable Oddball ledger rather than replacing them with compact
predicate returns or flattened conditions.
