# collision_move_point: genuine shared helper closure

Base: `3ea3ba1f31d6805565236b0a99e25a61f21fbb0a`, canonical
`jonas/exact-pilots`. Owner approved reopening on 2026-09-27 only if the full
consumer sweep and helper/link audits passed with zero exact losses.

## Result

`_collision_move_point` is strict exact: **4,744 meaningful / 4,752 padded
bytes**, 226 relocations, normalized SHA-256
`8b2f29007193d3aacd830e10fa99886acd6a3bc1e9cd604483343340f6117e2c`.
One new function, no exact losses, no new data credit, no object admission.
The existing semantic-COFF verifier credits the function without any scorer,
normalization, allowlist, symbol, compiler, or flag change. The raw objdiff
display remains 99.64639%; that is not the strict verifier's verdict.

Halo accepted code: 1,593,498 -> **1,598,242 / 1,770,166** meaningful bytes;
7,468 -> **7,469 / 7,574** functions. Complete Halo objects remain 389/468;
credited data remains 2,588,903.

## Minimal independently justified source packet

- Move `valid_real_plane3d` from its plain `matrix_math.c` definition into its
  existing owner `real_math.h` as an ordinary `__inline`, beside
  `valid_real_normal3d`. Remove the obsolete prototype and duplicate body.
- Restore the real helper call in the collision loop assertion, replacing
  its hand-expanded predicate. No local, scope, arithmetic or control-flow
  change is made to `collision_move_point`.
- Remove only the redundant `bitmap_delete` declaration in
  `bitmaps_internal.h`, which already includes `bitmaps.h`. Keep the genuine
  public declaration in `bitmaps.h` unchanged.

No stack-walk declaration choice, one-consumer header, public prototype
deletion, forced inline, assembly, decorative parentheses, cast, dead local,
or count filler is included. This resolves the old R3 3a/3b blocker by using
the genuine owner route in the current context, not by waiving a regression.

January expands the plane predicate in the loop at +0x485..+0x4E8, but calls
the helper out of line at +0xC80 (relocation +0xC81). A fresh read of the later
unoptimised build finds calls to the same helper at 0x7A5833 and 0x7A6118.
The helper at 0x6C7730 short-circuits the normal check before testing the
float at plane+0xC, agreeing with January and the existing implementation.
The existing function-scope `position` is retained; the later build also
initialises and reassigns the same local at 0x7A52DC and 0x7A5710.

This later build supplies corroboration, not January source text. Its SHA-256:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
The header placement is independently supported by its existing API,
predicate callees and plane-assertion macros.

## Controlled measurement and complete consumer census

The pre-registered scratch test first measured A (helper/owner/caller repair)
alone: target exact, but `bitmap_copy` regressed. A+B, deleting only the
genuine internal duplicate, restores `bitmap_copy`. No alternate declaration
position or public-copy deletion was searched. B alone was not tested.

Fresh controls and A+B were compiled for **467/467 Halo translation units**
(466 C, one C++): 273 consume `real_math.h`, nine consume
`bitmaps_internal.h`. An initial inventory omitted 13 wrapped Ninja rules
and the C++ unit; all 14 were compiled before accepting the complete result.

- Only target gain; zero inherited exact losses.
- The only other inherited code change is the already-residual
  `rasterizer_frame_statistics_draw`. Its prior owner-approved debit remains;
  this packet neither restores nor debits it again.
- Data, COMMON records, inherited code flags and warning multisets unchanged.
- Fresh control code, definitions and data reproduce canonical production.
- Only two new semantic definitions: `_valid_real_plane3d` in `collisions`
  and `render_cameras`.

## Helper ownership and links

The selected January provider is `math/matrix_math.obj`. All three current
copies match its 64 bytes and complete single relocation:
`+0xA REL32 _valid_real_normal3d`. Normalized SHA-256:
`5a34cbcdbbf344be1028b916761a963168854f9776601d3fa21031a312a54b4c`.
Code-section flags match. The rebuilt inline copies use COMDAT ANY (2);
the split provider and former plain definition use NODUP (1). This selection
change is explicit and required by the genuine shared-header repair.

The other new copy is used by already-exact `render_camera_mirror`
(relocation +0x118), not the unrelated residual `render_camera_build_frustum`.
The collision caller is now exact. Pair links against the rebuilt selected
matrix-math provider pass in both orders, with no LNK2005/LNK1169. Remaining
unresolved-external diagnostics are retained: these are bounded duplicate/
selection checks, **not a complete program-link or boot claim**. No helper
copy receives duplicate code credit.

## Canonical verification

- `ninja all_source progress build/report.json`: passes.
- Whole-board stable sweep: 8,252 owner rows; 7,640 -> **7,641 exact**;
  exactly one gain, zero regressions.
- Parks: **71 active, 0 stale, 0 invalid**. Target had no park to retire.
- Admission: **13 candidates, 0 contradicted, 1 rejected, 0 revocations**
  (one additional candidate, not an automatic Matching admission).
- Fake-match findings: unchanged at 26 inherited leads.
- Pytest: **1,161 passed, 5 skipped, 26 subtests passed**.
- No new warnings; `git diff --check` clean.

Receipts remain locally under `scratch/astra_next_function_20260927/`:
`gates/` (fresh baseline/final build, stable sweep, parks, admission, fake scan,
tests), `review/` (pre-registration, source generator, frozen full consumer
results, compact summary), and `math/` (independent source review, fresh raw
first-party readouts, complete helper identities and both-order linker logs).
Private reference binaries and generated objects are not published.

Untouched holds, other worktrees, inherited README edits and unrelated
untracked research remain outside this commit. No push was requested.
