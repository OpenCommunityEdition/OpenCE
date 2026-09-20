> Research evidence only; no source change or exact credit from this packet. Its measurements precede the separate vertex admission.

# Independent ARGB review

The two-pass ARGB implementation is supported. The initial shared-header trial is **rejected**: it regresses one inherited exact function and creates two new target-absent ARGB copies. A coherent migration of the two affected consumers to the existing bitmap declaration header was admissible to test, with the provider and control conditions below. **Final update: that second test also failed the same inherited screen-effect control. It is restored and rejected; no ARGB landing or further spelling trial is proposed.** See `../argb-migration-independent/review.md`. The discussion below records why the test was reasonable, not a successful correction.

## Source and assembly

I read the January assembly, HCEA DIA symbols/lines, old v4-versus-N1 source diff, root's exact proposed header, and all seven frozen before/after objects. January explicitly performs two conversions, compares them using the `verify == result` literal, and returns the first packed result. The first pass's alpha temporary and second pass's accumulator use EBP-8 at disjoint semantic times; the first packed result remains at EBP-24. Separate bounded scopes therefore model meaningful local lifetimes. Their exact braces and local spellings are reconstructed, not recovered from debug metadata.

The inline assembly expresses the directly evidenced x87 conversion/packing operation with typed input and named locals. Every temporary is written before use. There are no literal stack slots, padding, manual byte emission, fictitious dependencies or helper expansions into callers. The HCEA record authenticates the bitmap inline owner and const-color-pointer signature, but its PPC body and lack of locals do not prove January's local names or two-pass x86 source. The target literals and instructions provide that body evidence. The later N1 failure does not invalidate the earlier exact v4 body because N1 changed the lifetime model and returned a differently named accumulator.

## Initial trial: independent object result

`review.py` reads the root's frozen objects without invoking a compiler. Its strict comparisons independently reproduce `probe.json`:

* HUD gains the target-owned ARGB body, 320 padded / 308 meaningful bytes, 12 relocations, normalized SHA-256 `ae86b8fdd729000ffa35ff6657b0935762b82edd78d93263d6ae9bef4e7be0a3`. All 21 pre-existing HUD exact functions remain equal.
* Across seven consumers, 142 baseline exact functions become 141 unchanged plus one regression: `__rasterizer_screen_effect`. Its only aligned instruction-order difference moves `mov esi,[ebp+8]` across the adjacent instruction at +0x53e/+0x541. This is still a strict regression. The root trial changed both header-body visibility and its dependency include, so this evidence does not isolate the compiler's cause.
* Defined function-owner pairs increase from 179 to 182. The three additions are HUD's authentic missing ARGB and ARGB copies in dynavobgeom and screen_effect. There are no removals. The last two are not selected owners in those January objects and cannot be silently admitted.
* Point presence and COMMON remain unchanged. Full non-debug noncode deltas are recorded separately for all seven objects; HUD/dynavobgeom/screen_effect gain ARGB-associated data and are not described as unchanged data.

## Declaration-header interpretation and corrected ownership finding

My first message incorrectly said dynavobgeom and screen_effect had no existing alpha/RGB copies. I conflated missing January-selected owners with the full emitted baseline. The complete before objects contain:

| Consumer | Existing duplicate | Retained January-selected provider |
|---|---|---|
| dynavobgeom | alpha, 112 bytes / 6 relocs | active_camouflage |
| screen_effect | alpha, 112 bytes / 6 relocs | active_camouflage |
| screen_effect | RGB, 192 bytes / 8 relocs | HUD |

`duplicate-provider-review.json` independently verifies each copy's full bytes/relocations equal its selected January provider and that both selected providers remain exact in root's before and after objects. The affected callers use genuine out-of-line calls: dynavobgeom has one alpha and seven ARGB calls; screen_effect has five alpha and three RGB calls, while screen_flash has two ARGB calls. Those caller bodies contain no FISTP conversion expansion.

The initial blanket objection to removing those duplicates was too restrictive. I reread the original pasted user request. It requires “Existing exact functions must never regress,” “Never trade away an inherited exact function,” authentic ownership and no candidate-only behavioral helper introduced. It does not require preserving every target-absent duplicate COMDAT pair. Removing a redundant identical copy while preserving the selected provider and every accepted target function is not losing an accepted function. The removal must still be explicit in the owner census, and no accepted-ledger pair may be removed on that assumption alone.

Commit `389126d5` and `claude_two_block_reconciliation_20260919.md` establish the current architecture: genuine bodies in `bitmaps_inlines.h`, public declarations in bitmap-owned `bitmap_color_conversion.h`, and external callers including the owner declaration rather than borrowing HUD declarations. The existing declaration header was introduced in `105d9aca`; it is a repository reconstruction, not recovered January include text. Extending that established public header with the genuine ARGB signature, moving it out of `hud_draw.h`, removing the foreign local declaration in rasterizer_text, and migrating appropriate external-call consumers follows the same architectural pattern. It must not be presented as proof of original include history.

The two problematic consumers are therefore a reasonable, falsifiable declaration-boundary test. It introduces no suppression macro, inline directive, local façade or copied arithmetic. Admission requires every accepted/target-owned function and conversion call identity to remain exact, no new target-absent ARGB owner, selected alpha/RGB providers to remain exact, every selected data/COMMON provider to survive, and explicit disclosure of any removed redundant copies. A full consumer census remains root-owned. The prior failed shared-header probe stays rejected regardless of the outcome of a subsequent coherent test.

No source/header/config edits or compiler runs were made by this reviewer. The collision-debug packet was closed separately with no new shapes.
