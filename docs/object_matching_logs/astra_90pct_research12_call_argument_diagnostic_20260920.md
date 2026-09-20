> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# Packet12: bounded call-argument diagnostic

Disposition: **read-only diagnostic complete; no admission logic change, source probe or exact credit from this lane**. The final scanner is a locator for a narrow set of immediate, direct named-global-load and address arguments. It compares matching ordered named-call sequences and nearby explicit post-call ESP cleanup words. Unknown arguments remain unknown; manual January control-flow and ABI binding is required for every hit.

## Final measured scope

`result.json` records 274 residual functions and 3,567 comparator-exact sibling controls, with 53,783 call-site visits across both sides. Forty-four residuals have different ordered call sequences and are excluded from argument comparison. Controls expose 5,198 known argument words: 1,565 controls have at least one known word, while 2,002 have none. The final run reports one literal-identity hit in `_network_server_close_client_connection`.

These are coverage counts, not new matches or proof that all other arguments agree. Branch/target cuts, nested calls, unsupported arithmetic, pointer propagation, widths, indirect callees and conservative memory-store invalidation leave substantial unobserved scope. Nearby cleanup does not itself prove a function signature or argument arity. Equal call order does not establish equal branch roles. Exact controls check representation consistency in the covered cases, not the soundness of every abstract interpretation assumption.

The value-identity comparison uses the existing comparator's fail-closed alias rules on local copies of relocation identities whose location is set to zero solely for this diagnostic. Original identities, objects, metadata and admission comparisons are unchanged. Actual strict admission still requires complete normalized bytes and relocation identities at their real offsets, plus all existing controls and ownership guards.

## Initial defects, conservative fixes and discovery provenance

Independent raw-byte fixtures reproduced three initial defects: stale pending words survived unsupported ESP reassignment; partial-width stack operations were treated as four-byte slots; and missing Capstone access metadata allowed x87 stores to leave false known argument values. Root fixed unsupported ESP writes and partial stack operations by discarding pending facts, and explicitly invalidated x87 memory destinations/state saves. The final scanner passes those fixtures, including direct x87/SSE overwrites and partial-register clobbers. No heuristic was relaxed to force a positive result.

`initial-scan.py` and `initial-result.json` preserve the known-flawed discovery stage. That initial run reported both the network literal and an actor output-throttle length mismatch (`1.0f` versus current `1.6f`). The latter was independently authenticated through January immediate bytes, five argument roles, later executable literal bytes, RTC vector identities and the adjoining charge-decision block; see `../ai-debug-context/primary-length.json` and `../ai-debug-length-independent/review.md`. It remains valid because of that separate primary proof. The fixed final scanner does **not** cover this scalar site: intervening x87 stores conservatively erase its argument facts. The independently known swapped actor colors are also outside final coverage. Do not present these discoveries as expanded final-scanner coverage or exact gains.

## Remaining network hit

Root's `network_triage.py`, `network-triage.json`, `network-split.asm` and `network-base.asm` bind the surviving hit to the literal pushed at `+0x109` (relocation `+0x10a`) for `_error` at `+0x110`. This is an actual literal-byte difference, not merely different compiler-generated format-label names. Current text is `failed to remove a client endpoint from the server's endpoint set`; January appends ` (maybe it was already removed)`. The NUL-terminated literal hashes are `2e4b804e…8429df9` current and `24c17bb4…2ee7e0` January.

The same function remains 320 padded bytes / 24 relocations on each side with different normalized hashes. Its known independent boundary mismatch persists at `+0xe1`: January compares against five slots, while current source traverses the declared four-slot array. The September2 network matching log documents that a fifth access reaches the adjacent admission field. This diagnostic supplies no new array extent or ownership proof and does not justify restoring that unsafe access. No network source shape or compile was attempted; the safe current bound and all production code remain unchanged. The literal hit does not close the function.

## Review and release

Independent review is released as `independent-review.md` / `.json`, with reproducible raw fixtures in `independent-review.py`. Its final verdict is **PASS_NARROW_LOCATOR_WITH_EXPLICIT_COVERAGE_LIMITS**. It checks final scanner SHA `6358abe1ac52b7d5cded97a66a5c0f481679220c5c5ce75240fe2bdd3611e016` and result SHA `083840c102dfd8bfef549638b87b40b0d15da0cc3487ed118220394665ca7534`.

Diagnostic/network-triage source shapes:0. Compiler invocations:0. Admission or coverage gains claimed:0. The separate actor argument experiments are documented in their own packet and remain nonexact research. No further packet12 probes are authorized by this report.
