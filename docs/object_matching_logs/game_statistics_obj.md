# `game_statistics.obj` exact-match strategy ledger

## Scope and toolchain

- Translation unit: `source/game/game_statistics.c`
- January target object: `source/game/game_statistics.obj`
- Clean starting point: `codex/campaign-integration` commit
  `7f947ee8f7bbd1472e1f1738db04b8d565821dbb`
- Resumed closeout checkpoint:
  `e49e6a654230a4d70ae1b44e12e7e60e2b064e07`
- Isolated worktree branch: `codex/game-statistics-finish`
- Compiler: XDK 3911 CL 13.00.9254.1 for 80x86
- `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`
- Repository flags, unchanged:
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`, followed by the generated repository
  include-path set
- Acceptance oracle: `tools/coff_compare.py`, including padded function bytes,
  relocation identity/type/address/target/addend, and owned data
- Regression oracle: `tools.regression_gate`, freezing exact functions and the
  complete translation-unit non-code/symbol-ownership shape

No experiment used inline assembly, `_emit`, volatile byte forcing, undefined
behavior, object-byte patching, comparator weakening, gratuitous alignment
pragmas, or compiler-flag changes.

## Superseding exact closure (2026-08-31)

The former register-allocation park documented below is superseded.  Fable
commit `79275995c484a2637aa5db570030d42a9e02685d` supplied two ordinary source
corrections, and both were independently re-applied and revalidated at
canonical checkpoint `c71441a02e26a238bcc5e8643a8f903b2dfa63b7`:

1. Initialize `attacker_index` before taking the `attacker` pointer.  The donor
   experiment swept all 360 legal orders of the six loop-setup initializers;
   this one adjacent swap was the unique exact order.
2. Preserve the natural array decay of the typed `short kills[4]` member in a
   block-local `short *kills`, then increment `kills[0]`.  This keeps the
   credited-kill source order without an intrinsic barrier: XDK 3911 cannot
   prove that the pointer increment is disjoint from the adjacent
   `last_kill_time` field, so it does not hoist that load ahead of the kill
   increment.

Both changes preserve the existing logic and names.  They add no cast, inline
assembly, intrinsic, volatile access, dead branch, fake dependency, or manual
byte control.  The pointer type follows directly from the authenticated
`struct game_statistics::kills` declaration in `source/game/game.h`; the local
has an ordinary semantic purpose and is not an address-named artifact.

`tools/campaign/gate.py source/game/game_statistics --all` reports all four
target functions exact.  A direct hardened `tools/coff_compare.py` invocation
over all four functions reports `all_equal: true`:

| Function | Meaningful / padded bytes | Relocations T/B | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_game_statistics_start` | `8/16` | `1/1` | `4eb29bc953165a2d51bef0bb3528cd233ddf8a287aaf2beb597b6c4e135976ce` | strict exact |
| `_game_statistics_stop` | `132/144` | `6/6` | `8557a20c7974bd29c33d99a8cd1404b45e3a2211bd9d27c47f47c90502f64a5a` | strict exact |
| `_game_statistics_record_damage` | `1/16` | `0/0` | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | strict exact |
| `_game_statistics_record_kill` | `661/672` | `22/22` | `faed51b584f65f1a0f97f4416a0f599800dceeb2352a80dcf565d92d37724a69` | strict exact |

