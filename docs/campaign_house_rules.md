# Campaign house rules and working cadence

User-confirmed 2026-09-07. Apply these to reconciliation and new reconstruction.

1. Put a no-argument `void` on its own line inside the parameter list.
2. Put each parameter on its own line.
3. Use explicit returns: `return;` for void functions, an appropriate returned
   value for non-void functions. Do not add an invalid bare return to non-void C.
4. Use subsystem typed macros wrapping `tag_get`.
5. Use typed object-access macros instead of repeated raw-access casts.
6. Preserve January's inline schedule; do not emit a `point_from_line3d` COMDAT.
7. Give private functions authentic or descriptive project-style names, never
   `code` plus an address.
8. Likewise name globals; do not add `bss` plus address identifiers.
9. Put prototypes in their genuine owners, not unrelated consumer C files.
10. Helper/math functions may use inline assembly sparingly, with evidence.
11. If easy exactness fails, retain credible fuzzy code, document it and park.
12. Prioritize small objects in substantially reconstructed families.
13. Avoid manual/forced inlining; retain ordinary helpers and natural scheduling.
14. Use open-source Marathon as a style/semantic reference, not January byte proof.
15. When no dedicated header exists, use the closest genuine associated header.
16. Use correct enum constants in switch tables.
17. Combine declaration and initialization when practical.
18. Prefer suitable `cseries.h` macros over manual bit manipulation.
19. Use project types, e.g. `real`, rather than generic replacements.
20. Reject fake matching and source implausible for the original program.
21. Reject nonsensical logic even when bytes happen to match.
22. Use `/QIfist` where required by the target/compiler evidence. It is a
    per-TU option, not a blanket default or license to change unrelated flags.
    Check the complete TU and affected callers; conversion instructions and
    widths must agree. Existing usage is recorded in `config/config.json`.

## Batch workflow

Reconcile coherent multi-object or owner/family packets, normally several
thousand bytes when available. Review source quality, headers, ABI and emitted
ownership together; run one full build, tools suite and stable regression sweep
per batch. Use existing strict gates for ordinary exact leaves. Reserve new
bespoke runtime harnesses for ambiguous behavior/ABIs, not every tiny function.
Isolate blockers without holding unrelated admissible work. Preserve donor trees.

Publish only verified source/tooling gains to canonical and the authorized
GitHub branches. Donor-relative headline totals are not additive canonical
credit. Keep meaningful exact bytes distinct from padded and fuzzy counts.

User clarification, 2026-09-07: preserve the best available house-rule-compliant
Claude implementations in canonical and GitHub even when non-exact. Document
the donor, fresh measurements, remaining differences and research pointers;
do not leave useful fuzzy work stranded in donor trees. Closeness never
justifies undefined/nonsensical logic or inappropriate Matching credit.
Any accepted loss of exactness must be explicitly recorded and subtracted,
not hidden behind the batch's gross gains or a semantic-match override.

Refresh the canonical treemap after each **10,000 additional verified reconciled
meaningful code bytes**. Starting checkpoint for this cadence is **930,828**;
the first threshold is **940,828**. Record the next threshold in the queue.
Do not start a timer or unrelated recurring automation for this work threshold.

## Quick references

- [Constants, types, flags and representations](common_constants.md)
- [Assertions, warnings and halts](assertions.md): predicates express required
  invariants; diagnostics are lazy. The weapon example needs `!TEST_FLAG(...)`.
  VC7 uses a formatted message expression, not modern variadic macros.
- [Supplied-source reconstruction map](user_source_reconstruction_map_20260906.md)
- [Matching methodology](matching_methodology.md)
- [Reconciliation queue](reconciliation_queue_20260907.md)

Cross-build symbols/types are evidence, not interchangeable January layouts.
Analyze supplied executables as data; do not execute them. Preserve binaries
and private symbol assets locally, outside published source batches. Inlined
source and absent debug assets cannot be recovered automatically with certainty.
