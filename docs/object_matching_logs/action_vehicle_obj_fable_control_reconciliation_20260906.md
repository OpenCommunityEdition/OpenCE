# ActionVehicle: Fable control reconciliation (2026-09-06)

## Scope and verified runtime gate

The one genuine `action_vehicle_control` correction matches on its first
canonical compile and in the ordinary Ninja output. It adds **175 meaningful
/ 176 padded exact code bytes** and one function. ActionVehicle advances
**7 -> 8 / 17 exact**, with zero residual and nine unwritten functions; it is
not a complete object. `action_vehicle_begin` is deliberately not imported.

The local strict sweep is **6,256 exact / 8,245**, gaining precisely this
function with no inherited exact losses. Official local totals are
**916,726 / 2,198,102 meaningful exact code bytes**, **6,210 / 11,060 credited
functions**, **391 / 833 Matching objects**, and unchanged **2,061,020 /
4,176,062 data bytes**. The preceding published geosphere commit is
`b90402d960a75d312f2a3895c43850e9cca4ccb8` on both authorized refs.
Independent runtime checks now pass. These packet-local numbers precede the
independent HUD helper's four meaningful bytes and are not alone a claim of
publication. The joint local checkpoint is 916,730 meaningful bytes.

## Why this is a legitimate contract correction

The prior 2026-08-21 HCEA-based control attempt used the canonical facing
priorities 2/1. January writes **4/3**, independently named in `actors.h` as
`_primary_priority_locked_aiming` and `_primary_priority_aiming`. Fable's
literal first saved source already contains this semantic correction.
The only later pre-emission repair supplies a semantic TU-local name for
the proven idle-look value 4; the first preserved code-producing object is
exact. That parse/name correction is disclosed, not misrepresented as
literal-first-source exactness. Root made no post-compile source variants.

Begin has no analogous contract correction: its stronger donor shape only
introduces a local state pointer over already-correct reset/time/copy logic.
Its old source-shape boundary therefore remains held. Do not import the
whole donor source or count both claimed 256 padded bytes.