The owned `_game_statistics_active` BSS datum is also strict exact: `1/1`
byte, zero relocations, normalized SHA-256
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`.
Baseline-versus-candidate whole-object fingerprints preserve all 30 symbol
records, every section owner, both exact sibling functions, `.drectve`, BSS,
both `.debug$F` sections, and both compiler-constant `.rdata` sections.  The
only `.debug$S` byte differences are the four decimal PID characters in the
gate's temporary object path (`_gate_39136.obj` versus `_gate_35768.obj`);
logical size, section inventory, and ownership are unchanged.

The stale source-level `NonMatching` comment and superseded park entry were
removed, and the translation unit was admitted as `Matching` after the
canonical verification pass.

## Exhaustive provenance review

The January target and all surviving project history were inspected before
settling the source:

- every ref whose name contains `game-statistics`, every reflog exposed by the
  surviving worktrees, and every commit touching this translation unit;
- all ten surviving game-statistics worktrees, including dirty state;
- the preserved `target.txt` and `base.txt` disassemblies and relocation
  reports;
- the sole surviving untracked experiment object;
- all distinct historical source blobs: the stub `2a535...`, the initial
  reconstruction `018aa9...`, and the tightened reconstruction `e0f219...`;
- the relevant history families: `ef7e421`/`0281a653` for the leaves,
  `17fbdf8` for stop, `813605ee`/`34d9bc5` for the first kill
  reconstruction, and `eb84038`/`2f67d985` for the tightened kill topology.

The untracked historical object under
`lanes/game-statistics-kill/work/game_statistics.obj` has strict normalized
hash
`0176f61946f7067fcad75d879d97af2a279e14a5c767f54d2732808e6d4dd563`.
It preserves the same `0x2A0` padded size and all 22 relocation identities but
has additional later code-generation differences, so it is strictly
superseded by the campaign baseline.

Two external provenance checks were also exhausted:

- The May 2004 HEK Sapien implementation at `sapien.exe:0x658300` has the same
  logical reset order and the same credited-kill order: increment the kill
  counter, then read the prior last-kill time. Its compiler allocation differs
  (EDI is the shared zero and ESI holds `NONE`), so it is corroborating source
  topology rather than a byte donor.
- `artifacts/circular-pdb-extract.json/pdb_symbols.json` and the surviving PDB
  expose labels and globals but no local-variable or statement provenance for
  this function. No original Bungie C or exact January donor survived in the
  research tree.

## Initial measured baseline

At the clean starting commit, three of four functions were strict exact: 176
of 848 padded `.text` bytes. The sole owned target datum,
`_game_statistics_active`, was also strict exact.

| Function / datum | Padded size T/B | Relocs T/B | Target normalized SHA-256 | Initial base normalized SHA-256 | Initial result |
|---|---:|---:|---|---|---|
| `_game_statistics_start` | `0x10/0x10` | `1/1` | `4eb29bc953165a2d51bef0bb3528cd233ddf8a287aaf2beb597b6c4e135976ce` | same | strict exact |
| `_game_statistics_stop` | `0x90/0x90` | `6/6` | `8557a20c7974bd29c33d99a8cd1404b45e3a2211bd9d27c47f47c90502f64a5a` | same | strict exact |
| `_game_statistics_record_damage` | `0x10/0x10` | `0/0` | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | same | strict exact |
| `_game_statistics_record_kill` | `0x2A0/0x2A0` | `22/22` | `faed51b584f65f1a0f97f4416a0f599800dceeb2352a80dcf565d92d37724a69` | `ee9e334b928000272d26bfd08725d09579bb7e258685b0074a5608c54fa704f0` | two residual windows, first at `+0x67` |
| `_game_statistics_active` | `0x1/0x1` | `0/0` | `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d` | same | strict exact |

All 22 kill-function relocation identities, types, addresses, symbolic
targets, and addends already agreed. The initial normalized residual was:

```text
+0x67:
target:    xor esi,esi; ... mov [eax+0x92],si; ... mov [eax+0x94],si
baseline:  xor edx,edx; ... mov [eax+0x92],dx; ... mov [eax+0x94],dx

