> Research only: no admission or retained source/header/configuration change.

# Packet 7: infection swarm control, bounded primary-evidence study

Baseline `76dfc1fb779a082752550f795ca356d8c5f4b93f`. Scope was only
`source/ai/actor_type_infection.c::_infection_swarm_control`, 3,609 meaningful
January bytes, atlas similarity 63.40435%. **One new source shape; no landing,
no exact credit.** Production source is byte-for-byte unchanged. No header,
configuration, Ninja, commit, or target-metadata change was made.

## Frozen result

| Object | Swarm control padded bytes / relocations | Exact target functions | Disposition |
| --- | --- | --- | --- |
| January target | 3616 / 104 | target | authoritative |
| Current production, fresh gate | 3360 / 103 | 4 / 5 | unchanged |
| Prior n4 `v0`, freshly replayed | 3600 / 104 | 4 / 5 | old rejected research |
| New 32-bit flag accumulator | 3600 / 104 | 4 / 5 | rejected, research only |

The inherited exact functions remain `_infection_decide_action` (304 padded),
`_infection_swarm_aim_jump` (688), `_infection_wander_move_time` (96), and
`_infection_wander_pause_time` (96). The new shape preserves all 19 other code
sections relative to the fresh prior-v0 replay, including those four controls.
It preserves all 46 external/data owner records, 27 non-debug noncode sections,
and COMMON. Three compiler-local `$L` names/offsets change with control layout;
these are recorded explicitly, not silently equated to public/data owners.
The `_point_from_line3d` emission guard passes. Fake-source scan: zero leads.

The frozen production build and fresh baseline compile have equal function
sections, owners, noncode, and COMMON. Source SHA256:
`e894a7a6a01ed6800442438c638f7fc6700c4d4daed2be06195baf26f0bc603f`.

## Prior work and duplicate prevention

Read the helper/jump/census/owner ledgers (August 25, August 27, August 30,
September 9) and fresh-graphs, w1, w3c, n3a, n4 ledgers (September 14–15).
The low production similarity is not evidence of an undiscovered missing body.
The previously reconstructed `v0` already fixes member-loop `has_direction`
scope, first/third normalize inlining, aggregate stack homes, control-packet
stores and other known topology. It remains unlanded because it is nonexact.
The historical `v0` source/object were copied from
`C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/n4_actor_type_infection/`.

Do not repeat the broad branch/declaration census, helper respellings,
four-TRUE-exit w1 probe, or w3c `word control_flags = should_jump ? 2 : 0`.
The latter was byte-inert. Exact `_cross_product3d` in action_obey and exact
matrix construction already refute a blanket header commutation fix.

## Fresh primary evidence

`hcea-infection_swarm_control-sym.txt` and `-lines.txt` are fresh DIA queries
against the supplied 2011 HCEA PDB. They identify static function RVA 0x1827710,
length 0x1138, original source MD5 `428FF008D8342C51F720D0A9A38587E8`, and
typed locals `up_vector`, `movement_vector`, `vector_to_swarm` (real_vector3d),
`iterator` (prop_iterator), `control_data` (unit_control_data), and short
`aiming_speed` / `intermittent_action_index`. These records do **not** recover
the scalar control-flags name, signedness, or a January lexical block.
PPC register labels printed by DIA are not January x86 allocation evidence.

Fresh read-only Ghidra decompile/disassembly and independently decoded raw
Capstone bytes cover later function 0x481e60. The source assertion string is
`ai\\actor_type_infection.c`. Raw RTC descriptor 0x48377c at site 0x483751
contains ten local records: movement_vector 12, up_vector 12, iterator 8,
vector_to_swarm 12, left_vector 12, behind_point 12, left_vector 12,
vector_to_friend 12, left_vector 12, control_data 64. The raw re-read exactly
agrees with the earlier 2528-frame census. Distinct left-vector storage is
corroboration for existing aggregate lifetimes, not permission for arbitrary
scope changes.

The new fact is in **raw instructions**, not Ghidra's reconstructed C:

