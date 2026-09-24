# Claude fifty-object round 2: canonical reconciliation checkpoint

This checkpoint reconciles Claude's committed `f6d00a8c..5d3ca708` range
against canonical `20dd67c0` in the isolated
`reconcile/fifty-r2-20260924` worktree. Claude's lane remains active;
its untracked `research/fifty_objects_r2_20260924/` evidence was neither
copied nor discarded. This is not a final lane handoff.

The seven committed lane changes cherry-picked without path conflicts. One
object admission was quarantined before publication: the
`rasterizer_xbox_hardware_geometry` source, corresponding symbol-map edits,
and `Matching` flag were restored to their canonical state. The lane's added
`D3DVertexBuffer_MoveResourceMemory(d3d_vertex_buffer, D3DMEM_VIDEO)` call
is byte-inert; the wrapper names have 2001 map support, but its call-site
position and argument are not attested. No credit is claimed for this object.
The lane packet remains available for a later authenticity ruling.

| Halo metric | Canonical `20dd67c0` | Reconciled checkpoint | Delta |
| --- | ---: | ---: | ---: |
| Meaningful exact code bytes | 1,573,109 | 1,577,237 | +4,128 |
| Exact functions | 7,442 / 7,574 | 7,445 / 7,574 | +3 |
| Credited data bytes | 2,583,826 | 2,586,915 | +3,089 |
| Complete objects | 381 / 468 | 383 / 468 | +2 |

The code gains are `_actor_input_update` (2,384 padded bytes),
`_actors_spawn_from_unit` (672), and `_render_debug_trigger_volumes` (1,088).
The stable whole-board diff reports three gains / 4,144 padded bytes and
zero regressions. The 3,089 data bytes come from the lane's two separately
verified semantic data entries for `projectiles` and `object_lights`; they
are not new code credit.

Full `ninja progress` passes. Parks: 83 valid. Admission audit:
11 candidates, zero contradicted, 2 rejected, zero revoked. Tools tests:
1,161 passed, 5 skipped, 26 subtests passed. `git diff --check` passes.
The opt-in Xbox link probe still reports three duplicate providers and no
successful full link; all 621 input object digests are unchanged by the
probe. No proprietary SDK/reference files enter this checkpoint.

Before canonical integration, refresh this checkpoint with Claude's final
tip and curated research evidence, re-run the same gates, and retain the
hardware-geometry hold unless new first-party source evidence resolves its
call site.
