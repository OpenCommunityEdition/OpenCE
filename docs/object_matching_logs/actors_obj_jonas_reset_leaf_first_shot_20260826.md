# `actors.obj` Jonas reset-leaf first shot

## Result

This bounded source-only wave starts from clean cumulative commit
`c3f20ecb6074dd143ec413be6d50b22672390968`. The anonymous actor-update
sentinel reset matches in the first and only production candidate compile.
`source/ai/actors.obj` advances from 23/76 to 24/76 strict functions and from
1,448/19,130 to 1,459/19,130 meaningful code bytes. Its target-owned data
advances from 0/2,288 to 4/2,288 bytes. The object remains `NonMatching`.

| New retained item | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_0002a150` | 11 | 16 | 1 | `b9bd364577858b88535fa3f98c80f483828f2f1dacaee4666d0752ed07fecadf` |
| `_global_updating_actor_index` | 4 data bytes | 4 data bytes | 0 | raw payload `ff ff ff ff` |

The new function and all 23 inherited accepted functions pass the hardened
COFF comparator. Together they account for 1,459 meaningful / 1,648 padded
code bytes and 92 relocations. The function's only relocation is an i386
`DIR32` at `+0x02`, with zero addend, to the new external data owner.

## January object, ABI, and compiler evidence

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/ai/actors.obj`: 56,916 bytes, SHA-256
  `632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.
- Compiler `xbox/bin/vc7/CL.Exe`: 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural build uses the unchanged `/O2 /Oy- /DDEBUG /Dxbox` contract.

January defines `_code_0002a150` as an external function symbol, storage class
2 and type `0x20`, in one ordinary selection-1 `.text` COMDAT. Its complete
bytes are:

```text
c7 05 00 00 00 00 ff ff ff ff c3 90 90 90 90 90
```

The four zero bytes at `+0x02` are the relocation slot for
`_global_updating_actor_index`. The section contains 11 meaningful bytes,
five terminal NOPs, one relocation, and the normalized hash in the result
table.

January defines `_global_updating_actor_index` as external storage class 2,
type 0, at offset zero in the sole four-byte `.data` section. That section has
raw payload `ff ff ff ff`, zero relocations, and characteristics `0xC0300040`:
initialized data, four-byte alignment, readable, and writable. The first-shot
object reproduces the payload, owner, storage class, type, offset, relocation
count, and section characteristics exactly. Object-local section numbers
differ naturally: January uses function/data sections 34/137, while the
partial rebuilt object uses 32/3.

January's adjacent `_code_0002a120`, `_actor_delete`, and
`_code_0002d290` also reference this sentinel. The reset leaf has no caller in
the current split-object graph, and no other split object owns either new
symbol. The PDB address-name is therefore preserved without inventing a
semantic public name or header declaration.

## Retained typed source

The pre-wave `source/ai/actors.c` blob is
`e08b43bafa362cc567101cfef4fe6a62ff21a0b1`. The frozen retained blob is
`598cbbad64526c85329bd759deaa9c7b20e2f151`. Its worktree form is 17,836
CRLF bytes with 693 CRLF endings, zero bare-LF endings, and SHA-256
`543d51391995ff12e5b613dd413479c4e51a0a95b6c2f9c1a3b21b1350be1bf5`.

The complete retained source packet is:

```c
long global_updating_actor_index = NONE;

void code_0002a150(
	void)
{
	global_updating_actor_index = NONE;

	return;
}
```

The global follows the existing actor data-array globals. The function follows
`actor_change_encounter`, which preserves January's relative source order
around the missing anonymous `0x2A120` and `0x2A160` bodies. Neither symbol is
`static`; no prototype or `actors.h` edit is introduced. The ordinary typed
assignment naturally emits January's external owner, four-byte initialized
storage, instruction form, padding, and relocation.

## Claude documentation and tooling audit

All available Claude documentation relevant to this object, prototype/ABI
inference, compiler matching, knowledge-base boundaries, snapshot discipline,
and verification tooling was reviewed before the source freeze. The direct
Claude documentation repository is at clean production commit
`0e1fc785b7f5fb8c8d6fec8cae667005aa663957`; its only dirty tracked file is
the reviewed research cache `tools/equivalence/leaf_cache.json`. No dirty
production source was consumed.

Reviewed documentation identities are:

- `AGENTS.md`: 44,094 bytes, SHA-256
  `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`;
- `CLAUDE.md`: 47,280 bytes, SHA-256
  `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
