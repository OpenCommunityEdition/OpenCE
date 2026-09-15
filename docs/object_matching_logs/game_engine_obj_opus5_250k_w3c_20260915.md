# `game_engine.obj` opus5 250K house-clean lane, wave w3c (2026-09-15)

## Scope and provenance

- Translation unit: `source/game/game_engine.c` (the only file this worker edited).
- Target: `build/split/source/game/game_engine.obj` (January 2002 Xbox debug split) - the byte authority.
- Compiler: XDK 3911 VC7 13.00.9254, checked-in `build.ninja` flags; every candidate compiled through
  `tools/campaign/gate.py --source` only. No ninja, configure, header or config edit.
- Lane: `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`.
- Ledgers read in full: `game_engine_obj.md` (GE01-GE31), `_codex_checkpoint`,
  `_fable_salvage_admission_20260902`, `_fable_small_families_20260901`, the three `_jonas_*` ledgers,
  `_opus5_fresh_graphs_20260914`, `_opus5_100k_waveC_20260914`, `_opus5_150k_w1_20260914`, plus the w1
  worker notes (`scratch/workers/game_engine.md`) and the waveC/w1 scratch candidates.
- Laws applied as detectors over every non-exact row and the park: `scratch/w2/laws_w2.md` (A1-A22, R1-R12)
  and `scratch/w3/laws_w3.md` (A23-A45, R13-R19).
- Per-function worker blocks: `scratch/workers/w3c_game_engine.md`. Harness:
  `scratch/workers/w3c_game_engine/probe.py`; standalone compiler lab `scratch/workers/w3c_game_engine/lab.py`.

## Baseline and result

`python -B tools/campaign/gate.py source/game/game_engine --all --forbid-emitted-symbol _point_from_line3d`

```text
baseline  == exact 174  residual 6  unwritten 0  (of 180 listed)   guard passed
final     == exact 174  residual 6  unwritten 0  (of 180 listed)   guard passed
```

Row-by-row diff of the two `--all` listings, the only change:

```text
- residual   1152  _game_engine_generate_title_string  [size 1088!=1152, relocs 70!=73, sha]
+ residual   1152  _game_engine_generate_title_string  [sha]
```

No new strict-exact row (**newly_exact is empty**). Park drift 0/1, owner census PASS (41 inherited
candidate-only names equal `build/base`, none new, none missing), `tools/fake_match_scan.py` 0 leads,
`git diff --check` clean.

| Function | Outcome | Padded / relocs |
|---|---|---|
| `_game_engine_generate_title_string` | **FUZZY-LANDED** (zero credit, structural closure) | 1152 / 73 (target 1152 / 73) |
| `_drawline` | SKIPPED-EXHAUSTED (new mechanism recorded) | 112 / 7 |
| `_game_engine_post_rasterize_post_game` | NOT-LANDED (blocked on `_drawline`) | 2528 / 148 |
| `_game_engine_get_starting_location_rating` | SKIPPED-EXHAUSTED (EBX/EDI mirror) | 96 / 6 |
| `_default_starting_location_rate_function` | SKIPPED-EXHAUSTED (EBX/EDI mirror) | 176 / 10 |
| `_game_engine_should_spawn_player` (parked) | SKIPPED-EXHAUSTED (constant-register pinning) | 256 / 9 |

## Accepted control: the title-string team buffers are two `wchar_t[8]`

The previous body carried a hand-made storage union
(`union { wchar_t team_names[22]; struct postgame_statistic_entry entry; } branch_storage;` with
`&branch_storage.team_names[14]` for team 0) and per-arm `return`s. It was 1088/70 against a 1152/73 target
- 64 bytes and three call relocations short. The landed body removes the union entirely.

January's frame facts (`alndiff`, target offsets):

| Offset | Object | Evidence |
|---|---|---|
| `[ebp-0x330]` | `score_string[256]` | `lea eax,[ebp-0x330]` at 0x3d9 and 0x44b |
| `[ebp-0x130]` | `life_string[128]` | `lea eax,[ebp-0x130]` at 0xac/0xbd, terminator `mov word [ebp-0x32],0` |
| `[ebp-0x30]` | team 1 name buffer | `lea eax,[ebp-0x30]; push eax; push 1; call [ecx+0x54]` at 0x2d1 |
| `[ebp-0x20]` | `struct postgame_statistic_entry entry` (28 B) | 0x3ce, `rep movsd` ecx=7 at 0x3e8, 0x453 |
| `[ebp-0x14]` | team 0 name buffer | `lea edx,[ebp-0x14]; push edx; push 0; call [eax+0x54]` at 0x2c2 |
| `[ebp-4]` | `secondary_string` | stored at 0x22, reloaded at 0x31a/0x363/0x3aa/0x448 |

