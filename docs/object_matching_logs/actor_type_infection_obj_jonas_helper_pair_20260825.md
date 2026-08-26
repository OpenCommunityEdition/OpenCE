# `actor_type_infection.obj` Jonas helper-pair wave (2026-08-25)

## Result and fixed boundary

This bounded wave starts from clean cumulative commit
`45db07ae0dbfea48e85f7c20b01efe5dff40f09f` in isolated branch/worktree
`jonas/actor-type-infection-helper-pair-20260825`. The first and only normal
code-producing compile makes `_code_000273b0` and `_code_00027410` strict
exact while preserving inherited `_infection_decide_action` strict exact.

The object advances from 1/5 to 3/5 exact functions, from 296 to 477 exact
meaningful code bytes, and from 304 to 496 exact padded code bytes. This wave
therefore adds exactly two accepted functions, 181 meaningful bytes, and 192
padded bytes. The naturally required typed caller `_code_00027470` remains
nonexact and receives zero credit; `_infection_swarm_aim_jump` remains absent.
`actor_type_infection.obj` remains `NonMatching`.

Only `source/ai/actor_type_infection.c` and this ledger are changed. The
approval-gated `units.obj` shared-header package remains frozen: no Units
shared header or protected neighbor (`source/units/vehicles.c`,
`source/ai/ai_debug.c`, `source/ai/ai_script.c`, or
`source/ai/actions.c`) was touched. It may be reopened only after the user
supplies the required exact approval phrase.

## Immutable provenance and tools

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `actor_type_infection.obj` SHA-256:
  `e4e984644dcb3ed6fc72ebdc73005321c6b17596b6f7e7f34d29bfefa0ed7a6d`.
- Baseline commit:
  `45db07ae0dbfea48e85f7c20b01efe5dff40f09f`.
- Baseline source blob:
  `379ed90968148e5bc484c4045f02ddc3ade36823`.
- Baseline object SHA-256:
  `701b5e28649e06478acb33cd25abed28ad869f979638d4283a4ff39d749bfa94`.
- Frozen baseline manifest:
  `build/audit/actor_type_infection_helper_pair_baseline_20260825.json`,
  226,585 bytes, SHA-256
  `6fb60256b4b8821b23cfec2b645dce4b2d2969537a2f4a93f4fa9fdbd42fe756`.
- Frozen retained source blob / SHA-256:
  `c2b037841dc6c0d0456a1bf26170ea6695b16d90` /
  `4cb50b7af2e3f07462f60fdec11afe73464ac79b12b8be4d990f12631f410ef1`.
- Frozen source-diff SHA-256:
  `84182405864b3d9a8adb4bdc3e821a449ddc023a`.
- First-shot object:
  `build/audit/actor_type_infection_helper_pair_first_shot_20260825.obj`,
  12,633 bytes, raw SHA-256
  `a250bfdb576e6f75806720d54ce7b0a7a553b9c389bde9a1d7d58d38b4956fd7`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86; `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural edge remains `/O2 /Oy- /DDEBUG /Dxbox` with the repository
  include paths.

Whole-object raw digests are artifact sentinels only. Admission authority is
the hardened comparison of padded and meaningful function bytes, relocation
address/type/resolved destination/addend, symbol ownership, and relevant
runtime non-code sections.

## Claude, HCEA, and January evidence audit

All reachable Claude documentation and tooling relevant to this object was
audited before candidate selection. The durable Claude branch
`claude/near-complete-objects-20260816` retains source blob
`379ed90968148e5bc484c4045f02ddc3ade36823`; its infection history includes
the semantic reconstruction commit
`507796ff3662a7b17211dcd491bfe53d65553fb7`. The earlier Claude/Codex lane
transcript is preserved at
`C:\Users\isabe\.codex\sessions\2026\07\14\rollout-2026-07-14T20-18-51-019f63c8-b809-7f71-b7bd-45f8d84264bc.jsonl`
(SHA-256
`9275ae1b4dd4440faf8005533bf5dfee7a17b7baee42c10de4211c40f58825a0`).
It established the helper source shapes and the requirement for internal
linkage plus a real same-TU caller.

The preserved historical helper object is
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\artifacts\actor_type_infection_helpers_20260825\actor_type_infection_helper_pair_exact.obj`,
12,800 bytes, SHA-256
`e558adcc4481529f3ffe028a47f6229b08e337fde1100250ff6cfac7b5c7bb2e`.
It is corroborating provenance, not independent campaign credit.

