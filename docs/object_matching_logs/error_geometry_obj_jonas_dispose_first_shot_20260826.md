# `error_geometry.obj` Jonas dispose first shot (2026-08-26)

## Result and fixed boundary

This bounded wave starts from clean cumulative commit
`ab860d8aefe3de47d9d4264ec1afe4ce09f92cad`. The sole frozen production
candidate was `source/tool/error_geometry.obj:_error_geometry_dispose` plus
the four-byte file-pointer storage that both of its January data relocations
name. The ordinary C body and storage owner are strict exact on the first and
only object-producing compile and are retained.

`error_geometry.obj` advances from 0/17 to 1/17 exact functions and from
0/6,455 to 29/6,455 meaningful code bytes. Objdiff also recognizes the owned
four-byte BSS, advancing the object from 0/1,528 to 4/1,528 matched data
bytes. The object remains `NonMatching`; no whole-object completion or
semantic exception is claimed.

| Retained owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_error_geometry_dispose` | `29 / 32` | 3 | `002f6d2344aa94af0039847531bace2e6179667baa3383e8634f17a707cdbb8c` |
| `_bss_004561b4` | `4 / 4` logical zero bytes | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

The approval-gated Units package and its shared/protected neighbors were not
edited. The unresolved network, S3TC, input, and rasterizer wrappers were not
compiled, and the multi-owner devices initialization closure was deferred
before source work.

## Immutable inputs and first artifact

- January split object: 24,417 bytes, SHA-256
  `3fff59eb8ccafe28e46399032a19b4083d968b2b6aa2abc78d025f75ec8ee057`.
- Baseline source blob:
  `a6442d8021f30355cc63a0975b6663eb6eccf49f`.
- Frozen candidate source blob:
  `74e919f62df07122dfed4593b452d55bc114d546`.
- Frozen candidate source payload SHA-256:
  `3e6fa5b964a134c9747968b30f865588c1404e3d60fae9a6c9b016da95b79953`.
- Preserved baseline object: 475 bytes, SHA-256
  `ce8c3b4ca7c3be1876d11a194c9339f52e036409ded4d3e126b63d97b0def2dc`.
- Preserved first candidate object:
  `build/audit/error_geometry_first_shot_20260826/candidate-first.obj`,
  887 bytes, SHA-256
  `8354df5653493b36450ede34635a7acbf0d0ff180d7e31547552ed0abdecbe57`.
- First no-build objdiff report:
  `build/audit/error_geometry_first_shot_20260826/report.json`, 1,574,133
  bytes, SHA-256
  `addfef1785f72cf37544f918b2baa642de1e7c4c4f3128a71f7cf4e9c12c862a`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 from
  XDK 3911, 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The ordinary object edge uses `/O2 /Oy- /DDEBUG /Dxbox` and the repository's
unchanged include graph. No alternate compiler, flag, or object path was
used.

## Claude, HCEA, history, and tooling evidence

The pre-emission audit treated January COFF as admission authority and all
secondary trees as provenance only. Across 27 preserved historical
`C:/halo-worktrees/claude-*/build/base/source/tool/error_geometry.obj`
instances plus the current baseline, the candidate symbol count is zero.
Reachable Git `-S` history likewise contains no prior definition in this
repository.

Claude's
`C:/halo-worktrees/claude-near-complete-objects-20260816/research/residual_frontier.json`
(SHA-256
`db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`)
classifies `_error_geometry_dispose` as target-only. Its
`unwritten_string_synopsis.json` (SHA-256
`9786012cf2daa5dda7b228ae0ecef0ee80e3e37760f21d4d9bd4e538beb5f1d1`)
has no entry for the function, consistent with its owning and referencing no
string literal. The inspected Claude `source/tool/error_geometry.c` copies
preserve only the January symbol inventory and empty section skeleton. No
Claude document supplies a source body or candidate compile.

The clean HCEA tree at
`c168af2e747d3095d9a29418ae401f3a39544863` is not a
body donor for this function. The retained form is therefore not claimed as
a source transplant: the short control flow is fully determined by January's
bytes, relocations, external `FILE *` owner, and the CRT interface.

The production shape-transfer executor is not applicable. The function is
29 meaningful bytes, above the executor's terminal-leaf cutoff, and no
authenticated donor/recipient pair exists. The manual body remains ordinary,
typed C and was admitted only by direct hardened comparison.

## Exact January packet and typed source

January requires one external type-`0x20` function owner with 29 meaningful
and 32 padded bytes. Its instructions load `_bss_004561b4`, test the pointer,
return immediately when it is null, otherwise call `_fclose` with that
pointer, store null back to the same owner, and return.

The exact ordered relocations are:

| Offset | COFF type | Destination |
| ---: | --- | --- |
| `+0x01` | `DIR32` | defined non-code `_bss_004561b4 + 0` |
| `+0x0B` | `REL32` | undefined CRT `_fclose + 0` |
| `+0x14` | `DIR32` | defined non-code `_bss_004561b4 + 0` |

The target data owner is external storage class 2 at section offset zero,
with four logical zero bytes and no relocation. The retained source is:

```c
#include "cseries.h"

