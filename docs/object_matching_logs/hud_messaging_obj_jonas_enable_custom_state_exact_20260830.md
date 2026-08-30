# `hud_messaging.obj` custom-state enable exact first shot

## Result

The sole natural typed C89 candidate for
`_hud_enable_custom_state_message` is strict exact on its first and only
code-producing compile. It contributes 111 meaningful bytes, 112 padded
bytes, one function, and three ordered relocations. `hud_messaging.obj`
advances from 18/36 to 19/36 strict functions and remains `NonMatching`,
with three documented residuals and fourteen unwritten functions.

Target and candidate have normalized SHA-256
`e3d951e9e5cd8d5a5ae1e20da6070429a1821dc30196850a2fd42c245ff70918`.
The immutable first-shot production object is
`build/audit/hud_messaging_enable_custom_state_first_shot_20260830.obj`,
with whole-object SHA-256
`982d656ac1027d5ce9a1542a43715c184b502bec13ba449034ccde59198150ca`.
The January target object has whole-object SHA-256
`3c5c3b8162268f4803a4c98742a28d6a76e61c3d6632c6995075c2ecaf0d152c`.

The ordered relocation stream agrees exactly:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x05` | `IMAGE_REL_I386_DIR32` | `_bss_00453ab8` |
| `+0x53` | `IMAGE_REL_I386_DIR32` | `??_C@_11LOCGONAA@?$AA?$AA@` (`L""`) |
| `+0x5F` | `IMAGE_REL_I386_REL32` | `_ustrncpy` |

## Authority, provenance, and typed topology

January COFF and disassembly are authoritative. They establish the same
`0x460` datum authenticated by the adjacent exact text and icon setters. The
function indexes the datum with its short player argument, computes the byte
at datum `+0x45E` as the prior value OR whether old and new validity differ,
writes validity at `+0x458`, and clears the state-message definition pointer
at `+0x454`. The enabled arm clears that pointer again and calls
`ustrncpy(datum + 0x230, L"", 0xFF)`. The final store writes the enable value
to the custom-message byte at datum `+0x45F`. The meaningful region ends at
the `ret` at function offset `0x6E`; offset `0x6F` is identical padding.

HCEA's clean donor repository at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4` independently supplies the
readable source topology. `src/hud_enable_custom_state_message.c` has Git
blob `9da6ad951bb5e9170cae7397dcd0d114af34c29e` and physical SHA-256
`2bcc02ff3a669bbc631483df758f9a2d0a50a65ff6c40df596a85343dbb44700`.
Its datum header has Git blob
`74f8ef62a26634bf1f96d44f53a61dfda6cd90a7` and physical SHA-256
`7b0b891f16d5b32ee05711f51ad2c7d5ed0d4fdff6f5293a8194cf1a2a51830b`.
The independent HCEA caller `src/hud_show_action_response.c`, Git blob
`b0c641961a83945ce2e932d56b36461924109aea`, proves the public
`short, uint8_t` signature and both enable and disable call sites.

The freshly censused Pastudan January-PC tree at clean commit
`918af885935ec470a31256ecce9a977b12b01f80` corroborates the complete
control flow and field positions. Its HUD Messaging source blob is
`c81d8429d9517e160ce04417567ceb2f91749303`; its HUD caller blob is
`8e850e6e6d971beeef07bf1653ada654c4884018`. The clean Claude HUD raw-lift
worktree at commit `4e09cbab405386f2d8e55fd817d27dcf7b75eaa4` and source blob
`17422f0787299a41adb55bb352ac421d6f6db440` independently agrees. Raw
addresses in the PC lifts were evidence only; the retained implementation
uses the canonical typed global, datum, nested state-message record, named
fields, and array indexing.

The validity-transition expression has donor-authenticated semantics: the
existing `leave_first_line_blank` byte is set when validity changes so the
state-message layout is recomputed. The final byte is the donor's
`custom_message` flag. The only call is public `_ustrncpy`, declared in
`source/text/unicode.h`, implemented in `source/text/unicode.c`, and present
in the canonical symbol configuration. The wide empty string is a natural
translation-unit-local COMDAT, so the candidate is link-closed.

