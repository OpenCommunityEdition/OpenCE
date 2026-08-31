# `game_engine_slayer.obj` score quartet integration (2026-08-31)

## Result

Starting from canonical commit `8e658a818`, this packet integrates four small
unwritten Slayer score functions and the translation unit's 128-byte score
state.  It uses the productive part of Claude campaign commit `e02971dfd` as a
lead, then re-derives and gates each owner against the current canonical tree;
the donor translation unit is not copied wholesale.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_slayer_engine_initialize_for_new_map` | `34 / 48` | `4` | `4cdb016e0e1e11cf93bcf595cf19d24fe3bd520053ee29f592c538cc413b55a7` |
| `_slayer_get_score` | `59 / 64` | `4` | `4d6aed81aac168da1427d10c8ba620b0db70bfbcfbbdfe4c0a6e80cf195560a9` |
| `_slayer_get_score_string` | `42 / 48` | `3` | `d7bf56e34d7291483a7a7c46f4749b2e08f932499e433b33b43347204fe7ff11` |
| `_slayer_get_team_score_string` | `37 / 48` | `3` | `7114bd24ec12efa5d7fb5cbe0ab7007c0d0d197d8742aea419d60e52abbdf61a` |
| **Code gain** | **`172 / 208`** | **`14`** | |

The external `_slayer_globals` owner is also exact at 128 zero-filled bytes,
zero relocations, and normalized SHA-256
`38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca`.
Both target and base emit it as an external section symbol (`storage 2`), not a
COFF common symbol.

`game_engine_slayer.obj` advances from **14 exact / 0 residual / 13
unwritten** to **18 exact / 0 residual / 9 unwritten**, and from 272 to 480
strict padded code bytes.  A rename-aware whole-tree verdict comparison
reports the four `U -> E` gains and zero unexpected transitions.

## Source credibility and ownership

The retained source is direct typed C:

- new-map initialization clears the two 16-entry score arrays and returns
  `TRUE`;
- score lookup obtains the typed player owner, selects the team table only for
  `_get_score_team`, and otherwise masks the datum index through the canonical
  `DATUM_INDEX_TO_ABSOLUTE_INDEX` macro; and
- both formatting callbacks pass the selected score to `usprintf` with the
  target-owned `L"%d"` string and return their destination buffer.

The workspace HCEA corpus independently supplies the names, typed behavior,
`get_score_type` contract, `slayer_globals` topology, and engine-table slots in
`src/blam/game/slayer_*`, `src/headers/get_score_type.h`,
`src/headers/slayer_globals.h`, and `src/data/slayer_engine.c`.  January's
split object remains authoritative for the PC ABI, storage class, bytes,
relocations, array order, and function boundaries.

The global is represented as the natural owner-local structure
`team_score[16]` followed by `individual_score[16]`, with a compile-time
0x80-size check.  An explicit zero initializer is retained so VC7 emits the
actual external `.bss` section January owns rather than a tentative COMMON
record.  This matches the existing `game_engine_oddball.c` global-owner style;
it is not a byte array, alias, cast, or linker exception.

Fourteen pre-existing exact address-named callbacks in the same unit are
renamed to their independently recovered Slayer names at the configuration
source of truth and in the source definitions.  All fourteen remain exact.
The global alias is likewise renamed from `_bss_0043ed80` to
`_slayer_globals`.  No newly touched owner remains named as code/data/BSS plus
an address.

## Shared callback ABI

`game_engine.h` now exposes the authentic 32-bit `enum get_score_type` rather
than a one-byte `boolean`.  The latter compiled `_slayer_get_score` as a
residual because January compares a dword argument.  The three score-format
callbacks now correctly return `wchar_t *`; January and the recovered engine
tables return the destination buffer in EAX.

The structure size and all callback offsets remain unchanged.  Every direct
`game_engine.h` C consumer was compiled through a PID-local gate, protected
`units.obj` remained **189 / 0 / 0**, and the full dependency rebuild also
compiled the transitive CTF, King, render, players, main, random-math, and
game-engine consumers.

## Deliberate exclusions

The remaining nine functions are left unwritten.  In particular,
`slayer_engine_adjust_score` is not admitted as an isolated non-static helper:
January gives the private function an EAX/EDI internal calling convention that
VC7 reproduces only when a genuine caller is present.  Its natural kill-flow
caller still depends on the unfinished `find_next_target` and
`update_speed_for_score` cluster.  That cluster must be recovered together,
not manufactured with a public ABI, forced calling convention, dummy caller,
or source-shaping attribute.

No raw offsets, representation punning, hand-written bit logic, assembly,
pragma, forced inline/noinline, volatile/barrier, fake dependency, dead branch,
semantic exception, parked record, or completion label is added.

## Validation

- `tools/campaign/gate.py source/game/game_engine_slayer --all`:
  `18 exact / 0 residual / 9 unwritten`;
- direct strict gates for the quartet: exact at padded sizes 48, 64, 48, and
  48, with all 14 relocations identical;
- direct data comparison for `_slayer_globals`: exact 128 bytes, zero
  relocations, external storage on both sides;
- all ten direct `game_engine.h` consumers compile through isolated gates with
  no newly lost exact function; the two pre-existing `dead_camera` residuals
  and one pre-existing `items` residual remain unchanged;
- full configure/Ninja cache build, libcmt build, progress, and semantic
  progress: pass;
- rename-aware whole-tree strict verdict diff: four gains, zero regressions;
- semantic audit: 470 units, 5,037 functions evaluated, 4,929 semantic exact,
  4,939 accepted exact, and zero unit errors;
- campaign total: 628,823 / 2,198,102 meaningful code bytes and
  4,896 / 11,060 accepted functions;
- Halo total: 615,909 meaningful code bytes and 4,729 functions;
- exact data total: 1,866,670 bytes, a gain of 128 bytes;
- parked-function validation: 25 active, zero stale, zero invalid;
- object-admission state unchanged: one `player_rumble` ownership candidate and
  the known `action_converse` contradiction/revocation;
- tooling tests: 222/222 pass;
- changed-file fake-match scan: zero review leads;
- `config/symbols.json` parses and `git diff --check` passes.

No push is performed by this packet.