FILE *bss_004561b4 = NULL;

void error_geometry_dispose(
	void)
{
	if (bss_004561b4)
	{
		fclose(bss_004561b4);
		bss_004561b4 = NULL;
	}

	return;
}
```

`cseries.h` supplies the standard `FILE`, `NULL`, and `fclose` declarations
through its normal `<stdio.h>` include. Explicit zero initialization is the
natural repository spelling for an owned zero-initialized pointer and emits
ordinary BSS; it is not an address, layout, or compiler-control device.

## First-shot compile and strict gates

The frozen source first passed the exact production-flag XDK `/Zs` syntax-only
check. The untouched baseline object remained present and was copied to the
ignored audit directory. Its resolved absolute path was proved to remain
inside the isolated worktree, exactly that one file was removed with literal
path semantics, and absence was verified.

The sole production invocation reported exactly:

```text
[1/1] CL build\base\source\tool\error_geometry.obj
error_geometry.c
```

The emitted object was copied immediately to the preserved first-candidate
path. A subsequent selected-object Ninja dry run reports `ninja: no work to
do`; there is no retry candidate.

Direct `tools.coff_compare` returns `all_equal: true` for
`_error_geometry_dispose`. It independently verifies all 32 padded bytes,
the normalized hash, and the three relocation offsets, types, destinations,
and addends.

The ownership-aware `section_info_resolved` comparison returns equality for
`_bss_004561b4`: target and candidate are each 4 bytes, zero relocations, and
normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
The natural compiler object uses a non-file-backed BSS section while csplit's
target materializes the four zero bytes; the semantic byte span, external
owner, offset, storage class, flags, alignment, and relocation inventory are
the same.

The candidate defines exactly two runtime owners: the retained function and
the four-byte BSS. Its only undefined external is `_fclose`. It introduces no
string, `.rdata`, initialized `.data`, COMMON, extra function, alias, helper,
or synthetic owner.

The frozen pre-change regression manifest reports
`_error_geometry_dispose` as `NEWLY_EXACT`, `changed_nonexact: []`, and no
inherited exact function because the baseline TU was a zero-code skeleton.
Its generic failures are the expected new BSS/function symbol inventory and
their compiler debug records. Those are reviewed additions, not regressions;
the direct function and data comparators above are the admission gates. A
clean committed-state snapshot and replay remain required before cumulative
integration.

The first no-build objdiff report independently records 100.0% for the
function, 1/17 matched functions, 29/6,455 matched code bytes, and 4/1,528
matched data bytes.

## Deferred alternatives and closure boundary

The broader audit found several tiny wrapper-shaped functions, but final-link
closure was applied before source work:

- `_network_game_client_write` would call absent
  `_network_connection_write`;
- `_EncodeBlockRGB` would call absent same-TU `_EncodeBlockRGBColorKey`;
- `_input_frame_begin` would call absent same-TU `_code_000bebd0`;
- `_rasterizer_debug_line` would call absent same-TU
  `_rasterizer_debug_line_shaded`.

Those compile-only shapes were not production candidates. Full devices-map
initialization is link-closed in principle, but the closure spans three code
owners, 298 meaningful / 320 padded bytes, 24 relocations, five string
COMDATs, and nine inherited exact sentinels. HCEA authenticates public
semantics but not January's PC-only `_code_00084f70` helper split or exact
assertion scheduling; Claude supplies no body or prior compile. It was
therefore deferred rather than guessed. `_device_groups_data` was also
confirmed as an undefined external in January `devices.obj`, not storage that
this wave may define.

## Source-policy record

The selected source uses a typed standard-library pointer, readable control
flow, the required no-argument formatting, and an explicit return. It uses no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, force-inline
control, barrier, raw address or byte offset, pointer/integer reconstruction,
cast or union pun, inactive-union access, undefined behavior, synthetic
caller or anchor, byte patch, semantic/comparator exception, or alternate
compiler flag.

Only `source/tool/error_geometry.c` and this new Jonas-owned ledger are
selected for the implementation commit. No header, configuration, semantic
ledger, parked record, completion label, protected source, Units source, or
tracked deletion is changed. No push, amend, rebase, history rewrite, or
worktree removal is performed.

## Clean committed-state replay

Implementation-and-ledger commit
`8084c8774fbd9fc30a2b2fd0f4bfdaf75b407ddc` is clean and authored by
Jonas Volman. Its committed source blob is the frozen candidate blob
`74e919f62df07122dfed4593b452d55bc114d546`; no source byte was changed
after the first production compile.

The current no-build report and semantic audit were generated directly from
the accepted object graph. A one-unit whole-TU snapshot was then captured
from that exact clean commit at
`build/audit/error_geometry_first_shot_20260826/committed-manifest.json`.
It is 679,299 bytes with SHA-256
`1b844f053686078aa617117c68839b1651b1f3a29ffddd6c872cf843e219d695`.

The resolved
`build/base/source/tool/error_geometry.obj` path was proved inside the
isolated worktree. Exactly that file was deleted with literal-path semantics,
and absence was verified. The unchanged normal edge then reported exactly:

```text
[1/1] CL build\base\source\tool\error_geometry.obj
error_geometry.c
```

The replay is preserved at
`build/audit/error_geometry_first_shot_20260826/committed-replay.obj`, 887
bytes, phase-specific raw SHA-256
`1f08d830da9e2e03b0d397cf897c74e2f09cefb8c2655916c53cc8b61003bd22`.
Its raw object hash differs from the first candidate because VC7 compiler
debug records are phase-specific. The runtime evidence does not differ.

Direct hardened comparison again returns `all_equal: true` for the complete
32-byte function section and all three relocations. The resolved data
comparison again returns exact equality for the four-byte BSS. The clean
one-unit manifest check has no failure or warning, no newly exact or changed
nonexact entry, and exactly `_error_geometry_dispose` in `still_exact`. The
selected-object Ninja dry run reports `ninja: no work to do`.

## Full validation

The complete `halobetacache_build` plus `libcmt_build` graph completed all
568 required actions with exit zero. The already-current error-geometry
replay was not recompiled during that graph validation.

The copied worktree's Ninja log made the `progress` phony target consider the
already present `csplit.exe` and `objdiff-cli.exe` download edges stale. The
attempted refresh was denied by network policy before report generation. The
authenticated local binaries were then invoked directly with the exact rule
commands: objdiff report generation, `tools.audit_semantic_matches`, and
`configure.py progress`. No download, source change, split regeneration, or
object rebuild was required.

The final canonical report is 1,574,133 bytes with SHA-256
`addfef1785f72cf37544f918b2baa642de1e7c4c4f3128a71f7cf4e9c12c862a`.
It records 1/17 error-geometry functions, 29/6,455 meaningful code bytes, and
4/1,528 data bytes. The semantic ledger independently accepts the function
from both `objdiff` and `semantic-coff` evidence.

The strict semantic audit reports 470 units, 4,248 functions evaluated,
4,106 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 4,167
accepted exact, 36 ordinary-only, 35 structurally accepted, one rejected,
and zero unit errors. Its generated report is 2,974,100 bytes with SHA-256
`f47167943caa27ae2262a8bef6f7cc2021ced5aa2b6979d2813823d93aba1e29`.

Final campaign progress is:

- all categories: 375/833 matched objects, 4,141/11,060 functions,
  502,252/2,198,102 meaningful code bytes, and 1,835,092/4,176,062 data
  bytes;
- Halo: 273/468 matched objects, 3,974/7,574 functions,
  489,338/1,770,166 meaningful code bytes, and 1,829,900/3,923,451 data
  bytes;
- libcmt: 102/212 matched objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The remaining validation gates pass:

- object-admission audit: zero candidates and zero revocations; only the
  inherited unrelated `source/shell/shell_xbox` completion-label
  contradiction remains;
- parked-function validation: three active, zero stale, zero invalid;
- complete tooling suite: 179/179 tests pass;
- all configuration, report, semantic, and parked JSON files parse;
- `git diff --check`, tracked-path scope, deleted-path, protected-source,
  Units/shared-header, storage-owner, and banned-construct reviews are clean.

This replay and validation record is additive documentation only. The source
blob remains the implementation commit's blob, no configuration or semantic
exception is added, Units remains frozen, and no push is performed.

## Corrected cumulative-HEAD closure

The authoritative `jonas/units-integration-20260824` branch was
fast-forwarded from `ab860d8aefe3de47d9d4264ec1afe4ce09f92cad` through the
two isolated commits to
`9f2c28b3e8cd9060f80d7c03f78dbd91aca11df8`. Integration changed only the
error-geometry source and this ledger. The committed source blob remains
`74e919f62df07122dfed4593b452d55bc114d546`; no Units source, protected
neighbor, shared interface, configuration, semantic ledger, or parked record
changed.

At that clean cumulative HEAD, the prior 475-byte skeleton object was proved
inside the cumulative worktree, deleted literally, and confirmed absent. The
normal selected-object edge ran once and reported exactly one CL action. The
corrected replay is preserved at
`build/audit/error_geometry_corrected_head_replay_20260826.obj`, 883 bytes,
with phase-specific raw SHA-256
`7f51c4041e550fff928bb9bc5b2e31bff05558aff0f8520fca056fd28b53fb33`.

Direct hardened comparison again proves the function's 32 padded bytes and
three relocations exact. Resolved data comparison again proves the four-byte
BSS owner exact. The cumulative report and semantic-report hashes reproduce
the isolated results exactly:

- `build/report.json`:
  `addfef1785f72cf37544f918b2baa642de1e7c4c4f3128a71f7cf4e9c12c862a`;
- `build/semantic_report.json`:
  `f47167943caa27ae2262a8bef6f7cc2021ced5aa2b6979d2813823d93aba1e29`.

The corrected clean snapshot is
`build/audit/error_geometry_corrected_head_manifest_20260826.json`, 679,299
bytes, SHA-256
`94f16ca0e76993f48d69045adafa01ce903eeac0f43df8e4cad720214512a7d2`.
Its no-build check is fully clean: `_error_geometry_dispose` is the sole
`still_exact` function, `changed_nonexact` and `newly_exact` are empty, and
there are no failures or warnings.

The cumulative semantic audit and progress reproduce the full-validation
counts above. Object admission remains zero candidates / zero revocations,
parked validation remains three active / zero stale / zero invalid, all 179
tooling tests pass, and the selected-object dry run reports no work. The
authoritative worktree is clean apart from this additive record before its
final evidence commit. No push, amend, rebase, history rewrite, or worktree
removal is performed.
