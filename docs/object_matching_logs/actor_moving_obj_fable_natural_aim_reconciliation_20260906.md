# ActorMoving: natural Fable aim reconciliation (2026-09-06)

## Source and accounting boundary

This packet reconstructs `actor_aim_jump` in ordinary typed C, not Fable's
source-tuned exact form. It uses the independently corroborated three
component stores, existing actor field/enum owners, the genuine four-argument
swarm API from `actor_types.h`, and the normal `real_math.h` math definitions.
The public five-argument declaration belongs in `actors.h`. No local foreign
prototype, math-name alias, forced-inline directive or source-shape retry is
added. The old unrelated `normalize3d` import convention is not expanded.

The first PID-isolated compile and ordinary Ninja build produce the same
**256-padded-byte / four-relocation** residual, normalized SHA-256
`4087deef3f73fc2eae5eff84aa196f9f11f4c64d74b797c8e981124ab6eb5817`.
Objdiff reports **91.8764%**, not exact credit. ActorMoving becomes
**20 exact / one residual / 15 unwritten**. This adds one credible source
body and **zero exact code bytes**; the object is not Matching or link-ready.

The retained form is not the `set_real_vector3d` expression selected by
Fable's reported second shape. Fable's initialize function, 488-byte constant
packet and three unresolved COMMON arrays remain excluded. See
`actor_moving_obj_fable_independent_intake_20260906.md` for the complete donor
chronology and independent January/HCEA/Stian evidence.

## First owner-header gate

Only the six-line public prototype was added first. The full actual Ninja
dependency graph contains 47 consumers. A complete stable function sweep
preserves all **6,257 inherited strict functions**, with no gains or losses.
The frozen section audit classifies 3,104 shared records unchanged, 56 as
compiler-local label spelling only and one as a substantive review lead.
There are no new data/COMMON/helper owners or forbidden point helpers from
this declaration-only change.

That one lead is the already-fuzzy private
`ai_communication_update_speech_timers`. No source in that C file changes.
The complete 672-byte function, all 43 ordered relocations, private function
symbol, section flags/alignment and NODUPLICATES selection remain identical
except for this seven-byte block:

```text
before +0x167: mov ebx, dword ptr [ebp-8]
       +0x16A: mov di,  word ptr [ebp+0x10]
after  +0x167: mov di,  word ptr [ebp+0x10]
       +0x16B: mov ebx, dword ptr [ebp-8]
rejoin +0x16E
```

Both source slots and destination registers are disjoint. Neither load
changes flags or memory. The complete 27-branch graph is unchanged; two
branches enter at the block start, none enters its interior, and the debug
path jumps from +0x165 directly to the common rejoin after loading the same
values earlier. There are no indirect jumps or relocations in the block.
All masked bytes before and after it are identical.

Root reviewed and independently replayed the frozen static/runtime proof:

- `scratch/actor_aim_jump_speech_timer_schema_load_proof_20260906.py`,
  SHA-256 `211c0d8ea715e556520fb35f760831e900ce66d1c9509e2b1c9054ef73e7a5a3`;
- `scratch/actor-aim-jump-speech-timer-load-proof-root-20260906.json`,
  SHA-256 `3b201e94c06d078a695532f361d42792be5a1e3f8c79044111367aefe0bb1ffd`.

The real seven-byte blocks pass 24 varied dword/word/EDI-high-bit/flags/DF
states, comparing all GPRs, EIP, the full seeded memory page and zero writes.
A wrong-source-offset machine mutation is detected. This is a bounded block
execution plus a complete static commuting-load proof, not a claimed
full-subsystem execution. It justifies refreshing the fuzzy park from norm
`885fdb81966284acf5ba7138a4bbbd2221e7c61c89114fdd6f53b96ba89b81a0`
to `7db4861a5adf519e0457a0ffe785ff4d47e878fda47cc0a06206a529776ac91c`
and 97.67839% to 97.76382%. No comparator or admission rule is relaxed.

## Body and complete preservation review

After that genuine owner gate, root added one natural implementation and the
`actor_types.h` include. The first gate preserves all 20 inherited exact
ActorMoving functions and all 40 inherited runtime section records. Ordinary
Ninja also preserves the full 6,257-function strict census. The final
47-consumer comparison has the same speech-timer delta plus the intended
new aim function and three normal math fallback COMDATs; no other inherited
runtime body, data owner, COMMON, symbol contract or forbidden
`point_from_line3d` definition/reference is introduced.

