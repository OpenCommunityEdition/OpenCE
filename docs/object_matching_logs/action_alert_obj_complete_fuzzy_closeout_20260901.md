# action_alert.obj complete reconstruction and fuzzy closeout (2026-09-01)

## Result

`source/ai/action_alert.obj` advanced from five exact functions and seven
unwritten functions to eleven exact functions and one complete, documented
fuzzy residual.  There are no unwritten functions left in the object.

The newly exact functions are:

- `action_alert_setup`
- `action_alert_flush_position_indices`
- `action_alert_flush_structure_indices`
- `distance_squared3d`
- `action_alert_update`
- `action_alert_perform`

The remaining private function was recovered as
`action_alert_next_position`; the address-derived symbol was removed from
`config/symbols.json` and the symbol remains translation-unit private.

## Evidence and donor audit

The reconstruction was checked against all Action Alert ledgers, repository
history, registered branches and worktrees, the January target disassembly and
relocations, and the later HCEA semantic sources under:

`research-cache/halocea-full-blobs-20260830/src/blam/ai/action_alert_*.c`

Pastudan's setup/update reconstruction was also inspected.  Marathon contains
no direct Action Alert donor.  No registered worktree contains a complete
definition of either `action_alert_perform` or `action_alert_next_position`,
and no farther candidate than the retained source was found.

The HCEA source and AI definitions corroborate the complete behavior:

- disabled/passenger, no-order, and no-encounter rejection;
- encounter, squad, and move-position traversal;
- current-position, actor-distance, noncombat-sequence, and nearby-prop
  availability tests;
- cseries bit-vector tracking of unavailable positions;
- weighted random selection and forward, ping-pong, and per-tick-random
  traversal;
- dwell-time selection, animation startup, and move-position state updates.

Action Alert prototypes now live in the closest owner header, `actions.h`,
rather than as ad hoc declarations in `actions.c`.  Accesses use the project's
typed actor, unit, definition, tag-block, and scenario helpers.  The source
uses semantic cseries types and enum constants and keeps explicit returns.

## Residual measurement

Focused gate after reconstruction:

```text
residual    784  _action_alert_next_position  [size 768!=784, sha]
== exact 11  residual 1  unwritten 0  (of 12 listed)
```

Fresh full campaign-report similarity for the residual is `99.46479%`.
This value, rather than a separate aligned-disassembly score, is recorded in
`config/parked.json`.

- Target: 784 padded bytes, 18 relocations, normalized SHA-256
  `677fd6cc1a9e66de32123c4614de5a0cc1188501baee69ea20fd5bf74eac3fe4`.
- Candidate: 768 padded bytes, 18 relocations, normalized SHA-256
  `4e16c85b544ded3352df48ee7baa0e19b5a34a45e421a975cf11cd771184f371`.

Aligned instruction comparison is retained only as diagnostic evidence.  It
shows that the geometry, prop-iterator, bit-vector, random-selection, and
ordered-traversal body is byte-identical.  The remaining decoded differences
are limited to January preloading the `NONE` result in EAX, the resulting
AL/CL and EAX/ECX allocation choices in the entry guard, two equivalent
full-EAX versus AX `NONE` returns, and target-envelope padding.

## Bounded source-shape audit

Ordinary-C probes covered combined, separate, and enclosing entry guards;
short and long default-result lifetimes; declaration order; direct and
assigned random returns; natural `for` and `while` traversal; and switch
default/case arrangements.  The retained `while` traversal reproduces
January's initialization schedule, and initializing the traversal direction
before a semantic enum switch with no redundant default reproduces January's
entire switch body.  The remaining entry/result variants either returned to
the same compiler fixed point or disturbed larger exact regions.

No volatile or register forcing, compiler barrier, pragma, inline assembly,
fake dependency, raw structure offset, undefined behavior, forced inlining,
or nonsensical control flow is retained.  The function is parked as
`register-allocation` and should reopen only for authoritative January source
or local-variable records, or a natural compatible-compiler donor that
explains the default-result lifetime.

## Validation

The admission run passed:

- full `ninja`, including report, semantic report, and progress validation;
- an independent strict per-function sweep against a clean build of base
  `f73b19d30`: 5,026 exact to 5,032 exact, exactly the six Action Alert gains
  above, with zero lost functions and zero unit-read errors;
- focused gates: Action Alert `11/12` exact with one parked residual, Actions
  `58/63` exact with its five pre-existing parks, Units `189/189`, objects
  `121/121`, and object types `31/31`;
- `python -m tools.parked_functions`: 78 active, zero stale, zero invalid;
- `python -m pytest -q`: 255 passed;
- semantic audit: 5,032 strict exact functions, 134 hidden exact
  functions, and zero unit errors;
- fake-match scan of all changed C/header files: zero findings;
- object-admission audit: no contradictions or revocations (one unrelated
  pre-existing `player_rumble` ownership-review candidate);
- all-unit `point_from_line3d` owner comparison against the base: no new or
  lost COMDAT owner.
