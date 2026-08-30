# `hud_nav_points.obj` exhaustive closeout census (2026-08-30)

## Outcome

No source candidate was emitted.  The inherited translation unit remains the
strictly strongest authenticated build at 30/32 exact owners:

| owner | status | target bytes / relocs / normalized SHA-256 | current bytes / relocs / normalized SHA-256 |
| --- | --- | --- | --- |
| `_custom_render_nav_point` | unwritten | 1632 / 95 / `287a20cd338648964303d1f60b46e9cdf94753f43718b48f8c648a0dd9e60306` | absent |
| `_hud_render_nav_points` | residual | 400 / 17 / `f2bb689a08030b4b54e28e13307de141eab53b71b0acc334bc2bec2f1dd78391` | 416 / 17 / `56bf4e1f33a71e475c6a3108db3ca65ad4e44bfc8d23b8443e87d24e31eaa70b` |

The January object has raw SHA-256
`05f6bcce14e467330c57ee1a7bcd0e9ef8862a6f6818bf17fc252a8e67d095fa`.
The retained source remains Git blob
`5963f46d0291a56586e8ab7cc72bac2e4e6cfbc3`; no production candidate was
compiled and no tracked source changed.

## Complete local evidence census

The history sweep started from canonical commit
`251e9f780f610ee2c1962d93fc104c1154d28996` and compiled every one of the
eight distinct committed `source/interface/hud_nav_points.c` blobs reachable
from registered refs.  None improved the inherited 30 exact / 1 residual / 1
unwritten frontier.  The historical results were either weaker (29 or fewer
exact owners) or failed under the current tree and toolchain.

A broader read-only census covered `C:\halo-worktrees`, `C:\tmp`, and the
entire `C:\Users\isabe\Documents\Codex\2026-07-13\i-w` campaign tree.  It
found 1,615 source copies with 12 distinct content hashes and parsed 6,173
HUD-related COFF artifacts without a parser failure.  Renamed audit objects
were deliberately included.  The only residual-owner fingerprints were:

- `_custom_render_nav_point`: the January 1632/95 target in literal split
  copies, and one 1584/96/
  `3d425ef6ea26bbf2de725afc39f14de76c789824af72df07bd1c931fc50e22be`
  artifact from the already-rejected public-donor one-shot;
- `_hud_render_nav_points`: the January 400/17 target in literal split copies,
  the inherited 416/17/`56bf4e1f...` rebuild family, and the public-donor
  one-shot's 416/17/
  `0df4703281637b185fa8bd6f44392a688be3a3e4e2b11e6bc6314ae756823274`
  side effect.

There is no rebuilt exact instance of either residual owner and no non-target
artifact containing a complete exact object.  This also closes the audit-file
blind spot left by filename-only searches: the one-shot artifact is retained
as `build/audit/hud-nav-custom-render-one-shot/candidate-one-shot.obj` in its
original isolated lane and was measured explicitly.

## Authenticated public evidence

The current HaloCEA reference was reread at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  Its typed
`src/blam/interface/custom_render_nav_point.c` still has SHA-256
`351f7f549aad5023a54ff621c4c64b13b3d258b8d96d3149f43cfd36a5aa26fc`:
it is exactly the frozen donor already adapted and rejected by the 2026-08-28
one-shot.  Its render-loop reconstruction supplies no new typed +8 induction
topology.  Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf` and
Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80` contain no
competing implementation.  The punpckhdq mirror at
`fc47f9a15a0aea7f0081394c4d66db5c68ab5323` remains symbol/source skeleton
only.  Claude and Fable campaign trees were included in the local source and
artifact census.

## Remaining boundaries

January's `_hud_render_nav_points` diverges at the loop cursor setup: it biases
ESI to `reference_index` (`datum + 8`) and addresses `nav_index`, packing, and
`z_offset` at -8, -6, and -4.  The inherited legal typed C keeps ESI at the
datum base.  The prior scoped-member-pointer, explicit-value, and loop-carried
member-pointer families already reached fixed points or produced different
register/schedule topologies; the one 400-byte spelling was still non-exact.

The custom renderer's only authenticated complete donor already missed by 48
padded bytes and one relocation on its sole permitted compile.  Retuning that
same reconstruction would violate the frozen one-shot boundary.

Reopen this object only for newly authenticated original source, a genuinely
new legal typed-C topology that explains the +8 loop cursor, or a materially
different independent complete donor.  Raw pointer arithmetic, container
reconstruction, pointer/union punning, undefined behavior, synthetic anchors,
compiler controls, assembly, and object-byte forcing remain inadmissible.

## Preservation

The focused hardened gate remains 30 exact, one residual, and one unwritten.
All inherited exact owners, sections, data, and ordered relocations are
unchanged.  This closeout is documentation-only and performs no push.
