# `stack_walk_windows.obj` ABI and semantic-owner reconciliation

## Result

This packet corrects the public stack-walk ABI and declaration ownership without
claiming new exact code. The already exact owners remain exact:

| Object | Exact | Residual | Unwritten |
| --- | ---: | ---: | ---: |
| `cseries.obj` | 26 | 0 | 0 |
| `cseries_windows.obj` | 18 | 0 | 0 |
| `stack_walk_windows.obj` | 7 | 7 | 2 |

The focused count above used diagnostic aliases before the final global rebuild.
The authenticated HCEA private names are now also recorded in
`config/symbols.json`, so canonical reports do not expose address placeholders.
This metadata cleanup does not make any residual body exact and receives no byte
credit.

Meaningful exact bytes credited by this reconciliation: **0**. Padded exact bytes
credited: **0**. The reconstructed bodies remain useful, coherent fuzzy source,
but none is reported as exact.

## Primary ABI evidence

The recovered Bungie source at
`C:/Users/isabe/Downloads/haloleak2024/halo1/random_math.c` calls:

```c
stack_walk_with_context(random_seed_debug_file, 1, NULL);
```

That establishes the January-family call shape as `FILE *`, a frame-ignore count,
and `CONTEXT *`. The HCEA symbols database independently supplies the types and
names:

```c
void stack_walk_with_context(
	FILE *error_stream,
	short levels_to_ignore,
	CONTEXT *context_pointer);

void stack_walk(
	short levels_to_ignore);
```

The later-build symbols are cross-build naming and type evidence, not byte-match
evidence. They also identify the private owners as `is_valid_ebp`, `walk_up`,
`initialize_stack_walk`, `walk_stack_context`, and `walk_stack`; `old_ebp` is a
file-static `unsigned long *`. They authenticate the argument order for both walk
helpers as routine-address array, number of levels, ignored levels, then dumped
level count.

## Ownership changes

- The public declarations now live in `source/cseries/cseries_windows.h`, the
  closest associated header for `stack_walk_windows.c`.
- The incompatible consumer-local declarations were removed from
  `cseries_windows.c` and `cseries.c`.
- `generic_exception_filter` now passes its context through the authenticated
  `FILE *, short, CONTEXT *` interface.
- `cseries.c` includes the owning header and passes the numeric ignore count to
  `stack_walk`.
- The private functions and globals use semantic names rather than `code +
  address` or `bss + address` identifiers, in both source and symbol metadata.

January's target BSS relocation ownership is recorded as one 0x4008-byte aggregate:

| Offset | Semantic owner |
| ---: | --- |
| `0x0000` | `walk_up_current_frame` |
| `0x0004` | `old_ebp` |
| `0x0008` | `symbol_buffer[0x4000]` |

The candidate aggregate also totals 0x4008 bytes. This size equality is not data
identity and receives no exact-data credit.

## Honest fuzzy boundary

The two largest reconstructed functions remain plainly non-exact:

| Function | Target padded / candidate padded | Target / candidate relocations | Status |
| --- | ---: | ---: | --- |
| `stack_walk_with_context` | 688 / 672 | 49 / 51 | fuzzy, zero credit |
| `symbol_name_from_address` | 176 / 176 | 9 / 9 | fuzzy, normalized hash differs |

The five private helpers likewise remain residual under diagnostic aliases:
`is_valid_ebp` 32/32, `walk_up` 64/64, `initialize_stack_walk` 32/32,
`walk_stack_context` 208/176, and `walk_stack` 208/176 target/candidate padded
bytes. The two small inline-assembly moves in `walk_stack` capture EBP and ESP,
which is intrinsically low-level stack-walking work; they are not byte-forcing or
grounds for match credit.

## Verification

Focused universal gates, each with the `_point_from_line3d` emitted-symbol guard:

```text
cseries:               26 exact, 0 residual, 0 unwritten
cseries_windows:       18 exact, 0 residual, 0 unwritten
stack_walk_windows:     7 exact, 7 residual, 2 unwritten
```

All 18 translation units that directly include `cseries_windows.h` were also
gated after the header edit. Every prior exact owner was preserved. Units with
pre-existing residuals retained their prior exact counts: `debug_memory` 17,
`game_engine` 169, `files_windows` 23, and `stack_walk_windows` 7. The other
includers remained fully exact at their previous counts: `cache_files` 28,
`physical_memory_map` 7, `cseries_windows` 18, `cseries` 26, `errors` 10,
`progress` 2, `game_time` 20, `HaloAutoTest` 7, `attract_mode` 5,
`event_manager` 8, `marketing_beta_key_sharing` 5, `random_math` 17,
`game_state` 24, and `shell_xbox` 8.

`git diff --check` passes. No compiler flags, parked ledger, target object, or
comparison rule was changed. The integration build regenerates split metadata
and reports after applying the semantic symbol names.
