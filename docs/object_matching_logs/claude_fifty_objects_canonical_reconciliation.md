# Claude fifty-object lane: canonical reconciliation

The donor is `claude/fifty-objects-20260925` at `4e84c212`, based on
`e9e62b78`. The receiving canonical base is `jonas/exact-pilots` at
`dbcea3d6`. This integration used the existing objdiff 3.3.1 binary on
both sides; `configure.py`'s 3.6.0 pin was not used to re-score the lane.

## Measured result against current canonical

| Measure | Before | Reconciled | Net |
| --- | ---: | ---: | ---: |
| Halo objects marked Matching | 364 / 468 | 381 / 468 | +17 |
| Strict-exact functions, stable 8,252-row sweep | 7,600 | 7,614 | +14, 0 lost |
| Newly exact meaningful function bytes | | | +8,775 |
| Newly exact padded function bytes | | | +8,864 |
| Halo credited code bytes, including status/scorer effects | 1,564,286 | 1,573,109 | +8,823 |
| Halo credited data bytes | 2,580,532 | 2,583,826 | +3,294 |
| Valid active parks | 96 | 86 | 0 stale, 0 invalid |

The meaningful/padded function totals are the 14 gained section identities
from the stable sweep cross-checked against `semantic_report.json`. The
net credited-code delta is **not** substituted for newly exact function
credit: object-status/scorer effects account for its extra 48 bytes.

## Conflict and policy resolution

- `path_structure_bsp.c`: retained canonical's explicit eight-element
  `real const` arrays. The second table's authenticated symbol entry is
  present once in `symbols.json`.
- `hs_scenario_definitions.c`: retained canonical's separately defined,
  already-Matching data and private `hs_syntax_data_byte_swap` name. The
  source pointer and `symbols.json` name agree.
- `symbols.json`: kept the union of nonoverlapping donor names and canonical
  data rows; no JSON reserialization or descending-address insertion.
- `object_admission_rejections.json`: removed only rejections resolved by
  the merged source/provider repairs or by canonical's earlier independent
  work. The dynavobgeom rejection remains.
- `rasterizer_xbox_dynavobgeom.c`: retained the genuine type/layout assertions
  and the donor's source improvements, but did **not** mark the object Matching.
  Its 17 target functions and owned data remain exact. The stock SDK-table
  coalescing criterion conflicts with canonical's existing ruling, and
  `global_window_parameters` still has consumer-local declarations. No
  whole-object credit is granted pending a uniform policy/owner audit.
- Layer 2's two random-seed lock prototypes remain in `real_math.h`'s
  `RANDOM_MATH.C` section. The supplied first-party `random_math.c` includes
  `real_math.h`, not the reconstructed `random_math.h`, and `main.c` needs
  these declarations. Their simultaneous declaration-count effect is
  disclosed; byte evidence alone does not identify the exact original
  header placement. The newly moved no-argument prototypes were reformatted
  with `void` on its own line. A future full random-math header consolidation
  must retain the same whole-board no-regression gate.
- P7's removal of the unused `hud_draw.h` include from
  `rasterizer_xbox_models.c` remains. The include has no direct object-byte
  effect; its declaration-count interaction is disclosed, and the removal
  is an ordinary dependency cleanup rather than a filler declaration.

The donor's held owner queue and fuzzy experiments remain in
`research/fifty_objects_20260925/`. No January-bug candidates or the
five-object actor-combat provider packet were silently admitted.

## Gates

- Full Ninja build: passed with objdiff 3.3.1.
- Stable exact-function diff: 14 gained, zero regressions.
- Parks: 86 active, zero stale/invalid.
- Admission audit: zero contradicted, zero revoked; dynavobgeom is explicitly
  rejected, not silently admitted.
- `python -m pytest -q tools`: 1,159 passed, 5 skipped, 26 subtests.
- Production-path `git diff --check` (`config source docs tools`): passed.
- Fake-match scan: one new **zero-credit** lead in the parked
  `first_person_weapon_update` body, an empty then-arm attested in the
  donor's `/Od` evidence. No new lead in newly credited exact bodies.

The donor's raw research tree contains captured patch/diff examples with
intentional trailing whitespace. It is retained as evidence, not production
source, and is excluded from the production-path whitespace assertion.