The clean typed HCEA donor is
`research-cache/halocea-cseries-20260820/src/blam/units/infection_swarm_control.c`
at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, blob
`90300b54edb9bda55fcff1712937171d8c7e1f7a`, and file SHA-256
`35767fcd6c67334129da9bf1bfbb8c2a198f0012e8b63991bb92c52f1c770f25`.
It supplied typed semantic names, record structure, and control-flow guidance;
it is a later-build reference and is not January byte, ABI, layout, or
admission authority.

January-specific semantics were checked against
`C:\Users\isabe\Downloads\halo-main (1)\halo-main\src\halo\ai\actors.c`,
SHA-256
`6fe3338fb0d2cea48bfb15de2b0b647529372a8c7a717103c0a227e02d7a4fc9`,
especially lines 2199-2739. That raw-offset donor was used only as disassembly
and behavioral evidence. No raw offset, address, or untyped donor expression
was retained.

Thus no Claude or HCEA artifact grants exact credit. Claude supplied durable
history, topology, census, and comparator/tooling evidence; HCEA supplied
typed later-build semantics; January COFF plus the hardened comparator remain
the sole admission authority.

## Rejected raw scratch and lawful natural closure

The earlier raw-offset reconstruction of `_code_00027470` emitted the exact
104-relocation set and caused both helpers to emit exact, but its caller was
3,552 padded bytes versus January's 3,616. Its first divergence was the stack
allocation (`sub esp, 0xe0` versus target `sub esp, 0xe8`), followed by
register-allocation and cooldown-layout differences. That raw caller was
deliberately rejected and never committed.

The accepted wave instead ports the January behavior through named repository
types. It introduces TU-local typed 0x14-byte wander control, 0x40-byte swarm
component, and 0x40-byte unit-control records with compile-time layout checks,
typed getters/APIs, and the established out-of-line `real_random_range`
include-renaming pattern. The full caller is retained only because VC7 `/O2`
eliminates unreferenced static definitions and a real same-TU caller is
required to emit the exact helper pair.

The typed caller is readable support code, not an admission:

| Function | Candidate padded / relocs / normalized SHA-256 | January padded / relocs / normalized SHA-256 | Decision |
| --- | --- | --- | --- |
| `_code_00027470` | `3360 / 103 / 8efce2e0c5b5bde12c28fdbe02450a62381c813ff68d65e7ed976128b0df7bdb` | `3616 / 104 / 64eacc5c1e2a60caa70303a25621254f64fb070f72e5efa693892a0de1f19fee` | natural support, nonexact, zero credit |

## Strict exact admissions

All hashes below cover relocation-normalized padded function COMDAT bytes.

| Function | January RVA | Meaningful / padded | Relocs | Normalized SHA-256 | Disposition |
| --- | ---: | ---: | ---: | --- | --- |
| `_infection_decide_action` | `0x00027280` | `296 / 304` | `26` | `eb35b33e81b16d9147e6a415a67e22f37770d7ce97b975a5da34f6dd88ee8a25` | inherited exact |
| `_code_000273b0` | `0x000273B0` | `95 / 96` | `5` | `b4122501a9598b4c8b4a44a337cef4596c77eb5d6007390dd3d65effdca398de` | newly exact |
| `_code_00027410` | `0x00027410` | `86 / 96` | `5` | `2b2b9cfbfa9787831c80e50121f343babcf3f5e6839eefab4eb0ca752c3fb90f` | newly exact |

The helpers reproduce all ten ordered relocation identities and addends,
including their literals, `get_global_random_seed_address`,
`real_seed_random_range`, and `__ftol2` destinations.