- `docs/references/prototype-inference.md`: 610 bytes, SHA-256
  `ebd2a3e72206a2b727a70adf269fa5932f1392ced43a303167b969e6042be5b2`;
- `docs/references/abi-and-calling-conventions.md`: 3,680 bytes, SHA-256
  `8b9813456d0dd65275e8a9a25da2408273a2e89a7a93e5f3a6ad165ac48f138e`;
- `docs/references/kb-update-policy.md`: 1,921 bytes, SHA-256
  `4c9e2a4ae5b521f14ea45c6debe01c73e88258b35f6b51f50e53354528e0ea42`;
- `docs/ghidra-live-delinker.md`: 4,610 bytes, SHA-256
  `87069190158c3f6a05bf542aa5626b488539d8890df8f508b6d5d537ed6ddbae`;
- `docs/verification_policy.md`: 5,666 bytes, SHA-256
  `1a394e10a67b29926e6d82aa1303327e336053413f55a1b2ff545d8bebdf5922`;
- `docs/verification_explained.md`: 8,809 bytes, SHA-256
  `627702bd7b3033b3f671bc1137ba8b88a933c2d9a570972766100461213bcafe`;
- `docs/snapshot-verification.md`: 6,879 bytes, SHA-256
  `26abdd4ce21500c340948def8f1aa4485e41a5ec07437ff82328c42f514505e8`.

The associated machine-readable/tooling evidence was also reviewed:

- `tools/audit/check_arg_counts.py`: SHA-256
  `3e5842ca9b8f7c16142a0b35fa4417a49405d51ff422ce879ea38dea22c8ef32`;
- `tools/verify/vc71_scores.json`: SHA-256
  `93e71b2f0907266c259b4e9fdf823136d0e8d27409fbc52d8f1f37f68457e35c`;
- working `tools/equivalence/leaf_cache.json`: SHA-256
  `e6ccf76b86864210331042cd1cd541b3de3319125b02d0d4483eb93b0eacaf6a`;
- `kb.json`: SHA-256
  `6c5f1d3dc73686770b7888d656042cbbf43a9e3fa0289d4cc637e8cbf759ba52`;
- `kb_meta.json`: SHA-256
  `cda268fbb8fda91b71d3dbe83a7c08b2aeafeb68a3ba3ac3dc83f205e0bedfcc`.

Two actor-specific Claude commits supply semantic corroboration only:

- `c16cf9d1008d4e963783d85be95b22578aaff479`, actor source blob
  `0cc3402d9393a5bfeedf0fa1500520afe616d3c3`, documents a later-build actor
  activation routine setting raw `DAT_002c8728` to the active actor handle and
  restoring `0xFFFFFFFF` on every normal and early exit. It explicitly infers
  the datum as the currently activating actor.
- `ea6fbc266952a4423a8b009db5abed822c96913c`, actor source blob
  `74daeec154cabcc9e28b23c547b1bbc800daca79`, documents the later-build actor
  deletion assertion that its actor handle differs from the current updating
  actor.

Those commits authenticate the sentinel's activation/reset and deletion-guard
meaning. Their raw addresses, later-build layouts, compiler, and anonymous
function bodies were not transplanted. January's PDB, split object, current
typed declarations, and pinned XDK remain authoritative.

Current campaign method documents were pinned as well:

