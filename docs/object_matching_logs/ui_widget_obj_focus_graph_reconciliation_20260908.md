# `ui_widget.obj` focus-graph reconciliation

This packet reconciles the narrow list-navigation and focus graph from Opus
tip `08d6bea92b1b7da925bf3687b44310b6203953dd` against canonical
`7f7b72cc3819f05e3e85956101f314b544825e08`. The donor lane's headline total
is not used for credit.

## Exact accounting

The focused gate advances `ui_widget.obj` from 56 exact / 4 residual / 42
unwritten owners to **62 / 4 / 36**. Six new owners contribute **1,602
meaningful / 1,648 padded exact code bytes**:

| function | meaningful | padded |
|---|---:|---:|
| `widget_event_function_list_widget_goto_next_item` | 511 | 512 |
| `widget_event_function_list_widget_goto_previous_item` | 544 | 544 |
| `widget_instance_give_focus_by_tag` | 71 | 80 |
| `widget_instance_find_by_tag_index_recursive` | 65 | 80 |
| `widget_instance_give_focus_directly` | 354 | 368 |
| `widget_instance_can_handle_events` | 57 | 64 |

Hardened COFF comparison, including ordered relocation identities, reports all
six equal. No fuzzy bytes or whole-object/data completion are claimed.

## Ownership and source admission

The full donor replacement is rejected because it copied broad widget, HUD,
bitmap and string schemas plus unrelated subsystem prototypes into the C file.
The admitted packet instead adds only the three fields genuinely read by this
graph to `interface/ui_widget_definitions.h`: the event-handler block at
0x054, list flags at 0x150 and the already-known child count at 0x3E0. Typed
definition access stays behind `ui_widget_definition_get`, and list behavior
uses a named enum flag rather than a numeric mask.

Private functions have semantic names and private linkage. Parameters follow
the one-per-line style, no-argument lists use `void`, and every body ends in an
explicit return. `widget_instance_can_handle_events` remains `static
__inline` only because January contains both its private out-of-line owner and
three expansions in the genuine exact caller; ordinary storage classes do not
reproduce that measured schedule. The emitted-symbol guard confirms that the
packet does not create a `point_from_line3d` COMDAT.

## Header and inherited-debt checks

The edited definition header has three direct C consumers. Focused gates
preserve `ui_widget_event_handler_functions.obj` at 100/100 exact and preserve
the complete exact-owner set in `ui_widget_game_data_input_functions.obj`
(27 exact / 16 residual / 3 unwritten). `ui_widget.obj` has only the six gains.

`ui_widget_event_handler_functions.c` still carries inherited local
redeclarations of the two public event functions even though `ui_widget.h`
owns them. A direct owner-header cleanup is not folded into this packet:
including that broad header exposes pre-existing incompatible shadow
prototypes, including a local `void *widget_free(...)` versus the owner's
`void widget_free(...)`. The consumer remains 100/100 exact, so this is
recorded as existing prototype-ownership debt rather than hidden as completed
cleanup. It requires its own zero-regression header packet.

## Verification boundary

Focused gates, the fake-match scan and the forbidden-emission guard pass.
The orchestrator still must run the full build, rename-stable whole-tree sweep,
semantic/admission audits and full tests before publishing the batch.

Detailed intake artifacts are preserved locally at
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/scratch/ui-widget-focus-owner-packet-20260908/`.
