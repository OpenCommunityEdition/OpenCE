# Units vehicle scripting wave (Jonas, 2026-08-24)

## Result and boundary

This wave began from exact cumulative commit
`1fc18e1b99ad73454e1f6e629d4193990311d086` on isolated branch
`jonas/units-vehicle-scripting-wave-20260824`. The base `units.c` blob is
`154882f73005f1ca1a128cf7107fbf199c28c093`.

The independently fixed candidate set was exactly:

- `unit_scripting_vehicle_test_seat_list`;
- `vehicle_scripting_load_magic`.

`unit_enter_seat` was admitted only as typed natural-linkage support because
the retained load-magic body has a real REL32 call to it and the cumulative
source otherwise had no owner. It is not a candidate, is nonexact, and earns
no function or data credit.

The wave explicitly excluded `unit_scripting_enter_vehicle`,
`unit_try_and_exit_seat`, `unit_exit_seat_end`, `code_001a2030`,
`vehicle_scripting_unload`, and `unit_ready_desired_weapon`. No body was added
or changed for any excluded name. The inherited nonexact
`unit_ready_desired_weapon` remains independently nonexact; the other five
excluded target functions remain absent. The forbidden stale `units.h`
private-linkage closure was not entered.

Only `source/units/units.c` and this new ledger are tracked changes. There is
no header, configuration, protected-source, semantic-exception, parked-entry,
completion-label, writable-storage, or pre-existing-documentation edit.

## Evidence read before emission

The full Claude Units evidence packet was read before the candidate set was
frozen, including:

- `units_obj_seat_scripting.md` and
  `units_obj_vehicle_magic_load.md`;
- the standalone `units_seat_scripting_recovery_packet.md`;
- the compiler census, compiler provenance, and QFE hunt;
- the HCEA PDB atlas and HCEA scope ranges;
- the donor atlas, external-source provenance, and cross-build provenance;
- `docs/matching_methodology.md` and
  `docs/exact_match_acceleration_playbook.md`;
- every inherited Jonas Units ledger through the typed-damage and damage
  lifecycle waves.

The exact carrier is Claude commit
`deb5554ab8d16da80da934e2aa7081bc7d1f594f`, whose `units.c` blob is
`124f29e32497c091c4163b5faafc58dbf8890161`. Its raw Git payload is 305,068
bytes with SHA-256
`173fd6fdcc09d979ba6507aefa2cd77790f8f17f93413b9acbb679c32bb8f7c4`.
Brace-balanced extraction proves the three complete bodies in this source are
case-sensitive text-equal to that carrier: 54 lines for
`unit_scripting_vehicle_test_seat_list`, 76 for
`vehicle_scripting_load_magic`, and 124 for `unit_enter_seat`.

The clean HCEA semantic-donor repository is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Its later-build donor files were
used only to authenticate meaning and type intent, never as January byte
authority:

| donor | SHA-256 |
| --- | --- |
| `src/unit_scripting_vehicle_test_seat_list.c` | `35bd7f7cae19c35e1506ca96d852d2fc5b019229f66ab14a78d01fa2f2b0ae6b` |
| `src/vehicle_scripting_load_magic.c` | `cfd6b57cdb06800e65d5ca85df00a265829cd824f42462cceea655f4d4cdecf1` |
| `src/unit_enter_seat.c` | `cd7088ea6f5734a22a5a1bc655cc1e89522017d893cbb18d2a7bf5e4eca88045` |

The January executable remains the only byte authority. Its authenticated
raw SHA-256 is
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Authenticated csplit SHA-256 is
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`;
authenticated objdiff-cli SHA-256 is
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.
Fresh csplit regeneration produced the 138,090-byte target object with
SHA-256
`e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.

## Source and policy proof

The implementation adds 257 lines, comprising exactly the two fixed bodies
and the one required support body. The load-magic object view is guarded by a
runtime unit-type mask test before the documented typed cast. All code is
ordinary readable C. There is no inline assembly, volatile/barrier trick,
pragma, attribute, `__declspec`, register forcing, raw byte offset,
`offsetof` reconstruction, pointer/integer pun, undefined behavior, dummy
caller, export promotion, or synthetic reference.

The three emitted definitions are value-zero type-`0x20` external
storage-class-`2` COFF owners. Load-magic has its natural REL32 call to
`_unit_enter_seat`; the support therefore remains. The excluded
`_unit_exit_seat_end` and `_unit_try_and_exit_seat` are only ordinary
undefined external declarations where naturally referenced. No new `.data`,
`.bss`, COMMON, aggregate, or writable owner exists.

Before production emission, `git diff --check`, changed-path, deleted-path,
protected-path, header/config, policy, storage, linkage, and caller checks
passed. One parse-only compiler invocation using the real Units compile
contract plus `/Zs` exited zero and did not change the object hash.