- `docs/matching_methodology.md`: SHA-256
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`;
- `docs/exact_match_acceleration_playbook.md`: SHA-256
  `daea76d57fa4e9bf746d4d284d05843fdfcc458f95836d7f05a72fb8d0fa136c`;
- `docs/shape_transfer_executor_strategy.md`: SHA-256
  `072ee7b2b3a2900e401bec7f69bff475f57639456d9c6270d2ea51189aaefa83`.

The reviewed policies require binary-first ABI proof, readable C89-compatible
source, strict relocation identity, fail-closed snapshots, and no knowledge-
base or configuration mutation to manufacture matching credit. This wave
follows those boundaries.

## Independent corroboration and no-attempt census

The clean cross-build bridge at
`bf520b6938d256ecc9a55d93e994a9f5dd389ec1` classifies
`_code_0002a150` as `january_only`, reason `anonymous-symbol`, January size 11,
and `implemented_nonexact=false`. Its correspondence JSON has SHA-256
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`.
The clean HCEA repository at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains no direct symbol or
standalone analogue; that absence is treated as a boundary, not filled by a
guessed donor.

The ignored 431,129-byte January shape atlas has SHA-256
`99d1bf539a05a6d9af923f2808f4a4b8f158b3375e5780860822753fa06d2380`.
Five current exact donors independently prove the identical VC7 code shape:
`_damage_initialize_for_new_map`, `_draw_string_dispose_from_old_map`,
`_player_ui_end_editing_profile`, `_code_000d0800`, and
`_reset_last_player1_profile_index`. Each is an ordinary no-argument void
assignment of `NONE` to a four-byte external-storage lvalue at offset zero,
followed by a terminal return. The atlas is scheduling evidence only; it grants
no matching credit.

An all-ref `-S`/`-G` history search, actor-ledger census, split-object owner
search, and registered-worktree census found no previous definition or
production attempt for either retained symbol. The baseline object contains
neither symbol. An independent read-only audit repeated the target, Claude,
cross-build, HCEA, atlas, and no-attempt checks and returned GO before source
mutation.

## Frozen one-shot discipline

The isolated worktree was created at the exact cumulative base. During
untouched setup, an initial regression snapshot using the default Ninja path
failed before validation, and the explicit-Ninja retry exposed stale copied
Ninja timestamps by beginning an unrelated baseline graph. It stopped at a
network-denied tool download. This happened before source mutation and emitted
no candidate code. After `ninja -t restat`, `all_source` reported no work and
the tracked tree remained clean.

The untouched isolated `actors.obj` was then produced once for a stable local
baseline: 8,307 bytes, SHA-256
`46eb47b90aabddb28241c79088a84c56683db25e8ce60f575e2e1346f4eed2a0`.
All 23 inherited functions passed direct strict comparison. The final clean
pre-wave regression manifest is 1,344,843 bytes, SHA-256
`9c346865d465606245ae1172609bcac9fd1a97c2abd3f2b771fe276506377c38`,
and records base commit `c3f20ecb`.

After the two source additions were frozen, `git diff --check` passed. The
exact production-flag XDK `/Zs` parse-only invocation succeeded and left the
baseline object byte-identical. The resolved live object path was proved
inside the isolated worktree, the literal generated artifact was removed,
absence was verified, and the dry run showed exactly:

```text
[1/1] CL build\base\source\ai\actors.obj
```

That normal edge was invoked once. The immutable first-shot object is 8,671
bytes, SHA-256
`6dc70ce36d9dcbe293fd5db5103f9c67fc72e9cc4ed55a0609da3469f6eb9840`.
There was no declaration, spelling, control-flow, storage, source-shape, flag,
or compiler-tuning retry.

## Strict function, data, and regression boundary

The hardened comparator reports `all_equal: true` for `_code_0002a150` and
all 23 inherited functions:

`_actors_initialize`, `_actors_dispose`, `_actors_initialize_for_new_map`,
`_actors_dispose_from_old_map`, `_actor_is_noncombat`, `_actor_in_combat`,
`_actor_is_fighting`, `_actor_is_leaping`, `_actor_get_weapon`,
`_actor_has_ranged_weapon`, `_actor_target_unit_index`,
`_actor_derive_target_information`, `_actor_flush_position_indices`,
`_actor_has_unlimited_grenades`, `_actor_change_encounter`,
`_actor_unit_control_crouch`, `_actor_unit_control_jump`,
`_actor_unit_control_primary_trigger`, `_actor_unit_control_secondary_trigger`,
`_actor_unit_control_throw_grenade`, `_actor_unit_control_exact_facing`,
`_actor_unit_control_stop_animation_impulse`, and `_actor_delete_props`.

The generic pre-wave regression check reports exactly `_code_0002a150` under
`newly_exact` and `changed_nonexact: []`. It deliberately fails closed on the
23 inherited accepted fingerprints, non-code sections, debug records, and
symbol inventory because inserting a new early `.data` owner and `.text`
COMDAT renumbers object-local sections. No adjudication or waiver is added.

Direct acceptance-view fingerprinting removes only numeric section identifiers
and proves every inherited runtime non-code section unchanged: the existing
`.bss` owner and all seven `.rdata` owners have no vanished or changed entry.
The sole appeared runtime section is
`.data|owners=_global_updating_actor_index`. Direct external-symbol census has
no removal and adds exactly:

- `_code_0002a150`, value 0, type `0x20`, storage 2, `.text`;
- `_global_updating_actor_index`, value 0, type 0, storage 2, `.data`.

The target and candidate `.data` sections both contain exactly four `FF` bytes,
have zero relocations, and use characteristics `0xC0300040`. This closes the
intended function, relocation, data-owner, inherited-code, and inherited-data
boundary without relaxing the generic gate.

## Full validation and campaign progress

The complete Halo and libcmt object graphs pass and then report no work.
Canonical outputs are:

- `build/report.json`: 1,574,813 bytes, SHA-256
  `390e72f604cbc59a1b7eda692b631ca138df82ce9b7a5877b7b8b63e0c659db0`;
- `build/semantic_report.json`: 2,985,700 bytes, SHA-256
  `6620895737aa7d207c1a03de53a05ab682b1c307051e098efc3f78de98d58985`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,158/11,060 exact functions,
  502,621/2,198,102 meaningful code bytes, and
  1,835,212/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,991/7,574 exact functions,
  489,707/1,770,166 meaningful code bytes, and
  1,830,020/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,265 functions:
4,123 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,464 missing,
4,184 accepted exact, and zero unit errors.

Admission remains zero candidates and zero revocations, with only the
inherited unrelated `source/shell/shell_xbox` completion-label contradiction.
Its 384-byte JSON has SHA-256
`f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`.
Parks remain three active, zero stale, and zero invalid; the 3,950-byte JSON
has SHA-256
`cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.
All 179 tooling tests pass, and all 14 tracked/configuration/canonical-report
JSON documents parse. `all_source`, `halobetacache_build`, and `libcmt_build`
dry runs report no pending work.

## Scope and policy

The tracked implementation scope is exactly `source/ai/actors.c`; this new
Jonas-owned ledger is the only documentation addition. No header,
configuration, semantic exception, parked record, completion label,
pre-existing Markdown, Claude-owned file, or build rule changes. There are no
deletions.

The complete `source/units/**` tree and all protected paths have zero
intersection with the wave: `source/units/units.c`, `source/units/units.h`,
`source/game/game_engine.c`, `source/game/game_engine.h`,
`source/units/vehicles.c`, `source/ai/ai_debug.c`, `source/ai/ai_script.c`, and
`source/ai/actions.c` are untouched.

The retained source is readable typed C89-style code. It contains no assembly,
raw address or byte-offset access, pointer/integer reconstruction, pointer or
union pun, inactive-union access, undefined signed overflow, volatile or
`register` scheduling device, force-inline annotation, optimizer pragma or
barrier, synthetic anchor, object-byte forcing, or compiler-control change.
No push, amend, rebase, history rewrite, or worktree removal is performed.
