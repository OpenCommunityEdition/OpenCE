# `collision_usage.obj` small-family completion (2026-09-02)

## Outcome

The January object advances from 11/14 to 12/14 strict-exact functions. Both
formerly unwritten functions now have complete source: the 96-byte private
formatter is strict exact, and the 1,584-byte renderer is retained as a
credible fuzzy reconstruction. There are no unwritten functions left.

| Function | Before | After | Padded target / candidate | Relocations | Objdiff |
| --- | --- | --- | ---: | ---: | ---: |
| `_collision_log_format_usage` | unwritten | exact | 96 / 96 | 7 / 7 | 100% |
| `_collision_log_render` | unwritten | fuzzy parked | 1,584 / 1,536 | 79 / 76 | 82.027954% |
| `_collision_log_get_current_user` | residual | fuzzy parked | 256 / 256 | 23 / 23 | 99.14286% |

Strict matched padded code rises by 96 bytes. The two remaining functions are
defined ordinary C, so the object is now 12 exact, two residual, and zero
unwritten rather than being mislabeled complete.

## Source and donor evidence

The reconstruction was checked against all prior object ledgers and every
registered local research corpus. The historical Punpckhdq source preserves
the original lifetime/scope skeleton and the private name
`collision_log_format_usage`. The independent Stian PC reconstruction supplies
the complete semantic render path and a closely related formatter. HCEA
independently corroborates the exported debug controls used by the function.
The open Marathon source was searched for collision-profile, usage-log, debug
text, and timing analogues; it has no direct donor for this subsystem.

The retained renderer follows the January target topology: it scans eight
collision-function categories across three time periods, detects recorded
categories, aggregates all 22 collision-user records, sorts them by usage,
formats totals or up to six active users, configures terminal text rendering,
draws the line, and advances the screen position. The formatter uses project
types and the natural expression
`usage->elapsed_time * 1000.0f / frequency.QuadPart`; VC7 emits the exact
96-byte, seven-relocation January body. The renderer naturally inlines this
same helper at January's four formatting sites without forced-inline controls.

Private address placeholders were replaced with semantic names:

- `code_0013c830` became `collision_log_compare_overall_usage`;
- `code_0013ca00` became the donor-backed `collision_log_format_usage`;
- `code_0013d090` became the donor-backed
  `collision_log_get_current_user`.

The comparator's exact original private spelling was not present in the
available symbol sources, so its descriptive name follows the subsystem's
established naming convention. All four private helpers are marked static in
the symbol manifest. The renderer's needed public interface routine is now
declared in its owning `interface/interface.h`, not as a use-site declaration.

## Retained fuzzy boundaries

### `_collision_log_render`

The candidate is a complete 1,536-byte ordinary-C renderer against January's
1,584-byte envelope. The fresh report measures 82.027954% similarity. The
candidate has 76 relocations against 79. January reads only
`rasterizer_globals+16`; a natural typed `rectangle2d` copy makes VC7 also
materialize `+12`. Conversely, January directly materializes the aggregation
fields at `collision_usage_buffer+24`, `+32`, and `+36`, while the retained
typed-pointer spelling lets VC7 reuse one base calculation. These differences
propagate into a broad frame/register schedule rather than revealing missing
logic.

Prior ledgers record bounded declaration, scope, direct-expression,
frame-bound, source-pointer, and assertion-spelling families. Repeating them
either returns to this fixed point or perturbs broader control flow. The
fail-closed classifier reports `UNKNOWN`; the manifest therefore uses
`unclassified` rather than inventing a compiler recipe. Reopen only for
authoritative January source/local records or a natural compatible-compiler
donor that jointly explains the frame and aggregation lifetimes.

### `_collision_log_get_current_user`