Frame `sub esp,0x330` = 512 + 256 + 44 + 4, so the two team buffers plus the entry cell occupy exactly
44 bytes with the entry cell (28 B) overlapping them. Measured allocator behaviour for this block pair:

- two **16-byte** buffers: the first one used is placed at `-0x14` (packed over the top of the
  free-for-all `entry` cell) and the second below the entry cell at `-0x30` - **exactly January**;
- `[8]`+`[14]` and `[14]`+`[8]` (waveC G2a/G2b - declaration order is inert): the 28-byte buffer shares the
  28-byte `entry` cell at `-0x20` and the 16-byte buffer drops to `-0x30`, i.e. the two teams swap slots;
- `[14]`+`[14]` (waveC G2c): frame 0x33c.

So `wchar_t team0_name[8]; wchar_t team1_name[8];` is the unique measured fit, and it is the natural
spelling: two same-sized buffers, one per team, each handed straight to `game_engine->format_team_name`.

The control flow is waveC's G2i shape, which the SINGLE-EXIT LAW (brief section 9) and January's bytes
require: every postgame arm falls into a shared `ustrncpy` + `title_string[79] = 0` + epilogue tail that
VC7 tail-duplicates into each arm (the previous per-arm `return` body could not produce the three extra
`ustrncpy`/terminator copies, which is where the 64 bytes and 3 relocations were missing). HCEA
`game/game_engine_generate_title_string.c` uses the same `switch (did_win)` over the tri-state result.

Result: 1152/73, `relocdiff --count-by-target` 73/73 with 0 differing rows, frame and every stack slot equal
to the target, **391/391 instructions with a single transposed pair**:

```text
target 448  mov edx,[ebp-4] | 44b lea eax,[ebp-0x330] | 451 push edx | 452 push eax
ours   448  mov edx,[ebp-4] | 44b push edx            | 44c lea eax,[ebp-0x330] | 452 push eax
```

That is the last free-for-all `usnprintf`. January emits `mov; push; lea; push` (our order) at both team
`usnprintf` sites (0x31a, 0x3aa); the free-for-all site is the only one whose next argument is a
register-ABI static call (`get_place_string(&entry)`, `lea eax,[ebp-0x20]; call`). A standalone lab TU
(`scratch/workers/w3c_game_engine/l2.c`, `l3.c`) reproduces our order and shows VC7 hoisting a different
argument-input load into the same `mov`/`push` gap, so the remaining difference is a gap-filling scheduler choice on
an identical DAG, not a structural or store-order fact.

Landed under brief section 6 (structurally incomplete row, zero credit): the body is complete, natural,
strictly closer in every measure, and no sibling, park, owner or fake-scan status changes.

## Experiment matrix (this wave)

| ID | Function | Shape | Result | Decision |
|---|---|---|---|---|
| p1 | generate_title_string | G2i control flow + `team0_name[8]` / `team1_name[8]` | **1152/73, 391/391, 1 insn** | landed (as p2) |
| q1 | generate_title_string | p1 + `&score_string[0]` argument | identical | rejected |
| q3 | generate_title_string | p1 + swap `life_string`/`score_string` declaration order | identical (slots unchanged) | rejected |
| q4 | generate_title_string | p1 + `score_string` declared in the free-for-all block | identical (frame preserved) | rejected |
| p2 | generate_title_string | p1 minus a stray blank line | identical | **landed** |
| inc1 | whole unit | `#include "main/console.h"` (the C4013 at game_engine.c:3806) | all 180 rows identical | not landed (see below) |
| d1 lab | drawline | 12 ordinate spellings in one lab TU | 11 straight-line spellings identical to ours | rejected |
| l1-l4 lab | generate_title_string | free-for-all call shape in a standalone TU | reproduces our push order | diagnostic |

## Residual classification

