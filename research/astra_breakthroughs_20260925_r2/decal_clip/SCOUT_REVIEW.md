# Independent adversarial review: scout R2 packet

Verdict: **no blocker for the narrowly stated compiler-mechanism result**.
The evidence supports the reference-weight rank crossing and consequent four-byte
frame reduction. It does not authorize either held game-source form, explain the
remaining arithmetic operand-order residual, or earn exactness credit.

Review was read-only except for this file. No compilation, fresh source probe,
compiler modification, or production edit was performed.

## Checks performed independently

- Read the complete scout README, trace/control/export scripts, tests, input
  manifest, control patches, results, and bounded sorter/packer disassembly.
- Ran `python -B research/astra_breakthroughs_20260925_r2/scout/test_mechanism.py`:
  all seven tests passed.
- Re-read all 18 saved `observed.obj` / `stock.obj` pairs. Each recorded raw
  SHA-256 matches its file; every pair is byte-identical after zeroing only COFF
  bytes 4..7. The two file lengths also agree in every pair. This independently
  verifies whole-object equality rather than trusting receipt booleans.
- Every saved raw-log hash matches `trace_hashes.json`. Current CL/C1/C2 hashes
  match `input_manifest.json`. All scratch `dbg_force.txt` files are empty.
- Diffed `trace_window.py` against the pinned R1 helper: its only change is the
  documented inclusive gate-range support.

## Central causal claim

The comparison really changes only the two scalar computed-left projection
assignments to the held aggregate projection. Across all 38 late ranking rows,
the only changes are computed-left's count 6 -> 8 and rank 34 -> 33, plus
horizontal-forward's rank 33 -> 34. Horizontal-forward's count remains seven.
There are no duplicate normalized names in either ranked list.

All 241 scalar-run and 243 aggregate-run additions observed at `0x107595c5` have
increment one. The computed-left records specifically progress through counts
0..5 and 0..7 before their respective additions. Thus the final six/eight values
are dynamic compiler measurements, not source-token counts or assumed weights.

The assembly listings independently match the six versus eight claimed accesses.
The scalar form has three stores and three later loads; the aggregate form adds
two integer reloads. The actual function prologues allocate 152 and 148 bytes,
respectively. These are not frame sizes inferred only from the most-negative
named local displacement.

The disassembly supports the relevant mechanism: ascending size ordering,
descending accumulated weight within equal-size entries, and a backwards scan
of existing compatible groups before expanding or allocating a group. The live
packing events select exactly the two different group assignments described in
the README. Only those two rows change in the packing event comparison.

Taken together, those observations explain this local four-byte difference;
the claim is stronger than a bare correlation between source syntax and frame
size. It remains a result for these controls and the pinned compiler.

## Identity and interference

The computed-left construction join is robust for the observed runs: creation
hit 36 joins the final `_vehicle_left$26624` by both frame pointer and descriptor
ordinal 26624. The other `_vehicle_left` is separately identified; arena reuse
does not silently merge the two.

For each of the four targeted symbols, the graph export reads both linked
bitset nodes: bases 0 and 32, with the second node's next link equal to zero.
The full sets of neighbors for those four symbols—not merely the six pairwise
relationships among them—agree after rank-to-unique-name normalization.
Alpha/left interference and alpha/forward noninterference therefore have direct
support. The packet does not establish equality of the entire function's
interference graph, and should retain its existing four-target qualification.

## Independent control safety and limits

`control.c` uses a genuine nested `pair` member. All local fields are initialized
before their first consumer; the six input elements cover every subscript. There
is no aggregate type punning, fake dependency, forced register assignment, or
uninitialized-data trick. The external calls are reasonable compile-only
consumers. The const-interface cell is a separate declaration control, not a
runtime/linking equivalence claim against an implementation with another type.

The toy's final displacements and actual prologues confirm 32 -> 28 bytes. Its
observed left reference weight changes 4 -> 6 across forward's unchanged five,
and the const-interface negative leaves ordering, slots, normalized bytes, and
relocation records unchanged. Nothing here demonstrates runtime numerical
equivalence of scalar and aggregate x87 code, nor is such a claim needed for
this compiler-mechanism experiment.

## Non-blocking nits / useful hardening

1. **Keep "reference weight" distinct from memory accesses.** In the game
   computed-left case the six/eight counts match actual loads/stores. In the
   toy, left's four/six include one `lea` address reference in addition to three/
   five loads/stores. Forward's five likewise include its address reference.
   Add this short qualification before generalizing a future prediction rule.
   Counting only memory-reading/writing instructions would mispredict the toy.

2. **Label the toy group-sharing conclusion as inferred from final slots.**
   `independent_control.py` records ranking and final frame records, but does not
   execute `packing_trace.py` for the toy. The shared -28 slot strongly supports
   the predicted sharing, while the exact intermediate group-join history is
   inferred from the already traced algorithm. This does not weaken the direct
   game-TU packing trace or require another broad probe.

3. **Tighten offline regression coverage.** The current tests check the final
   counts and four-target graph equality, but do not assert that
   `addition_counts` equals the ranked weights, that the only complete-list
   changes are the two rank-crossing rows, or that the frame size comes from the
   actual prologue. These facts independently checked out in this review; small
   assertions would guard later export/parser changes. The test named
   `test_independent_prediction_and_initialized_control` does not itself test
   initialization; initialization was established by reading `control.c`.

4. **Prediction timing is not independently archived.** The results establish
   that the independent model realizes the described prediction. The packet
   alone cannot authenticate "predicted before compilation" without a saved
   pre-run prediction record. Treat that chronology as the researcher's report,
   not as an additional independently verified result.

No nit above warrants promotion, new source experiments, or altered holds. The
existing aggregate/view restrictions and the separate +628 residual remain.