- 0x483599 writes zero to a **dword** local at EBP-0x1bc.
- 0x4835a3 tests the jump byte at EBP-0x6d.
- The two arms OR bit 2 / AND ~2 into that same dword and write it back,
  including the unused expression-result temporary characteristic of SET_FLAG.
- 0x4835e3..0x4835ee clear the separate 64-byte control packet.
- 0x4835f6..0x4835fd copy the local's low 16 bits into the packet flag field.

January independently converts its jump byte before `_csmemset` at offsets
0xcba..0xcce and retains the result in ESI. Thus a separate initialized 32-bit
flag accumulator is source-plausible and predicts the relevant lifetime.
The old reconstructed HCEA C donor erased this distinction into a ternary;
it was not treated as original source.

Executable SHA256:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
HCEA PDB SHA256:
`0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.
The executable is a 2020/v140 instrumented cross-build. Same-source-revision
identity with January is unverified; it also uses a later unit_control ABI.
Only the local operation compatible with January was tested.

## The one new source test

Starting from the known prior `v0`, `shape1-flags.c` adds only:

```c
unsigned long control_flags = 0;
SET_FLAG(control_flags, _unit_control_jump_bit, should_jump);
/* existing csmemset and packet initialization */
control.control_flags = (word)control_flags;
```

The local name is semantic, not debug-recovered. The 32-bit extent and
initialize/update/copy lifecycle are primary-backed. Unsigned long follows
existing unit/actor control-flag storage and avoids claiming unavailable
debug signedness. There is no artificial second use, wiped field assignment,
qualifier, helper replacement, header change, or branch invented for matching.

This test **changes code** and supplies a genuine missing source distinction:
the jump boolean is now caller-saved CL and is converted into ESI before the
call; the unit pointer uses EDI. That corrects the prior n3/n4 assertion that
only a fake use or dead store could make the boolean dead at memset.
It does not recover the full January register/branch schedule. January uses
AL for jump and CX for component flags; the new shape uses CL and AX, merges
the TRUE exits, and schedules the pre-call instructions differently. It
remains nonexact with the same 16-byte padded deficit.

The three existing x87 issues also remain: prop-distance term ordering,
normalize reciprocal lifetime at the first join, and up.i-only multiplication
commutation at three cross-product sites. No second or third shape is justified
by the new evidence. The saved candidate is a research result, not an admission
proposal. Existing strict comparison and target metadata were not changed.

## Reopening boundary and durable files

Reopen only with an additional authentic caller-side fact explaining the
remaining AL/CX and TRUE-exit context plus x87 clusters, or a same-compiler
source donor explaining the complete register web. Do not repeat the old
claim that a pre-call flag lifetime is intrinsically inadmissible: this packet
demonstrates an ordinary primary-backed source mechanism, but not exactness.
Do not apply this body to production for fuzzy improvement.

All files are under `scratch/astra-wave7/infection/`:

- `evidence.json`, `inspect_evidence.py`: hashes, RTC, complete code/data/owner
  census, strict control results, and negative disposition.
- `baseline.c`, `baseline-build.obj`, `baseline-fresh.obj`, `baseline-gate.log`;
  `january-target.obj`, `january-control.asm`.
- `prior-v0.c/.obj`, `prior-v0-fresh.obj`, `prior-v0-fresh-gate.log`;
  `shape1-flags.c/.obj`, `shape1-flags-gate.log`, `fake-scan.txt`.
- `hcea-infection_swarm_control-{sym,lines}.txt`, `raw-481e60.asm`,
  `ghidra_00481e60.{asm,c}`, `infection_source_xrefs.tsv`, `ghidra.log`, and
  the isolated Ghidra launcher/runner. The initial shared-project lock was
  resolved by waiting for the other reader; no lock was forced or removed.
- `*-aligned.txt` are diagnostic raw alignment outputs. They include literal
  ownership spelling and disassembled jump-table data, so their edit counts
  must not be equated to historical curated instruction counts or coverage.

`IDA_AUTOMATION_UNAVAILABLE`. All supplied local artifact avenues are closed;
the user confirmed no additional original January headers/full-symbol build.
There was no further artifact request. File ownership is released.
