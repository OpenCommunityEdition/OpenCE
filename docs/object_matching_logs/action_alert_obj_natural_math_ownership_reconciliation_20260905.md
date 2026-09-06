# Action Alert: restore genuine math-header ownership (2026-09-05)

## Disposition

Root has applied the single source-only normalization packet to canonical
Action Alert. The actual full canonical Ninja build passes, and the complete
stable sweep remains 6,217 exact functions out of 8,245, with zero losses.
Root's actual all-section comparison and full tooling/tests also pass.
The actual Ninja object is runtime-identical to the reviewed PID candidate:
SHA-256 `c15f370e118de1a78c8cbcab7e3d4cd07f255d2c50af6c4b81e7c2cf391b46da`.
The exact baseline-to-actual changes are only the three selection transitions
and two unreferenced alias removals documented below. All 281 parks validate,
the admission audit has no contradictions or revocations, and 288 tests plus
26 subtests pass. The pytest cache permission warning affects only caching.
Root evidence: `scratch/math-owner-schema-actual-action_alert-20260905.json`,
`scratch/math-owner-schema-actual-summary-20260905.json`, and
`scratch/math-owner-schema-canonical-first-20260905.json`.

The first natural candidate retains all
11 strict exact functions, the identical existing complete residual, all six
runtime data owners, and every gameplay ABI and instruction. No exact bytes
are gained or lost. The object remains 11 exact / 1 residual / 0 unwritten,
not whole-object Matching.

This is an ownership correction, not a new gameplay or residual experiment.
Canonical production source, headers, configuration and build outputs were
not changed by this worker.

## Source provenance and prior adjudication

Read in full before the test:

- `docs/matching_methodology.md` and `tools/campaign/README.md`;
- `action_alert_obj_jonas_prefix_recovery.md`;
- `action_alert_obj_jonas_safe_geometry_pair_20260826.md`;
- `action_alert_obj_jonas_distance_squared_fixed_point_20260826.md`;
- `action_alert_obj_jonas_setup_store_order_rejection_20260830.md`;
- `action_alert_obj_complete_fuzzy_closeout_20260901.md`;
- `action_charge_obj_jonas_math_helper_recovery_rejection_20260826.md`;
- `action_charge_obj_jonas_math_helper_recovery_v2_20260826.md`;
- `action_charge_obj_jonas_real_random_recovery_20260826.md`;
- `game_sound_obj_obstruction_current_owner_boundary_20260905.md`.

The geometry-recovery ledger explicitly introduced local pre-header aliases
and duplicate ordinary external math bodies to add selected code owners while
the real gameplay callers were still unwritten. It described the macros as
duplicate-definition avoidance; they nevertheless changed ordinary COMDAT
ownership and now materialize two additional alias-named helper bodies.
The complete 20260901 reconstruction introduced the genuine gameplay uses.

The old distance fixed point tested a fourth renamed wrapper, not removal of
the existing alias layer to restore normal header topology. The old setup and
next-position source-shape exhaustion is unrelated and is not reopened.
No prior natural owner-header normalization experiment is documented in these
ledgers. The new concrete premise is the demonstrated current canonical
LNK2005 from Action Alert's ordinary selection-1 duplicate when natural real
callers in Game Sound and Observer emit the same header API as selection 2.

`source/math/real_math.h` owns the actual APIs and bodies:

- line1027: `__inline real_vector3d *vector_from_points3d(...)`;
- line1049: `__inline real magnitude_squared3d(...)`;
- line1090: `distance_squared3d`, which genuinely invokes both above;
- line1359: `__inline real real_random_range(...)`.

`source/ai/actions.h:12` already includes that header, and Action Alert also
explicitly includes it. The existing distance callers in update, next-position
and perform plus the random-range caller in perform naturally expose all
three helpers. No synthetic caller, forced-inline annotation, compiler flag,
header edit, or code-generation control is required.

## Exact minimal edit

Apply only the three replacement records in
`scratch/action_alert_natural_math_owners_20260905.edits.json` to
`source/ai/action_alert.c`:

1. Remove three pre-include `#define original original_inline` aliases.
2. Remove their three corresponding `#undef` lines.
3. Remove the duplicate ordinary external definitions of real_random_range,
   vector_from_points3d and magnitude_squared3d.

Every include, gameplay body, private declaration, type, constant, enum, and
call remains untouched. This restores existing owner APIs; it does not add
inlining or relax the January point_from_line3d schedule.

## First natural gate and full runtime audit

Commands from the canonical worktree, with ordinary per-TU flags:

```text
python tools/campaign/gate.py source/ai/action_alert --all --out scratch/action-alert-natural-math-baseline-replay-20260905.obj --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/ai/action_alert --edits scratch/action_alert_natural_math_owners_20260905.edits.json --all --out scratch/action-alert-natural-math-first-20260905.obj --forbid-emitted-symbol _point_from_line3d
python scratch/action_alert_natural_math_owner_audit_20260905.py
python scratch/action_alert_natural_math_link_review_20260905.py --scan --link
```

Exactly one untouched baseline replay and one natural changed compile were
used. No source retry followed. Both gates report 11E/1R/0U and no forbidden
point_from_line3d owner. The residual next-position remains 768 candidate
versus 784 target padded bytes; its complete section fingerprint is unchanged.

The complete all-section audit includes anonymous/code/data/debug/directive
sections, COMMON and symbol inventories, offsets, storage and type, section
flags and selection, and ordered relocations including full target-symbol
metadata. It does not mistake function-only equality for whole-owner equality.

