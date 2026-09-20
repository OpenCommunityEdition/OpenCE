> Research-only: zero new exact credit; no production changes.

# Wave 5 shared scheduling study — 2026-09-20

**No supported new source/compiler test was found. Zero shapes, zero compiler invocations, zero exact credit.** The fresh comparison identifies three related x87 residuals, but their actual operand classes fall outside the documented local-numbering mechanism. The integer near-matches belong to different mechanisms. No common header or compiler-flag change is justified by this evidence.

This is a bounded read-only test of proposed generalizations, not another equivalent-spelling search. Baseline is clean `7a6ae71e614638753d04d6d04acbec56c7b8f636`. `study.py` reads current COFF, checks source against that commit, preserves complete target/current disassemblies and unmodified alignment output, and writes `study.json`. The final source diff is empty. No production, header, metadata, policy, or compiler configuration changed.

## Current inventory and actual scope

The schema-4 atlas's current records are `wave3_after`; the top-level percentages are frozen Stage 0. Using the current records and real function-section owners gives **106 unaccepted rows at least 95%, totaling 137,665 meaningful bytes**. That is a generous upper bound, including any still-vetoed owner. The campaign still needs 153,244 bytes, so even closing this entire pool would leave **15,579 bytes** to recover from below 95% or other unaccepted owners. Percentage ranking alone cannot complete the goal.

The core x87 cohort is three independent translation units and **2,159 meaningful bytes**:

| Function | Meaningful | Current % | Target/current padded; relocations; instructions | Fresh mismatch |
| --- | ---: | ---: | --- | --- |
| `_ai_communication_get_player_rating` | 826 | 99.98419 | 832/832; 49/49; 259/259 | At `+0x9b..0xa6`, January reduces k² then i² then j²; current reduces k² then j² then i². |
| `_bsp3d_test_sphere_recursive` | 792 | 99.985664 | 800/800; 22/22; 287/287 | At leaf `+0x1e7..0x1f3`, January evaluates y product before z; current evaluates z before y, with the same stack cleanup interleave. |
| `_biped_find_nearby_support_surface` | 541 | 99.98837 | 544/544; 23/23; 175/175 | At `+0x157/+0x15a`, January loads `[ebp-0x14]` then multiplies `[ebp-0x44]`; current reverses that product pair. |

Every ordered relocation identity in these three functions is equal under the existing hardened comparator. The mismatches are in real machine instructions, not unresolved aliases or different call sets. Each row remains parked; nothing in this packet satisfies its source-evidence reopening criterion.

Three separately classified integer/scheduling rows contribute **5,751 meaningful bytes**, not additional members of the x87 law:

| Function | Meaningful | Current % | Fresh signature |
| --- | ---: | ---: | --- |
| `_rasterizer_frame_statistics_draw` | 4,165 | 99.982864 | Two integer global-load register pairs: four relocation addends and two SIB bytes differ. All 1,178 instructions otherwise align, with 4,176 padded bytes / 322 relocations. |
| `_action_obey_control` | 984 | 99.94574 | Two-component copy assigns i/j to opposite ECX/EDX registers and reverses final stores at `+0x38b/+0x391`; 992 bytes / 16 relocations / 266 instructions on both sides. |
| `_actor_look_idle_find_prop` | 602 | 99.14508 | January directly compares EBX to `_ai_debug+56` at `+0x8d`; current first loads EAX then compares. Subsequent relocation positions and alignment padding shift. 608 bytes / 20 relocations on both sides, 199 versus 203 decoded instructions including padding. |

The six-row exposure is **7,910 meaningful bytes**. That is the size of the examined functions, not a predicted gain from one fix. Complete, unfiltered alignments use each function name as their filename stem in this directory.

## Falsifiable hypotheses checked against current exact controls

**Hypothesis: a single shared math-helper operand order or compiler-wide commutation rule is wrong.** A uniform correction should predict the same product ordering at equivalent sites and explain the discrepancy without breaking currently exact helper/inline controls. The direct byte checks refute that simple form:

