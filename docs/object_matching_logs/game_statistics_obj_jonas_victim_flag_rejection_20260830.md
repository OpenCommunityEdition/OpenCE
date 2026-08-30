# `game_statistics.obj` victim-flag donor packet (2026-08-30)

## Result

This packet started from local campaign commit
`191721f7144b07bd12ee3366a66accada20d8ebe` and investigated the sole
non-exact owner in `source/game/game_statistics.obj`:
`_game_statistics_record_kill`.

The one fresh, authenticated-donor candidate was rejected and the production
source was restored byte-for-byte.  The object remains **3/4 functions exact**
(`176/848` padded bytes).  The candidate did prove a useful narrower result:
the HCEA-style two-valued victim flag, used to index `kills[]` and `assists[]`,
legally fixes January's independent credited-kill scheduling window without a
barrier.  It does not change the earlier ESI/EDX allocator choice, so the whole
owner is still non-exact and no source change is retainable.

## Evidence and provenance reviewed

The existing exhaustive ledger was read before editing:

- `docs/object_matching_logs/game_statistics_obj.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- `tools/c2dbg32/IR_LAYOUT.md`, including its later correction withdrawing the
  old physical-register interpretation of node `+0x28`

The full commit history for `source/game/game_statistics.c`, all surviving
game-statistics refs/worktrees recorded by the main ledger, and the three
distinct historical source families were recensused.  No unintegrated exact
object or new source family was found.  In particular, the punpckhdq/Stian and
Pastudan copies do not contain a body usable as a byte donor.

The newly useful authenticated donor was read in full at:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-cseries-20260820\src\blam\game\game_statistics_record_kill.c`

Its duplicate under `_research_cache/halocea` was byte-identical.  Unlike the
current January reconstruction, that donor gives the live-player path a
two-valued flag (`NONE` or zero), then uses the surviving zero as the statistics
array index.  This is consistent with the already-recorded May 2004 Sapien
topology and supplies a real, defined-C alias/scheduling premise that E01-E46
did not test.

## Frozen baseline

The strict baseline was snapshotted before the experiment in
`build/game_statistics_regression_manifest.json`.

| Owner | Padded size T/B | Relocs T/B | Target normalized SHA-256 | Baseline normalized SHA-256 | State |
|---|---:|---:|---|---|---|
| `_game_statistics_start` | `16/16` | `1/1` | `4eb29bc953165a2d51bef0bb3528cd233ddf8a287aaf2beb597b6c4e135976ce` | same | exact |
| `_game_statistics_stop` | `144/144` | `6/6` | `8557a20c7974bd29c33d99a8cd1404b45e3a2211bd9d27c47f47c90502f64a5a` | same | exact |
| `_game_statistics_record_damage` | `16/16` | `0/0` | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` | same | exact |
| `_game_statistics_record_kill` | `672/672` | `22/22` | `faed51b584f65f1a0f97f4416a0f599800dceeb2352a80dcf565d92d37724a69` | `ee9e334b928000272d26bfd08725d09579bb7e258685b0074a5608c54fa704f0` | non-exact |
| `_game_statistics_active` | `1/1` | `0/0` | `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d` | same | exact datum |

All 22 relocation addresses, types, symbolic targets, and addends were exact at
baseline and remained exact in the candidate.  The relocation addresses are
`7, 26, 48, 56, 62, 75, 139, 159, 170, 186, 262, 363, 394, 401, 419, 466,
489, 518, 577, 584, 602, 647`.  The only target-owned datum is the exact one-byte
`_game_statistics_active`; the whole-TU snapshot also froze every non-code
section and symbol owner.

The two baseline code windows were:

```text
+0x67
target:   xor esi,esi; stores SI to [eax+0x92] and [eax+0x94]
baseline: xor edx,edx; stores DX to [eax+0x92] and [eax+0x94]

+0x1AE
target:   inc [esi+0x98]; movsx ecx,[esi+0x96]
baseline: movsx ecx,[esi+0x96]; inc [esi+0x98]
```

The functions reconverge immediately after each window.

## E47: authenticated two-valued victim flag

The bounded candidate added a natural `long dead_player_flag`, assigned it
`NONE` for an absent player and zero for a live player, retained the existing
live-player body behind `dead_player_flag != NONE`, and changed only the two
statistics array indexes:

```c
if (dead_player_index == NONE)
	dead_player_flag = NONE;
else
	dead_player_flag = 0;

