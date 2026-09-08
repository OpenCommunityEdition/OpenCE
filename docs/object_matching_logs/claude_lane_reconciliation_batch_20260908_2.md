# Claude lane reconciliation batch 2 (2026-09-08)

## Result

This batch reconciles reviewed work from Fable lane
`fable/50k-resume-20260906` and Opus lane
`opus/30k-ui-script-profiles-20260906` against published canonical commit
`a7a32f6c7`. The donor lanes started from older, different baselines, so their
headline totals are not additive. Every number below comes from a fresh build
and comparison in canonical after replaying only the accepted owners.

| accepted packet | strict functions | meaningful code bytes | padded code bytes |
| --- | ---: | ---: | ---: |
| `hs_compile` parser owners | 26 | 5,667 | 5,952 |
| `playlist_profile` owner graph | 9 | 2,418 | 2,464 |
| UI game-data leaves | 8 | 2,543 | 2,608 |
| **strict subtotal** | **43** | **10,628** | **11,024** |

The global semantic report additionally admits `_hs_parse_enum` as a
459-byte relocation-alias exact match. It is not included in the strict
owner table because its object bytes differ only at relocation-bearing
addresses, while relocation targets, addends, instruction shape and
meaningful bytes are identical. Canonical meaningful exact code therefore
advances from **951,120 to 962,207 bytes**, and credited functions advance
from **6,373 to 6,417**. Initialized/read-only playlist strings and the owned
playlist runtime prefix add **1,032 exact data bytes**, taking canonical data
from 2,149,740 to **2,150,772 bytes**. Complete-object admission remains
390/833; this is a partial-owner batch and does not invent whole-object
credit.

## Reconciliation boundaries

The accepted HS bodies were reconstructed in their real translation unit.
Externally linked parser declarations live in
`hs_library_internal_compile.h`; private helpers have semantic names and
`static` linkage. The packet retains `verify_hs_syntax_node_size`, uses
`HS_MAXIMUM_DYNAMIC_SOURCE_DATA_BYTES` instead of a magic `0x400`, and leaves
the best credible non-exact relocation cases explicitly parked. Donor bodies
for `hs_compile_dispose`, `hs_parse_boolean`, `hs_parse_inspect`, and
`hs_parse_set` were rejected or repaired because their apparent matches
contained double-free, undefined-read, allocator-carrier, or invalid `%s`
argument logic. Coincidental bytes did not override semantics.

PlaylistProfile now owns its exact 116-byte runtime-prefix BSS and 14/14 exact
code functions. Its 105-byte initialized default-data owner remains external,
so no whole-object or missing data credit is claimed. Public playlist helper
prototypes live in the narrow associated `game_engine_playlist.h`; placing
them in the broad game-engine header changed unrelated C2 allocation and
regressed two race owners. Explicit representation casts in PlayerUI and UI
game-data prevent a false 704-byte type-based credit.

Eight UI game-data functions were accepted. The 1,416-byte multiplayer
settings-list owner is admitted through `config/semantic_matches.json` only
after hardened COFF comparison proved equal size, all 73 relocation rows
equivalent, and normalized text SHA-256
`adce7f9ab32aee72ec9d542efc297df9295a839a27406848eb8467ffea97594f`.
The ordinary objdiff score is lower only because compiler-local switch-table
labels cannot be paired by name. This is exact relocation-aware evidence,
not fuzzy credit.

Fable's wholesale Bink, Aim, Physics and Geometry candidates remain rejected:
they introduced helper COMDATs, ownership/header violations, unsafe naming,
or other cross-owner effects. The lane's uncommitted `encounters.c`,
`xbox_texture_cache.c`, `collisions.c`, and ledger changes remain queued for
separate review and were not imported blindly. Four otherwise useful donor
implementations remain documented as credible fuzzy progress with zero exact
credit. Fifty-one donor owners were already exact in canonical and receive no
duplicate credit.

## Tooling

This batch also adopts the reviewed general-purpose campaign tools:

- `merge_candidates.py` for deterministic candidate-set comparison;
- `relocdiff.py` for relocation-aware COFF evidence;
- `stdcall_scan.py` for calling-convention review.

Each has focused tests and documentation. Experimental donor utilities that
could silently grant semantic matches, refresh park evidence, or report
ambiguous relocation counts were not imported.

## Verification

- Full `ninja` build and semantic progress report: pass.
- Rename-stable whole-tree comparison: 43 strict gains / 11,024 padded bytes /
  zero regressions.
- Full tracked tools suite: 1,140 passed, 2 skipped, 26 subtests passed.
- Focused gates: `hs_compile` 53 exact / 11 residual / 0 unwritten;
  `playlist_profile` 14/0/0; `player_ui` 42/0/0; UI game-data 35/8/3;
  `game_engine_race` 35/1/0; HS runtime 445/3/0.
- Park manifest: 336 active, zero stale, zero invalid.
- Object-admission audit: zero candidates and zero contradictions; prior
  documented rejections remain rejected.
- Changed-source fake-match scan: zero findings.
- January `point_from_line3d` emitted-symbol guard: pass for every focused
  gate.
- `git diff --check` and JSON parsing: pass.
- Local multi-lane treemap force-refreshed from this rebuilt report at
  `http://127.0.0.1:8765/`.
