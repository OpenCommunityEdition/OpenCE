# `hud_messaging.obj` state-message icon exact first shot

## Result

The sole natural typed C89 candidate for `_hud_set_state_message_icon` is
strict exact on its first and only code-producing compile. It contributes 95
meaningful bytes, 96 padded bytes, one function, and two ordered relocations.
`hud_messaging.obj` advances from 17/36 to 18/36 strict functions and remains
`NonMatching`, with three documented residuals and fifteen unwritten
functions.

Target and candidate have normalized SHA-256
`7be2462be07e052e451075aaf95585364b2c928c2cc5bdfe9705f16b07b58633`.
The immutable first-shot production object is
`build/audit/hud_messaging_state_icon_first_shot_20260830.obj`, with
whole-object SHA-256
`fd0239d853634e508d53130354c47ba1a8f478b56036b1c1f5d9da67296a4bdd`.
The January target object has whole-object SHA-256
`3c5c3b8162268f4803a4c98742a28d6a76e61c3d6632c6995075c2ecaf0d152c`.

The ordered relocation stream agrees exactly:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x09` | `IMAGE_REL_I386_DIR32` | `_bss_00453ab8` |
| `+0x22` | `IMAGE_REL_I386_DIR32` | `_hud_scripted_globals` |

## Authority, provenance, and typed union

January COFF and disassembly are authoritative. They establish the same
`0x460` datum and valid/help/source-definition gates authenticated by the
adjacent exact text setter. The accepted arm stores the third promoted dword
at datum `+0x434 + custom_icon_index*4`, then clears the corresponding bit in
the byte at datum `+0x459`. The meaningful region ends at the `ret` at
function offset `0x5E`; offset `0x5F` is identical padding.

HCEA's clean donor repository at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4` supplies three independent typed
proofs. `src/hud_set_state_message_icon.c`, Git blob
`2184b82b2d88557cdc9962c45995a68d85f04e6e` and physical SHA-256
`b63b677135d4a33ab4cd42c882e42632c24f526d7ff7afb6cf6c167ecbec7864`,
uses a pointer to `icon_hud_element_definition`. `src/headers/state_info.h`,
Git blob `fd3fcbfd5cbed0fc8a8cc537eb8736542c311850`, proves that each four-byte
runtime slot is a union of that pointer and the existing short/Boolean text
record. `src/hud_show_action_response.c`, Git blob
`b0c641961a83945ce2e932d56b36461924109aea`, independently calls the function
with the typed address of a weapon HUD interface's messaging icon.

The freshly censused Pastudan January-PC tree at clean commit
`918af885935ec470a31256ecce9a977b12b01f80` corroborates the complete branch,
store, and bit-clear topology. Its HUD Messaging source blob is
`c81d8429d9517e160ce04417567ceb2f91749303`; its HUD caller blob is
`8e850e6e6d971beeef07bf1653ada654c4884018`. The clean Claude HUD raw-lift
worktree at commit `4e09cbab405386f2d8e55fd817d27dcf7b75eaa4` and source blob
`17422f0787299a41adb55bb352ac421d6f6db440` independently agrees. Raw
addresses and integer placeholders in those PC lifts were evidence only; the
retained code uses a named typed union, pointer member, fields, and indexing.

## Census and one-shot discipline

The lane started in a fresh isolated worktree from canonical commit
`2a3b0a00526b4e4c54d4d68dbbd8bf4d67cab807`. Every applicable project,
Claude, tooling, acceleration, and HUD Messaging ledger was read before
mutation. Branch history contained eight unique committed HUD source blobs;
none defined this owner. The 1,529 registered campaign worktrees resolve to
1,317 unique committed HEADs and seven unique HUD source blobs; none defined
the function. No prior candidate or measured boundary exists.

The pre-shot source Git blob was
`da80247c295fe97b43eb9bc38040fce23a9b8f81`, with checkout SHA-256
`b9161891454d33412af0db0849e8bc83b165fd074f2fdf91b15c562675afe1ef`.
The retained source Git blob is
`9e25971299ada7d50c538dde9e2548df501f71fe`, with checkout SHA-256
`8b4ae8aeda259a739a13c18ea368102700559b8289fd8e307023bc7a28de164a`.
The preserved baseline object SHA-256 is
`a880ee1b63fd2925f66e4af011625322052d87b8f31b184ea45f7335fa900e6a`.

The candidate refines the existing four-byte info record into its
donor-authenticated named union, retains the existing text record as the
`text` arm, adds the forward-declared icon pointer arm, and implements the
public setter immediately before its text sibling. Exactly one candidate
production edge was compiled:
`ninja build/base/source/interface/hud_messaging.obj`. No alternate spelling,
declaration, branch, expression, cast, tuning pass, sweep, flag experiment, or
retry followed.

Direct hardened comparison reports `all_equal: true`. Exact-set regression
moves from seventeen inherited exact functions to eighteen, with
`lost_exact=[]`; the only newly exact owner is
`_hud_set_state_message_icon`. The union refinement preserves the adjacent
`_hud_set_state_message_text` at 112 padded bytes, two ordered relocations,
and normalized SHA-256
`f8c5e6845d72c0574da8e0dd0fd229d688a9addfae2d9e9e6839b26ec78681e0`.

The retained source uses no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw runtime address or offset, pointer/integer
reconstruction, undefined behavior, synthetic anchor, byte forcing, object
patch, or tool/compiler exception. The named union is not used for punning or
inactive-member reads: the icon setter writes the icon member and the text
setter writes the text member. The compile-time union-size assertion only
authenticates the four-byte layout. `git diff --check` and the changed-line
policy scan pass.

## Validation

- Full `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass across 572 actions.
- Semantic report: 470 units scanned, 4,941 functions evaluated, 4,824
  semantic exact, 131 hidden exact / 78,940 hidden bytes, one ordinary-only,
  one ordinary-structural, zero ordinary-rejected, zero unit errors, and
  4,834 accepted exact.
- Progress report: 384/833 files; 609,264/2,198,102 meaningful code bytes;
  4,795/11,060 functions; 1,856,050/4,176,062 data bytes. Halo-only progress
  is 282/468 files; 596,350/1,770,166 meaningful code bytes;
  4,628/7,574 functions; 1,850,858/3,923,451 data bytes.
- `python -m pytest -q`: 212 passed.
- Admission audit: zero candidates, contradicted entries, or revoked entries.
- Parked audit: twelve active, zero stale, zero invalid.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact,
  1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `build/report.json` SHA-256:
  `8b591b6d6683530fdb72d72b16cee6fbbe794b9cc70ad715669172d8adb5497c`.
- `build/semantic_report.json` SHA-256:
  `64ed5a71b18c7e9171d31c76744be41600990e33b3123954da46f1f489c723b2`.

Units, Vehicles, Matrix Math, AI Debug, configuration, shared headers,
semantic exceptions, parks, completion labels, and build rules are untouched.
This lane is a local source-and-ledger commit only; it was not pushed.
