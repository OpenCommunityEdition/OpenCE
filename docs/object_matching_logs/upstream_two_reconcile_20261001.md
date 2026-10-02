# Upstream two-function reconciliation, 2026-10-01

Base: `6bfd95673719d54b92d0b23f01dc9df4c0679768`.
Donor: punpckhdq/halo `6a23ce979ac09f7ca032449ca205ceb82eac4b81` (CC0).
Scope: cleaned triangle predicate and custom waypoint renderer only.
User authorized reconciliation and publication after the research review.

| Function | Meaningful bytes | Padded bytes | Relocations |
| --- | ---: | ---: | ---: |
| `_triangle_coplanar` | 380 | 384 | 11 |
| `_custom_render_nav_point` | 1,618 | 1,632 | 95 |
| Total | 1,998 | 2,016 | |

## Fresh production checkpoint

- Clean baseline and final builds of all source outputs, unchanged compiler
  flags and production comparator. The first baseline attempt encountered
  a compiler intermediate-file permission failure; both successful clean
  builds use a dedicated scratch TEMP/TMP directory. No source workaround.
- Rename-stable whole-board comparison: 8,252 rows, exactly these two gains,
  zero losses and no other verdict changes (7,646 -> 7,648 all-category exact).
- Halo functions: 7,473 -> 7,475 of 7,573. Halo code:
  1,597,714 -> 1,599,712 of 1,770,166 meaningful bytes (90.37%).
- Halo data unchanged: 3,311,591 / 3,922,163. Objects unchanged: 396 / 468.
  Fresh Q10 receipts retain precisely 661,012 bytes / 93 records.
- Parks: 68 -> 66, exactly the two new exact functions retired by line
  surgery; zero stale or invalid parks. Earlier research history preserved.
- Admission: 7 -> 8 review candidates (HUD becomes a candidate only);
  zero contradicted, rejected or revoked admissions. No status flips.
- Full default-build warning set unchanged. Independent /W3 compiles add
  only the six disclosed HUD C4244 narrowing warnings; triangle adds none.
- Fake scan: the same 26 inherited leads; no new leads.
- Tools tests, baseline and final: 1,663 passed, 5 skipped,
  159 subtests passed. Whitespace checks pass.
- Canonical's inherited dirty README and unrelated research are preserved
  and excluded from the commit. Source/config scope is exactly two bodies,
  a plain epsilon constant and two park retirements. No tool/scorer changes.

## Independent ownership and provider audit

Final production objects equal the cleaned research candidates in all
73 HUD and 27 triangle non-debug sections, including symbol attributes
and COMDAT metadata. January's 46 HUD and 13 triangle owned symbols agree.
All 39 surplus code/data definitions match both January and current
selected providers. All six newly emitted helper copies have the correct
ANY selection, flags and alignment. No new COMMON, import or storage
disagreements are introduced. The triangle's sole inherited residual is
find_or_add_vertex; its unrelated selection discrepancy remains disclosed.

18 provider pairs in both orders (36 probes) produce only the expected
unresolved-external exit 1120; no duplicate definitions, unexpected linker
diagnostics or output images. 864 pinned inputs remained unchanged.
This is bounded duplicate/provider evidence, not a whole-program-link
proof. Surplus copies receive no duplicate credit.

Independent source review found no new filler, instruction emission,
representation-punning view or held-donor bleed-in. The held vertex,
bitmap text, plane-header and assembly-dependent upstream forms remain out.

## Local receipts (private assets not published)

Reconciliation directory: `scratch/astra_upstream_two_20261001/` in the
canonical checkout. Separate independent review directory:
`scratch/punpck_rest_check_20261001/` in the task workspace.
The following SHA-256 pins refer to persisted receipt files, not binaries.

| Receipt | SHA-256 |
| --- | --- |
| reconciliation `final.json` | `822618c8f985e735fd4c545959d4bd412055cc02e3b541a104f09436dff387b8` |
| reconciliation `final_build.log` | `b3bde6cacbe2040bc652717cf65671f59e04bc0e410186171d19acc7162b486c` |
| reconciliation `final_pytest.log` | `a731d14e8c2f720f60272bf412646de8d03ebb089744c6e3ad92097af242e92f` |
| independent `admission/REPORT_final_hud_final_triangle.json` | `b2690904dca233fdee7a05da2b1ce391c54b48b8d6d786bf67324ab4902da627` |
| independent `connected_geometry/INDEPENDENT_FINAL_POLICY_REVIEW.json` | `235fc01a4762151d4caa5ff9b12e3b00778fe0c0d978c732afaeb7d9ae6ab58d` |

Publication destinations authorized in the current task: both bnunu/halo
and bnunu/halo-1 `jonas/exact-pilots`, plus bnunu/halo-1 `main` through a
normal merge preserving main's README. No force push or other repository.
