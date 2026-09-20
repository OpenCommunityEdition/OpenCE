> Research evidence only; no source change or exact credit from this packet. Its measurements precede the separate vertex admission.

# Second ARGB trial — independent failed admission

**Reject and close.** The coherent bitmap declaration-header migration still regresses inherited exact `__rasterizer_screen_effect`. It cannot land, and the exact ARGB body receives zero accepted credit. The earlier declaration-boundary recommendation was a falsifiable hypothesis, not a demonstrated correction. No further declaration-order, include-order, local-scope or spelling trial follows from this result.

I independently loaded all 25 frozen before/after objects from `scratch/astra-wave6/argb-migration/` and compared them with their January split objects, using full normalized bytes and relocation identities. No compiler or production edit was used by this reviewer. `review.py` reproduces the audit, complete code-owner lists, noncode deltas, COMMON and selected-provider checks in `review.json`.

| Result | Independent measurement |
|---|---:|
| Consumers | 25 |
| Inherited exact controls | 575 |
| Inherited exact regressions | **1** |
| New target-exact body, not admitted | HUD ARGB, 320 padded / 308 meaningful bytes, 12 relocs |
| Defined function-owner pairs | 714 before / 712 after |
| New target-absent code owners | 0 |
| Removed redundant converter pairs | 3 |
| Point presence or COMMON changes | 0 |
| Accepted gain | **0** |

The lost control is the same one seen in the first header trial. `screen-effect-before-after.aln.txt` shows the adjacent schedule change: the candidate moves `mov esi,[ebp+8]` to +0x53e, while January/baseline places it at +0x541. Both contain 1,028 instructions. The small extent of this difference does not relax exact admission. The already residual `_render_weapon_hud` also changes; it was not among the inherited exact controls and earns no credit.

The ownership portion of the hypothesis worked: dynavobgeom no longer emits its redundant alpha copy, and screen_effect no longer emits redundant alpha/RGB copies. `duplicate-provider-review.json` independently proves all three removed bodies are byte/relocation-identical to retained selected providers; alpha in active_camouflage and RGB in HUD remain exact in the trial. HUD gains the authentic ARGB owner, and no other consumer emits a new ARGB body. This resolves the first probe's extra copies but does not resolve the screen-effect regression.

The noncode census is explicit: HUD adds the 17-byte `verify == result` string and 49-byte ARGB diagnostic string. Dynavobgeom removes two associated inline-converter strings; screen_effect removes four. All other audited non-debug noncode multisets remain equal. COMMON and point ownership remain equal in every unit. These removals do not justify claiming blanket data equality, and no whole-object admission is claimed.

All six touched production files were independently hashed against `baseline.json` after the trial. Every hash matches its frozen pretrial value: the two bitmap headers, HUD header, dynavobgeom, screen_effect and rasterizer_text. The trial is fully restored. The supported low-level ARGB source and its exact isolated object remain evidence only.
