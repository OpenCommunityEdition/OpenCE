# `game_engine_king.obj` — LAW Z park re-investigation (Opus5 250K lane, wave n5, 2026-09-16)

## Result

Scope: `source/game/game_engine_king.c` only. The single non-exact row is parked, the re-investigation did not
reach strict EXACT, so the real file is **unchanged** (`git diff --stat` empty) and no credit is claimed.

| function | January padded / relocs / sha16 | real-file candidate | outcome |
| --- | --- | --- | --- |
| `_king_calculate_hill_state` | 448 / 32 / `9c852f781c6a691a` | 448 / 30 / `e9d7a30d3245b8ff` | **NOT-LANDED** (best scratch 448 / 30 / `36aa8dce51998e31`) |

Baseline real-file gate (with the `_point_from_line3d` guard): `== exact 28  residual 1  unwritten 0`.
Final real-file gate: identical. Guard passed. `scratch/parkcheck.py`: `parks 1 drift 0`.
`tools/fake_match_scan.py`: 0 review leads. Owner census unchanged (nothing landed).

## Duplicate prevention

Read: the three earlier King ledgers (`_fable_salvage_20260902`, `_jonas_public_leaves_20260828`,
`_post_rasterize_codex_20260903`), the w2 ledger `game_engine_king_obj_opus5_150k_w2_20260914.md`, the
`config/parked.json` entry for this function, `scratch/w2/laws_w2.md` (section D lists
`_king_calculate_hill_state` as a spent "store merge" tie) and `scratch/w3/laws_w3.md`. The w2 lane's best scratch
body is `scratch/workers/game_engine_king/v2.c`; this wave reproduced it on the current (post-reopen) real file and
confirms the same fingerprint `36aa8dce51998e31`.

## What the January bytes say

`_king_globals+128` = `on_the_hill[]`, `+400` = `hill_state`, `+404` = `hill_controlled_count`,
`+408` = `hill_previous_controller`. Three "controlled" tails (teams blue, teams red, free-for-all) are merged by
VC7 into one block that keeps the new state constant in EAX. The entire residual is the counter's residency in
that merged block:

```
JANUARY                                    OURS
 a4  cmp dword [hill_state], eax            b7  cmp dword [hill_state], eax
 aa  jne 0xd4                               bd  jne 0xc6
 ac  inc dword [hill_controlled_count]      bf  mov ebx, dword [hill_controlled_count]
 b2  mov dword [hill_state], eax            c5  inc ebx
 b7  cmp dword [hcc], 0x12c                 c6  cmp ebx, 0x12c
 ...                                        cc  mov dword [hcc], ebx
 d4  mov dword [hcc], ebx ; jmp 0xb2        d2  mov dword [hill_state], eax
```

January keeps the counter in memory: a memory increment in one arm, a separate `mov [hcc],ebx` reset in the other
(teams at 0xd4, free-for-all at 0x18e), and a re-read for the `== HILL_CONTROL_TIME` test. Our VC7 **sinks both
arm stores to the join and phis the value into the zero-pinned EBX** — the reset value 0 is already in the pin, so
that arm becomes empty. That removes the separate reset store and the re-read, which are exactly the two missing
relocations (30 vs 32), and it is what puts the two extra defs on EBX in the LAW Z profile. January's code uses no
register for the increment at all, so the orchestrator's "January computes that global increment in a SCRATCH
register" premise does not hold.

## Shapes (5)