+0x1AE:
target:    inc [esi+0x98]; movsx ecx,[esi+0x96]
baseline:  movsx ecx,[esi+0x96]; inc [esi+0x98]
```

Both windows reconverged immediately. The frame remained `0x28`, and every
branch, relocation, and instruction outside those windows agreed.

## Retained improvement and final object state

A Microsoft compiler intrinsic barrier immediately after
`credited_player->statistics.kills[0]++` is retained. It emits no machine
instruction. It only prevents XDK 3911 from moving the independent
last-kill-time load ahead of the January kill-count store. This restores the
entire `+0x1AE` window:

```text
target/current +0x1AE:
66 ff 86 98 00 00 00    inc word ptr [esi+0x98]
0f bf 8e 96 00 00 00    movsx ecx,word ptr [esi+0x96]
```

The final strict state is:

| Function / datum | Padded size T/B | Relocs T/B | Base normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_game_statistics_start` | `0x10/0x10` | `1/1` | `4eb29bc953165a2d51bef0bb3528cd233ddf8a287aaf2beb597b6c4e135976ce` | strict exact |
| `_game_statistics_stop` | `0x90/0x90` | `6/6` | `8557a20c7974bd29c33d99a8cd1404b45e3a2211bd9d27c47f47c90502f64a5a` | strict exact |
| `_game_statistics_record_damage` | `0x10/0x10` | `0/0` | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | strict exact |
| `_game_statistics_record_kill` | `0x2A0/0x2A0` | `22/22` | `726db672a9d0b8eba834f038f71f84584c3ef76a7279e538064ae020f898b94c` | non-exact; only three encoding bytes differ |
| `_game_statistics_active` | `0x1/0x1` | `0/0` | `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d` | strict exact |

The complete final normalized residual is:

```text
+0x67 target/current:
33 f6 / 33 d2                   xor esi,esi / xor edx,edx

+0x6E target/current ModRM:
b0 / 90                        mov [eax+0x92],si / mov [eax+0x92],dx

+0x7E target/current ModRM:
b0 / 90                        mov [eax+0x94],si / mov [eax+0x94],dx
```

Thus the residual is exactly three register-encoding bytes. Size, frame,
control flow, all 22 relocations, and every other normalized byte agree.
Objdiff reports 99.904305% for the function and 99.92113% for the unit, but
those percentages are navigation aids and do not grant match credit.

## Accepted controls

- `best_attacker_index` remains a `short`.
- The explicit attacker-pointer initialization, independent loop-index zero,
  saved `attackers` pointer, comparison operand order, and direct indexed
  attacker-field loads are retained because they are the strongest measured
  topology.
- The no-code `_ReadWriteBarrier` is retained because it is the only tested
  defined-C/compiler control that restores the independently corroborated
  January credited-kill schedule without changing size or relocations.
- The three exact function siblings and `_game_statistics_active` are frozen
  controls. Every experiment compiled the complete translation unit and
  checked all of them.
- Candidate-only compiler COMDAT constants, `.debug$S`, `.drectve`, symbol
  ownership, and all other non-code shape are protected by the whole-TU
  regression manifest even though only `_game_statistics_active` is an owned
  target data match.

## Experiment matrix

All experiments used the exact XDK 3911 command line. Unless marked retained,
each source change was reverted immediately after measurement. Every row
preserved all three exact function siblings and `_game_statistics_active`.

Hash keys:

```text
H   0176f61946f7067fcad75d879d97af2a279e14a5c767f54d2732808e6d4dd563
B   ee9e334b928000272d26bfd08725d09579bb7e258685b0074a5608c54fa704f0
R   726db672a9d0b8eba834f038f71f84584c3ef76a7279e538064ae020f898b94c
X06 3da9d27b5098cba3d3bc02a20860c4e204f06180a6fee10199e2601a807648c2
X07 75877bd0ddcbf909fbd4d7b4ef91422739a36596a7c8ae1ee44964f1b431d961
X15 93ca52f08c26f2e3ebf9c13ab5a7489fbdc01a9c011c87bfbdbf6658f9ec33e3
X16 7c36de0780bacf0361f34d0a518702e99fe42b00152a11ef161fa79b00cd7422
X17 ce42d38b3ae07eb62eb5699ddd878a2aaca1651fd6f351323ebd4bb28d4c9abd
X21 38ecf41b67e87513692566b4e102fed9f282519b819ac43d6cc77069840b575b
X30 e5554be8a69f294a4d59414ac496f0085021404946b6c1c60bc8ca064dd269e9
X31 a4008ddb77261c76573b6c7b92b1a5525a4f2d40464cb12e78eddcf266adea4e
X32 44213d19aff6e5104570adf1b4012a3e32609c32f7edcf0747ccf33201a1022f
X40 f46d7452246a313589da1b1f7303e4ce1d5569291f37e910cb1fda744a90c4a3
```

