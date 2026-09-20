> Research-only packet15: zero production change and zero exact credit. Validated production remains wave13.

# Packet15: bounded call-argument diagnostic extension

**Reviewed locator; no new source lead or exact credit.** Final scanner SHA256 `47a457d5f928cc252c1df1b139d786a2e72f04e2a5a6b819d979e3fc7c555a07`; full-result SHA256 `1cc00b7196fdb6c10c3c6d5e1d71ac20bdc6dccc4935f2b393add436e934d35c`. The raw result retains its producer `UNREVIEWED_DIAGNOSTIC_ONLY` label; the separate bound independent review records the subsequent PASS. Neither label grants source admission.

The packet12 locator discarded outgoing argument words at every memory write. This scratch extension retains them only where a known ESP/EBP coordinate proves the write disjoint. Exact byte overlaps invalidate the affected words; indexed, segmented or unknown writes discard facts. It also recognizes supported finite literal x87 transfers into outgoing slots. It does not evaluate floating arithmetic or promote local spill reloads.

Frame coordinates use actual same-side COFF provider bodies and reachable ordinary RET cleanup amounts. A same-object provider takes precedence only when its private/static ownership is proved; external and COMDAT providers must agree. These summaries are conditional on ordinary normal return with frame restoration and preserved caller EBP. They do not prove termination or a complete ABI. Unknown tail/indirect/far exits, unresolved jump tables, unclassified edges and missing fallthrough remain unsupported. Branches, calls and targets reset straight-line value facts. Read-only literal data must agree with its encoded value, have no relocation overlap and have no conflicting or invalid provider. Unsupported FP state changes or overflow taint the tracked FP values.

The independent reviewer found and root fixed seven concrete defect classes: cleanup lookahead crossing an ESP write; FXCH's implicit ST0 operand; missing conditional fallthrough; silent ninth-push x87 truncation; invalid literal providers being ignored; external provider disagreement hidden by local precedence; and stale facts across MMX/EMMS/XRSTOR state changes. Initial scripts/results and failing-case history remain archived. Final review passes 51 call-site cases, 16 return-summary cases, eight actual literal-index cases and four actual provider-selection cases. These overlapping tests validate the bounded implementation, not universal x86 soundness.

| Final full scan | Count |
| --- | ---: |
| Exact control functions | 3,568 |
| Controls with known argument words | 1,581 |
| Known control words | 5,343 |
| Residual functions visited | 273 |
| Call-site visits, both sides | 53,783 |
| Different ordered call sequences excluded | 44 |
| Residuals with compared known words | 151 |
| Compared residual word pairs | 1,292 |
| Known differences | 1, already known |

Compared on the same 3,567 packet12 controls, known words rise from 5,198 to 5,336 (+138), and controls with coverage rise from 1,565 to 1,580 (+15). Sixty-eight controls gain facts and none lose them. The additional exact control, wave13's already-admitted action_charge_setup, contributes seven words; it is not a diagnostic or production gain in this packet. There are still 1,987 zero-word exact controls and 122 residuals without compared words, including excluded call sequences. Store-event counts (3,079 disjoint, 3,042 overlaps, 58 literal x87 stores) are internal events, not independently covered calls or source gains.

The only full-run hit is precisely the old packet12 network error string in network_server_close_client_connection. Its longer January parenthetical was already decoded. The independent four-versus-five array-bound hold still blocks the function; no new source probe follows. No other residual difference was exposed by the covered words. This does not establish equivalence or exhaust the uncovered words, operations or functions.

Five manually authenticated paired call windows test real-object behavior. The earlier-version witness replay recovers green/blue vector facts and preserves the matrix overwrite guard. Known purple and circle positives remain unknown because actual tail/indirect exits destroy frame coordinates; no synthetic cleanup hints or no-return assumptions were added. Those witness receipts explicitly identify their older scanner version. Selected-result.json likewise predates the final metadata-bound full result and is historical evidence, not the final corpus count.

Ordered named-call correspondence is required before comparing argument words. Every new hit would still require manual signature, branch-role and raw argument binding plus independent primary evidence. Existing strict bytes/relocations, admission, source, ownership and regression guards are unchanged. Named-global load provenance is not proof of immutable numeric contents. No general compiler law or original lexical source is recovered.

**Zero source shapes, C compiler calls, production changes, admitted functions, meaningful bytes or padded bytes.** Full production validation remains wave13; full tools tests remain wave9. IDA_AUTOMATION_UNAVAILABLE and the user-confirmed local artifact limits remain closed. This packet ran no fresh Ghidra/DIA extraction or artifact search.
