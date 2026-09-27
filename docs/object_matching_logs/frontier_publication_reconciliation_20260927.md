# Remaining-frontier publication reconciliation, 2026-09-27

Canonical baseline: `e3aea795e8ec1804ffbe4242e163b1ee1c4922b9`.
Donor reviewed: `9736deed9c45cb3d7ab527852941a6b051e1c4f3`, read-only.
Both authorized GitHub `jonas/exact-pilots` branches were at
`cea8606242047d35c68c08cf3d3b17ae0ff202c5` before this reconciliation.
The inherited README edit and untracked research directories are not part of it.

## Deliberate debug-crash command

Reconciles donor `d18a7cee65697afbd9acba3f877002e089c7ca50` under the
owner's explicit BW-P1 original-deliberate-crash ruling. Only its ten source
lines are imported; no new trigger, compiler control, assembly, or header
migration is introduced. The existing `hs.c` consumer-local declaration has
the same `void(char const *)` ABI. Its owner-header debt remains for the
separately reviewed complete declaration correction, not an ad-hoc prerequisite.

January at 0x4f14d0 emits `c7 05 00000000 d89a6700 c3`: store the address
of `chucky was here!  NULL belongs to me!!!!!` through address zero, then
return. This is the existing script command described as `crashes (for debugging).`.
The source retains the approved BUG disclosure and deliberately ignores `str`.
This site-specific approval does not generalize to accidental null writes.

Independent stock-compiler control/candidate builds on this canonical base:

- main: 93 exact / 1 residual / 1 unwritten -> 94 exact / 1 residual / 0 unwritten.
- `_main_crash`: 16 padded / 11 meaningful bytes, one relocation;
  normalized SHA-256 `abe944925d4f3b974a0bd6e1ec5523804233ecf1d23afb41bf05be96ee3ddc59`.
- Added only this function and its 42-byte literal COMDAT; all inherited
  non-debug sections, data, storage and COMMON remain unchanged.
- Stock warnings 0 -> 0; independent /W3 check 25 -> 25.
- Unchanged objdiff 3.3.1 confirms main's credited data gain of 1,796 bytes.

Canonical full build and 8,252-row stable sweep: one gain, zero losses;
7,642 strict-exact owner rows. Halo credited code becomes 1,598,253 / 1,770,166
(7,470 / 7,574 credited functions); data 2,590,699 / 3,923,451.
Objects remain 390 / 468. Parks 72 active, zero stale/invalid; admission and
26 inherited fake-scan leads unchanged. Tools: 1,161 passed, 5 skipped,
26 subtests. `git diff --check` passes. No park is retired for this formerly
unwritten function.

Local, unpublished receipts are in `scratch/astra_publish_20260927/`, including
the independent `main_review/` control/candidate objects and comparisons.
Private reference/compiler assets are not publication artifacts.

## Q11: independently verified HS data, not code

The approved PA+PC and PB tool changes are separate no-credit commits,
reconciled from `c32e53c2` and `dccd2e95`. Only the HS entry from `e3c053cd`
is appended here; all preceding canonical entries and the shell generated-name
binding remain unchanged. There is no actions entry or scorer upgrade.

Fresh independent review verified 910 full data sections, 2,207 relocations,
and 20 surplus literals against unique January and current providers in 17
units, with the 833-object target census sealed. Member coverage, section
symbol identity, COMDAT selection, complete payloads and resolved targets
are checked; missing-member, missing-surplus and wrong-provider controls fail.
The live report is rebound to a fresh run of SHA256-pinned objdiff 3.3.1
(`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`).
PB alone leaves the complete report unchanged. The HS-only entry supplies
53,122 raw data bytes plus 1,658 modeled alignment-padding bytes: **54,780
additional data credit**, no code/function/object credit. HS remains incomplete.

Production full gates after each stage preserve all 8,252 function verdicts,
72 valid parks and the admission results. Final tool suite: 1,318 passed,
5 skipped, 100 subtests. Halo data becomes 2,645,479 / 3,923,451. The separately
reviewed verifier cases exercised all 205 tests, including an isolated-run
scorer-path skip subsequently closed against the actual pinned binary.
Receipts: `scratch/astra_publish_20260927/q11_review/` and `q11.*` logs.
This review is specific to the approved HS entry, not blanket permission for
future extent-model entries or surplus definitions.

## Scope boundary

B3 and INC-3 are not imported by this packet. B3's two new incompatible-pointer
warnings remain under the owner's requested investigation. Other donor source,
storage, data-verifier and admission packets require their own current-base
reconciliation; donor-relative credits are not automatically additive.
