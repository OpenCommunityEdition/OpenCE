# `hud_messaging.obj` state-message text exact first shot

## Result

The sole natural typed C89 candidate for `_hud_set_state_message_text` is
strict exact on its first and only code-producing compile. It contributes 105
meaningful bytes, 112 padded bytes, one function, and two ordered relocations.
`hud_messaging.obj` advances from 16/36 to 17/36 strict functions and remains
`NonMatching`, with three documented residuals and sixteen unwritten
functions.

Target and candidate have normalized SHA-256
`f8c5e6845d72c0574da8e0dd0fd229d688a9addfae2d9e9e6839b26ec78681e0`.
The immutable first-shot production object is
`build/audit/hud_messaging_state_text_first_shot_20260830.obj`, with
whole-object SHA-256
`85eb319c14b62177936c8ec46f2618113becaea671cd1170130a2b33cf265ccb`.
The January target object at `build/split/source/interface/hud_messaging.obj`
has whole-object SHA-256
`3c5c3b8162268f4803a4c98742a28d6a76e61c3d6632c6995075c2ecaf0d152c`.

The ordered relocation stream agrees exactly:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x09` | `IMAGE_REL_I386_DIR32` | `_bss_00453ab8` |
| `+0x22` | `IMAGE_REL_I386_DIR32` | `_hud_scripted_globals` |

## Authority, provenance, and typed topology

The January COFF and disassembly are authoritative. They establish a datum
stride of `0x460`, the state-message runtime payload at datum `+0x230`, the
eight four-byte text-info records at payload `+0x204`, the state-message
definition pointer at payload `+0x224`, the valid byte at payload `+0x228`,
and the text flags at payload `+0x229`. The target gates on the valid byte,
the global HUD-help Boolean, and the state-message pointer; then it stores the
short string index and Boolean scenario-name selector and sets the indexed
text bit.

The independent HCEA donor
`C:/Users/isabe/Documents/Codex/reference/halocea/src/hud_set_state_message_text.c`
at repository commit `aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`
and Git blob `b50b89d4b265469653b35c366f25c3d7ad1efb55` corroborates the
same semantic and typed layout. Its physical SHA-256 is
`81b3272c7c1aba3bd5330b8075ff1d4e56ab999046dc76bd5c779aa1a63911e0`.
The clean Claude HUD raw-lift worktree
`C:/halo-worktrees/claude-hud-messaging-3` at commit
`4e09cbab405386f2d8e55fd817d27dcf7b75eaa4` and source blob
`17422f0787299a41adb55bb352ac421d6f6db440` independently agrees with
the control flow and field positions. Both donors were read only; raw
addresses from lifts were used only as evidence. The retained implementation
uses canonical typed globals, structures, fields, and array indexing.

## Census and one-shot discipline

The lane started from canonical commit
`b654ee4f9e1a84181d5b6eeed816f2d2c1fa198a`. Every canonical HUD Messaging
ledger, applicable project/Claude/tooling instruction, registered worktree,
branch history, target disassembly, and available donor was audited before
mutation. No prior candidate or measured boundary for this function was
found. A scan of all 1,525 registered campaign worktrees found no existing
canonical-source definition to harvest.

The pre-shot source Git blob was
`8a4bffa85d9811964ea79449c2166f1cd76c43fa`, with physical SHA-256
`61c7c7f31b6319c3f5a1b20a8b60923ae507b806464255affc104ae1dcc57cf7`.
The retained source Git blob is
`da80247c295fe97b43eb9bc38040fce23a9b8f81`, with physical SHA-256
`a296b84ce3656032a89d90099b9463a90cd92bb3daea4a00b71c052e947111df`.
The baseline production object SHA-256 was
`54c1ac96aaeba27f79f3d59069ef3de80a3bd4af4c3680acdb3358b7aab02e9b`.

The candidate added the named four-byte text-info record, exposed the natural
typed fields within the already authenticated `0x22C` runtime payload, and
implemented the donor-concordant public function. Exactly one production
edge was compiled:
`ninja build/base/source/interface/hud_messaging.obj`. There were no source
spelling variants, sweeps, tuning passes, compiler experiments, or retries.
Direct comparison reports `section_infos_equal: true`; the complete padded
COMDAT and both ordered relocation identities agree.

The exact-set regression moved from sixteen inherited exact functions to
seventeen, with `lost_exact=[]`; the only newly exact function is
`_hud_set_state_message_text`. The retained source uses no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, barrier, raw runtime
address or offset, pointer or union pun, undefined behavior, synthetic anchor,
byte forcing, object patch, or tool/compiler exception. The hexadecimal
constants added by this lane occur only in compile-time `offsetof` assertions
that authenticate the named structure layout. `git diff --check` and the
changed-line policy audit pass.

## Validation

The full production build and reports completed successfully:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass across 572 actions.
- Semantic report: 470 units scanned, 4,940 functions evaluated, 4,823
  semantic exact, 131 hidden exact / 78,940 hidden bytes, one ordinary-only,
  one ordinary-structural, zero ordinary-rejected, zero unit errors, and
  4,833 accepted exact.
- Progress report: 384/833 files; 609,169/2,198,102 meaningful code bytes;
  4,794/11,060 functions; 1,856,050/4,176,062 data bytes. Halo-only progress
  is 282/468 files; 596,255/1,770,166 meaningful code bytes;
  4,627/7,574 functions; 1,850,858/3,923,451 data bytes.
- `python -m pytest -q`: 212 passed.
- Admission audit: zero candidates, contradicted entries, or revoked entries.
- Parked audit: twelve active, zero stale, zero invalid.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact,
  1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `build/report.json` SHA-256:
  `99f1ac428563c34b4689a79b01911479d8ff6343f7aefe50b5b6d159a156bb13`.
- `build/semantic_report.json` SHA-256:
  `3dc0fe879c946a9787f368dfe8d0cc20bac9454722dc1530aa166424d0f3ccb5`.

Units, Vehicles, Matrix Math, AI Debug, configuration, semantic exceptions,
parks, completion labels, and build rules are untouched. This lane is a local
source-and-ledger commit only; it was not pushed.
