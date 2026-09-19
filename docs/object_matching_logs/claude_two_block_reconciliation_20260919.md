# Claude two-block reconciliation (2026-09-19)

## Scope and lineage

The imported work is one linear history rooted at GitHub canonical
`c3e257e9ab9a`, not a set of additive peer lanes.  It contains two effective
campaign blocks.  The second block ran through numbered Wave 21; the numbered
wave commits are not in numeric order because disjoint packets completed in
parallel.

| range | accepted functions | accepted meaningful bytes | strict stable functions | strict padded bytes | regressions |
| --- | ---: | ---: | ---: | ---: | ---: |
| `c3e257e9a..9062b1e96` | +343 | +207,308 | +344 | +209,152 | 0 |
| `9062b1e96..9b6ad2c75` | +122 | +86,494 | +122 | +87,488 | 0 |
| combined | **+465** | **+293,802** | **+466** | **+296,640** | **0** |

The strict meaningful-byte delta is +293,020.  It is intentionally reported
separately from the accepted-ledger delta: the latter includes authenticated
semantic/objdiff admissions and excludes bytes already credited at the base.
Checkpoint labels such as +10K, +40K, +75K and +85K are cumulative, not
additional gains.

The final accepted ledger is 7,437 functions and 1,433,963 meaningful code
bytes out of 11,060 functions and 2,198,102 bytes (65.24%).  The final strict
snapshot is 7,403 of 8,245 comparable functions.  These metrics have different
denominators and must not be combined.

`fable/comdat-research-20260917` is an alias of the final donor tip and adds no
work.  The separate `faae809ab` networking child is not an ancestor; its three
files were independently superseded by this chain and must not be cherry-picked.

## House-rule reconciliation

The final donor was rebuilt and compared before and after each correction.
The cleanup changes no strict verdict relative to `9b6ad2c75`: zero gains and
zero regressions.

- Authenticated `_actor_perception_refresh_danger_zone` in `symbols.json` and
  marked it private.
- Moved actor-combat external math declarations from the consumer translation
  unit into the math-owned `real_math_declarations.h`.  The existing alias
  COMDAT topology remains a disclosed whole-object owner debt and receives no
  extra credit.
- Restored `real_rgb_color_to_pixel32` to `bitmaps_inlines.h`, before the later
  alpha converter as required by January's assertion lines.  Its public
  declaration now belongs to `bitmap_color_conversion.h`; callers include that
  owner rather than borrowing a declaration from `hud_draw.h`.
- Corrected `rasterizer_dynamic_screen_geometry_parameters.map_offset` to the
  writable point type and removed six const-removing casts.
- Replaced the newly introduced raw weapon flag OR with `SET_FLAG`.
- Removed inert `register` spellings from geometry without changing any of its
  25 exact rows.
- Corrected the Bink trap comments and historical ledgers: `_bink_alloc@4` has
  an evidenced site-local `int 3`; it is not the only `int 3` in the image and
  is an explicit one-site owner ruling rather than a generic math-helper rule.

The RGB helper's single assembly block and Bink's single trap remain the two
narrow owner-approved exceptions.  RGB SELECTANY copies emitted naturally by
real callers are inline-header ownership artifacts and receive zero additional
credit.  No conditional suppression, fake dependency, forced inline, or
matching-only source was added.

Moving the RGB declaration to its authentic owner changes only the allocation
hashes of two already-fuzzy rasterizer-model bodies.  Both retain identical
size and relocation counts, stay at zero credit, and were remeasured with dated
evidence in `config/parked.json`.  No exact function was lost.

## Remaining whole-object owner debt

The donor owner census found 113 new owner pairs across 31 translation units.
The authentic RGB inline move adds four identical caller-owned SELECTANY
copies absent from the corresponding January splits, for 117 disclosed owner
pairs after cleanup.  Of the donor pairs, 105 are byte-identical SELECTANY
helper or stock-XDK table emissions and three are supported static data labels.
The four RGB copies are the same zero-credit inline-header class.  The
remaining source-topology debts are:

- `actor_combat`: invented alias owner
  `_actor_combat_real_random_range_inline`;
- `actor_moving`: nonidentical surplus `_distance3d`;
- `projectiles`: nonidentical surplus `_distance3d` and
  `_valid_real_vector3d_axes2`.

These objects remain `NonMatching`.  The debts do not invalidate independently
verified target-function credit, but they block whole-object admission until
authentic inline/header topology or ordinary link evidence resolves them.

## Final verification

- `python configure.py`
- `ninja all_source progress semantic_progress`
- stable diff from `c3e257e9a`: +466 functions, +296,640 padded bytes,
  zero regressions
- stable diff from original donor `9b6ad2c75`: zero gains, zero regressions
- parks: 190 active, 0 stale, 0 invalid
- admission audit: 28 candidates, 0 contradicted, 8 rejected, 0 revoked
- tests: 1,151 passed, 5 skipped, 26 subtests passed
- `git diff --check`: clean
- all seven direct `bitmaps_inlines.h` consumers passed the
  `_point_from_line3d` emitted-symbol guard
- fake-match review adds no unexplained lead: the Bink trap and RGB helper asm
  are the documented narrow exceptions

The intended publication target is the remote branch
`refs/heads/jonas/exact-pilots`.  Do not use the similarly named stale local
branch as the push destination.
