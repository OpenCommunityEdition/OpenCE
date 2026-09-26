# Compiler-application: Q6, Q9 and actor destination reconciliation

Canonical baseline: `cc6080369a3a9dda3fb287880d15a681b391b9f4`.
Donor lane: `claude/compiler-application-20260925`, source commit
`455dffad89aa3e90098f85e8aa4a129b1f7aa941`.

On 2026-09-26 the owner explicitly approved the two Q6/Q9 functions and
publication to halo-1 and exact-pilots, then separately requested inclusion
of the 968-byte actor-destination landing if independently exact. Only those
three source packets are integrated. No other pending bug, ABI, duplication,
source-shape or ownership hold is lifted. Donor worktrees are untouched.

## Fresh result, using the unchanged canonical scorer

| Function | Credited code bytes | Padded bytes | Relocations |
| --- | ---: | ---: | ---: |
| `_hud_update_weapon_local_player` | 1,472 | 1,472 | 68 |
| `_decal_clip_to_surface` | 1,768 | 1,776 | 59 |
| `_actor_destination_update` | 968 | 976 | 24 |
| Total | **4,208** | **4,224** | |

Halo credited code: **1,584,418 -> 1,588,626 / 1,770,166**.
Halo credited functions: **7,454 -> 7,457 / 7,574**.
Halo complete objects stay **388/468**, data stays **2,587,011**.
There are no whole-object admissions, scorer-only gains or duplicate helper
credits in this batch.

The donor reported Q6 as 1,471 and Q6+Q9 as 3,239. Canonical's existing
accepted ledger counts Q6 as 1,472, including the section's final byte after
its switch data. Therefore its Q6+Q9 gain is 3,240. The scorer, normalization,
extent model and credit rules have not changed to obtain this difference.

Normalized SHA256:

- HUD: `0524ed47a94ce36e27dc00a12f707c819bc31fbd241a50d7b5d03f2d1511d108`.
- Decals: `d875457fa2f021767305a52fdf7879167eef3a756992184c611bfe0a8a3f1109`.
- Actor: `4c6c5fda99e932ed49fb72a5cacb82d5743dea6dc0a2078249065533b8069089`.

## Q6: exhaustive HUD state dispatch, not a reachable indeterminate read

The private loop visits exactly 0 through 18. The current crosshair-state
enum is contiguous over those values, and each of its 19 case arms assigns
`result`. The default arm is therefore unreachable in defined execution;
the approval does not authorize a reachable uninitialized read. This proof
does not depend on adding a compiler noreturn attribute.

The patch includes more than removal of the default's assignment. The
four leading-OR ammo predicates are corroborated by the later unoptimized
build at `0x63853f..0x638589`, `0x638869..0x6388b3`,
`0x6388ea..0x638934`, and `0x638a4f..0x638a99`. The flash predicates contain
real redundant tests; their provenance is recorded rather than treated as
permission for arbitrary redundant source. The secondary-state primary-trigger
test is supported by January's shared tail and the later `0x638a07` site,
and is disclosed in the source. State-pointer binding, local unit index and
case ordering are part of the reviewed reconstruction.

Private first-party evidence:
`C:/halo-worktrees/claude-structural-20260923/scratch/lane/w/interface__hud_weapon/REPORT.md`
and its `od_638150.txt`. The reference executable itself is not published.

The independent integrated-object audit finds only this function's section
changed: the other 50 sections, all 93 owner records, COMMON inventory and
COMDAT selections/associations are unchanged. The TU is now 14 exact / 2
residual; `_crosshairs_draw` and `_render_weapon_hud` are not included.

## Q9: genuine flag-set predicate and real geometry helpers

The combined test uses named `FLAG` constants for bits 0, 1 and 3 of an
ordinary, nonvolatile unsigned-byte field. It rejects exactly the same flag
values as the three original tests, without side-effectful evaluation.
The later build's separate tests are recorded as a difference, not evidence
that the combined spelling was recovered verbatim. The approved form is
meaningful set-membership logic, not an inert compiler-control decoration.

The packet also restores the genuine geometry header/calls, removes the
project-point suppression/local declaration, and scopes the clipping work.
The complementary branch rewrite preserves the relevant NaN behavior.
The clipping input pointer denotes the final output wherever vertices are
emitted. The in-place `point_from_line3d` replaces handwritten arithmetic.

The strict TU advances **31 -> 32 of 33**, only this function changes, and
`_decal_new_from_collision` remains residual. Exactly three helper definitions
are added; none removed. Existing symbol metadata, COMMON, and all 89
non-debug non-code sections remain unchanged.