| ID | Source/control shape | Padded size T/B | Relocs T/B | Hash | First target divergence and effect | Decision |
|---|---|---:|---:|---|---|---|
| H00 | Preserved pre-tightening historical object | `0x2A0/0x2A0` | `22/22` | H | `+0x67`, plus additional later differences | superseded |
| B00 | Clean campaign baseline | `0x2A0/0x2A0` | `22/22` | B | `+0x67`; independent load/store transpose at `+0x1AE` | predecessor |
| E01 | One typed `short zero` used for both reset stores | `0x2A0/0x2A0` | `22/22` | B | compiler scalarized to the same EDX sequence | reverted |
| E02 | Duplicate `kills_in_a_row++` in both multiple-kill branches | `0x2A0/0x2A0` | `22/22` | B | common-code motion reproduced both residuals | reverted |
| E03 | Scoped `short last_kill_time` snapshot after the kill increment | `0x2A0/0x2A0` | `22/22` | B | no scheduling effect | reverted |
| E04 | Typed `struct game_statistics *statistics` credited subobject pointer | `0x2A0/0x2A0` | `22/22` | B | pointer folded; no code effect | reverted |
| E05 | Direct first team lookup plus distinct dead-player pointer lifetime | `0x2A0/0x2A0` | `22/22` | B | no allocation effect | reverted |
| E06 | Ternary assignment for `multiple_kills` | `0x2A0/0x290` | `22/21` | X06 | `+0x67`; commoned a `game_time_get` call and changed relocation topology | reverted |
| E07 | Inverted multiple-kill condition | `0x2A0/0x2A0` | `22/22` | X07 | `+0x67`; wrong branch topology later | reverted |
| E08 | Restore historical `long` killing-team parameter | `0x2A0/0x2A0` | `22/22` | B | compiler canonicalized to baseline | reverted |
| E09 | Swap attacker and attackers declaration order | `0x2A0/0x2A0` | `22/22` | B | no lifetime/allocation effect | reverted |
| E10 | Value-forward second reset from `kills_in_a_row` | `0x2A0/0x2A0` | `22/22` | B | no zero-register effect | reverted |
| E11 | `register short statistic_reset` | `0x2A0/0x2A0` | `22/22` | B | register hint ignored | reverted |
| E12 | No-code `_ReadWriteBarrier` after credited kill increment | `0x2A0/0x2A0` | `22/22` | R | `+0x67` only; `+0x1AE` becomes exact | retained |
| E13 | Additional early barrier after deaths increment, on top of E12 | `0x2A0/0x2A0` | `22/22` | R | no effect beyond E12 | extra barrier reverted |
| E14 | Early-declared typed statistic-reset local, on top of E12 | `0x2A0/0x2A0` | `22/22` | R | no zero-register effect | reverted |
| E15 | Split friendly-fire computation through a boolean | `0x2A0/0x2A0` | `22/22` | X15 | `+0x67`; introduced setcc/neg branch-shape differences | reverted |
| E16 | Explicit friendly-fire `if`/`else` assignment | `0x2A0/0x2A0` | `22/22` | X16 | `+0x67`; changed friendly-fire branch topology | reverted |
| E17 | Barrier between last-kill and multiple-kill reset stores | `0x2A0/0x2A0` | `22/22` | X17 | `+0x67`; moved comparison/store order away from target | reverted |
| E18 | Saved event dead-player index plus separate reset local | `0x2A0/0x2A0` | `22/22` | R | no effect beyond E12 | reverted |
| E19 | Reuse overwritten dead-player index as the reset zero | `0x2A0/0x2A0` | `22/22` | R | no effect beyond E12 | reverted |
| E20 | Defined attacker-null expression as an alternate zero lifetime | `0x2A0/0x2A0` | `22/22` | R | no ESI/EDX coalescing effect | reverted |
| E21 | Chained reset assignment | `0x2A0/0x2A0` | `22/22` | X21 | `+0x67`; reset-store order no longer matches | reverted |
| E22 | Preserve the event index separately, then zero `dead_player_index` with `&= 0` and use it for both reset stores | `0x2A0/0x2A0` | `22/22` | R | XDK constant-folded the self-update and still selected EDX | reverted |
| E23 | Same preserved-index topology, but zero `dead_player_index` with `^= dead_player_index` | `0x2A0/0x2A0` | `22/22` | R | canonicalized byte-for-byte to R | reverted |
| E24 | Same preserved-index topology, but zero `dead_player_index` with `-= dead_player_index` | `0x2A0/0x2A0` | `22/22` | R | canonicalized byte-for-byte to R | reverted |
| E25 | Same preserved-index topology, but zero `dead_player_index` with `*= FALSE` | `0x2A0/0x2A0` | `22/22` | R | canonicalized byte-for-byte to R | reverted |
| E26 | Split the block-local `dead_player_index` declaration and initialization | `0x2A0/0x2A0` | `22/22` | R | C89-style split initialization did not alter allocation | reverted |
| E27 | Move `dead_player_index` to function scope and assign it inside the active branch | `0x2A0/0x2A0` | `22/22` | R | wider source scope canonicalized byte-for-byte to R | reverted |
| E28 | Split the first `dead_player` and `dead_team_index` declarations from their assignments | `0x2A0/0x2A0` | `22/22` | R | declaration/assignment topology did not alter the early live ranges | reverted |
| E29 | Combine function-scoped `dead_player_index` with split player/team initialization | `0x2A0/0x2A0` | `22/22` | R | combined C89-style topology also canonicalized to R | reverted |
| E30 | No-code barrier after all three death-statistic reset stores | `0x2A0/0x2A0` | `22/22` | X30 | left EDX selected and delayed the `killing_player_index == NONE` compare from `+0x69` to `+0x80` | reverted |
| E31 | No-code barrier between the first zero reset and the `NONE` reset | `0x2A0/0x2A0` | `22/22` | X31 | left EDX selected and moved the compare from `+0x69` to `+0x70` | reverted |
| E32 | Initialize `attacker_index` at declaration, use it for both reset zeros, and carry that source value into the first attacker loop | `0x2A0/0x2A0` | `22/22` | X32 | widened lifetime reshuffled allocation from `+0x13`; reset zero was still EDX | reverted |
| E33 | Assign `attacker_index = 0` only for the resets, then assign it again before the loop | `0x2A0/0x2A0` | `22/22` | R | disjoint assignments were constant-folded to R | reverted |
| E34 | Initialize `friendly_fire = FALSE`, use it for both reset zeros, then overwrite it with the real predicate | `0x2A0/0x2A0` | `22/22` | R | disjoint boolean lifetime was folded to R | reverted |
| E35 | Initialize `credited_friendly_fire = FALSE` for the reset stores while retaining its later required initialization | `0x2A0/0x2A0` | `22/22` | R | disjoint boolean lifetime was folded to R | reverted |
| E36 | Take separate typed pointers to the two short reset fields and store through them | `0x2A0/0x2A0` | `22/22` | R | both legal field aliases folded away before allocation | reverted |
| E37 | Add the `register` storage-class hint to `dead_player_index` | `0x2A0/0x2A0` | `22/22` | R | hint ignored; exact R object | reverted |
| E38 | Qualify `dead_player_index` as `const` | `0x2A0/0x2A0` | `22/22` | R | qualifier did not affect the live range | reverted |
| E39 | Qualify `dead_player_index` as both `register` and `const` | `0x2A0/0x2A0` | `22/22` | R | combined qualifiers also produced R exactly | reverted |
| E40 | Reverse the suicide equality operands to `dead_player_index == killing_player_index` | `0x2A0/0x2A0` | `22/22` | X40 | emitted `cmp esi,ecx` instead of target `cmp ecx,esi`; reset zero remained EDX | reverted |
| E41 | Reverse the outer validity predicate to `NONE != dead_player_index` | `0x2A0/0x2A0` | `22/22` | R | predicate canonicalized to R | reverted |
| E42 | Reverse the friendly-fire sentinel predicate to `NONE == killing_player_index` | `0x2A0/0x2A0` | `22/22` | R | predicate canonicalized to R | reverted |
| E43 | Combine the E40 and E42 operand reversals | `0x2A0/0x2A0` | `22/22` | X40 | friendly-fire reversal had no interaction; retained the wrong suicide compare and EDX resets | reverted |
| E44 | Define the reset zero in both arms of the suicide branch and merge it through a `short` local | `0x2A0/0x2A0` | `22/22` | R | identical branch values folded before allocation | reverted |
| E45 | Express the suicide increment and reset-zero merge as a conditional/comma expression | `0x2A0/0x2A0` | `22/22` | R | conditional form canonicalized byte-for-byte to R | reverted |
| E46 | Feed the conditional merge with two independently defined self-subtraction zeros | `0x2A0/0x2A0` | `22/22` | R | both guaranteed-zero expressions and their merge folded to R | reverted |

