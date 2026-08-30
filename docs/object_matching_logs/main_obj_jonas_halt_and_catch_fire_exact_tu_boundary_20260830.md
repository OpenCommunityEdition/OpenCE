# `main.obj`: `halt_and_catch_fire` exact-function / TU-owner boundary (2026-08-30)

## Result

The first and only production candidate for `_halt_and_catch_fire` is strict
function-exact C:

| Measurement | January | Candidate |
| --- | ---: | ---: |
| meaningful / padded bytes | 658 / 672 | 658 / 672 |
| ordered relocations | 45 | 45 |
| normalized SHA-256 | `977808e8b668b9a057951107579de6cd64c5450470d8f54efa4f06a369fbeafb` | `977808e8b668b9a057951107579de6cd64c5450470d8f54efa4f06a369fbeafb` |

The function was not retained.  The same-path fail-closed translation-unit
manifest rejected the candidate because its natural new function, type, and
constant owners changed the accepted associative-debug fingerprints and the
complete `main.obj` symbol/section ownership packet.  Production source was
inverse-restored exactly.  This commit intentionally records only the measured
boundary and grants no code, data, semantic, parked, admission, or completion
credit.

The restored Main board remains 70/95 exact functions and 3,360/13,792 strict
padded bytes.  With the transient candidate present it was 71/95 and
4,032/13,792; no runtime function packet was lost.

## Base and provenance census

- isolated worktree:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\unintegrated-exact-harvest-20260830-2`
- branch: `jonas/unintegrated-exact-harvest-20260830-2`
- clean canonical base:
  `58387f05c8a6a168fafae289bce9f9b67257825a`
- clean historical exact donor tip:
  `f50be1825f6291c8837c7dcaba078d983bf9e8a5`
  (`agent/main-strict-complete`)
- exact donor source blob: `4abe5197a1bdd3062e6715fb085c87f87ec6aa6e`
- exact donor ledger blob: `eef44838c72f1b1aed16416d55302e9a517aa95c`
- donor object SHA-256:
  `15ae03fe46e63b175dde666a5a609940ad1441ee5cf96bf9c7a9356478006e85`
- HCEA named donor:
  `C:\Users\isabe\Documents\Codex\reference\halocea\src\halt_and_catch_fire.c`,
  SHA-256
  `6edab09d05fa8f9a576fd9350c7c16d3a680a9b555324497d536f2af0dcee5cf`
- Pastudan corroboration:
  `C:\Users\isabe\Documents\Codex\reference\pastudan-halo\src\halo\main\main.c`,
  SHA-256
  `1240dba1d8e9cbedc64b247bc7ac888fc2f1567785ccbfd8e80a4410f335238b`

Before selecting Main, the lane audited all reachable and unreachable local
refs, registered build-bearing worktrees, canonical object ledgers, Claude
documentation, and 171,932 relevant object artifacts in 1,250 worktrees.
`_halt_and_catch_fire` was strict exact in five historical artifacts.  No other
recoverable, unintegrated, nonexcluded source commit supplied a function of at
least 512 padded bytes that was not already present at the canonical base.

The larger exact historical `_main_loop` artifact was not selected because its
ordinary source depends on the paired `rasterizer_target_index` lifetime and
anonymous-union topology rejected by the standing Main policy ledger.
`_main_rasterizer_throttle` likewise depends on the broader typed vblank
counter owner and is explicitly excluded by the public-nine ledger.  Exact
artifact hits for `bitmap_group::_code_00065240` and
`game_engine::_code_0009bdf0` were standing synthetic-anchor/protected
boundaries; `action_flee::_code_00003210` had no recoverable matching source
blob.  Units, Vehicles, Matrix Math, AI Debug, Motion Sensor, Path Smoothing,
LRA/LRUV fixed points, and every standing frozen candidate were excluded.

The following current instructions were read in full before emission:

- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `docs/object_matching_logs/README.md`
- every `main.obj` ledger, including the public-nine and crash closeouts
- `tools/campaign/README.md`
- external `work/halo/CLAUDE.md`, SHA-256
  `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`
- external `work/halo/AGENTS.md`, SHA-256
  `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`

## January contract and frozen natural candidate

January's complete packet uses a `0x274`-byte stack frame.  The first two
relocations are DIR32 references at `+10` and `+36` to the split-owned
`_global_screenshot_count + 0x38A` recursion lock.  The remaining ordered
packet covers the scenario lookup, four-gamepad rumble shutdown, terminal-font
fallback, frame/window setup, typed render camera and fog copies, two error
strings, transparent/debug drawing, present/input loop, and final `_exit` call.
The strict comparator verified every padded byte, relocation address, type,
destination, addend, and order.

The frozen candidate was the readable ordinary-C donor topology, with only
current named-layout adaptation:

- a split-owned typed `_screenshot_and_framerate_globals` declaration whose
  `count` remains at field zero and whose `halt_recursion_lock` is asserted at
  `0x38A`;
- field-zero spelling for the two existing screenshot-count consumers;
- current typed `rasterizer_globals.reserved04.frame_bounds` access;
- direct public header includes and typed declarations for the public cdecl
  callees; and
- an explicit `return;` after the XDK `__declspec(noreturn)` `exit(0)` call,
  which emits no additional code.

The historical ledger independently fixes the three statement-order facts
needed by VC7: viewport x fields precede y fields, fog maximum precedes
minimum, and the draw cursor local precedes the bounds local.  HCEA agrees on
the control flow and named semantic layout.  Pastudan is later-build
corroboration only; none of its raw-address material was copied.

The candidate used ordinary readable typed C89.  Its diff added no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, optimizer barrier, raw
address or raw-offset access, pointer/integer reconstruction, pointer or union
pun, undefined behavior, synthetic anchor, object patch, compiler exception,
comparator exception, or byte-forcing construct.

## One-shot evidence

The first `/Zs /W3` syntax-only pass emitted no object and identified two
mechanical current-header differences: the named nested frame-bounds field and
the absent public declaration for the camera FOV helper.  Only those typed
pre-emission corrections were made.  The second syntax-only pass passed with
the TU's existing warnings, and the baseline object SHA-256 remained
`606a2637375ebf6b345530a52efeaa1636cd9b009c4f538be1518b92db7bf940`,
proving no candidate object had been emitted.

Exactly one code-producing candidate edge was then run:

```text
ninja build\base\source\main\main.obj
[1/1] CL build\base\source\main\main.obj
```

The transient candidate object SHA-256 was
`7b83548586866c21c5c305304f1ed5ad2ff846005550ffb6aa3d06b67fa00a47`.
The January split object SHA-256 was
`abd673d4db1cc315662dde43eac3d4bdec2914ca2384077fa5e112fbcdda1255`.
Direct hardened comparison reported `all_equal: true` for
`_halt_and_catch_fire`.  Independent direct comparisons also kept the two
typed-owner consumers exact:

| Existing function | Padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_main_present_frame` | 144 | 12 | `5ad92723dc9df3e7e18ec0f699037ce430d77af403e299b5b0cc2df9a6ef34bc` |
| `_main_taking_screenshot` | 32 | 2 | `dcc32fee602803afff787533ac9af3ae8adc4ae0b283977a56e780248ff76440` |

