# `periodic_functions.obj` evaluator and builder closures (2026-08-31)

## Result

The Claude/Fable recovery closes four previously unwritten or residual
functions with ordinary source and returns the translation unit to the
repository-default compiler flags.  Together with the inherited exact
initializer and disposer, January now has six strict-exact target-owned
functions out of seven.  The unit remains `NonMatching` because the variable
period builder is an honest two-instruction scheduling residual and the
translation unit emits a conflicting `fast_ftol` COMDAT.

| Function | Padded bytes / relocations | Normalized SHA-256 | Result |
|---|---:|---|---|
| `_periodic_functions_dispose` | `112 / 8` | `09f94698f1709bc4480b5131f6d69ee770b376da2f558dea97876e2f1c6c2651` | inherited strict exact |
| `_periodic_function_evaluate` | `304 / 20` | `eca198fa43797115a038cd9fab4922b35a1d6ee5b69f27ca5cb6177c78e1a38c` | strict exact |
| `_transition_function_evaluate` | `288 / 17` | `a82ce883c702fcbd0b59e57b2d5ac7fce4d165d46fb5256f92c158fd0114e2c6` | strict exact |
| `@periodic_function_build_variable_period_x_table@4` | `256 / 18` | target `802ef4da...`, base `af8e65f5...` | fuzzy-parked |
| `_transition_function_build_table` | `304 / 25` | `60a30a4692d9a5a09e520029c9476e6fc8de9a28848c2b5d024a2645c3536927` | strict exact |
| `_periodic_function_build_table` | `688 / 46` | `b77d7e7cffc5da24aee75cf7b9d48a90fadf2c58f3279f0eadfeda9d54a3c4ad` | strict exact |
| `_periodic_functions_initialize` | `192 / 17` | `dd0e4caee119b1cdbd7398ed568e471d96ea1a5048f82105140b81057243f154` | inherited strict exact |

## Provenance and naturalness

The integration was transplanted from the audited Fable donor rather than
cherry-picking a multi-object campaign commit.  The donor reconstructs the
waveform formulas, lookup-table interpolation, normalization, and transition
curves already corroborated by HCEA and January's symbol atlas.  The retained
source uses semantic scalar amplitudes and cosine temporaries, explicit
minimum/maximum comparisons, the shared `fast_ftol`, `FLAG`/`TEST_FLAG`, and
the real `real`/`byte` cseries types.  These are meaningful calculations and
state transitions; none is dead compiler steering.

The owner C file now uses the atlas-backed private names
`periodic_function_build_variable_period_x_table` and
`transition_function_build_table`, plus the semantic globals
`global_periodic_functions_enum_strings` and `periodic_functions_globals`.
The private declarations remain in the owner C file.  The existing public
periodic API remains in `periodic_functions.h`, and the shared enum definition
comes from `tag_files/tag_groups.h`; no header is edited for TU-private state.

Removing the old per-file `/Op /QIfist` override is required.  Repository
default flags naturally produce January's `__CIfmod`, `__CIpow`, `fcos`, and
`fsin` lowering and close all four functions above.

## Honest residual and link boundary

The variable-period builder has January's exact 256-byte envelope, calling
convention, and all 18 relocation destinations.  Twenty-one bounded natural
source experiments reduced the difference to an EDX/ECX ordering tie around
the first cosine calculation.  No volatile access, barriers, pragmas, dead
dependencies, source-level register tricks, or inline assembly are retained.
It is recorded in `config/parked.json` as `instruction-scheduling` and should
reopen only for authentic January source or new same-compiler scheduler
evidence.

The object also emits a candidate-only 32-byte `_fast_ftol` COMDAT with no
references.  Its bytes match the actor-combat copy, but its selection value is
`2` while January's selected copy is `1`; a direct XDK link with the split
actor-combat object fails `LNK2005`.  The unit therefore cannot be admitted as
`Matching` even though six target-owned functions are exact.

Validation uses the isolated campaign gate, direct COFF comparison including
the decorated fastcall residual, csplit regenerated from the semantic symbol
map, the full semantic/progress build, parked-function validation, and the
tooling regression suite.
