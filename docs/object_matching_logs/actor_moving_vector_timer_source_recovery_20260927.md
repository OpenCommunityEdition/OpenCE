# Actor vector-avoidance timer: source-fidelity reconciliation

Baseline: `cea8606242047d35c68c08cf3d3b17ae0ff202c5`.
Result: **zero new exact functions, zero credited bytes, zero completed objects,
zero exact losses**. This is a narrow improvement to the retained fuzzy source,
not a byte-matching closure or a new compiler breakthrough.

## Change and source evidence

`_actor_move_vector_avoidance` now updates/clears the sharp-turn timer in a
complete statement immediately before selecting the sharp-turn movement body.
Previously those timer operations were embedded at the tops of the two movement
arms. Both movement bodies and all mathematical expressions remain unchanged.

This reconciles RF-G HIGH1 / RF-X X02, originally retained as research in
`claude-remaining-frontier-20260926`. A fresh independent reviewer decoded the
two reference artifacts as data, rather than accepting the worker's summary:

- January's COFF function loads the timer at `+0xbca`, compares with NONE at
  `+0xbd1`, and branches at `+0xbd5`. Setting zero falls through to the body at
  `+0xbe4`. The increment is out of line: `+0xce6 inc eax`,
  `+0xce7 mov [ebx+0x5f0],ax`, `+0xcee jmp +0xbe4`. Non-sharp paths clear the
  timer at `+0xcf6` before entering their movement logic.
- The later first-party `/Od` function at `0x469ce0` has a complete timer
  if/else at `0x46aff0..0x46b03e`, then tests the same live `sharp_turn` flag
  again to select the movement body. The false body at `0x46b1cc` has no
  additional timer clear. Its different actor offset (`+0x552`) is not
  imported into January's structure.

The later build directly supports the separate statement; January supports its
state ordering and block topology. January does not uniquely prove lexical
braces. The source claim is appropriately corroborated reconstruction, not
recovered original source text.

## Semantics and boundaries

Each path performs exactly the same one timer transition before the same calls
and arithmetic. `sharp_turn` is initialized, nonvolatile and unescaped, and no
call or assignment to it occurs between the two tests. The fixed-size movement
locals have no initializer or pre-use escape. Moving their scope start past the
timer statement has no observable C effect. No new read, integer conversion,
floating operation, precision change, helper, symbol, cast or macro is added.

The original uninitialized-weight, spill/view, helper and grouping holds remain
untouched. No compiler flags, scorer, target attribution or admission rule
changes. The rule permitting credible fuzzy source retention applies; the
exact-caller helper exception is not needed because emissions are unchanged.

## Fresh measurements

| Version | Padded section bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| Before | 4,128 | 135 | `4a1c49f3b9489a044a3cd60f2dbbc1efb8344a78e08484e2b876312ed69bdc20` |
| Retained | 4,144 | 135 | `b7c2aad3c3f24ce95635e182a3f78c57436612102f82a6997dfd91aab8d0ea7a` |
| January | 4,144 | 135 | `a8a8010c11375e82d6773df6110a10efc53469969382f39cc4d5aa0fc91449bf` |

Only this function's section changes. All 33 exact sibling rows and the two
other residual rows are identical; the TU remains 33 exact / 3 residual.
There are no added/removed sections or definitions and no changed storage,
COMMON, undefined-symbol inventory, data or surplus-helper copies.

The out-of-line timer increment appears as predicted. The stronger prediction
that no other block placement would change was **not** met: VC7 also relocates
the existing emergency/PIN block. Equal padded section size does not establish
equal meaningful code or exactness. The frame remains `0x60e4` versus January's
`0x60e0`; the other documented residuals remain. V2 was not triggered because
V1 was non-inert. The redundant fixed-test V3 was not tried.

## Verification

- Full `ninja all_source progress build/report.json`: passes.
- Stable 8,252-owner sweep: 7,641 exact before/after, zero gains/losses; complete
  stable snapshots are identical.
- Production object matches the reviewed candidate across every nondebug
  section and its storage/metadata; the rebuilt baseline also reproduced the
  prior production object.
- Explicit `/W3` A/B: the same 12 warnings, no additions; both warning-build
  objects match their normal-build counterparts.
- Park, admission and fake-scan reports are identical: 71/0/0 parks;
  12 candidates, 0 contradicted, 1 rejected, 0 revoked; 26 inherited scan leads.
- Tools suite: 1,161 passed, 5 skipped, 26 subtests passed.
- CRLF preserved; whitespace check clean. The user's README and inherited
  untracked research are untouched.

Halo totals remain **1,598,242 / 1,770,166 meaningful code bytes**, 7,469 / 7,574
credited functions and 390 / 468 complete objects. Data remains 2,588,903 bytes.
No treemap code-credit threshold was crossed. No push is part of this packet.

Local reproducibility receipts are in `scratch/astra_vector_timer_20260927/`:
`PLAN.md`, candidate source/diff and primary excerpts, independent
`review/REVIEW.md`, `before.json`, `after.json`, full build/test logs and
`final_verify.json`. The reference executable SHA-256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`;
the January COFF SHA-256 is
`92fb8f9c925d2dd19b06d06ff1716aea80b3e8a21c5183dee898547af689ae00`.
No private binaries are included in the commit.

Reopen the remaining function only on new evidence for its frame/spill or other
source differences. Do not repeat this timer experiment or infer permission for
held forms from this zero-credit landing.
