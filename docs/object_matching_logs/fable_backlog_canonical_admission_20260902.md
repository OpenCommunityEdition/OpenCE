# Fable backlog canonical admission (2026-09-02)

## Outcome

The preserved unfinished Fable small-family work has been reconciled into the
current canonical branch in three independently gated lanes. Relative to the
post-Opus canonical snapshot at `853c62aa6`, the stable target-section sweep
adds **46 strict-exact functions and 15,568 padded code bytes with zero
regressions** across all 8,245 tracked functions.

| Object | Before | Admitted | Strict gain |
| --- | ---: | ---: | ---: |
| `bitmap_extract.obj` | 0 exact / 0 residual / 21 unwritten | 6 / 3 / 12 | 6 functions / 1,744 bytes |
| `ui_widget_game_data_input_functions.obj` | 1 / 0 / 45 | 27 / 16 / 3 | 26 functions / 5,120 bytes |
| `rasterizer_xbox_environment.obj` | 19 / 0 / 25 | 33 / 2 / 9 | 14 functions / 8,704 bytes |
| **Total** |  |  | **46 functions / 15,568 bytes** |

The regenerated project report now credits **712,393 / 1,770,166 Halo code
bytes** and **5,150 / 7,574 Halo functions**. The all-library view is 725,463 /
2,198,102 code bytes and 5,322 / 11,060 functions.

## Admission decisions

- The 0xB5 rasterizer environment state is a semantically named TU-private
  definition, not an unresolved address-named extern. Correcting its ownership
  naturally closed one additional 400-byte function.
- Five instruction-identical UI callbacks initially referenced abbreviated
  donor diagnostics. Restoring the assertion strings exposed by January made
  all five exact without code-generation steering.
- Six bitmap helpers that are named but still unwritten are private in symbol
  metadata. Their ordinary C declarations are documented unresolved private
  shims until their bodies can be defined `static` in the owner TU.
- Two otherwise-correct extraction declarations were removed from the broad
  `bitmap_group.h`: together they changed C2 scheduling in an unrelated exact
  bitmap utility and game-engine function. There are currently no outside
  callers, so retaining those declarations had cost without interface value.
- The five compiler/frontier residuals with completed evidence audits are now
  recorded in `config/parked.json`. The manifest validates at 122 active, zero
  stale, and zero invalid entries. The UI residuals remain open because the
  isolated lane did not establish enough evidence to classify them as compiler
  ties rather than continuing source-reconstruction work.

No forced inline/noinline annotation, volatile or register steering, fake
dependency, optimizer pragma, raw-byte emission, undefined behavior, invented
address name, or nonsensical branch was admitted. The rasterizer work preserves
the January inline schedule and emits no `point_from_line3d` COMDAT.

## Preserved dirty-worktree closure audit

The intentionally preserved dirty donor at
`C:\halo-worktrees\fable-small-families-20260901` was re-audited after the
canonical admission rather than treated as an implicit backlog. Its 18 changed
production translation units were compiled in place with the PID-local gate and
compared with the same 18 canonical units:

| View | Exact | Residual | Unwritten | Total |
| --- | ---: | ---: | ---: | ---: |
| preserved Fable donor | 515 | 53 | 272 | 840 |
| canonical at `9b0d6fc3e` | 664 | 46 | 130 | 840 |

Canonical is equal or ahead by whole-unit gate on every donor-touched object:
**149 more exact functions, seven fewer residuals, and 142 fewer unwritten
functions** in aggregate. No function that is written only in the donor remains
unwritten under its canonical identity. Apparent name-only differences in the
older split (`code_<address>`) are the same January functions after canonical
PDB/HCEA-backed naming, and therefore are not regressions or omitted donor
credit. The donor's `gate.py` additions are also present canonically; canonical
additionally corrects its stale `scratch/gate.py` usage text.

This establishes that the dirty worktree is a historical donor archive, not an
unreviewed implementation queue. It remains untouched so its experiments and
scratch evidence can still be inspected.

## Verification

- Fresh `configure.py`, regenerated target split, and full `ninja all_source`:
  pass.
- Stable exact verdict diff from `853c62aa6`: 46 gained / 15,568 padded bytes;
  zero regressions.
- `tools/fake_match_scan.py --fail-on-findings` over all 21 changed source and
  header files: zero review leads.
- `python -m pytest tools -q -p no:cacheprovider`: 261 passed.
- `source/units/units`: 189 exact / 0 residual / 0 unwritten.
- Park manifest: 122 active / 0 stale / 0 invalid.
- `git diff --check` and JSON validation: pass.

## Opus accounting

The preceding Opus wave is not omitted from this campaign state. Its dedicated
reconciliation at `docs/object_matching_logs/opus_wave_canonical_reconciliation_20260902.md`
classified all 48 Opus-only exact sections (15,696 padded bytes): 25 sections /
11,456 bytes were already preserved canonically as honest fuzzy work, one
704-byte exact function was newly admitted, and 22 sections / 3,536 bytes were
rejected as unwritten or non-creditable ownership/COMDAT artifacts. Those
results were committed and pushed before the Fable backlog baseline used here.
