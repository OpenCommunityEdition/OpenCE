> Archived research proposal: NOT LANDED; zero exact gain. Production remains at wave3.

# Independent decal review

PASS for root review as zero-credit source corrections. No source was edited by this reviewer.

Independently rebuilt the frozen `decals/baseline.c`, `decals/parameter-order.c`, and combined `decals/authenticated.c` with the unchanged campaign gate into this directory. Reproduced the original baseline and combined candidate function rows. Each gate reports 30 exact / 3 residual / 0 unwritten and passes the forbidden `_point_from_line3d` guard.

The parameter-only change preserves every function and non-debug data section. The combined change alters only `_decal_new_from_collision`; it preserves 55 function sections, the complete 152-owner inventory, all 89 non-debug data sections, COMMON, and all 30 strict exact controls. No helper/code owner is added or removed. The two physical residual rows remain:

| Function | Candidate size / relocs | Candidate normalized SHA256 |
|---|---:|---|
| `_decal_new_from_collision` | 6176 / 210 | `7a7cd219704120b6daa6782b18f6750b7aa90b3962b79b0638d63ce4380ca1ef` |
| `_decal_clip_to_surface` | 1776 / 59 | `2cc51c87ea49685240b5b05638abcfb8e33d3a4b1584371e0322232f71c84307` |

The candidate source SHA256 is `8cf06a1f5b57d1a682226aa5bc32fa7d546f8fb78e3f767e618eaa1382a457a4`.

Independent direct PE reads verify callee EBP+8 is checked against the `projection` assertion string and EBP+12 against `geometry`. HCEA's typed signature agrees, and the prototype, definition, and both regular/wrapped callers are consistently updated. The private VC7 optimized argument allocation remains unchanged; this is an authenticated signature correction, not an ABI-gain claim.

Direct PE bytes verify four failure-arm stores of `NONE` to the incoming definition-index slot and the common backedge to the loop test. The source faithfully routes all four failures through that test. Both geometry counts are signed short integers, so the positive conjunction exactly complements the prior nonpositive failure predicate. No body operation executes between assigning `NONE` and the test. The loop then terminates and reaches the final `return`. The successful path still unlocks, updates geometry inheritance, and advances to the next definition. Existing cache-delete behavior is preserved in both failure arms that own an allocation. No new uninitialized read or continuing-chain failure was found.

January optimizes these failure arms into epilogues; it does not retain four `NONE` stores. Thus the precise source stores are explicit later `/Od` evidence, compatible with January's control flow. Later source revision identity remains unauthenticated. Restoring a frame size or register role is useful corroboration but is not exact credit.

Reproducibility and raw-byte receipts: `review.py`, `review.json`, and three independent `.obj`/`.gate.txt` pairs in this directory. `review.json` includes SHA256 provenance, raw assertion strings and instructions, the loop test/failure tail/caller stack spans, owner/data/COMMON comparisons, and exact-control inventory.
