# `encounters.obj` Opus two-leaf reconciliation (2026-09-03)

## Scope

This packet selectively reconciles the two strict-exact leaves preserved in
the interrupted Opus session at
`C:\halo-worktrees\opus-small-families-30k-20260902`.  That donor remains
read-only; its large unfinished encounter bodies and unrelated translation
units were not imported.

The retained functions are:

| function | January padded bytes | result |
| --- | ---: | --- |
| `_encounters_create_for_new_map` | 192 | strict exact |
| `_encounters_update` | 256 | strict exact |

The complete focused gate moves `encounters.obj` from 39 to 41 strict-exact
functions.  Its remaining four residual and sixteen unwritten functions are
unchanged.

## Source reconstruction

`encounters_create_for_new_map` now preserves `create` as the semantic local:
the encounter definition's initial-creation flag supplies its initial value,
the selected debug encounter sets it to `TRUE`, and the final predicate creates
the encounter.  This recovers January's branch topology without inventing an
operation or changing behavior.

`encounters_update` computes the current encounter's modulo phase in a
descriptive `short encounter_phase` local, then passes the iterator's actual
datum index to each update routine.  The recovered narrow phase and local
lifetime reproduce January's code while retaining the existing typed iterator
and subsystem calls.

No header, prototype, global, owned data, or linkage change is part of this
packet.  The retained source has no address-derived identifier, raw field
offset, cast-based tag/object access, inline request, volatile/register
forcing, compiler barrier, pragma, assembly, fake dependency, undefined
behavior, or nonsensical branch.  Both functions keep the campaign formatting
and explicit final `return;`.  The rebuilt object contains no
`point_from_line3d` symbol or COMDAT.

## Verification

- focused gate: 41 exact / 4 residual / 16 unwritten;
- rename-stable whole-tree comparison:
  `+2` exact functions / `+448` padded bytes / zero regressions across 8,245
  functions;
- full `ninja` build: success, semantic unit errors `0`;
- semantic progress: `+2` functions and `+422` meaningful code bytes;
- parked manifest: 187 active / 0 stale / 0 invalid;
- object-admission audit: 0 candidates / 0 contradicted; the two pre-existing
  reviewed rejections are unchanged;
- fake-match scan: 0 review leads in `source/ai/encounters.c`;
- tooling tests: 261 passed;
- `git diff --check`: clean for the retained source and this ledger.

Stable snapshots:

- `scratch/before-encounters-leaves-20260903.json`
- `scratch/after-encounters-leaves-20260903.json`
