# HUD, Geometry, and Sound caller-packet reconciliation (2026-09-08)

## Canonical result

This packet advances the semantic exact ledger from **1,035,078 meaningful
code bytes / 6,600 accepted exact functions** to **1,037,674 meaningful code
bytes / 6,611 accepted exact functions**. The net admitted result is **+2,596
meaningful / +2,688 padded code bytes across 11 functions**, with zero losses
in the 8,245-function rename-stable census.

| Family | Exact functions | Meaningful bytes | Padded bytes |
| --- | ---: | ---: | ---: |
| HUD messaging | 3 | 562 | 576 |
| Sound manager | 8 | 2,034 | 2,112 |
| Geometry | 0 | 0 | 0 |
| **Total** | **11** | **2,596** | **2,688** |

The exact functions are `_compare_messages`, `_render_state_text`,
`_render_state_bitmap`, `_sound_idle`, `_sound_update_time`,
`_update_channels`, `_channel_set_properties_hardware`,
`_channel_queue_sound`, `_channel_get_state`,
`_sound_manager_master_gain`, and `_sound_channel_summary_build`.

## Honest residual accounting

The packet also replaces unwritten stubs with **8,688 padded bytes** of
complete, typed, reviewed source. These functions remain fuzzy and receive
**zero exact credit**:

| Family | Complete fuzzy target scope |
| --- | ---: |
| HUD messaging renderer | 2,928 padded bytes |
| Geometry convex-hull seed and expansion | 3,296 padded bytes |
| Sound channel update/select graph | 2,464 padded bytes |
| **Total** | **8,688 padded bytes** |

Together, the exact and fuzzy work reconstructs 11,376 padded target bytes.
The individual ledgers retain target/candidate hashes, relocation counts,
provenance, residual classifications, and reopen conditions. No fuzzy body is
counted in the canonical exact-byte total.

The final review keeps valid source ahead of superficial similarity. Bounds
are validated before Geometry forms indexed pointers; HUD's icon helper is
const-correct; and Sound definition flags, instance-limit constants, and
cross-TU prototypes live with their genuine owners. The four corrected
Sound-manager private names are supported by January caller relocations and
the release-PDB/HCEA semantic map. Renaming them earns no byte credit.

## Validation

The canonical integration was configured and rebuilt as a whole. Focused
gates report HUD **33 exact / 3 residual / 0 unwritten**, Geometry **18 / 8 /
4**, and Sound **40 / 13 / 12**. All three pass the forbidden
`_point_from_line3d` emission guard. The stable before/after census reports
**11 gains / 2,688 padded bytes and zero regressions**. The source-policy scan
reports zero fake-match review leads in the changed C files, the object
admission audit adds no unsupported whole-object candidate, and the test suite
reports **1,147 passed, 2 skipped, and 26 subtests passed**.

All retained residuals are registered in `config/parked.json`; the park audit
reports **359 active, zero stale, and zero invalid**. The packet does not cross
the **1,041,486 meaningful-byte** treemap cadence threshold, so the canonical
treemap is intentionally not refreshed by this commit.

## Detailed ledgers

- `hud_messaging_obj_renderer_caller_closure_20260908.md`
- `geometry_obj_convex_hull3d_begin_expand_reconciliation_20260908.md`
- `sound_manager_obj_idle_channel_spine_20260908.md`