| # | shape | measurement |
| --- | --- | --- |
| s1 | per-branch counter declarations (teams `red_count`/`blue_count`, free-for-all `player_count`/`controller`) | 448 / 30 / `36aa8dce51998e31`; **prefix 0x00-0x89 byte-identical to January**, esi/edi roles now match, aligned diff rows 74 -> 48 |
| m8 | single shared uncontrolled tail at the bottom plus early `return;` in every other arm (January's block layout) | 464 / 33 — worse in size and relocations |
| m9 | teams blue/red unified through a `long new_state;` local (one source site for the controlled tail) | byte-identical to s1 |
| n1 | the `== HILL_CONTROL_TIME` test nested inside the increment arm | 512 / 37 — worse |
| n2 | free-for-all else-arm store order swapped (`hill_previous_controller` before `hill_controlled_count = 0`) | byte-identical to s1 |

s1 is the confirmed w2 `v2` shape; it fixes the LAW Z register-role permutation and the entry schedule, and it
is the right starting body for any future attempt. It is NOT landed: the function is parked and s1 is not strict
EXACT (R11 forbids a zero-credit park remeasure that is not tied to a reopen).

## Why this is not a source defect (measured census)

**Standalone lab** (`scratch/workers/n5_game_engine_king/l1.c`-`l4.c`, driven by `lab.py` with the unit's own
cflags). The SHIPPED spelling reproduces January's memory form exactly when the enclosing function contains no
zero-valued store (`lab_v1`, `lab_v2`, `lab_l`): `inc dword [hcc]` / `mov dword [hcc],0` /
`mov [hill_state],eax` / `cmp dword [hcc],0x12c`. Adding ANY zero-valued store elsewhere flips it to the sunk form
(`lab_h` = `hill_state = 0` alone, `lab_i` = `hcc = 0` alone); a non-zero store does not (`lab_j` = `hcc = 7`,
`lab_k` = `hill_state = 7`). Inverted polarity (`lab_c`), `+= 1` (`lab_g`), compare-before-state-store (`lab_b`),
a saved previous state (`lab_e`), the free-for-all two-store else arm (`lab_d`) and a `new_state` local
(`lab_one`) all sink.

**Flag census.** `/Ob0 /Ob1 /Os /Ot /Ow /Oa /Op /Oi- /Gy /GF /Og- /O1` on top of the unit's `/O2 /Oy-` — none
restores the memory form. This is not a flag-provenance row.

**Real-file mutation census** (`mutate.py`, `x_*.c`; semantics deliberately broken, diagnostics only):

| mutation | memory increments | size / relocs |
| --- | ---: | --- |
| m0 = s1 unmutated | 0 | 448 / 30 |
| m1 remove the teams uncontrolled arm | 0 | 432 / 28 |
| m2 remove the free-for-all uncontrolled arm | 0 | 448 / 32 |
| **m3 remove BOTH uncontrolled arms** | **3** | 432 / 31 |
| m4 both uncontrolled arms keep only `hill_state = king_hill_uncontrolled` | 0 | 432 / 29 |
| m5 remove the teams contested reset | 0 | 432 / 29 |
| m6 remove the whole free-for-all branch | 0 | 256 / 17 |

m3 still pins zero in EBX at January's exact offsets (`xor esi,esi` @0x0e, `xor ebx,ebx` @0x10,
`xor edi,edi` @0x24) and emits three separate `inc dword [hcc]` with no tail merge at all. So the sink is not
simply "a zero register exists"; it is driven by the
`king_globals.hill_state = king_hill_uncontrolled; king_globals.hill_controlled_count = 0;` blocks — and January's
bytes prove those blocks are in January's source (the shared block at 0x1ab/0x1b1, entered from 0x39, 0xde, 0x178
and from the `hill_previous_controller = NONE` store at 0x19f). Removing either arm alone does not stop the sink;
keeping only its `hill_state` half does not either.

Every remaining route to January's two relocations is a redundant zero store, a carrier variable, `volatile`, a
fake dependency or an operand permutation, i.e. brief section 5 prohibitions. Per the wave instruction ("if the
only route to the pin is an illegitimate store, report NOT-LANDED with the census"), this row is reported as
NOT-LANDED.

## Law proposed (measured here)

**ZERO-PIN STORE SINK.** With `/O2`, VC7 13.00.9254 rewrites `if (c) g++; else g = 0;` followed by a later read of
`g` into `t = phi(g+1, 0); g = t;` with `t` coalesced into the function's zero pin, whenever the function contains
another zero-valued store to a global (the store seeds the zero web; comparisons against zero do not). Without such
a store it keeps the memory form `inc [g]` / `mov [g],0` / reload. The rewrite removes one store relocation and one
load relocation and adds two defs to the pinned register. Detector: a January `inc dword ptr [<global>]` whose value
is re-read after an intervening store, against our `mov reg,[global]; inc reg`. Consequence for LAW Z: a pin whose
extra defs are a global read-modify-write of a counter that is reset to zero in a sibling arm is NOT necessarily a
source defect — check this sink first.

## Reopen criterion

Reopen only for a January-side artefact showing the counter statement is not a plain
`if (state == X) count++; else count = 0;` pair (an authoritative source/local record, or a compatible-compiler
donor whose global optimizer leaves a two-armed global store un-sunk while a zero pin is live), or an owner ruling
admitting a form the house rules currently forbid. Start from `scratch/workers/n5_game_engine_king/s1.c`
(448 / 30 / `36aa8dce51998e31`), never from the parked body; the w2 criterion ("a natural form that keeps
`hill_controlled_count` un-promoted") is now measured to be unsatisfiable in plain C with this compiler.

No header or config change is needed or proposed.
