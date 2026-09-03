# `actors.obj` large unwritten-function wave — 2026-09-02

## Outcome

Four previously unwritten January functions now have complete, typed, ordinary-C
implementations:

| Function | Target | Candidate | Relocations | Objdiff |
| --- | ---: | ---: | ---: | ---: |
| `actor_swarm_attach_unit` | 656 | 656 | 43 / 43 | 94.25247% |
| `actor_customize_unit` | 640 | 672 | 26 / 26 | 65.71359% |
| `actor_create_for_unit` | 624 | 624 | 29 / 29 | 93.40611% |
| `actors_spawn_from_unit` | 672 | 672 | 29 / 29 | 54.82266% |

The isolated object moves from 65 exact / 3 residual / 8 unwritten to 65 exact /
7 residual / 4 unwritten. No strict exact function regresses. The raw objdiff
percentage for `actors_spawn_from_unit` understates its aligned similarity: after
the early register-allocation divergence, its placement, object creation, biped
settling, actor creation, failure cleanup, activation, randomized launch, and
loop topology all retain the January calls, constants, and relocation order.

## Source and naming evidence

January's split object is the byte authority. The independent HCEA lifts under
`work/halocea-reference/src/actors_spawn_from_unit.c` and the neighboring actor
files corroborate behavior, parameter meanings, actor-default-state names, and
the `biped_accelerate` signature. The target's string relocation resolves to
`WARNING: cannot create actor to be spawned from unit`.

The reconstruction uses `actor_get`, `unit_get`,
`actor_definition_get`, `actor_variant_definition_get`, and
`TAG_BLOCK_GET_ELEMENT` instead of raw datum/tag casts. The newly exposed public
prototypes live in `actors.h`; `actor_action_get_default_state` lives in
`actions.h`; `biped_accelerate` lives in `bipeds.h`; and the weapon-add mode is
owned by `units.h`. The color-record layout remains TU-private because no public
consumer requires it and moving unrelated definitions through shared headers is
known to perturb VC7 allocation.

The expanded actor-variant flag names are independently corroborated and replace
numeric flag use. Removing the obsolete duplicate private flag definition from
`actor_type_flood.c` preserves that object's 1 exact / 1 residual verdict.

## Residual boundary

The retained functions were tested with bounded, source-plausible declaration,
branch, loop, and expression forms. The remaining deltas are register allocation,
callee-save shrink wrapping, independent store scheduling, and x87 stack
scheduling. No register hint, volatile access, barrier, pragma, forced
inline/noinline, assembly, raw offset, representation pun, fake dependency,
invented branch, or undefined behavior is retained. They are parked under the
campaign's fuzzy rule pending authoritative January source/local records or a
natural same-compiler donor.

## Verification

- Isolated `actors.obj`: 65 exact / 7 residual / 4 unwritten.
- `units.obj`: 189 exact / 0 residual / 0 unwritten.
- `actor_type_flood.obj`: 1 exact / 1 residual / 0 unwritten.
- Full configure, build, split, report, and progress gate: pass.
- Rename-stable exact sweep: zero gains and zero regressions, as expected for a
  semantic residual wave.
- Fake-match scan over every changed source/header: zero review leads.
- Tool tests: 261 passed.
