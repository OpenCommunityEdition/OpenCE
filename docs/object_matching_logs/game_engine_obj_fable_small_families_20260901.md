# `game_engine.obj` small-families wave (2026-09-01)

> Historical Fable worker ledger.  The reconciled source, header ownership,
> honest parks, full-tree regression result, and correction to this ledger's
> `/W3 /WX` claim are recorded in
> `game_engine_obj_fable_salvage_admission_20260902.md`.

## Scope and provenance

- Translation unit: `source/game/game_engine.c` (only file edited besides this
  ledger and `scratch/workers/game_engine*`).
- Target object: `build/split/source/game/game_engine.obj` (January 2002 split,
  byte authority). Candidate objects were compiled by
  `tools/campaign/gate.py` only; `build/base` was not rebuilt.
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1 with the checked-in build.ninja
  flags (`/O2 /Oy- /DDEBUG /Dxbox`). No flag, header, config or symbols.json
  change was made.
- Strict oracle: `tools/coff_compare.py` `section_infos_equal` (padded bytes,
  normalized bytes, relocation count/identity) through `gate.py`.
- Evidence used: the target disassembly (`gate.py --disas`), `scratch/xrefs.py`
  relocation lists, the January assert strings, the exact sibling functions in
  this TU (`netgame_verify_equipment`, `multiplayer_message`,
  `game_engine_player_is_out_of_lives`, `game_engine_update`), the prior
  GE18 checkpoint source `e311fa706078abeb2ed8cc7b556909cc1de68c51`
  (read-only via `git show`, kept at `scratch/workers/game_engine_e311.c`) whose
  six residual bodies were target-equal in size/relocations, the HCEA
  reconstruction cache (`halocea/src/blam/game/*.c`, names/semantics only), and
  the prior ledgers `game_engine_obj.md` (GE19-GE31 do-not-repeat packets),
  `game_engine_obj_codex_checkpoint.md`, the 2026-08-30/31 Jonas ledgers, the
  player-added family and oddball-predicate ledgers.

## Validated baseline

`python tools/campaign/gate.py source/game/game_engine` before any edit:
`== exact 166  residual 8  unwritten 6  (of 180 listed)`;
`board.py`: `25856/33760 B, 166/180 fn`.

UNWRITTEN at baseline (padded bytes): `_game_engine_should_spawn_player` 256,
`_game_engine_get_state_message` 320, `_game_engine_update_item_spawn` 336,
`_game_engine_get_place` 368, `_find_closest_player_index` 448,
`_game_engine_player_killed` 832. The eight residuals and the PARKED map
verifier were not touched.

## Result

Final whole-unit gate on the installed `source/game/game_engine.c`:
`== exact 169  residual 11  unwritten 0  (of 180 listed)`. All 166 baseline
exact functions remain exact; the eight pre-existing residual lines are
byte-for-byte the same as at baseline. `CL /W3 /WX` compiles the unit clean.
`python tools/fake_match_scan.py source/game/game_engine.c` reports 0 leads.

Newly exact (target padded bytes / relocations / normalized SHA-256, identical
on both sides):