| Added helper | Padded bytes / relocations | January selected provider |
| --- | ---: | --- |
| `_point_from_line3d` | 48 / 0 | `source/ai/action_charge` |
| `_project_point3d` | 144 / 10 | `source/ai/path_obstacles` |
| `_set_real_point2d` | 32 / 0 | `source/ai/path_obstacles` |

Each matches both January and the current canonical provider, including
hardened relocation identity, and uses select-any COMDAT selection. January
decals already references `_project_point3d`; the other two additions fall
under the all-inlined shared-header exception. The donor packet's blanket
zero-new-symbol claim is not used.

## Actor destination: helper recovery, not new arithmetic steering

Only `_actor_destination_update` is taken from the donor source commit.
The six calls are two `vector_from_points2d`, two `dot_product2d`, one
`point_from_line2d`, and one `magnitude_squared2d`. The later unoptimized
function at `0x4627a0` independently identifies these calls, their arguments,
and the two RTC-named eight-byte vectors `actor_to_point` and `next_step`.
The previous handwritten double temporary and offset arithmetic are removed.

The strict TU advances **31 -> 32 of 36**. Only this function changes among
existing runtime sections. Non-debug sections advance 148 -> 150 solely
through `_dot_product2d` (32 bytes, zero relocations; action_charge provider)
and `_point_from_line2d` (48 bytes, zero relocations; action_vehicle provider).
Both match January and current canonical providers and are select-any COMDATs.
No data, COMMON or unrelated source change is admitted.

The seven point/vector prefix-view casts have per-site first-party argument
and layout evidence. The donor's broad strip control also removed unrelated
casts, so it is not used alone as an isolated seven-cast proof; independent
target-only strip measurements are retained with the reconciliation receipts.
Fresh production-flag scratch compiles remove only the target's seven casts,
without local renames or other source edits: all 150 non-debug sections remain
identical. Six additional single-site controls also remain identical. The
repeated step-point cast at the second call is covered by the all-seven test,
not claimed as a seventh isolated control. Expected incompatible-pointer
warnings occur only in these diagnostic cast-free copies, never production.

## Verification and limits

- Full `ninja` passes before and after; objdiff remains **3.3.1**, SHA1
  `3130e4288d483d259d1588092c8159f8e0230e08`.
- Stable whole-board sweep: **8,252 owners, 7,626 -> 7,629 exact**;
  precisely the three named gains, **zero regressions**.
- Parks: **77 active / 0 stale / 0 invalid**; no park edits required.
- Admission: **10 candidates / 0 contradicted / 1 rejected / 0 revoked**,
  unchanged.
- Fake scan: **26 inherited leads**, unchanged by path/rule/snippet.
- `/W3` has no new diagnostics. HUD retains C4146 x1 and C4244 x13;
  decals retains C4146 x1 and C4244 x11. Actor retains C4146 x1 and removes
  two C4244 warnings on its old handwritten double-to-real assignments
  (13 -> 11). No diagnostic is suppressed.
- Tools tests before and after: **1,161 passed, 5 skipped, 26 subtests**.
- `git diff --check` passes; existing source CRLF is preserved.
- No shared header, config, compiler, scorer or admission-status edits.

Selected-provider probes run both input orders for each affected provider.
They report no duplicate-definition conflicts. These deliberately incomplete
links retain expected unresolved dependencies; they are bounded coalescing
proofs, not successful whole-game links or boot tests. No executable is run.

Private receipts are preserved at:

- `scratch/reconcile_q6_q9_20260926/`: build, stable snapshots, original
  objects, accepted ledgers, parks, admission, warnings, fake scan and tests.
- `scratch/astra_q6_independent_audit_20260926/Q6_AUDIT.md`.
- `scratch/astra_q6q9_integration_20260926/q9_independent/`: source/section
  inventory, helper identities, four provider link logs and verification JSON.
- `scratch/astra_actor_destination_review_20260926/`: actor source/argument
  audit, helper/section comparison and fresh provider probes.

Exact supplied patch hashes:

- HUD: `16dadb1e6dec15177a34ac5d3b97924a54f1aa97004c8988fe9b1d4aeb31464e`.
- Decals: `4b9635d246620abf427bf8e8267e2c73d56c25ac27870ef89b19f5b01d941314`.
- Actor: `4e814be36415cbdcbe44e36557414e878f07b7510682f9272c23703b842ae252`.

The source deltas are those patches, with original CRLF retained. No donor
research bundle, binary, PDB, SDK asset or other untracked work is published.
