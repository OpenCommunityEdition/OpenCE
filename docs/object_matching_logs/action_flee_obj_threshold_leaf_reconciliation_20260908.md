# ActionFlee small-leaf reconciliation (2026-09-08)

## Result

This reconciliation admits three complete Fable-lane leaves into their real
`action_flee.c` owner and moves their public declarations from the unrelated
`actions.c` dispatcher into the closest associated header, `actions.h`.

| January owner | meaningful bytes | padded bytes | relocations | verdict |
| --- | ---: | ---: | ---: | --- |
| `_action_flee_flush_position_indices` | 39 | 48 | 2 | exact |
| `_action_flee_modify_color` | 99 | 112 | 4 | exact |
| `_action_flee_replace_prop` | 48 | 48 | 2 | exact |

The batch adds **186 meaningful / 208 padded exact code bytes** and three
strict functions. The two inherited exact ActionFlee owners remain exact.

## Source and ownership review

The implementations use the existing typed `actor_get` accessor, the genuine
`flee_state_data` fields, `NONE`, project booleans, and the canonical ARGB
color owners. Parameters remain one per line and every void body ends with an
explicit `return;`. No inline control, assembly, raw offset, address-derived
name, fabricated dependency, or manual bit logic is introduced.

The declarations belong to `actions.h` because this translation unit has no
dedicated header. All ActionFlee declarations now live there, including the
setup/dispatch callbacks and `action_flee_blind_panic`; their duplicate local
declarations were removed from `actions.c` and `actor_type_grunt.c` without
changing either consumer's code owners.

## Focused evidence

The canonical source gate reports `5 exact / 0 residual / 8 unwritten` for
`action_flee.obj`; `actions.obj` remains `58 exact / 5 residual / 0 unwritten`.
Both gates reject any emitted `_point_from_line3d`, and the changed-file
fake-match scan reports zero findings. The packet author also compared all 22
other initially compilable `actions.h` consumers without a count change; the
enclosing canonical build compiles the complete consumer set. The separate
`ai_debug.c` Path-interface repair receives no ActionFlee proof or credit.

The full-build, rename-stable sweep, park validation, admission audit and test
suite are recorded in the enclosing reconciliation batch. These three bodies
receive exact credit only if that global admission gate reports no inherited
loss.