- `_game_engine_generate_title_string`: argument-schedule tie (one transposed pair). Structural debt closed.
- `_drawline`: **new mechanism.** January computes `row*9` twice (`lea edx,[esi+esi*8]; shl edx,1` for `y0`,
  then `lea eax,[esi+esi*8]; lea ecx,[eax+eax+0x1a]` for `y1`); ours keeps `row*18` alive in EAX and adds
  0x1a. Twelve arithmetic spellings compiled in one lab TU (`d1.c`: `(row+1)*18+8`, `row*18+26`,
  `18*row+26`, `row*9*2`, `2*(row*9)+26`, named `short` ordinate, reversed statement order,
  `(short)(row*18)+26`, `(short)((row+1)*18)+8`, `row*18+18+8`, `(row*2+2)*9+8`) all emit our bytes, so the
  arithmetic spelling is not the lever. The real difference is where the `justification` argument load
  (`mov edx,[ebp+0xc]`) is scheduled: January puts it *between* the two ordinate stores, which frees the
  register holding `row*18` and forces the rematerialisation; ours issues it first, right after
  `offset_rectangle2d` returns. Class: argument-load scheduling / rematerialisation tie.
- `_game_engine_post_rasterize_post_game`: six inlined `drawline` copies carry the same rematerialisation
  gap, plus the EBX/EDI colouring of the hoisted 350/500 and 0/1 constants and the `entry_count` spill
  order. Best body remains waveC P22 (`scratch/workers/game_engine/pr_p22re.c`, 2528/148, 751/752
  instructions, frame 0x670 and every slot equal); it is not strict exact and the row is `[sha]` only
  (size and relocations already match), so section 6 does not admit a fuzzy landing.
- Rating pair: private-ABI mirror only - January keeps `player_index` in EBX, ours in EDI (plus one
  `mov eax` vs `mov ecx` in the callee). 46/46 and 72/72 instructions, everything else equal.
- `_game_engine_should_spawn_player` (parked): January widens the boolean TRUE store to `mov ebx,1` and
  then spends EBX as the 32-bit compare operand for `respawn_timer == 1`; ours writes `mov bl,1` and
  rematerialises `cmp eax,1`. Recurring constant-register pinning; no w2/w3 law explains it.

## C4013 census item (orchestrator lead)

`game_engine.c(3806): warning C4013: 'console_printf' undefined`. The genuine owner header is
`source/main/console.h` (`void console_printf(boolean clear, const char *format, ...);`). The call sits in
`_game_engine_update_teleporter`, which is **already strict EXACT** with the implicit declaration, and the
measured include (`scratch/workers/w3c_game_engine/inc1.c`, `#include "main/console.h"` before
`"main/main.h"`) leaves **all 180 rows identical**. It therefore unlocks nothing and is not landed here;
it is available as a byte-inert hygiene hunk if the orchestrator wants the warning gone (laws_w3 E18 class).

## Do-not-repeat (adds to the w1/waveC lists)

- generate_title_string: q1 (`&score_string[0]`), q3 (buffer declaration order), q4 (free-for-all-block
  `score_string`) - all byte-identical to p1; do not re-spin team-buffer sizes: `[8]`+`[8]` is the fit,
  `[8]`+`[14]`, `[14]`+`[8]`, `[14]`+`[14]` and `team_names[22]` are measured.
- drawline: the twelve `d1.c` lab spellings; no arithmetic spelling defeats the CSE.
- rating pair, should_spawn: no shapes spent; the recorded ties are unchanged and no law fires.

## Reopen criteria

- `_game_engine_generate_title_string`: a measured VC7 control that moves the `lea` of a stack array above
  the `push` of a spilled pointer when the next argument is a register-ABI call. Apply it to
  `scratch/workers/w3c_game_engine/p2.defs`; everything else is already byte-identical.
- `_drawline` / `_game_engine_post_rasterize_post_game`: a control that delays a stack-parameter argument
  load past a store to an escaped local (freeing the ordinate register). Close `_drawline` first, then
  re-gate P22.
- Rating pair: a callee register-pressure lever that moves an incoming private-ABI parameter from EDI to EBX.
- `_game_engine_should_spawn_player`: a measured control where a boolean TRUE store is materialised as a
  32-bit constant register reused by a later int compare.

## Disposition

Real file stays 174/180 strict exact (no credited bytes this wave). `_game_engine_generate_title_string` is
now structurally complete (size, relocations, frame, slots and 390 of 391 instructions equal to January) and
free of the storage union; the object stays `NonMatching` and active.