The strict code board showed all 70 inherited target functions still exact and
only `_halt_and_catch_fire` newly exact.  The candidate therefore had no
runtime-byte or ordered-relocation regression.

## Fail-closed TU boundary

The same-path pre-edit object had been preserved before mutation and was
captured with the hardened `tools.regression_gate` fingerprints.  Against that
object, the candidate manifest reported:

- `_halt_and_catch_fire`: `NEWLY_EXACT`;
- all 70 accepted Main functions: changed complete evidence because their
  associative `.debug$F` ownership/fingerprints changed;
- `_main_loop`: changed nonexact evidence;
- four natural target-exact COMDAT owners appeared or changed:
  `old tags\\internal system plain`, the January build/version string,
  `__real@3f400000`, and `__real@3ff0000000000000`;
- the anonymous debug-section sequence and multiple existing string-owner
  fingerprints changed; and
- the complete translation-unit symbol ownership set changed.

No adjudication or exception was used.  Function equality is necessary but is
not sufficient under the current owner-admission policy, so the exact body and
all support were inverse-restored without a second candidate form.

After the normal restored rebuild, the same manifest comparison returned
`ok: true`, 70 `still_exact`, no newly exact or changed-nonexact functions,
zero failures, and zero warnings.  `source/main/main.c` is byte-for-byte the
base Git blob `00c98986aaf1018a1a0ff4401797b1b6b5586898` and has working-file
SHA-256
`de10a0a3fae59486293765467fda4c467d5f66dd4df8e64810038e3e25508961`.

## Restored validation

- complete `halobetacache_build`, `libcmt_build`, and regenerated report: pass;
- Main board: 70/95 exact, 3,360/13,792 padded bytes;
- semantic audit: 470 units, 4,954 functions evaluated, 4,837 semantic exact,
  4,847 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 complete objects and 4,808/11,060 accepted functions;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, zero invalid;
- complete tooling suite: 212/212 tests pass;
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `git diff --check`: pass;
- production source/config/header diff after restoration: empty.

Restored artifact/report hashes:

- `build/base/source/main/main.obj`:
  `63860b9eb6b47a6be77039184c58574481019e6ebfa31cc7b797bc38a0e00123`
- `build/report.json`:
  `fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`
- `build/semantic_report.json`:
  `3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`

Toolchain hashes were unchanged: VC7 `CL.Exe`
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
`csplit.exe`
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`,
and `objdiff-cli.exe`
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

## Reopen condition

Do not repeat this function body or statement-order candidate.  Reopen only
with an atomic, authenticated Main owner/type topology that can insert the
already-proven exact function while independently preserving all 70 accepted
function/debug fingerprints, every existing data/string owner, the anonymous
debug sequence, and the complete TU symbol set, or after an explicit reviewed
change to the fail-closed ownership protocol.
