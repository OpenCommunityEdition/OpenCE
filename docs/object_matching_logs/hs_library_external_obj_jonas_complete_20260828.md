# `hs_library_external.obj` Jonas complete-object closeout (2026-08-28)

## Result

`source/hs/hs_library_external.c` is now fully exact under the strict
target-owner campaign rule.  The strict campaign gate advances from 33/36
functions and 3,312/3,824 padded bytes to 36/36 and 3,824/3,824.  The three
recovered functions contribute 479 meaningful code bytes and 512 padded
bytes:

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hs_objects_can_see_object` | `115 / 128` | `4 / 4` | `8885822a97ceb331a2eaee781196ff3b1022b3425a1ac33d3235895f03ca107b` |
| `_hs_object_destroy_all` | `185 / 192` | `11 / 11` | `d9ddb540a4fdce5ad23a34c0e5a40062fb84cf5e254ed782a8392d2ccd2603fc` |
| `_hs_object_set_permutation` | `179 / 192` | `8 / 8` | `bf3c9df8d3723226b14ec93bb5ac9b975f63afc3c0604ba388fee6ef2f389716` |

For every row the target and rebuilt fingerprints above are identical, and a
verbose `bijection_walk` reports zero events.  An independent census also
compared all 33 inherited exact function fingerprints against the preserved
baseline object and found no sibling regression.  The unit's five target-owned
runtime RDATA sections are exact as well:

| Section symbol | Bytes / relocs | Normalized SHA-256 |
| --- | ---: | --- |
| `??_C@_0BA@NMABAOHF@?$CGflag?9?$DOposition?$AA@` | `16 / 0` | `13a3311de5d988e60827b04447ab3c027d4ce47386581910c59b1df6ba4b435f` |
| `??_C@_0BO@NDHEDPHG@the?5sound?5?8?$CFs?8?5does?5not?5exist?$AA@` | `30 / 0` | `2efb79af36dd48aec3fdfc46a6d1fd2d2a642e409f0aa159628de68ee70fd30e` |
| `??_C@_0CI@GDKOMGNN@c?3?2halo?2SOURCE?2hs?2hs_library_ext@` | `40 / 0` | `23e255f777267bc6515b7af6b25b600fb21521ebaca2c8ce4f1b5e8d1bb73b60` |
| `??_C@_0CN@IIANAKMD@WARNING?3?5object_create?5?9?5?8?$CFs?8?5al@` | `45 / 0` | `f3f13cc479d757f9487025fe45da15b12c2d087b437314c305e1fb8004eff1a1` |
| `??_C@_0FP@BIAHJJOL@?$CD?$CD?$CD?5ERROR?5a?5script?5tried?5to?5dele@` | `95 / 0` | `33b9dea6ba39bf6a00d8befde9c2e79dbf39b4029f26a78d7d9746b5ca93f1c4` |

`config/config.json` index 284 therefore moves from `NonMatching` to
`Matching`.

## Frozen provenance

This wave uses isolated branch/worktree
`jonas/hs-library-external-closeout-20260828`, based on integration commit
`c9b31750e830065bd2cd43e72a1bba8f674fa64c`.  The immutable executable is
`cachebeta.exe`, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
The regenerated January split object has phase-local raw SHA-256
`c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
The final rebuilt object has phase-local raw SHA-256
`67abdf3b73deff593af87a17e04b45d5741aab61431a1fec15aa060f46952d9d`.
Whole-object digests are artifact sentinels only; strict authority is the
normalized COFF section fingerprint, including relocation address, type,
destination and addend.

The raw COFF files are intentionally not described as byte-identical.  The
candidate contains five pre-existing stock-header helper COMDATs plus pooled
compiler/debug/directive sections that are not January target owners.  Those
same helper COMDATs already exist in the preserved baseline, so this wave adds
no symbol-shape regression.  Completeness here means all target-owned code and
runtime RDATA pass the strict fingerprints above.

The pre-edit rebuilt object is preserved at the ignored audit path
`build/audit/hs_library_external_closeout_baseline_20260828.obj`, SHA-256
`9f2dfa80ba3da37f06ac2947fa19d495e94aca72be0981b7d08750026f4a954d`.
The final rebuilt object is preserved at
`build/audit/hs_library_external_complete_20260828.obj`, SHA-256
`67abdf3b73deff593af87a17e04b45d5741aab61431a1fec15aa060f46952d9d`.
Intermediate exact artifacts are retained as
`hs_library_external_set_permutation_exact_20260828.obj`, SHA-256
`20abfa2fd7dda0c0e157ae2b38124c2edae073ec55e3fce6610ddb0311e3e5d5`,
and `hs_library_external_visibility_permutation_exact_20260828.obj`, SHA-256
`90c7e762f6b55d56ad1c1412824454835ce70f802fa1d61f937b5f68a803f236`.

The compiler is XDK 3911 `CL.Exe` 13.00.9254.1, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
using the unchanged repository `/O2 /Oy- /DDEBUG /Dxbox` edge.  Pinned csplit
is SHA-256
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
and objdiff-cli is SHA-256
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

## Documentation and donor audit

Before the final recovery, all thirteen existing `hs_library_external.obj`
ledgers, `docs/matching_methodology.md`, `tools/campaign/README.md`, and both
current/Stian `CLAUDE.md` files were read in full.  The HCEA donor bodies for
all three residuals were read as behavior and topology evidence only.  HCEA,
Halo PC, Stian, Claude history, PDBs and community repositories are not byte
authority; every retained spelling below is admitted only by the January COFF
gate.