if (dead_player_flag != NONE)
{
	/* existing body */
	credited_player->statistics.kills[dead_player_flag]++;
	/* ... */
	assisting_player->statistics.assists[dead_player_flag]++;
}
```

This is ordinary typed C89.  It introduces no intrinsic, barrier, volatile
access, register hint, address/offset trick, type pun, undefined behavior, or
synthetic unused dependency.

The candidate compiled to `672` padded bytes with all `22` relocations still
exact.  Its normalized hash was
`726db672a9d0b8eba834f038f71f84584c3ef76a7279e538064ae020f898b94c`,
the previously observed legal-code shape R.  It made the entire later window
exact:

```text
+0x1AE candidate/target:
66 ff 86 98 00 00 00     inc   word ptr [esi+0x98]
0f bf 8e 96 00 00 00     movsx ecx,word ptr [esi+0x96]
```

The early window remained baseline-exact rather than target-exact:

```text
+0x67 target:    33 f6 / 66 89 b0 ... / 66 89 b0 ...   (ESI/SI)
+0x67 candidate: 33 d2 / 66 89 90 ... / 66 89 90 ...   (EDX/DX)
```

Thus the donor flag provides a legal scheduler dependency/alias shape, but it
does not alter the private register-allocation tie.  The strict regression gate
reported all three inherited exact functions `still_exact`, the kill owner
`changed_nonexact`, no failures, and no warnings.  Because no whole owner became
exact, the candidate was rejected and `source/game/game_statistics.c` was
restored to blob `e0f219cf23fc879ed277c23bcf1ef2557a7c83ca`.

## Restored-state validation

- The focused whole-TU regression check is clean: all three inherited exact
  functions are `still_exact`, the restored kill owner is unchanged non-exact,
  and there are no failures or warnings.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` gates pass.
- The semantic audit covers 470 units and 4,957 functions: 4,841 semantic
  exact, 4,851 accepted exact, and zero unit errors.
- Object admission reports zero candidates, contradictions, and revocations.
- Park validation reports 12 active entries, zero stale, and zero invalid.
- `python -m pytest -q` reports 212/212 tests passing.
- The strict board remains 277/619 exact objects, 667,863/1,922,669 padded
  bytes, and 4,823/8,246 exact functions.
- `git diff --check` passes.  The Units sentinel is unchanged at Git blob
  `c000b070f97406b480ae33d7e7f6f11def3ae69b`.

## Claude-ready boundary

The remaining blocker is now only the three ESI/EDX encoding bytes at offsets
`+0x68`, `+0x6E`, and `+0x7E` within `_game_statistics_record_kill` (the opcodes,
store order, sizes, branches, data, and all relocations agree).  January reuses
the just-freed ESI value class for a shared short zero; XDK 3911 currently
coalesces that zero with EDX.  The donor-derived victim flag fixes the unrelated
`+0x1AE` schedule but leaves this tie unchanged.

Do not repeat E47, the victim-flag `kills[]`/`assists[]` indexing family, or any
E01-E46 family from the main ledger.  Do not reintroduce `_ReadWriteBarrier`:
E47 proves the schedule can be recovered in defined C, while neither spelling
solves the allocator residue.  Useful next evidence must be one of:

1. original Bungie source or an exact January-family donor that exposes the
   value graph feeding the reset zero;
2. a genuinely new, readable typed-C lifetime/coalescing premise demonstrated
   against the corrected compiler internals; or
3. a corrected map of the XDK 3911 integer register assignment/coalescing pass.

The compiler-RE handoff must start from the correction section in
`tools/c2dbg32/IR_LAYOUT.md`: earlier text claiming `0x10715873` writes a
physical register is withdrawn; it writes an operand width class.  Repeating
the former preferred-register breakpoint family cannot resolve this tie.

Absolute production pointers for Claude/Fable:

- source:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\game-allegiance-closeout-20260830\source\game\game_statistics.c`
- exhaustive prior ledger:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\game-allegiance-closeout-20260830\docs\object_matching_logs\game_statistics_obj.md`
- this E47 ledger:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\game-allegiance-closeout-20260830\docs\object_matching_logs\game_statistics_obj_jonas_victim_flag_rejection_20260830.md`
- authenticated donor:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-cseries-20260820\src\blam\game\game_statistics_record_kill.c`
- corrected compiler notes:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\game-allegiance-closeout-20260830\tools\c2dbg32\IR_LAYOUT.md`