E22-E25 were a bounded continuation batch. Each row was rebuilt with XDK
3911, measured by the strict comparator across all four functions, and checked
against the same-path whole-TU regression manifest. All four exact siblings
remained exact, `_game_statistics_active` and all frozen non-code/ownership
evidence remained unchanged, and the gate returned `ok: true` without warnings.

E26-E29 were a second bounded continuation batch with the same strict
comparator and whole-TU gates. All four shapes produced the exact R hash and
preserved every frozen sibling, datum, section, and ownership invariant.

E30-E31 were a third bounded continuation batch with the same gates. Both
barriers emitted no instruction and preserved size and all 22 relocations, but
each introduced a new scheduling difference while leaving the three EDX/ESI
bytes unresolved. The exact siblings and complete TU regression manifest still
passed.

E32-E35 were a fourth bounded continuation batch with the same gates. A true
cross-region attacker-index lifetime damaged the allocation topology, while
three disjoint source lifetimes disappeared before register allocation and
produced R exactly. No sibling, datum, or TU invariant regressed.

E36-E39 were a fifth bounded continuation batch with the same gates. Legal
typed field aliases and storage/const qualifiers were all erased or ignored
before allocation and produced R exactly. No sibling, datum, or TU invariant
regressed.

E40-E43 were a sixth bounded continuation batch with the same gates. Sentinel
predicate operand order canonicalized away; reversing the suicide operands
survived as the wrong `cmp` encoding but did not change the zero-register tie.
No sibling, datum, or TU invariant regressed.