The math fallback copies are `square_root` (16), `scale_vector3d` (48) and
`magnitude3d` (48). Their machine bodies exactly agree with every other
canonical/January definition, but their SELECT_ANY ownership conflicts with
ActionCharge's existing NODUPLICATES owners. This is a real whole-object
linkage blocker, not an excuse to claim a finished object. No helper receives
target credit; none is suppressed or renamed to force the desired schedule.
The same partial-source boundary was previously documented for ActionObey
and Geometry. See `actor_moving_obj_fable_math_owner_audit_20260906.md` for
the full 572-base/833-target owner census and ordinary-link precedent.

## Frozen identities

All scratch paths are relative to canonical. The full manifests also retain
the source/header/config inputs and all 572 base-object hashes.

| Artifact | SHA-256 |
| --- | --- |
| before manifest | `1a42a86d6218b423a0991ff50b5fb834a1207216f074116dfb9d942a04afcc95` |
| schema manifest | `4588a68469f60e1795bdc11753759348ff77af740d76bafc8e96d245a199f26b` |
| final manifest | `730570a9d5f35f33db39dd9e080b95c1c22a5c6686dc38a479428126b7386f73` |
| January ActorMoving object | `d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa` |
| first isolated object | `7f5804d1041cdf8ec25b61eac25cb4ac65f6339459653ace3776c9a844f4b38c` |
| ordinary built object | `0e478075c11e850ecc001bda9663fdf9221681b0dd57039d0794323c5a9d3f69` |
| retained C byte digest | `4da4c271a6756fb2bacb673af8af063f87e62c3769c39438b9d5b20a315aafc4` |
| owner header byte digest | `d70ecd89c16bd4cb800558391b7ea0defbc41da84a2ff2b57af97cbbe15be6de` |

Capture tool: `scratch/actor_aim_jump_fable_natural_capture_20260906.py`.
Manifest/build/stable/runtime prefixes:
`scratch/actor-aim-jump-fable-natural-{before,schema,final}-20260906`.

## Runtime and publication

Root fully reviewed and independently replayed the public bounded oracle:

- `tools/audit/actor_aim_jump_runtime_differential.py`, SHA-256
  `6d98aaaf2171082691dbdc7c3fe4ebfe54d5cfcabf8c332d02c3262bf8ae8307`;
- `tools/test_actor_aim_jump_runtime_differential.py`, SHA-256
  `2d69bf20aaafa9b03d11b44574eabd574b9063c889c7ba86e85c5740e3cae53a`;
- `scratch/actor-aim-jump-runtime-differential-public-root-20260906.json`,
  SHA-256 `5a4bfe1ab269cb5ca714879bb5706a2cbac554487e20a11fd8f02ab4d4019ea6`.

The 22 finite scenarios under x87 control words 0x027F and 0x037F give
**44 executions per target/first/actual role**. They cover vehicle, swarm,
inactive-target, explicit leap, charge/melee override and clamp boundaries.
The real scale machine body executes; only datum lookup and swarm dispatch
are stubbed at their genuine typed API boundaries. All 44 output vectors are
bit-identical across roles, with the tested call arguments/order, actor
envelope writes, vector write topology, cdecl return, nonvolatile registers,
DF, control word and x87 status checks passing.

This is not blanket floating equivalence: on `leap_true_unscaled`, both
control modes produce a **one-ULP internal magnitude residual**, January
0x40F6CF49 versus candidate 0x40F6CF4A. That branch does not call scale and
its returned vector is unchanged. The report records those two executions,
not just the bit-identical output count. The corpus uses separate output
storage; equivalence for aliased outputs, arbitrary nonfinite values or all
possible inputs is not established. The 0x800-byte actor buffer is a seeded
test envelope, not a claim about `sizeof(actor_datum)`; handles are abstract
forwarding inputs supplied to a lookup stub.

Three precise real-instruction negative controls are detected by their
intended checks: a wrong datum argument slot, writing one instead of clearing
the targeted flag, and returning false in AL. Thirteen fixture-free tooling
tests pass. No mutation is retained in a source or object file.

The combined aim/HUD final suite passes **502 tests plus 26 subtests**;
parks are 290 valid / zero stale / zero invalid, admission has zero new
candidates/contradictions/revocations and five existing rejections. Publication
must still be verified against the resulting commit on both authorized refs.
