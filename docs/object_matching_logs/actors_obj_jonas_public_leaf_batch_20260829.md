# `actors.obj` public-leaf first-shot batch

## Result

This bounded batch began from cumulative commit `e2276a68` and tested three
history-unattempted public actor leaves using one natural typed C89 source
spelling per function. `_actor_braindead` is strict exact and is retained.
`_actor_attacking_target` and `_actor_get_running_blind_vector` are strict
misses; their source and temporary support were inverse-reverted without
tuning. `source/ai/actors.obj` advances from 24/76 to 25/76 strict functions
and gains 96 padded code bytes. The object remains `NonMatching`.

| Function | Result | Target meaningful / padded | Candidate meaningful / padded | Relocations | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- | --- |
| `_actor_braindead` | exact, retained | 83 / 96 | 83 / 96 | 5 / 5 | `103fe26b5028ff7b9b6e5703467d68db5fe17e31bf53f373c132a5f9c1a5c5dc` | same |
| `_actor_attacking_target` | miss, frozen | 175 / 176 | 183 / 192 | 7 / 7 | `fe4a0a53ab6cdd1b74f6b1bd2a80cf30a3adee989c61f234b8b5bb54435c5b4f` | `8ee76b9420bf6c2a20e57cf0538a0623683aa634b6ccd2cd79761b8d5a9141e9` |
| `_actor_get_running_blind_vector` | miss, frozen | 203 / 208 | 196 / 208 | 8 / 8 | `540f25ae00455afc875d616faa5b09c84b0f320afd05e8ec1595f96d890766bb` | `c461f2e59eb170a8f186bde908eb7c75f1a3d8a8552a799ac3c52760292576e6` |

The immutable combined first-shot artifact is
`build/audit/actors_public_leaf_batch_first_shot_20260829.obj`, SHA-256
`a07ec21025aead362d92f13bb4dabe3f689b2b21d06da33e0ab5ec908d981e88`.
The retained rebuilt `actors.obj` is 9,016 bytes with phase-specific SHA-256
`a228d1330e64a97b1239b624ac69bcc357786fc93a68c6a219310e46e8b2f176`.
The January split object is 56,916 bytes, SHA-256
`632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.

## Retained exact source and ABI proof

The pre-wave `source/ai/actors.c` blob is
`598cbbad64526c85329bd759deaa9c7b20e2f151`; the retained blob is
`0fa2c2b6b482ae7abb0c4e609c8cf826c0a67afd`. The retained implementation is
the ordinary typed actor lookup followed by the two mode branches:

```c
void actor_braindead(
	long actor_index,
	boolean braindead)
{
	struct actor_datum *actor = actor_get(actor_index);

	if (braindead)
	{
		actor->state.mode = _actor_mode_braindead;
		actor->state.action = _actor_action_none;
		actor_delete_props(actor_index);
		code_00029e70(actor_index);
		actor_set_dormant(actor_index, FALSE);
	}
	else if (actor->state.mode == _actor_mode_braindead)
	{
		actor->state.mode = _actor_mode_alert;
	}

	return;
}
```

The strict comparator reports identical 96-byte padded COMDATs. Both have
five ordered relocations: `_actor_data` at `+0x04`, `_datum_get` at `+0x0F`,
`_actor_delete_props` at `+0x2B`, `_code_00029e70` at `+0x31`, and
`_actor_set_dormant` at `+0x39`. Types, addends, destinations, meaningful
bytes, terminal padding, and normalized hash all agree. The only supporting
source retained is the two named mode constants and ordinary prototypes for
the already-existing external callees.

## Frozen miss boundaries

For `_actor_attacking_target`, all seven relocation identities and addresses
agree. January forms addresses for the two vectors by adding the field offset
to a preserved actor base before copying through that adjusted pointer. The
natural donor-concordant typed expression instead emits direct base-plus-
offset loads. That source-level scheduling difference adds eight meaningful
bytes, crosses the next padding bucket, and leaves 95 normalized byte
differences. No cast, alias, pointer-shape rewrite, or scheduling device was
attempted.

For `_actor_get_running_blind_vector`, the ordered eight relocation
destinations agree, but candidate relocation addresses are earlier:
January uses `+0x08,+0x12,+0x2C,+0x31,+0x36,+0x3D,+0xA9,+0xAF`; the candidate
uses `+0x08,+0x11,+0x29,+0x2E,+0x33,+0x3A,+0xA4,+0xAA`. January preserves the
`have_vector` state in `BL` and therefore saves `EBX`; the natural typed
boolean spelling chooses a different allocation and schedule. The candidate
has 167 normalized byte differences. No declaration tuning, register forcing,
or alternate control-flow spelling was attempted.

Both misses are frozen until genuinely new January-PC source or compiler
provenance explains the code-generation boundary. They must not be retried by
source-shape sweep.

## Documentation, donor, and tooling evidence

Before candidate selection the complete object-specific ledgers were read:

- `actors_obj_codex_checkpoint.md`;
- `actors_obj_jonas_maintenance_pair_20260821.md`;
- `actors_obj_jonas_reset_leaf_first_shot_20260826.md`.

The campaign `AGENTS.md`, `CLAUDE.md`, matching methodology, exact-match
acceleration playbook, verification policy, snapshot policy, prototype and ABI
references, knowledge-base policy, object tooling documentation, and the
current actor source/header/type declarations were also applied. Their
constraints make the January split object and pinned VC7 build authoritative,
require complete padded bytes plus ordered relocations, and prohibit compiler
control, byte forcing, raw offsets, address transplants, aliasing tricks, and
knowledge-base exceptions.

Independent readable semantic evidence came from the clean HCEA donor at
commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`. The relevant donor blobs are
`ed422283acebfb562e5118c7f18dcb72770799c3` for `actor_braindead`,
`5a7d6cff071eeccde89d49b636e23e37e32c530b` for
`actor_attacking_target`, and
`8d92b068e71d3c8438cb365fce7d3d7e5b9f4664` for
`actor_get_running_blind_vector`. Complete January disassembly, symbol,
relocation, and caller/callee evidence was checked for each function before
mutation. The history and ledger census found no earlier candidate for any of
the three.