E44-E46 were a seventh bounded continuation batch with the same gates. Even
when the zero was source-defined on both suicide-branch edges, XDK folded the
constant/phi before register allocation and produced R exactly. No sibling,
datum, or TU invariant regressed.

## Do-not-repeat list

- Do not return to `long best_attacker_index`, an address-taken selected
  attacker, eager `attackers` assignment, or the older loop family; the
  preserved historical object proves that topology is worse.
- Do not retry typed zero locals, reset-value forwarding, register hints,
  declaration-order swaps, saved event-index zeros, or defined null-derived
  zeros; XDK 3911 emits B or R unchanged.
- Do not retry compound self-zeroing of a preserved dead-player index (`&= 0`,
  self-XOR, self-subtraction, or multiplication by `FALSE`); XDK 3911
  constant-folds every form to R without preserving ESI affinity.
- Do not retry C89-style split initialization or wider source scopes for
  `dead_player_index`, `dead_player`, or `dead_team_index`; both isolated and
  combined shapes canonicalize byte-for-byte to R.
- Do not add barriers after the first or final reset store. They leave EDX in
  place and move the already-exact `killing_player_index == NONE` comparison.
- Do not carry the reset zero into the attacker loop through `attacker_index`;
  it changes allocation from the prologue onward. Reusing a separately
  overwritten attacker or friendly-fire variable has no effect and returns R.