## Frozen one-shot production emission

Immediately before emission the frozen set was the two candidates plus the
one required support body listed above. Exactly one natural production
command targeted `build/base/source/units/units.obj`; Ninja ran exactly one
`[1/1] CL build\\base\\source\\units\\units.obj` edge. No source edit,
tuning, retry, alternate formulation, or second candidate compile followed.

The immutable first-shot artifact is
`build/audit/units_vehicle_scripting_firstshot.obj`: 128,583 bytes, SHA-256
`b61b7a504c1bbb22ab10a3b43b9a6a984a5b86975da799c661d387ee8c613a5c`.
The live candidate remained byte-identical through the full graph and every
initial gate.

Strict hardened comparison gives:

| body | result | padded bytes | relocs | normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_unit_scripting_vehicle_test_seat_list` | exact candidate | 272 | 9 | `7a51970653aa3c26aa91166844c539e83d94234345168dd2935abb6b39a6e90f` |
| `_vehicle_scripting_load_magic` | exact candidate | 384 | 10 | `f6df83b287af8e5318bed6dda05ef6d9b77261bacab5d2362cae4ca3101c389e` |
| `_unit_enter_seat` | nonexact support, no credit | 592 | 34 | target `12dc030d6e58e40e4b7e48afdbb7b37c2403fcbb46d1a15330cdcaad9648ff53`; candidate `0c59533c556a7ae7e5b3881832a72b5e7b82609650e177fed4b5c0f70f42d5f7` |

The two exact candidates match every padded byte and every relocation
address, type, destination, and addend. Support has equal size and relocation
count but differs beginning at the documented instruction schedule around
the ready-weapon call; it was retained without credit or tuning.

## Corrected cumulative preservation proof

An initially copied baseline object was discovered to belong to the
integration worktree after that worktree had advanced from this wave's
`1fc18e1b` base to `05cd9967`. It was current for that later source, not a
valid object baseline for this branch. This is a baseline-copy provenance
correction, not a cumulative rebuild defect. The copied object was discarded
as evidence, and no candidate recompile or source tuning followed.

The exact `1fc18e1b` inherited-name manifest was instead reconstructed from
authenticated clean committed branch objects: inventory closure commit
`8795191ea3846a08ff7df9c48738b063c9e63f2f` supplies the 133-name foundation,
then the committed seat-query, policy, and damage artifacts supply 14 unique
later accepted names. The union is exactly 147 inherited exact target
function owners.

The direct first-shot census across all 189 January function owners is 149
exact, 11 present nonexact, and 29 absent. Removing only the two fixed gains
from the 149-name exact set yields exactly the authenticated 147-name
inherited manifest: zero missing and zero unexpected names. The 11 nonexact
owners are `_code_00198fd0`, `_unit_animation_set_state`, `_unit_died`,
`_unit_enter_seat`, `_unit_euler_aiming_update`,
`_unit_preprocess_node_orientations`, `_unit_ready_desired_weapon`,
`_unit_set_or_test_seat_and_weapon_label`,
`_unit_start_animation_impulse`, `_unit_throw_grenade_release`, and
`_unit_update`.

The separate census across all 129 January runtime non-code owners is 123
exact, zero present nonexact, and six absent, totaling 6,359 exact logical
bytes and seven relocations. The authenticated inherited union is exactly 122
owners; all 122 remain exact. The sole additional exact owner is the
support-only assertion RDATA
`??_C@_0CH@KNBNIKNG@unit?9?$DOobject?4parent_object_index@`, and earns no
candidate credit. The strict `.data` owner `_magic_base_animation_seat_index`
remains 1,564 bytes / seven relocations with normalized SHA-256
`6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7`.
`_unit_globals` remains four BSS bytes / zero relocations with normalized
SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

## Initial full gates

- Full `halobetacache_build libcmt_build` passed all 568 catch-up edges. The
  frozen Units object was already up to date and was not rebuilt. Warnings
  were inherited in scenery, `network_game_manager`, `flying_camera`, TIFF,
  and protected `ai_debug.c`; `units.c` emitted no warning.
- Fresh csplit plus direct objdiff report generation passed.
  `build/report.json` is 1,572,874 bytes with SHA-256
  `b07bf8e07abf5e29548040823bca0ab59ea864d1d6e9ddaefc1d498f88845eb6`.
- Semantic audit: 470 units, 4,217 functions evaluated, 3,512 missing-base
  symbols, 4,076 semantic exact, 107 hidden exact / 62,984 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 local symbols
  skipped, 4,137 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,119/11,060 functions,
  498,832/2,198,102 code bytes, and 1,835,088/4,176,062 data bytes.
- Halo progress: 273/468 objects, 3,952/7,574 functions,
  485,918/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- Ordinary `units.obj` report: 142/192 exact functions and
  25,350/53,478 meaningful code bytes; 4/6,752 ordinary data bytes and
  73.287155% fuzzy score. The hardened 189/129 censuses above remain the
  acceptance authority.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- Final initial `git diff --check`, tracked-path scope, source-policy, symbol
  storage, and Units Ninja dry-run checks pass; Ninja reports no work.

No committed-state replay is claimed in this initial record. After Jonas
commits this source and ledger, a clean one-unit snapshot, verified same-root
object deletion, normal one-edge rebuild, immediate regression check, direct
two-candidate/support comparison, full 189/129 census, storage audit, and
no-work dry run will be appended in one additive ledger-only commit. No
amend, push, history rewrite, or worktree removal is authorized.

## Actual committed-state forced replay

Implementation-and-initial-ledger commit
`d6dcf78a11f5f4088499d15f71d80a82ccdf6868` was authored and committed by
Jonas Volman `<theunknowentity@gmail.com>`. Its committed source is blob
`5c6a24b10a40bfc82251481910ed0480c5e72b42`: 227,841 raw Git payload
bytes with SHA-256
`83c4c23ca574f0f6f2aceae134a24643aa3898dbfced8faabef8697a7bcddd51`.
Its initial ledger is blob `55885b77f3bd2c82e6d821d9b4e33916cadce9f3`:
10,867 raw Git payload bytes with SHA-256
`6d43e641416fb1822f366e67924acc065b59f6c207f882f39ffae56d1f053ae3`.

`git status --porcelain=v1 --untracked-files=all` printed no path at that
implementation commit, and a Units-object Ninja dry run printed
`ninja: no work to do`. A fresh one-unit committed-state snapshot was then
captured with `--no-build` from that exact clean commit:

```text
build/regression_units_vehicle_scripting_committed_20260824.json
commit: d6dcf78a11f5f4088499d15f71d80a82ccdf6868
size: 5,726,400 bytes
SHA-256: 6da5ae8628f9d770c38ed26b49de06cfc433756d73f9969ce2851c458c15157a
```

The generated object path resolved to
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-vehicle-scripting-wave-20260824\build\base\source\units\units.obj`.
Its normalized absolute path was proven to begin with this isolated
worktree's normalized absolute root before deletion. The snapshotted object
was 128,583 bytes with SHA-256
`b61b7a504c1bbb22ab10a3b43b9a6a984a5b86975da799c661d387ee8c613a5c`.
Only that exact file was deleted, and absence was verified.

