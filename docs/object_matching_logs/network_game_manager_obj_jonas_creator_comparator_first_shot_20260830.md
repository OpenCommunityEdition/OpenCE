# `network_game_manager.obj` creator/comparator first shot (2026-08-30)

## Result

The only coherent unattempted packet in this object was reconstructed from
January disassembly and authenticated public lineage: private qsort comparator
`_code_0011a750` and its sole owner
`_network_game_create_game_objects`. One typed C89 packet was compiled once.
Neither function is strict exact, so the complete packet was inverse-reverted
without a spelling, declaration-order, predicate-order, or lifetime retry.

`source/networking/network_game_manager.obj` remains at 13/19 exact functions
and 1,536/3,264 strict padded code bytes. All thirteen inherited exact owners
remain unchanged. The production source is restored to Git blob
`a17029f58010eb920d40b58f73cc7d18ca95e3f1`; no header, configuration,
storage owner, semantic exception, parked entry, Units source, or unrelated
source is changed.

## Scope and evidence census

- Isolated worktree:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/game-allegiance-closeout-20260830`.
- Isolated branch: `jonas/game-allegiance-closeout-20260830`.
- Integration base for the experiment:
  `bc1850aa9999319d61503d05131d313f8f4711d3`.
- January split object SHA-256:
  `ed9e3a538ee47b2cfd9472bb7ae53329da63c35387a2142ddc8c986f99128213`.
- The three earlier object ledgers were read first. They freeze the public
  donor families for add/remove player and add/remove machine; none was
  reopened here.
- The campaign methodology, acceleration playbook, gate/comparator tooling,
  and authenticated Stian/Pastudan `CLAUDE.md` and `AGENTS.md` doctrine were
  read before emission.

The target-version public reconstruction is Stian commit
`66ebbca24` with the comparator/creator correction commit `fd97cbeae`; the same
bodies are present in the authenticated Pastudan history. Stian recorded only
69.4% and 78.7% VC71 scores for the later PC functions, so these bodies were
hypothesis generators rather than acceptance evidence. The pinned HCEA tree
independently corroborates creator semantics in
`src/blam/networking/network_game_create_game_objects.c`, but it has a later
32-player layout and different connection-role behavior. January COFF alone
selected constants, field offsets, call order, loop bounds, and acceptance.

January proves the private 320-byte function is the qsort comparator used by
the creator. It validates two typed 32-byte network-player records, sorts valid
records before invalid records, orders valid records by machine then
controller index, and asserts on duplicate machine/controller identity. The
creator builds a 0x10C-byte typed game-options local, copies the named map,
selects the random seed by connection role, unloads the menu before testing an
old game, loads the variant/map, qsorts sixteen players, and spawns each valid
record.

The frozen source named every accessed field through typed structures. It used
no assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw address or byte offset, pointer/integer reconstruction, pointer or
union pun, undefined behavior, synthetic anchor, code-generation forcing,
object patch, compiler change, flag change, or comparator exception.

## Frozen candidate

The first mechanical preflight stopped before object emission because the
newly named map aggregate required `&game->map` at one inherited `csmemset`.
Correcting that type-only expression produced the one and only candidate
packet. Its ignored immutable artifacts are:

- source `build/audit/network_game_manager_creator_comparator_first_shot_20260830/candidate.c`,
  13,369 bytes, Git hash
  `ae347dc8a86ec92be4a54f363cec6367485d4193`, SHA-256
  `6bb4439ffe915450e75d5c0b97016a61f92225560f1459e22d7e3dc129ac8631`;
- object `build/audit/network_game_manager_creator_comparator_first_shot_20260830/candidate.obj`,
  9,985 bytes, SHA-256
  `69fa689554f506776dcfaf00fdcade0f6dd2a52a2da0c86e186b39fea42b5645`.

The full gate reported 13 exact inherited functions, two residual candidates,
and four still unwritten functions.

### `_code_0011a750`

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded bytes | 320 | 320 |
| Relocations | 8 | 8 |
| Normalized SHA-256 | `bddf314f7828a42dbaa54e8de2065aeb60898f3f84730ae8b01928b8b27ba7b7` | `abdb3fd8ef2bc2273604ae8b27ae4341e9e0af2ea3b8d61244366687c2ac8d05` |

All eight relocation destinations agree in order. January relocation sites are
`+30,+35,+40,+47,+277,+282,+287,+294`; the candidate sites are
`+27,+32,+37,+44,+279,+284,+289,+296`. January carries `p1` in ESI, `p2` in
EDI, and a terminal zero in EBX. The candidate reverses the player-register
allocation, has no EBX lifetime, and emits 136 decoded instructions versus
January's 152. This is a source/CFG boundary, not an admissible register mirror.

Do not repeat the direct typed expansion of the public donor's three validity
predicates. Reopen only with original January comparator expression/scope
topology, target-version local-variable evidence, or another authenticated
source record that explains the EBX zero lifetime and repeated validity CFG.

### `_network_game_create_game_objects`

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded bytes | 432 | 416 |
| Relocations | 25 | 25 |
| Normalized SHA-256 | `a79f30466a9619636c22f5a3141f0dc2ff1bc320599c1b6244e7c6773e66d146` | `f4eb962716c167ca02c4f36ff924cb2372c02543a6550bb3256139b29fbc8e33` |

All 25 symbolic destinations agree in order. The first structural divergence
is independently attributable: January sign-extends the `short` return from
`game_connection` into EAX and performs 32-bit tests/comparisons; the candidate
keeps the declared `short connection` in AX. January also saves only ESI on
entry, then makes EBX/EDI live inside the successful-load arm, preserving the
game in ESI, the loop count in EBX, and a field-oriented player cursor in EDI.
The candidate saves EBX/ESI/EDI on entry, preserves the game in EDI, and walks a
record pointer in ESI. The later relocation sites therefore move despite exact
destination identity.

Do not repeat the short connection local or record-base cursor loop. Reopen
only with provenance for the original promoted connection local and the
success-arm scope/cursor topology. A future candidate must account for both
the 16-byte envelope deficit and the January callee-save lifetime before
emission.

## Restoration boundary

The production source was restored byte-for-byte at the Git-content level
immediately after measurement. The remaining unwritten functions are
`_network_game_add_machine`, `_network_game_add_player`,
`_network_game_remove_machine`, `_network_game_remove_player`,
`_code_0011a750`, and `_network_game_create_game_objects`; all six now have a
documented first-shot boundary. Object closure is not justified without new
provenance satisfying one of the explicit reopen criteria above.

## Restored-state validation

- strict object gate: 13 exact, zero residual, six unwritten;
- strict campaign board: 277/619 objects, 667,863/1,922,669 padded code
  bytes, and 4,823/8,246 functions;
- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,957 functions evaluated, 4,841 semantic
  exact, 131 hidden exact, 4,851 accepted exact, and zero unit errors;
- progress: 384/833 linked objects, 4,812/11,060 functions,
  610,646/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, and zero invalid;
- protected `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes;
- complete tooling suite: 212 passed; and
- `git diff --check`: pass.