- Target:12 code owners,6 data owners.
- Actual baseline and untouched replay:14 code owners,6 data owners.
- Candidate:12 code owners,6 data owners.
- No new runtime owner, no COMMON owner or COMMON change.
- The only removed runtime owners are `_vector_from_points3d_inline` and
  `_magnitude_squared3d_inline`, each48 padded bytes. Neither exists in January.
- The only inherited runtime changes are COMDAT selection1 to selection2 for
  the three actual public math helpers. Each is otherwise completely identical
  in bytes, ordered relocations, symbol metadata/offsets and section flags.
- All other inherited code, data and runtime records are completely identical,
  including the parked body; there is not even a compiler-local-label change.
- Actual baseline and untouched PID replay have no runtime differences.
- COFF runtime header characteristics are unchanged. Debug/output-path records
  are separately preserved in the raw audit and are not runtime credit.

The target selection1 metadata is retained in the audit. This packet does not
patch csplit metadata or claim that its original source-object selection was
recovered from the linked January image. The natural header source generates
ordinary select-any owners and repairs real current-source ownership debt.

## All-owner census and ordinary link checks

The census parsed all833 configured January targets and572 actual canonical
bases, with zero errors. Among current base definitions:

| Helper | Base owners | Selection-1 owners |
|---|---:|---|
| vector_from_points3d |51|Action Alert only|
| magnitude_squared3d |67|Action Alert only|
| real_random_range |8|Action Alert only|

Every other base copy is selection2. Both alias-named extras exist only in
Action Alert; there are zero incoming relocations to either alias across all
target and base objects. Their removal cannot leave a referenced alias owner
unresolved.

All magnitude and random-range copies have the January size/hash. Fifty
vector copies have the January hash; the existing Breakable Surfaces copy has
hash `a947d351c7b0abf577d0b6e41f02a4f0a7418104d3099e62f6835a862b355b57`.
That frozen48-byte/zero-relocation copy differs only by moving the argument
load `mov edx,[ebp+8]` before rather than after the first FLD. All26 decoded
instructions and dataflow otherwise agree. It uses the actual real_math.h
API and is semantically/ABI equivalent, not byte-identical. No expression or
source change was made to suppress or force this ordinary pooled copy.

Ordinary XDK Link.Exe `/NOLOGO /DLL /NOENTRY /NODEFAULTLIB` tests with unchanged
frozen natural Action Alert + Game Sound and Action Alert + reviewed Observer,
each in both input orders, now have no LNK2005. The previous actual-baseline
Game Sound pair has LNK2005 in both orders in its durable rejection ledger.
The new runs still end in expected unrelated LNK1120 unresolveds (57 for Game
Sound,40 for Observer), and none produces a DLL. Thus this is a passed bounded
duplicate-symbol check, not successful whole-link proof. No /FORCE, /IGNORE,
object patches, synthetic stubs, header shim or linker accommodation is used.

## Frozen evidence

All following paths are relative to the canonical worktree.

| Artifact | SHA-256 |
|---|---|
| scratch/action-alert-natural-math-target-20260905.obj |351ff0de9ddfea9b3c03a9038abab75b0c4b48290443b33e09c33105b41ab61a|
| scratch/action-alert-natural-math-before-20260905.obj |b82fd9a16f40c01fc45ac810240ad9ea9e02287ae629827c01306b17f7e38d30|
| scratch/action-alert-natural-math-baseline-replay-20260905.obj |35bbfb9e7f3f0690af27b6772189d4360e1fbddad71ba81b57b98f027397f549|
| scratch/action-alert-natural-math-first-20260905.obj |62ffdb7cd1e7c082b6f03216d4c10bc3437a8f3d0d11d01ba454b0981fc8ac44|
| scratch/action_alert_natural_math_before_20260905.c |3a57d07c1c1173cb26903ca40da1b4fc714d11779e65e34bfe8e6f7a40e1ce6d|
| scratch/action_alert_natural_math_candidate_20260905.c |27d9268805f0e92024ec10304fbb3dee1398d54055cfb01219d8796f7f0f28af|
| scratch/action_alert_natural_math_owners_20260905.edits.json |a91e4428803f9cc697e4d06b52abfbe82c69f8425b9d518360dede80a9624374|
| scratch/action_alert_natural_math_owner_audit_20260905.json |9c4bcb9e971e188518b8dda1a9ea20d894bfbd56f12e7fd7e7426902a5eef889|
| scratch/action_alert_natural_math_all_owners_20260905.json |737872e034ecd35ee428462306ed87cf6b12f244413944be5625f4701c07ac92|
| scratch/action_alert_natural_math_ordinary_links_20260905.json |f1b5c5d828edd62cffa6882d9469c59f50c14a2d985547394852c9a4c169d682|

## Canonical integration status

Root read the complete source packet, manifest and primary scratch ledger,
then applied only the minimal Action Alert source removals. The full canonical
Ninja build passes. The complete stable sweep reports 6,217 exact functions
out of 8,245, with zero exact losses and no new exact credit from this cleanup.
These are actual canonical results, separate from the frozen scratch results
above.

Full actual runtime-owner comparison and the remaining tooling/tests are
pending at the time this durable ledger is authored. Root must complete those
checks before treating this batch as fully verified. The existing residual
and park remain unchanged, and no whole-object completion or new-byte claim
is made. The ordinary two-object linker diagnostics above remain bounded
duplicate checks, not a successful full-game link.