Target and candidate share the 256-byte padded envelope and 23 relocations.
Aligned normalized disassembly differs at one successful-return instruction:
January emits `mov ax, si`, while VC7 emits `movsx eax, si`. A short-return
experiment changed the compiler's tail coalescing and shrank the function to
240 bytes, with 94.77143% report similarity, so it was reverted. Stian and
Pastudan's independent PC machine-code lifts infer that short signature, but
neither carries source or PDB type provenance and the spelling does not
reproduce January's distinct `NONE` exits. The retained natural `long` return
comes from the historical Xbox source donor and preserves the complete target
control topology without a steering construct. The assertion text and
punctuation now match the target-owned strings.

The target relocation spelling is important semantic evidence rather than a
cosmetic alias: `collision_usage_current+2974` resolves two bytes before
`global_current_collision_users`. It therefore proves the authentic stack-top
access subtracts one from `global_current_collision_user_depth` before indexing
`global_current_collision_users`. That expression is retained and its resolved
relocation destination is January's exact address, 5,835,422. The classifier
still reports `UNKNOWN` for the remaining narrow-return instruction, so this
helper is parked as `unclassified`.

## Strict measurements

| Function | Artifact | Padded size | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_collision_log_format_usage` | January / candidate | 96 / 96 | 7 / 7 | `d8093612e311c192f3d7d221a5cef430f5675929c89930ee732bb2137f65f218` |
| `_collision_log_render` | January | 1,584 | 79 | `dc4c9ded347bbcf3f6a5577d375e9611458a2c6b06039e89a01b7b47ceb1a213` |
|  | candidate | 1,536 | 76 | `f586255e6af97eacfd630c01c08aaa2ff43060eea09eaf9fe4257f2d00ecb94b` |
| `_collision_log_get_current_user` | January | 256 | 23 | `fc370d7e130011908802a1d09d62cadaccb97dac976f7c5705341b7eb44d1cb3` |
|  | candidate | 256 | 23 | `0f1289b9c2652df52cbd35e38ba0778b3d2ada82a9dd87a9528fb5928c017bf3` |

## House-rule audit

The retained implementation uses semantic private names, project `real` and
`boolean` types, named structures, interface enum constants, multiline
parameters, an explicit `return;` in every void function, and an owner-header
prototype. It introduces no address-derived symbol, raw-offset field access,
manual bit forcing, forced inline/noinline annotation, volatile steering,
barrier, pragma, assembly, undefined behavior, fake dependency, or
nonsensical exactness scaffold. No touched function calls or emits
`point_from_line3d`; January's inline/COMDAT schedule remains unchanged.

## Verification

The owner-header addition affects seven translation units, so admission used a
full-tree rebuild and a same-symbol-map regression sweep rather than relying on
the focused gate alone.

- `ninja all_source`: pass across all 569 source/library translation units;
- `ninja halobetacache_build libcmt_build progress semantic_progress`: pass;
- focused PID-safe gate: 12 exact, two residual, zero unwritten;
- mapped whole-tree strict sweep: 5,038/5,038 baseline-exact functions remain
  exact, one newly exact function, zero regressions, and 5,039 final exact;
- resolved relocation audit: target and candidate stack-top loads both resolve
  to image address 5,835,422;
- `python -m tools.parked_functions`: 88 active, zero stale, zero invalid;
- semantic audit: 472 units, 5,189 functions evaluated, 5,048 semantic exact,
  134 hidden exact, 5,059 accepted exact, and zero unit errors;
- object-admission audit: zero contradictions and zero revocations; the sole
  candidate is the unrelated pre-existing `player_rumble.obj` ownership audit;
- scoped fake-match scan: zero findings;
- `python -m pytest`: 255 passed;
- `git diff --check`: pass;
- built-object symbol audit: no `point_from_line3d` symbol or COMDAT.

The raw baseline diff naturally reports the exact comparator under its former
`code_0013c830` name as removed and under its semantic name as gained. Mapping
that one target-proven rename before comparison yields the zero-regression
result above; no match credit is created by the mapping.
