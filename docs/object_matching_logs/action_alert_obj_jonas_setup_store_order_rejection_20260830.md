# `action_alert.obj`: setup store-order one-shot rejection (2026-08-30)

## Scope and result

This lane tested one independently sourced ordinary-C shape for
`_action_alert_setup` and no other residual owner.  The candidate was rejected
after its first compile.  The source was restored immediately; the object
remains **5/12 exact** and **224/2240 padded bytes exact**.

The five inherited exact owners were rechecked after restoration:

- `_action_alert_begin`
- `_action_alert_control`
- `_vector_from_points3d`
- `_magnitude_squared3d`
- `_real_random_range`

## New premise

The fresh premise came from the readable Pastudan history at commit
`1146f5b799111ab72f2f5e37303a4c050945b88b`, file
`src/halo/math/vector_math.c` (the action-alert implementation around lines
368-388 in that snapshot).  Unlike the previously tested HCEA-derived shape,
it preserves the January target's post-`csmemset` field-store order:

1. conditional `move_position_order`
2. `target_move_position_index = NONE`
3. `pending_move_position_index = initial_move_position_index`
4. `move_position_increasing = TRUE`
5. `wait_ticks = 0`
6. `must_play_animation = FALSE`

The canonical declaration was retained unchanged:

```c
boolean action_alert_setup(
	long actor_index,
	short move_position_order,
	long initial_move_position_index,
	struct alert_state_data *state_data);
```

This was deliberately only a store-order adoption, not the complete Pastudan
lift.  The Pastudan snapshot's recovered formal sequence at line 372 is
`int actor_handle, int value, short field, void *state`: its second argument is
full width and its third is narrow.  This lane retained the canonical
`long, short, long, pointer` declaration rather than changing a cross-TU ABI
as part of the source-shape test.

The body used typed fields, `actor_get`, the line-35 `state_data` assertion,
`csmemset(state_data, 0, sizeof(*state_data))`, and the store sequence above.
It used no inline assembly, `volatile`, `register`, pragmas, raw structure
offsets, type punning, or byte-forcing constructs.

## Strict comparison

Only `build/base/source/ai/action_alert.obj` was scheduled; the Ninja dry run
showed exactly one `CL` edge.  The first and only candidate compile produced:

| Measurement | January target | Candidate |
|---|---:|---:|
| Padded section size | 128 | 128 |
| Relocations | 7 | 7 |
| Normalized SHA-256 | `225015f380c74c1a19d563730b7820375cd37490eb40cb6d67e3404bc46c9cc0` | `daee1bee0eae9da35692ad6ae86ae233ed996083745a65e1f1303842181b29c2` |

All seven relocation offsets and destinations were identical.  The first
normalized byte divergence was at section offset `+0x55`: target `0x03`,
candidate `0x04`; 40 normalized bytes differed in total.  The local contexts
were:

- target: `0c84c0740433c0eb038b450c668b55106689065f66c74606`
- candidate: `0c84c0740433c0eb040fbf450c668b55106689065f66c74606`

Decoded at `+0x54`, the target has `eb 03 8b 45 0c`: it jumps over a
three-byte `mov eax, dword ptr [ebp+0x0c]`.  The candidate has
`eb 04 0f bf 45 0c`: it jumps over a four-byte sign-extending word load.  The
first miss therefore reflects the retained `short move_position_order`
formal, not a difference in the Pastudan-derived post-memset store sequence.
This is recorded as provenance evidence, not used as a tuning instruction.

The earlier rejected setup candidate was 144 padded bytes with seven
relocations and normalized SHA-256
`47b3dc2358c61240e9d48276b9d00caf88d8384cfc505e764f2929cf32ba7491`.
The new provenance therefore improved the section extent and relocation
topology, but strict byte identity still failed.

## Restoration and boundary

The setup body was removed immediately, the baseline object was rebuilt only
to restore it, and `tools/coff_compare.py` reconfirmed all five inherited
owners exact.  The restored source blob is
`8de8debb9ac7108acc7352c53e36e305f75d06f7`; the January split object SHA-256
is `bb7a70aae881cdf5672ae48c3b9c5b7d0cdcc9a30a90730672f04b71fa1cab5f`.

Do **not** tune this Pastudan store-order candidate from the observed byte
differences and do not retry it.  Reopen `_action_alert_setup` only with a new,
independent prototype/source-shape adjudication.  A defensible future premise
would need to establish the likely `long, long, short, pointer` prototype and
its cross-translation-unit impact in `source/ai/actions.c`; the byte difference
alone is insufficient.  The two flush helpers,
`_action_alert_update`, `_distance_squared3d`, `_action_alert_perform`, and
`_code_00000950` were not compiled in this lane.  In particular, current HCEA
flush topology is not a new enough premise to override their existing
fail-closed boundaries.

## Documentation and tooling consulted

- `docs/matching_methodology.md`
- `tools/campaign/README.md`
- `docs/object_matching_logs/README.md`
- the three earlier `action_alert.obj` ledgers
- repository `CLAUDE.md` guidance from the canonical, Stian, and Pastudan trees
- January `build/split/source/ai/action_alert.obj`
- `tools/coff_compare.py` and `tools/campaign/board.py`
- current HCEA action-alert history and the Pastudan snapshot named above

The Discord note that Bungie's fast `ftol` may have been handwritten applies
only as Matrix Math provenance.  It did not alter this lane's ordinary-C-only
admission rules.