## One-shot and source-policy record

The frozen candidate passed an XDK `/Zs` parse-only check. The subsequent
Ninja dry run contained exactly one
`CL build/base/source/ai/actor_type_infection.obj` edge. That edge was invoked
once, and the emitted object was immediately preserved under `build/audit`.
No source spelling, candidate retry, scheduling tune, alternate compiler,
configuration change, or second production compile followed.

The retained source is ordinary typed C. It contains no assembly, `volatile`,
`register`, pragma/barrier, intrinsic/attribute, raw offset or address,
pointer/integer reconstruction, pointer or union pun, inactive-union access,
undefined behavior, synthetic anchor/caller, object-byte patch, comparator
exception, or alternate compiler flag.

## Data and ownership

The hardened non-debug target-data census has twelve sections. Seven are now
strict exact: the inherited ten-byte `"infection"` literal plus the source
path, two assertion strings, and three real-constant COMDATs naturally emitted
by the typed caller. No inherited exact data section is lost.

The 32-byte `_actor_type_infection` `.data` owner remains nonexact because its
ordered ownership-sensitive relocation fingerprint differs, even though both
sides have four relocations and the same normalized-byte hash. Four target
literals remain absent. Ten candidate-only support COMDAT functions and
fifteen candidate-only `.rdata` constants are natural closure expansion and
receive no credit. The object is incomplete, so this wave claims no data
progress and changes no source storage owner.

## Regression, admission, and progress validation

The generic pre-wave whole-TU manifest returned nonzero exactly as expected
for this natural physical expansion. It identified the two helper COMDATs as
`NEWLY_EXACT` and `_code_00027470` as `changed_nonexact`. It also reported
`_infection_decide_action` as `UNKNOWN` because adding the natural closure
changes VC7 local-label symbol numbering, even though direct hardened
comparison proves that function's normalized bytes and all 26 relocation
semantics remain identical. Target-owned caller literals, internal callback
symbols, the symbol set, physical sections, and debug metadata also expand as
expected.

That generic manifest deliberately fingerprints physical ownership and does
not grant new credit, so it is not used to waive any finding. The independent
hardened whole-object census proves the inherited exact function remains exact,
admits exactly the two named helpers, and reports no inherited exact
function/data loss. No regression adjudication or semantic exception is added
for this wave.

- Hardened census: 3/5 target functions strict exact, four emitted, one
  absent; 7/12 target non-debug data sections strict exact; inherited losses
  empty. Audit JSON is 10,450 bytes with SHA-256
  `54669265e76bd6342f1df8ef28b3d3b862b1d386408df020d7a8c3ff951ffdde`.
- Semantic audit: 470 units, 4,242 functions evaluated, 4,100 semantic exact,
  114 hidden exact / 64,705 hidden code bytes, 4,161 accepted exact, and zero
  unit errors. This is exactly +2 accepted functions.
- Ordinary-only review: 36 entries, 35 structural, one rejected.
- Campaign progress: 375/833 complete objects, 4,135/11,060 exact functions,
  502,099/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 complete objects, 3,968/7,574 exact functions,
  489,185/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Full Halo and libcmt graphs: pass; the dry run and real invocation both
  report no work, so the first-shot object is not rebuilt.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source-policy, deleted-path, changed-path, protected-file,
  and frozen-scope checks pass. Only the intended source and this ledger are
  tracked changes.

No object-status change, config edit, semantic-ledger edit, park entry, storage
declaration, protected-file edit, or shared-header edit is part of this wave.

## Committed-state replay

The committed-state literal delete/rebuild replay is intentionally deferred
until the implementation-and-ledger commit exists. Its evidence will be added
in a ledger-only follow-up; the frozen first-shot source and object remain
unchanged until then.

## Corrected cumulative-HEAD closure

Integration and the corrected cumulative-HEAD replay are intentionally
deferred until the isolated committed-state replay passes. The approval-gated
Units package remains frozen throughout; this ledger does not constitute the
required exact approval phrase.
