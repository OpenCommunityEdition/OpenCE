# `sound_manager.obj` Jonas wave 2

This Jonas/Codex ledger records a bounded second recovery wave for
`source/sound/sound_manager.obj`. The object remains `NonMatching`; this wave
adds four independently exact public functions and makes no initialized-data
or completion claim. No pre-existing or Claude-authored Markdown was edited.

## Scope and provenance

- Integration base: `a3a3549b4946d40fb2984eb7a8536f4512f50111`.
- The previous Jonas checkpoint and its source worktree were reviewed first;
  its four donor bodies were not repeated as candidates.
- Signature and behavior evidence came from the HCEA applied source/type
  corpus and its PDB-backed function labels. The January PDB2.00 labels and
  target COFF disassembly independently identify `sound_dispose` at
  `0x001BB310`, `sound_pause` at `0x001BB390`, `sound_try_and_get` at
  `0x001BB450`, and `sound_scripted_dialog_is_playing` at `0x001BB480`.
- The retained code is reconstructed from those typed semantics and the
  January instruction/relocation shapes. No Demon or other third-party code
  is copied.
- Only `source/sound/sound_manager.c` and this new ledger are changed. There
  is no header, configuration, frozen-object, or existing-Markdown edit.

## Strict code result

The untouched base measured 6/65 strict exact functions, 77 meaningful code
bytes, 128 padded bytes, and six relocations. The final rebuilt object measures
10/65 strict exact functions, 271 meaningful bytes, 352 padded bytes, and 27
relocations. The strict gain is therefore four functions, 194 meaningful
bytes, 224 padded bytes, and 21 relocations. All six baseline functions remain
exact.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_sound_dispose` | 92 | 96 | 12 | `441deffcd618a158bb839ebb91485b308585c30493f8a3f6c36578c794bec41b` |
| `_sound_pause` | 51 | 64 | 5 | `44273d95b04b7b9d1b4cc25d4ab70bb70d5e7fb8a968ede0f171bc4e4f6cbf41` |
| `_sound_try_and_get` | 30 | 32 | 2 | `0f99dbeb5e73f08c867c1933002b9d4b0d169efff0406a44bd9e877eb561438a` |
| `_sound_scripted_dialog_is_playing` | 21 | 32 | 2 | `83f1c215998fdaedb66e675d2e289086a302d075782fc403c2d4749e9752bbc6` |

For every retained function, target and candidate have identical normalized
bytes, padded extent, relocation addresses/types/destinations, and one unique
external function symbol with matching type and storage class.

## Bounded rejections

`sound_valid_for_channel` was included in the one candidate compile because
its five-parameter signature and channel-format predicate are independently
attested. Its natural typed-C body produced the target's 112-byte padded
extent and zero relocations, but not its normalized bytes: target
`871d420ee5ca15118e9203c06aa1defa3724de44cfbd6b532aab0bf6f8a16472`
versus candidate
`b0476f35431d7f6157dc23a2855e88a9b33c38ddd1efe20e69f754224e6f17ed`.
It was removed without source-shape or code-generation tuning; the final
candidate has no definition or symbol for it.

`sound_reconnect_to_structure_bsp` was rejected before compilation because
the January PC target traverses a different state/array topology from HCEA's
implementation. That divergence is not a safe basis for an inferred typed
body.

## Typed layout and data ownership

The manager state refines the already-established prefix with `paused` at
`+0x02` and the scripted-dialog deadline at `+0x04`. The platform dispatch
table is represented as a typed 0x40-byte structure; compile-time checks hold
`dispose` at `+0x08`, `set_pause` at `+0x28`, the two manager fields at their
target offsets, and the manager aggregate at 0x178 bytes.

The candidate continues to emit exactly one common BSS owner named
`_bss_004d2d60`, value/size 376, external storage class 2, type 0, with no
payload or relocations. This is the same sole COMMON owner inventory as the
untouched base. The final candidate emits no `.data` or `.rdata` section and
no subordinate BSS owner. The target's 3,952 non-code bytes remain entirely
uncredited.

## Source policy

All retained bodies are readable defined C with typed state, data-array, and
platform-dispatch access. No-argument definitions use `void`, parameters are
listed one per line, and each recovered void function has an explicit terminal
`return;`. There is no assembly, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, raw address reconstruction, pointer or
integer cast, union pun, signed-overflow match, synthetic anchor, or
byte-forcing construct.

## Reproducibility and gates

- Target object SHA-256:
  `5942253e0f66d9bc8b7927767b20c747de07b6159a05b21e2dc8eaffe859d729`.
- Rebuilt object SHA-256:
  `fce426df49cd1196d435dcef76a8c41ee15c0c4a60734370be11f7fe313e9790`.
- Final source SHA-256:
  `2d0626acfc043545005cfa2728011aa50aaa0365f18fb6acc7893959140c1658`.
- Full Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,957 functions evaluated, 3,817 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,878 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,865/11,060 exact functions,
  470,801/2,198,102 exact code bytes, and 1,834,808/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, and zero invalid.
- Tool suite: 179/179 tests pass.

The five user-frozen large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted.