## Census and one-shot discipline

The lane started in a fresh isolated worktree from canonical commit
`ef51cab6de891c65c4a5fac7b249edbf8043ffe7`. Every applicable project,
Claude, tooling, acceleration, and HUD Messaging ledger was read before
mutation. Branch and ref history contained nine unique committed HUD source
blobs; none defined this owner. The 1,533 registered campaign worktrees
resolved to 1,320 unique committed HEADs and eight unique HUD source blobs;
none defined the function. Untracked and scratch artifacts were excluded.
No prior candidate or measured boundary exists.

The pre-shot source Git blob was
`9e25971299ada7d50c538dde9e2548df501f71fe`, with checkout SHA-256
`fb43a0aa0a7a49040e583ab53127554c408a1516a4a5c017bb685834ffd06add`.
The retained source Git blob is
`bc3a19806130d21137b65b1bf075ff6c0df06f56`, with checkout SHA-256
`2078a5a3d4f45fd9eae0fcda25915ee795603edf9e803daa736bfe65957f0b2e`.
The preserved baseline production object SHA-256 is
`3ef62fb0965cb37a972a107ad970d0722b2e981c1bf8b6eb242baa601c2369a2`.

The candidate adds only two compile-time field-offset assertions and the
donor-concordant public function. Exactly one candidate production edge was
compiled: `ninja build/base/source/interface/hud_messaging.obj`. There was no
alternate spelling, declaration, branch, expression, cast, tuning pass,
sweep, flag experiment, or retry. Direct hardened comparison reports zero
differing padded bytes, identical ordered relocation destinations and
addresses, and the identical normalized hash above.

Exact-set regression moves from eighteen inherited exact functions to
nineteen, with `lost_exact=[]`; the only newly exact owner is
`_hud_enable_custom_state_message`. In particular, the adjacent exact text
and icon setters remain exact. The retained source uses no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw runtime
address or offset, pointer or union pun, undefined behavior, synthetic
anchor, byte forcing, object patch, or tool/compiler exception. The raw
hexadecimal constants added by this lane occur only in compile-time
`offsetof` assertions authenticating named fields. `git diff --check` and
the changed-line source-policy audit pass.

## Validation

- Full `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass across 572 actions.
- Semantic report: 470 units scanned, 4,944 functions evaluated, 4,827
  semantic exact, 131 hidden exact / 78,940 hidden bytes, one ordinary-only,
  one ordinary-structural, zero ordinary-rejected, zero unit errors, and
  4,837 accepted exact.
- Progress report: 384/833 files; 609,485/2,198,102 meaningful code bytes;
  4,798/11,060 functions; 1,856,050/4,176,062 data bytes. Halo-only progress
  is 282/468 files; 596,571/1,770,166 meaningful code bytes;
  4,631/7,574 functions; 1,850,858/3,923,451 data bytes. Canonical
  `ef51cab6` includes other integrated exact owners since the adjacent icon
  lane; the isolated contribution of this lane is the 111 meaningful bytes
  and one function measured above.
- `python -m pytest -q`: 212 passed.
- Admission audit: zero candidates, contradicted entries, or revoked entries.
- Parked audit: twelve active, zero stale, zero invalid.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact,
  1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `build/report.json` SHA-256:
  `83c69c52d212b43d62c517f7298a3e70122113c255d95fda0b6376637215999d`.
- `build/semantic_report.json` SHA-256:
  `00cd143d7a65422bb518bfd59b5ed087e8e2a9569588ab8880a7f33f65a498f3`.

Units, Vehicles, Matrix Math, AI Debug, configuration, shared headers,
semantic exceptions, parks, completion labels, and build rules are untouched.
This lane is a local source-and-ledger commit only; it was not pushed.