| Target symbol | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_game_engine_get_state_message` | 320 | 10 | `20ff545777ef3592b92e0608f9abc73f010429f15a655e4a3645f1ef1842b831` |
| `_game_engine_update_item_spawn` | 336 | 11 | `7e6f5319e160462229c51753194e9aa87f9ce25176cb9f2239cb508aaa885ee8` |
| `_game_engine_player_killed` | 832 | 39 | `4d4a8b42b57d29707d911f742b640cbb3e9783404a2bc369cabb7e73760e15a9` |

Three further bodies are retained as honest fuzzy reconstructions (target-equal
size and relocation count, normalized bytes differ): `_find_closest_player_index`
(448/448, 18/18), `_game_engine_should_spawn_player` (256/256, 9/9) and
`_game_engine_get_place` (368/368, 12/12). They receive no credit.

## Accepted source shapes

### `game_engine_get_state_message` (exact)

The GE18 body (`e311`) manually expanded the first `multiplayer_message_internal`
call into `game_engine->format_message(...)` plus `internal_rasterize_score(...)`
and left the other two as calls; its residual was the shrink-wrapped prologue
(GE21). Writing all three arms as ordinary calls to the existing static
`multiplayer_message_internal` is the natural source: XDK 3911 inlines the
first call by itself (the January bytes show the inlined format-message /
internal-score sequence there and out-of-line calls for the other two arms),
and the callee-save placement then matches the target. No inline attribute or
schedule change was needed; the helper keeps its target-emitted out-of-line
body.

### `game_engine_update_item_spawn` (exact)

The target loop is the same idiom as the exact sibling `netgame_verify_equipment`:
`struct scenario *scenario = global_scenario_get();`, a `short equipment_index`
`for` loop bounded by `scenario->netgame_equipment.count`, and
`TAG_BLOCK_GET_ELEMENT(&scenario->netgame_equipment, ...)`. This reproduces the
`lea ebx,[eax+0x384]` / `movsx eax,ax` / spilled-index tail that the GE18
`long` index + `do/while` + `(short)` cast form could not (GE23). The
item-collection respawn time is read through the TU-local typed
`struct item_collection_definition` (`respawn_time` at `+0xC`, the same offset
the GE18 body dereferenced raw) via `item_collection_definition_get`; the
at-rest flag uses `_object_at_rest_bit` (bit 5) from `objects.h` instead of a
bare `5`. The spawned object is fetched with `item_get` (`_object_mask_item`
= 0x1C as pushed by the target).

### `game_engine_player_killed` (exact)

The GE18/GE19c/GE31 residual was pure block layout: the quitting-player
broadcast block sat at the end of the function instead of between the direct
betrayal message and the event-5 broadcast. The retained source keeps the
event computation and `game_show_score_extended` inside
`if (!dead_player->unknown_d1) { ... } else { broadcast 0x1C to every player }`
with `long event_type = NONE;` initialised at declaration and **no** `return`
in the else arm; the event-5 / event-4 handling follows the if/else as
`if (event_type == 5) ... else if (event_type == 4) ...`. XDK 3911 threads the
`NONE` value through both tests (the quit arm returns directly, as in the
target) and places the three out-of-line blocks in the January order. The
callback at `game_engine + 0x60` is invoked through a locally typed function
pointer because `game_engine.h` still declares that slot as
`void (*unknown60)(void)`; see the header request below.

### Fuzzy bodies retained

- `find_closest_player_index`: GE18 search body; tail written as
  `if (best_object_index == NONE) { search; if (found) return convert; }
  else return convert; return best_object_index;`.
- `game_engine_should_spawn_player`: `boolean should_spawn`, an if/else-if
  chain whose `deaths == 0` arm carries the time-slot check and whose final
  `else` decrements the respawn timer (with the countdown sounds) and then
  applies the same time-slot check when the timer expired. XDK 3911
  cross-jumps the two checks into the single January block; the whole control
  flow, prologue, exits and relocations now match.
- `game_engine_get_place`: GE18 body unchanged.

## Experiment matrix

Sizes are ours/target, relocations ours/target. "sibling regression" was
checked with the whole-unit gate at v10 and on the final install (none).

| ID | Function | Source shape | Size | Relocs | First divergence | Decision |
|---|---|---|---:|---:|---|---|
| v1 M1 | get_state_message | three plain `multiplayer_message_internal` calls | 320/320 | 10/10 | none | **EXACT, retained** |
| v1 F1 | find_closest | GE18 search + single outer `if (best != NONE) return convert; return best;` | 432/448 | 17/18 | one conversion call only (no fast-path copy) | rejected |
| v1 K2 | player_killed | `if (!unknown_d1) {compute; show} else {broadcast; return;}` then event tests | 832/832 | 39/39 | 0x13d branch target: quit block still last (`[C][D][B]`) | superseded |
| v1 S1 | should_spawn | `boolean`, if/else-if chain, single `if (should_spawn && time)` after it | 256/256 | 9/9 | 0x09 `xor al,al` + shrink-wrapped esi/edi; folded FALSE exit; `mov bl,1`/`cmp eax,1` | superseded |
| v1 P1 | get_place | GE18 body | 368/368 | 12/12 | 0x18 register choice; `tied`/`all_tied` byte slots swapped; `test dl,cl` vs `and dl,cl` | retained fuzzy |
| v2 I1 | item_spawn | sibling `short` for-loop idiom, typed collection struct, `_object_at_rest_bit` | 336/336 | 11/11 | none | **EXACT, retained** |
| v2 E6 | find_closest | inner `if (found) return convert;` + outer `if (best != NONE) return convert; return best;` | 448/448 | 18/18 | 0x12 `push ebx` sunk into search block; failure folded to `-1` | rejected |
| v3 E3 | find_closest | inner early return; `else return convert;`; `return best;` | 448/448 | 18/18 | 0x1a1 failure `or eax,-1` before fast conversion (edx) | retained fuzzy |
| v4 K3 | player_killed | K2 with `switch (event_type)` | 816/832 | 39/39 | body shrank | rejected |
| v4 S2 | should_spawn | S1 with `long should_spawn` | 272/256 | 9/9 | dword boolean ops | rejected |
| v4 P2a | get_place | rename `tied` -> `a_tied` | 368/368 | 12/12 | no change (slots inert to names) | rejected |
| v5 K12 | player_killed | K2 with `long event_type = NONE;` and no `return` in the quit arm | 832/832 | 39/39 | none | **EXACT, retained** |
| v5 P2b | get_place | rename `all_tied` -> `z_all_tied` | 368/368 | 12/12 | no change | rejected |
| v6 P4 | get_place | declare `all_tied` before `tied` | 368/368 | 12/12 | no change | rejected |
| v7 P6 | get_place | `boolean all_tied, tied;` | 368/368 | 12/12 | no change | rejected |
| v8 P8 | get_place | `SET_FLAG(..., 1, all_tied && tied)` | 384/368 | 12/12 | branchy AND | rejected |
| v9 P5 | get_place | initialise `tied` before `all_tied` | 368/368 | 12/12 | store order only, slots unchanged | rejected |
| v10 | whole unit | E3+K12+I1+M1+S1+P1 | | | `== exact 169 residual 11 unwritten 0`, no regression | checkpoint |
| v11 S7 | should_spawn | FALSE conditions in one compound `if`, timer block guarded by `deaths != 0 &&` | 272/256 | 9/9 | second `deaths` test not threaded | rejected |
| v11 E7 | find_closest | inner early return + `return best;` inside block, `else return convert;` | 448/448 | 18/18 | same tail as E3 | rejected |
| v12 E8 | find_closest | HCEA-style early return inside the autoaim block, unconditional search | 432/448 | 17/18 | conversions merged | rejected |
| v13 P10 | get_place | `all_tied &= tied;` before the flag builder | 384/368 | 12/12 | +16 bytes | rejected |
| v14 P11 | get_place | `all_tied = all_tied && tied;` | 384/368 | 12/12 | +16 bytes | rejected |
| v15 P12 | get_place | `if (!tied) all_tied = FALSE;` | 384/368 | 12/12 | +16 bytes | rejected |
| v16 S10 | should_spawn | `deaths == 0` arm carries the time check; final `else` does timer + guarded time check | 256/256 | 9/9 | 0x2c `mov bl,1` vs `mov ebx,1`; 0x99 `cmp eax,1` vs `cmp eax,ebx`; decrement in ecx vs eax | retained fuzzy |
| v16 F12 | find_closest | `closest_player_index` result local, single `return` | 448/448 | 18/18 | same tail as E3 | rejected |
| v17 S13 | should_spawn | S10 with `long should_spawn` | 256/256 | 9/9 | `xor ebx,ebx`, `mov ebx,0`, `test ebx,ebx`; compare still immediate | rejected |
| v18 S19 | should_spawn | S10 without the `respawn_timer` local | 256/256 | 9/9 | identical to S10 | rejected (inert) |
| v19 S20 | should_spawn | diagnostic only: compare timer with `should_spawn` | 256/256 | 9/9 | identical to S10 (XDK folds the known constant) | never a candidate |
| final | whole unit | E3+K12+I1+M1+S10+P1 installed in `source/game/game_engine.c` | | | `== exact 169 residual 11 unwritten 0`; `/W3 /WX` clean; fake scan 0 | retained |

## Residual classification (functions touched, not exact)

- `_find_closest_player_index` (E3, 448/448, 18/18): identical through
  `0x1a0`. Target orders the tail as [fast-path conversion via EAX][failure
  `mov eax,[ebp-4]`]; every natural spelling tried (F1, E3, E6, E7, E8, F12)
  either merges the two conversion calls or folds the failure return to
  `or eax,-1` and allocates EDX for the fast conversion. Same class as the
  GE19d-i packet: tail block layout / constant folding tie.
- `_game_engine_should_spawn_player` (S10, 256/256, 9/9): control flow,
  prologue, exits, both `game_time_get` blocks and all nine relocations match.
  Residual is the January register pin of the constant 1 (`mov ebx,1` reused
  by `cmp eax,ebx`) plus the resulting `dec eax` vs `dec ecx`; XDK 3911 folds
  the known constant into an immediate for every legal spelling (boolean, long,
  compare-against-variable diagnostic). Same class as GE24 and the campaign
  "zero-register pinning" residual.
- `_game_engine_get_place` (P1, 368/368, 12/12): the two byte locals land in
  the opposite `[ebp-1]`/`[ebp-2]` slots and `all_tied & tied` is fused into
  `test dl,cl` instead of an early `and dl,cl`; names, declaration order,
  comma declaration and initialisation order are all inert; every form that
  materialises the AND separately grows the function by 16 bytes. Same class
  as GE25: stack-slot allocation tie.

## Do-not-repeat

- get_state_message: do not reintroduce the hand-expanded first arm; the plain
  calls are exact.
- item_spawn: do not go back to `long` index / `do-while` / `(short)` cast
  (GE23) or raw `+0xC` dereference.
- player_killed: do not add `return;` to the quit arm or move the event tests
  inside the `!unknown_d1` block (that restores the GE19c/GE31 layout); do not
  use `switch` (816 bytes).
- find_closest: F1, E3, E6, E7, E8, F12 above plus GE19d-i.
- should_spawn: S1, S2, S7, S13, S19 above plus GE24a-f; do not try
  `respawn_timer == should_spawn` as a "lever" (S20 shows it folds anyway, and
  it is nonsense).
- get_place: P2a/b, P4, P5, P6, P8, P10, P11, P12 above plus GE25a-c.

## Reopen criteria

Reopen a fuzzy body only with first-party source, a new target-backed local
lifetime/type lead, or a demonstrated legal-C control for the identified tie
class (tail-block order + constant fold; constant register pin; byte-slot
assignment). Do not reopen with `volatile`, `register`, pragmas, barriers,
attributes, flags or storage overlays.

## Name provenance

All six target symbols already carried their names in `config/symbols.json`
(imported as authenticated January/HCEA names); no rename was needed and no
`code_`/`bss_` identifier was written.

| Target symbol | Source name | Provenance |
|---|---|---|
| `_game_engine_get_state_message` | `game_engine_get_state_message` | unchanged (symbols.json; HCEA same name) |
| `_game_engine_update_item_spawn` | `game_engine_update_item_spawn` | unchanged (symbols.json; HCEA same name) |
| `_game_engine_player_killed` | `game_engine_player_killed` | unchanged (symbols.json; `game_engine.h` prototype; HCEA) |
| `_find_closest_player_index` | `find_closest_player_index` | unchanged (symbols.json; January atlas) |
| `_game_engine_should_spawn_player` | `game_engine_should_spawn_player` | unchanged (symbols.json; `game_engine.h` prototype) |
| `_game_engine_get_place` | `game_engine_get_place` | unchanged (symbols.json; HCEA) |

## Header requests (not applied; headers are frozen for workers)

- `source/game/game_engine.h`: retype the `0x60` callback slot from
  `void (*unknown60)(void);` to
  `void (*player_killed)(long killing_player_index, long damage_type, long dead_player_index, boolean friendly_fire);`
  so `game_engine_player_killed` can drop its local function-pointer cast.
- `source/game/game_engine.h`: add prototypes for the public
  `game_engine_get_state_message` and `game_engine_get_place` (the latter needs
  `struct game_engine_place { short flags; short place; };` moved from the .c).

## Disposition

`game_engine.obj` advances from 166/180 to 169/180 strict exact with zero
regressions. It stays active and `NonMatching`; the three retained fuzzy
bodies are parked candidates for the orchestrator, not credit.