The three natural bodies were applied together and the ordinary production
`actors.obj` edge compiled exactly once. There was no flag, compiler,
declaration, source-shape, or control-flow retry. The two misses and their
temporary declarations were then inverse-reverted; only the strict exact body
and minimal named support remain.

## Acceptance and scope boundary

`python -m tools.campaign.gate source/ai/actors --all` reports 25 exact, zero
residual, and 51 unwritten functions. Direct hardened comparison passes the
new function and all 24 inherited accepted actor functions. The implementation
scope is only `source/ai/actors.c`; this ledger is the sole documentation
addition for the batch.

The retained source is readable typed C89-style code. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, compiler attribute, optimizer
barrier, raw address/offset access, pointer/integer reconstruction, union or
pointer pun, undefined behavior, synthetic anchor, or object-byte forcing.
Units, Vehicles, Matrix Math, AI Debug, campaign configuration, semantic
exceptions, parked records, completion labels, and build rules are untouched.

Full cumulative build, semantic, progress, admission, park, protected-Units,
and test results are recorded after the retained implementation-and-ledger
commit so the authoritative hashes and counts refer to committed state.

## Committed cumulative validation

The implementation-and-ledger commit is
`fae120929afbaeab5f854b47f278a9acbf7e1b46`. From that committed source,
the complete `halobetacache_build`, `libcmt_build`, report, semantic-report,
and progress graph passes. The generated reports are:

- `build/report.json`: 1,599,033 bytes, SHA-256
  `5e3fb65e727801f3d29a03602a9c997038a466894c986af418642c8fbd7c8bfe`;
- `build/semantic_report.json`: 3,267,073 bytes, SHA-256
  `2e71fed15f6b1b42f00a5c28ee836613adc38f3fbd0e3cf6add8514e53589739`.

The strict semantic audit scans 470 units and evaluates 4,918 functions:
4,801 semantic exact, 131 hidden exact / 78,940 hidden code bytes, one
ordinary-only, one structural, zero rejected, 12 local skips, 2,811 missing,
4,811 accepted exact, and zero unit errors.

Campaign progress is 384/833 linked objects, 4,772/11,060 functions, 606,270
of 2,198,102 code bytes, and 1,856,050 of 4,176,062 data bytes. This is a
gain of one function and 83 meaningful / 96 padded code bytes over the
pre-wave cumulative board. Admission is zero candidates, zero contradictions,
and zero revocations. Parks are 12 active, zero stale, and zero invalid.

All 212 tooling tests pass. The protected Units sentinel
`_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
The tracked worktree is clean apart from the pre-existing ignored/untracked
`scratch/` research directory.
