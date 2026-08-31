# `circular_queue.obj` credible fuzzy park

## Outcome

- Seven of eight functions are strict exact.
- `_circular_queue_dequeue_data` is parked at 93.947365%: 256 target padded
  bytes versus 240 candidate bytes, with all 12 relocation identities present.
- All 251 bytes in the five owned read-only-data sections are exact.
- The object remains `NonMatching`; parking grants no byte-complete credit.

## Recovered identity

The January symbol atlas names the helper at `0x508590`
`circular_queue_verify` in `circular_queue.obj`. The September 2001 linker map
independently records the same helper name and compiland position. The source
and `config/symbols.json` now use that recovered private name and mark it
source-static instead of retaining an address-derived placeholder.

## Source credibility

The retained queue code is ordinary C and passes the fake-match scan. The only
house-rule source cleanup besides the recovered name is declaration-time
initialization of the dequeue result; the compiler emits the same baseline
function. Public declarations remain in `circular_queue.h`, the file's own
associated header.

No barrier, `volatile` pun, `register` hint, inline assembly, forced byte,
undefined behavior, empty branch, inert expression, or synthetic call edge is
retained. The Marathon source tree contains no matching general circular-queue
implementation, while the later HCEA `simple_circular_queue` family has a
different API and topology; neither was treated as a source donor.

## Residual mechanism

January and the October 2001 build both initialize the false result in `BL`,
reuse `EBX` for `read_offset`, and retain separate success and failure tails.
The compatible VC7 builds available here put `queue` in `EBX`, put
`read_offset` in `EDI`, and fold false directly into `AL`.

Thirty-seven documented legal-C experiments isolated an optimizer/inliner
coupling. A single-iteration loop naturally creates the target `BL` lifetime,
but it also causes a second inline of `circular_queue_verify`, changing the
function to 336 bytes and 17 relocations. Preventing that clone removes the
desired lifetime. Taking the verifier's address reaches a closer 256/12 shape
but changes its private ABI and regresses already-exact siblings, so it is not
an admissible reconstruction.

## Reopen criteria

Reopen only for authoritative January source/local records, a strict VC7 donor
with the same disjoint byte/long register lifetime, another authentic compatible
compiler/QFE, or a natural general compiler control that prevents the nested
clone without changing the helper ABI. Do not resume local-declaration or
synthetic-control-flow sweeps already covered by the experiment ledger.