The independent source/history/COFF intake is
`action_vehicle_obj_fable_control_intake_20260906.md`. Root read all seven
prior ActionVehicle ledgers, the donor report/facts and complete control
diff, the HCEA control body, and the genuine current type/enum/API owners.
The user-authorized Fable donor remains read-only at
`C:/halo-worktrees/fable-50k-small-families-20260904`, tip
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`.

## Genuine owner boundary and full consumer verification

The sole prerequisite moves the unchanged public `action_vehicle_control`
prototype from the consuming `source/ai/actions.c` to the genuine owning
`source/ai/actions.h`, under its normal source-family section. The existing
header owns `vehicle_state_data`; `actors.h` already supplies typed actor
access, priorities, direction kinds and the real `actor_path_has_path` API.
No new facade header, duplicate C-local prototype, global, alias, schema,
compiler option or configuration exception is introduced.

Before that edit root captured all **47 actual Ninja consumers**, both
sources, the header, January target, configuration and all 572 base hashes.
The schema-only build preserves all 6,255 exact functions. Across the full
runtime closure, **3,107 sections are unchanged and 53 differ only in
compiler-local-label spelling**. There is no lost/changed runtime content,
new COMMON storage, or new `point_from_line3d` definition/reference.

After adding the body, the same inherited 3,160 runtime sections survive.
ActionVehicle alone adds the intended control section. It adds no helper,
data or BSS owner. Existing helper copies are unchanged, not new progress.
The genuine Actions callback table retains its `DIR32` control reference
at `.rdata+536`; no artificial caller keeps the body alive.

Source uses `actor_get`, authentic named fields and enums, a real vector
copy, explicit `FALSE` flags and terminal return, with one parameter per
line. The locked branch copies the saved facing and skips path lookup;
the unlocked branch chooses aiming/movement only when the real path API
succeeds. All three branches set idle look and clear the six genuine
shoot/movement order flags. No assembly, punning, raw offset, manual
helper expansion, register/volatile steering or fake matching is introduced.

## Exact and frozen identity

Control target/first/actual agree on normalized SHA-256
`81612c85cf900b15c2de8ea6d52646f5727a96fbea5e2f13018e6f9613fcd293`.
All three ordered relocations agree: `DIR32 _actor_data` at +4,
`REL32 _datum_get` at +17, `REL32 _actor_path_has_path` at +89.
The target symbol is an external cdecl public function, not a private
address-named body requiring an invented ABI.

All paths below are canonical-relative:

| Artifact | SHA-256 |
| --- | --- |
| `scratch/action-vehicle-fable-control-before-20260906.json` | `fa933a6deb8788e398a06037db7730cace93f8ab47a7efae636b012ae97430d4` |
| `scratch/action-vehicle-fable-control-schema-20260906.json` | `6ddf088f8265e4fc707d16a76711df93682d5406965628a4a85bbb22b9462289` |
| `scratch/action-vehicle-fable-control-final-20260906.json` | `081017c7ca9d82bf5b240e64a27115ec3d187901a7f2b04f77e291182b8ddf7b` |
| frozen January object | `49f00a0115cecf578e51102fc33f665f876a4fafcb78ee3b53520545406ef769` |
| `scratch/action-vehicle-fable-control-first-20260906.obj` | `6bc330954c197c8a5dc54c3ed269e39b5d894edafc4d0a4f265b235686b46c2f` |
| frozen actual object | `0f80fc38c93779cce5e03eceea3dffa9788386a497300ac6c18e307c66c4f82c` |
| frozen ActionVehicle source | `a45f810e182ca07d962da22deb8f9ea0063382650205248d74f66530f7a7725b` |

Capture: `scratch/action_vehicle_fable_control_capture_20260906.py`.
Full runtime comparisons:
`scratch/action-vehicle-fable-control-{schema,final}-runtime-20260906/summary.json`.
Final stable snapshot:
`scratch/action-vehicle-fable-control-final-stable-20260906.json`.
Build/gate logs: `scratch/action-vehicle-fable-control-{first,schema-build,final-build}-20260906.log`.

## Independent execution

Root reviewed the complete public oracle and its 24 fixture-free tests, then
replayed it against the frozen January, first reconciled candidate and actual
owner-correct object. All **18 cases pass for each object**, with identical
trace SHA-256
`87fe79ca103e97d9c177ba4def03ab82e8e91b2ce4c9f41253b761cba9341ed7`.
All four pinned real-byte mutants are detected by both semantic checks and
target/candidate comparison.

The oracle checks the entire seeded 0x800-byte actor-memory envelope, precise
write order/values, genuine API call order and forwarded handle bits, caller
return/argument window, stack balance, clear DF and preserved nonvolatile
registers. Locked facing exercises raw vector-word copies, including unusual
bit patterns; unlocked paths vary the boolean AL byte while poisoning upper
EAX and volatile registers. There is no floating-point arithmetic in this body.

The envelope is a test boundary, not a `sizeof(actor_datum)` claim. Arbitrary
and NONE-shaped handles are forwarding tests with the datum stub returning
the modeled owner; they do not claim those handles are valid engine data.
The void function's volatile registers are differential observations only.
Only genuine `datum_get` and `actor_path_has_path` dependencies are stubbed.

| Artifact | SHA-256 |
| --- | --- |
| `tools/audit/action_vehicle_control_runtime_differential.py` | `1f6a27acb013afe57f9ebf43a0020e04fb0720dd56f07653e21dd61ce3af2f79` |
| `tools/test_action_vehicle_control_runtime_differential.py` | `cc4f093dec9026fbff5c4495f70ccf3e67915bbed985f46d5952b68478df7327` |
| `scratch/action-vehicle-control-public-runtime-root-20260906.json` | `71a50adc3ddea8014a8bf19dc538d970966b47534485db21b017f3e60d455b34` |

The subsequent ModelAnimations owner-only experiment is completely restored:
all 1,523 sections across 13 consumers match the pre-trial state, including
Units. No graph implementation was compiled. Its rejected boundary is in
`model_animations_obj_fable_node_owner_trial_rejected_20260906.md`.

Final joint publication checks are recorded in the reconciliation backlog;
remote refs must be verified against the resulting commit, not this ledger.
