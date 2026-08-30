# `rasterizer_xbox.obj` Windows assert-pair first-shot boundary

## Result

The sole natural production candidate for
`__rasterizer_windows_begin` and `__rasterizer_windows_end` is rejected and
fully inverse-restored. Both January functions are 42 meaningful / 48 padded
bytes with five ordered relocations. The candidate emitted 25 meaningful / 32
padded bytes with three relocations because this translation unit did not own
the cseries `match_assert` macro: VC7 emitted an ordinary `_match_assert` call
instead of January's inline `display_assert` / `system_exit` arm.

| Function | Target padded / relocs / SHA-256 | Candidate padded / relocs / SHA-256 |
| --- | --- | --- |
| `__rasterizer_windows_begin` | `48 / 5 / 8ace56a97635564f33b7aca70f47b3e1296869f968a35eaa087e39abc8e35fe6` | `32 / 3 / f7d73f5a443291e94473f739167409221e271566bd4b69376cc54c4621e657ba` |
| `__rasterizer_windows_end` | `48 / 5 / 332b3172c18cb19d89a10f413969aec641dabb6fab3010de440f28b362cc0014` | `32 / 3 / 7aafdd8d12447a6971fef3383226e3842bd0da0fa981ab5885f63a68ba0e306f` |

The January split-object SHA-256 is
`039e8296db80d1391e59f6d8ce59fefc49618513926edbbe4e9621d8b7202182`.
The frozen candidate object's SHA-256 is
`e3a2cbb140de05072658c2c15347ce17443c6ef2c618cd1859745b1194448425`
and it is preserved locally as
`build/audit/rasterizer_xbox_windows_pair_first_candidate.obj`. The untouched
baseline object SHA-256 is
`1548e795aab1c5815756e2c69fa3fee37492965d7902fcc8252e79927d68767d`.
Raw object hashes include debug metadata; the normalized packets and resolved
relocations are authoritative.

## Evidence and provenance

The lane began from canonical commit
`65ab75f046bd13ec52a51538e61653398798c881`. All current
`rasterizer_xbox.obj` ledgers, the campaign/methodology/tooling documents,
applicable Claude/AGENTS guidance, current source and header topology,
repository history, January disassembly, HCEA, Pastudan, and Stian donors were
reviewed before mutation. Current ledgers and ref-reachable history contain no
prior production compile for this exact pair.

January, Pastudan, and Stian independently agree on the complete semantics:
each routine asserts the global D3D device pointer with the same source path,
uses line `0x533` for begin or `0x65D` for end, and otherwise returns. HCEA's
later private bodies have evolved render-window state logic and were not used
as January byte authority; its public/private wrapper topology remains useful
ABI corroboration.

The target's five relocations for each function are:

1. DIR32 at `+1` to `_global_d3d_device`;
2. DIR32 at `+17` to the target source-path literal;
3. DIR32 at `+22` to the target `global_d3d_device` literal;
4. REL32 at `+27` to `_display_assert`; and
5. REL32 at `+34` to `_system_exit`.

The target begins `mov eax, [_global_d3d_device]; test eax, eax; jne` and
ends with `ret`. Both definitions are therefore authenticated `void` routines,
not the later HCEA boolean worker.

## Frozen candidate

The one readable C89 candidate placed the pair in natural January order after
the two spin no-ops and before the existing vblank callback:

```c
void _rasterizer_windows_begin(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox.c",
		0x533,
		global_d3d_device);

	return;
}

void _rasterizer_windows_end(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox.c",
		0x65D,
		global_d3d_device);

	return;
}
```

The header declaration was simultaneously corrected from an object to the
donor- and January-proven pointer type:

```c
extern D3DDevice *global_d3d_device;
```

Only `rasterizer_xbox.c` and
`marketing_and_strategic_business_development.c` include this owner header;
the latter does not consume the device global. Its baseline object was
preserved for dependent-TU comparison. The pointer declaration itself is
correct evidence, but it was reverted with the rejected pair because it did
not independently recover a target owner and retaining an otherwise unused
broad declaration change is outside this lane.

The candidate's three relocations are DIR32 `+1` to the device global, DIR32
`+12` to the source path, and REL32 `+17` to `_match_assert`. This proves the
failure is header topology, not a branch or register-allocation tie. The
current unit includes `<xtl.h>` followed only by `rasterizer_xbox.h`; unlike
the exact Xbox Widgets sibling, it does not include cseries before XTL. The
prior window-fog ledger already documents that adding the typed cseries stack
after XTL collides on `LONG_MAX` and `M_PI`. Because this assignment allowed
one natural production candidate and forbade tuning or variants, no include
order or macro retry was attempted after the code-producing miss.

## Independent full-section boundary

The frozen whole-TU gate also rejects retention independently of the function
miss. Inserting the two target-order external COMDATs before the existing
vblank callback and stencil getter changes both accepted owners' full numeric
section fingerprints, three associative debug records, the shared source-path
owner, and the symbol inventory. Direct hardened comparison proves all five
inherited runtime functions remain strict exact, and `changed_nonexact`
contains only the two new nonexact candidates. There is no lawful general
external-COMDAT insertion recipe or one-off adjudication.

Do not retry the body, declaration spelling, or placement. Reopen only with a
newly authorized one-shot that begins from authenticated cseries-before-XTL
header topology and a repository-wide fail-closed ownership model that can
represent the target-order COMDAT insertion without weakening inherited
evidence.

## Restoration and gates

The two definitions and pointer declaration were inverse-applied. The final
source/header blobs are exactly their canonical inputs:
`abfec76012f3d29bde4e85d35e10c64c819edb63` and
`c41ec24a916b27759cda6d856783152e11292b2f`. The preserved baseline objects
were restored. The original regression manifest is fully green with all five
accepted functions `still_exact`, no failure or warning, and empty newly exact
and changed-nonexact sets.

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,950 functions evaluated, 4,833 semantic exact,
  4,843 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: 0 candidates, 0 contradictions, 0 revocations.
- Park validation: 12 active, 0 stale, 0 invalid.
- Tooling suite: 212/212 tests pass.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Ordinary report SHA-256:
  `1ec44dd8b0cbde37aa82f84ea6ced758ffa611add74e6fdb77535a2cd93982fe`.
- Semantic report SHA-256:
  `91463baac7970260eb520e071cf77cbadcac80eace4d8c7df2c625ab0445b4b2`.

The rejected candidate used readable typed C89, one parameter per line, and
explicit returns. It used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, raw address or offset, representation
pun, undefined behavior, byte forcing, object patch, compiler/tool exception,
alternate compiler, or alternate flags. Only this additive boundary ledger is
retained; nothing is pushed.