- Do not retry typed pointers to the reset fields or `register`/`const`
  qualifiers on the dead-player index; all four variants produce R exactly.
- Preserve `killing_player_index == dead_player_index` operand order. Reversing
  it adds an independent compare-encoding mismatch, while reversing either
  sentinel comparison has no code-generation effect.
- Do not retry branch-edge or conditional-expression construction of the
  reset zero; identical constants and legal self-subtraction zeros are folded
  before allocation and produce R.
- Do not retry branch-local increment duplication or an explicit
  last-kill-time snapshot; neither affects the scheduler.
- Do not retry ternary/inverted multiple-kill logic, friendly-fire rewrites,
  chained reset assignment, or barriers in the reset window; each changes
  branches, relocations, or store order without resolving the allocator tie.
- Do not perturb exact siblings, target data, translation-unit headers,
  ownership metadata, or build flags to influence three ModRM/register bytes.
- The prohibited forcing techniques listed in the scope remain out of bounds.

## Residual classification

The scheduling component is solved and retained. The only residual is
methodology class C: a private register-allocation tie.

At `+0x67`, the earlier player-index ESI live range has ended and XDK may use
either ESI or EDX as the shared zero. EDX is overwritten by the next argument
load at `+0x85`, so both choices are semantically interchangeable. The tested
defined-C lifetime controls either canonicalize to EDX or damage independently
exact code. The continuation batches E22-E46 additionally cover compound reuse
of the ending ESI source variable, C89 scope/initialization changes, no-code
reset-boundary barriers, existing later-zero lifetimes, legal field aliases,
storage qualifiers, commutative predicate spellings, and branch-edge zero
merges. No remaining evidence-backed source-level dependency or lifetime
control selects ESI.

This is therefore a rigorous park, not a match.

## Verification gates

- Hardened comparator, all four functions plus `_game_statistics_active`:
  three functions exact, datum exact, kill `0x2A0/22` versus `0x2A0/22`,
  normalized base hash `726db672...`, and no residual outside the three
  encoding bytes.
- Same-path clean regression snapshot:
  `build/game_statistics_regression_manifest.json`, based on
  `7f947ee8f7bbd1472e1f1738db04b8d565821dbb`.
- Whole-TU regression check: `ok: true`, no failures or warnings; exact
  siblings are `still_exact`, `_game_statistics_record_kill` is correctly
  reported as `changed_nonexact`, and all frozen non-code/ownership evidence
  is unchanged.
- After E22-E46, the retained source was restored and rebuilt from the resumed
  checkpoint shape. The final comparator again reported kill hash
  `726db672...`, size `0x2A0`, 22 relocations, and only the three documented
  register-encoding bytes; the whole-TU gate again returned `ok: true` with no
  failures or warnings.
- A cross-worktree manifest was deliberately rejected because XDK embeds the
  absolute object output path in `.debug$S`. It was not waived; the clean
  baseline was regenerated in the same isolated path, after which the strict
  check passed.
- Consolidated default Ninja build: success. Csplit, objdiff report, semantic
  report, and progress completed; `unit_errors: 0`, and the progress verifier
  accepted all seven parked compiler ties.

## Reopen criteria

Reopen only for one of:

1. original Bungie C or January local-variable/statement provenance for
   `game_statistics_record_kill`;
2. an exact XDK 3911 donor containing the same dead-statistic reset live-range
   context; or
3. a newly demonstrated defined-C dependency/lifetime control not represented
   by E01-E46.

## Historical disposition and matching safety (superseded 2026-08-31)

`_game_statistics_record_kill` remains `NonMatching` and is recorded in
`config/parked.json` as a register-allocation fixed point.

Safe to mark `Matching` under the former candidate: **no**.

The equal size, relocation parity, three-byte residual, and semantic
equivalence are strong evidence for a compiler tie, but they are not
byte-for-byte identity and therefore are not match credit.

The superseding two-change candidate documented at the top of this ledger is
byte-for-byte exact and source-plausible.  It is safe for the canonical
orchestrator to accept once campaign metadata is updated under the normal
admission workflow.