- `_cross_product3d` in `action_obey.obj` is strict exact at 64 bytes / zero relocations. Its emitted helper loads the first source operand first in the relevant products.
- `_matrix3x3_from_forward_and_up` is strict exact at 112/0 while its inlined cross product already commutes several corresponding operands. The current compiler and helper can therefore produce both patterns correctly.
- `_actor_get_stopping_distances` is **currently strict exact** at 464/11. Its vehicle dot at `+0x89` loads velocity i (`+0x18`) first; its biped dot at `+0xf6` loads forward i (`+0x24`) first, despite the same source argument/member order. Both target and current streams reproduce this mixed choice.
- `_collision_bsp_test_vector_recursive` is **currently strict exact** at 800/13. It is an active plane-distance control, not an unresolved residual as in the older September 14 ledger.
- `_actor_look_valid_aim_vector` is strict exact at 128/3, supplying another current inline-normalization control in the affected AI subsystem.

All five controls were verified directly from current and January COFF, including complete section bytes and ordered relocation identities. The studied eight translation units contain 226 distinct strict control owners in total; their names are saved in the measurements. This is a current read-only census, not a rerun of a historical cached detector.

There are also stronger same-function counterexamples. January's sphere node computes z before y at `+0x33`, while its leaf computes y before z at `+0x1e7`, although both source sites call `plane3d_distance_to_point`. And current find-prop already emits the direct global-memory comparison at its second identical source predicate (`+0x169`), while the first differs. Thus a universal global-field type, direct-memory-compare rule, or fixed helper operand ordering does not explain these observations.

These counterexamples do not prove that all compiler context is irrelevant. They establish that the broad proposed mechanism lacks a sufficient predictor. No compiler-flag sweep follows from them.

## Why the documented M8 mechanism does not authorize a new probe here

The September 16 `opus5_next150_research_campaign_20260916.md` narrows M8 to equal-rank bare leaves of inline parameter temporaries or pointer locals. It explicitly excludes frame-local aggregates/scalars and says j/k `*(p+offset)` terms follow tree shape rather than M8. The current three-row signatures are:

1. Rating: frame-resident vector components, with k retained on the x87 stack and a differing i/j **term order**.
2. Sphere: differing nonzero-offset j/k terms, not a bare offset-zero pointer leaf.
3. Support surface: two EBP-folded aggregate fields. Its current handwritten distance expression is also explicitly flagged in the September 16 ledger for reassociation review. It is not treated here as an authenticated donor or a license to permute it further.

Consequently, applying dead-local counts, declaration filler, prototype counts, or operand swaps would repeat an already excluded category rather than test an evidence-backed missing source fact. Earlier magnitude/dot spellings, plane helper expansion, swapped dot operands, per-field plane copies, and register-copy ternaries are all retained as negative history. No such shape was recompiled.

The positive stopping-distance control is instructive: the September 15 “unreachable tie” claim became stale. September 16 closed it through **two genuine facts together**: remove an unnecessary local and use the source's switch, which preserves int-promotion width. A direct `if` without the local fixed the dot but produced the wrong 16-bit compare; dead locals were diagnostic instruments, never the landing. This prevents treating an old “exhausted” classification as a proof of impossibility, while also preventing transplantation of that switch/local change into unrelated functions without evidence.

## Concrete next evidence boundary

No new source test is proposed. The next admissible step for this cohort requires a named January-side fact: an actual local inventory/type distinction for the frame-vector cases, the distinct source expression or helper context for the sphere leaf, or an authenticated live-range/copy representation for the integer cases. The supplied January PDB lacks the required local/lexical records; later records do not recover January's IL ordinals. An IR diagnostic that exposes the compared nodes could support a future predictor, but no existing artifact in this bounded packet supplies that proof.

A future proposed mechanism must make a before-compilation prediction for all three x87 residuals and preserve the five exact controls above, including stopping-distances' opposite i-term choices and sphere's different node/leaf term orders. Until then, the result is **no supported shared test**, not a general “backend differences cannot close” claim.

Principal prior evidence read: the September 14 communication w1/w3, collision_bsp wave-D/w2 and bipeds wave-D/w3 ledgers; September 15 actor_moving w3d and infection N3a counterexamples; September 16 research campaign's M8 statement and independently qualified stopping-distance closure; September 19 actor_looking evidence closeout; and the fresh Wave 4 frame-statistics evidence packet. The fresh COFF results supersede stale per-row status wherever they disagree.