The HCEA SHIP and RELEASE PDBs agree that `hs_object_destroy_all` has two
overlaid stack locals, both named `iterator`: one `struct data_iterator` and
one `struct object_iterator`.  Their repeated line records prove three-clause
`for` loops for both traversals.  No Claude session contained a previously
compiled alternate source for this body.  The PDB result ruled out artificial
sentinel locals and focused the successful experiment on the field expression
itself.

## `_hs_objects_can_see_object`

The preserved first shots returned literal `TRUE` inside the loop and
`FALSE` after it.  Both produced the same 112-byte candidate with normalized
SHA-256
`39d372a2b4c42d3c47c6743b3f0aada102c1d75b8a3004f79aa316a18b5f60dd`;
their four relocations were uniformly two bytes early.  Adding an explicit
nonempty-list guard was compiler-equivalent and did not recover January's
early `xor bl, bl`.

The decisive legal-C lifetime is instead:

```c
result = FALSE;
unit_index = object_list_get_first(object_list_index, &reference_index);
while (unit_index != NONE)
{
	if (unit_try_and_get(unit_index) &&
		hs_unit_can_see_object(unit_index, object_index, degrees))
	{
		result = TRUE;
		break;
	}
	unit_index = object_list_get_next(object_list_index, &reference_index);
}
return result;
```

Initializing `result` before the first lookup keeps BL live across that call.
Assigning success and breaking, rather than returning a literal, preserves the
three January epilogues.  Relocations are exact at `+0x12`, `+0x27`, `+0x39`
and `+0x4a`, in order to `_object_list_get_first`,
`_object_try_and_get_and_verify_type`, `_hs_unit_can_see_object` and
`_object_list_get_next`.

## `_hs_object_destroy_all`

The reconstructed preserved first shot is 171 meaningful / 176 padded bytes,
with 11 relocations and normalized SHA-256
`8e83237461caf53fcee5b8334b4b8ea204979e9abc52e95730fdff1fe500c0f2`.
It copied `player->unit_index` to a named local before testing it, making VC7
emit `mov esi,[eax+34]; cmp esi,-1`.  Function-scope sentinels, mask locals,
split first-next/`while` forms, three-clause loops with the named local, and a
nested post-guard local all reproduced the same rejected allocation shape.

The PDB-backed direct expression is exact:

```c
if (player->unit_index != NONE &&
	object_get_ultimate_parent(player->unit_index) != player->unit_index)
{
	unit_exit_seat_end(player->unit_index);
}
```

VC7 now guards the field in ECX, preserves it in ESI only after the guard, and
naturally keeps `NONE` in EDI across both loops.  This reproduces January's
`mov ecx,[eax+34]; cmp ecx,edi; mov esi,ecx`, both alignment sleds, all 11
relocation addresses, and the 192-byte allocation.  The two nested iterator
scopes and both three-clause `for` loops remain exactly as supported by the
PDB line map.

## `_hs_object_set_permutation`

The HCEA donor established the behavioral shape: resolve an optional model,
scan its regions case-insensitively, retain `NONE` when absent, and call
`object_permute_region`.  The exact source uses `model_definition_get` and
`TAG_BLOCK_GET_ELEMENT` rather than raw tag access.  The remaining January
width boundary was the desired region index: changing
`desired_region_index` from `long` to `short`, alongside the already-short
loop index, reproduced all 192 padded bytes and eight relocations.  The added
`models/model_definitions.h` include exposes the typed model structures and
accessors without duplicating layout.

## Policy audit

All new functions use one parameter per line.  The no-argument destroy-all
body writes `void` on its own line.  Both new `void` functions have an explicit
terminal `return;`, and the boolean function has one terminal return.  Typed
object/model/tag access uses the repository accessors and typed macros.  No
assembly, volatility, `register`, pragma, intrinsic, barrier, attribute, raw
offset/address, pointer/union pun, undefined behavior, synthetic anchor,
compiler flag, object-byte patch, comparator exception, or parked credit is
used.

## Validation

- Fresh pinned csplit regeneration leaves the January target object SHA and
  every strict comparison unchanged.
- `tools/campaign/board.py source/hs/hs_library_external` reports `1/1`
  fully exact, `3824/3824 B`, `36/36 fn`.
- `tools/campaign/gate.py source/hs/hs_library_external --all` reports
  `exact 36`, `residual 0`, `unwritten 0`.
- Direct `coff_compare` over all 36 symbols has zero mismatches.  The 33
  inherited target/base fingerprints are unchanged from the preserved
  baseline.
- The generated report records 3,555/3,555 meaningful code bytes and
  232/232 runtime data bytes, with complete code/data/unit status at 100%.
- `halobetacache_build` and `libcmt_build` are both satisfied.
- Semantic audit: 4,457 strict-exact functions, 4,484 accepted exact,
  zero unit errors and zero ordinary rejections.
- Parked audit: three unrelated active entries, zero stale and zero invalid.
- Object-admission audit: zero candidates, contradictions and revocations.
- All 203 tooling tests pass.
- The only compiler diagnostic is the inherited C4047 pointer-level warning
  in the existing effect-marker call; this wave merely shifts its line number.

The tracked closeout is limited to the source file, this ledger, and the
single `NonMatching` to `Matching` configuration admission.