A normal one-target Ninja rebuild then ran exactly one
`[1/1] CL build\\base\\source\\units\\units.obj` edge and succeeded without
a Units warning. The rebuilt object is again 128,583 bytes. Its
phase-specific whole-file SHA-256 is
`73fe4f705eb53f33f780633d240ae85a8dd943b857ca2fb39006e8cfa055825a`;
the whole-object hash difference is confined to rebuild metadata. Every
runtime acceptance fingerprint is unchanged.

The immediate committed-state regression check is authoritative for all
captured function, non-code, relocation, and symbol-ownership evidence:

```text
ok: true
failures: 0
warnings: 0
still_exact: 149
newly_exact: 0
changed_nonexact: 0
```

Independent replay proofs also passed:

- the two-name candidate comparison remains all exact with the same padded
  sizes, relocation counts, destinations, addends, and normalized hashes;
- `unit_enter_seat` remains the same 592-byte / 34-relocation nonexact support
  body with no credit;
- complete function census: 149 exact / 11 present nonexact / 29 absent
  across all 189 January function owners;
- complete target-data census: 123 exact / zero present nonexact / six absent
  across all 129 January non-code owners, totaling 6,359 exact logical bytes
  and seven relocations;
- first-shot versus replay status across all 189 function owners and all 129
  non-code owners has zero change;
- `.data` remains strict at 1,564 bytes and seven relocations;
  `_unit_globals` remains strict at four BSS bytes and zero relocations;
- both candidates and the support definition remain value-zero type-`0x20`
  external storage-class-`2` owners; all excluded linkage remains at the
  recorded absent, undefined-external, or inherited-nonexact boundary;
- the committed source blob remains
  `5c6a24b10a40bfc82251481910ed0480c5e72b42`;
- a final Units-object Ninja dry run reports `ninja: no work to do`.

Only this ledger is modified for the additive replay record. No source
tuning, additional candidate compile, header/protected/config/storage change,
adjudication, amend, push, history rewrite, or worktree removal occurred after
the implementation commit.
