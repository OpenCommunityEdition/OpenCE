# Periodic functions global ownership (2026-09-24)

Baseline: canonical `fdedd9c9`. The old 76-byte `periodic_functions_globals` aggregate and its 3-byte padding were invented. The later HCEX PDB records separate file statics `periodic_function_tables` (`byte *[12]`) and `transition_function_tables` (`byte *[6]`); January's bare `!function_tables_initialized` assertion string corroborates the standalone boolean. Three initialized file statics, in January's section order, restore its 4-byte `.bss` alignment and symbol layout. `periodic_function_build_table` is file-static per HCEX and absent from January's public list.

The rebuilt 96-byte periodic/transition enum `.data` has the same normalized payload, storage, section flags and 20 resolved relocation destinations as January. Objdiff 3.3.1 undercredits its folded string-literal names. A single-section semantic-data entry credits **96 data bytes**; it does not credit the residual variable-period builder or mark the object Matching.

Verification on the isolated worktree: full `ninja progress` succeeds; the six previously exact functions remain exact and `@periodic_function_build_variable_period_x_table@4` remains residual at 256 bytes/18 relocations. Stable board diff: 0 gained functions, 0 regressions. Parks: 83 active, 0 stale/invalid. Admission: 0 contradicted, 0 revoked. Fake-match scan: 0 leads. Pytest: 1,161 passed, 5 skipped, 26 subtests passed. `git diff --check` clean. Halo data credit moves 2,586,915 → 2,587,011; Halo code stays 1,577,237 / 1,770,166.

The remaining one-function gap is the documented `inc edx` / x87 load scheduling choice; no unsafe source-shape workaround was introduced. The object still requires that closure and selected-provider link review before whole-object admission.
