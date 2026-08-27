# `progress_bar.obj` Jonas D3DX identity first shot (2026-08-27)

This ledger records one frozen, source-only production experiment for
`_D3DXMatrixIdentity`. The canonical pinned-XDK assignment chain is strict on
its first code-producing compile and is retained. `progress_bar.obj` remains
`NonMatching`; this is one function admission, not a whole-object claim.

## Authority and scope

- Cumulative base: `1855f227` (`Match recorded animation vector helpers`).
- Baseline source blob: `df09225b`.
- Baseline physical source SHA-256:
  `a295a2e0124593557dbe141028d364ab13a3b10bee7bad2bcc09d2e57f16615f`.
- January split object SHA-256:
  `3926fd05ff81cae4ec93dd9a21328559b9edb309f7e61bc3e36a1f518a421911`.
- Compiler: the pinned Microsoft VC7/XDK compiler at `xbox/bin/vc7/CL.Exe`
  through the unchanged ordinary Ninja edge.

January COFF remains the byte, extent, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is in
`source/interface/progress_bar.c`; this ledger is the only other tracked file.
No configuration, semantic allowlist, parked-function record, comparator,
compiler flag, header, or completion label is changed.

## Source and research provenance

The pinned SDK file `xbox/include/D3DX8Math.Inl` has SHA-256
`6f118d38e87da13dc1998a2b7b0e560505e41a39ed8087b1e878e6f0d710785b`.
Its `D3DXMatrixIdentity` implementation supplies the exact source topology:
twelve off-diagonal chained assignments to `0.0f`, followed by four diagonal
chained assignments to `1.0f`, then the output-pointer return. The existing
local `struct d3dx_matrix` is layout-compatible and avoids adding a shared
header surface.

The available Claude near-complete, finish-HS, untried-object, libcmt-stream,
and progress-bar/every-frame trees were screened before mutation. They contain
no prior body, candidate, retry packet, or conflicting result for this owner.
The applicable Claude methodology and instruction copies require binary-first
ABI verification, padded-byte and relocation equality, C89-readable source,
one frozen production shot, and whole-TU regression review. Those requirements
are followed here. Claude's unrelated functional-decomp, runtime, patching,
and later-XBE tooling are not used as January admission mechanisms.

## January packet

January defines one external cdecl function named `_D3DXMatrixIdentity`. Its
complete COMDAT is 64 bytes, with zero relocations. The normalized padded
SHA-256 is:

```text
4aa6805bb2566575ffd5d1210121322f1d9726c633ae21d0aa7546d4c0808d0b
```

The meaningful body loads the output pointer, zeros offsets `0x04`, `0x08`,
`0x0c`, `0x10`, `0x18`, `0x1c`, `0x20`, `0x24`, `0x2c`, `0x30`, `0x34`, and
`0x38`, then stores IEEE-754 `1.0f` at offsets `0x00`, `0x14`, `0x28`, and
`0x3c`. It returns the original pointer and ends with two NOP padding bytes.

## Retained source

```c
struct d3dx_matrix *D3DXMatrixIdentity(
	struct d3dx_matrix *output)
{
	output->elements[0][1] = output->elements[0][2] =
		output->elements[0][3] = output->elements[1][0] =
		output->elements[1][2] = output->elements[1][3] =
		output->elements[2][0] = output->elements[2][1] =
		output->elements[2][3] = output->elements[3][0] =
		output->elements[3][1] = output->elements[3][2] = 0.0f;
	output->elements[0][0] = output->elements[1][1] =
		output->elements[2][2] = output->elements[3][3] = 1.0f;

	return output;
}
```

This is readable C89 source with one parameter per line and an explicit
return. It contains no assembly, `register`, `volatile`, pragma, intrinsic,
attribute, barrier, raw address, pointer/integer reconstruction, type pun,
undefined behavior, synthetic anchor, object patch, comparison exception, or
alternate compiler control.

## Frozen experiment and strict proof

Before mutation, the fail-closed whole-TU manifest was written to
`build/audit/progress_bar_d3dx_identity_baseline_20260827.json`. The selected
production edge was clean. After the source was frozen, exactly one normal
code-producing build edge ran:

```text
[1/1] CL build\base\source\interface\progress_bar.obj
progress_bar.c
```

The first artifact was immediately preserved as
`build/audit/progress_bar_d3dx_identity_first_shot_20260827.obj`. The preserved
artifact is 6,944 bytes with SHA-256
`a6fef8efe15b4a5adb33f0f72c9c6095fcf806af8cd888c15f6a77a3f53c0bc0`.
No second candidate compile or source-shape adjustment exists.

The hardened direct comparator reports `all_equal: true`:

| Owner | Padded bytes | Relocations | Target SHA-256 | Base SHA-256 |
| --- | ---: | ---: | --- | --- |
| `_D3DXMatrixIdentity` | 64 | 0 | `4aa6805bb2566575ffd5d1210121322f1d9726c633ae21d0aa7546d4c0808d0b` | `4aa6805bb2566575ffd5d1210121322f1d9726c633ae21d0aa7546d4c0808d0b` |

The pre-wave regression check identifies precisely `_D3DXMatrixIdentity` as
`NEWLY_EXACT`, reports `changed_nonexact: []`, and retains all nine inherited
strict functions as `still_exact`. Its symbol-set, debug, read-only-data, and
later-function evidence findings are the expected COFF section-index churn
from inserting a real external COMDAT. They are reviewed rather than waived:
the new owner is directly strict, every inherited accepted strict owner stays
strict, and no admission/comparator exception is added.

## Validation and progress

The complete Halo-plus-libcmt graph and both campaign reports build. The
canonical report hashes are:

- `build/report.json`:
  `50e467428d16c37b8ebf5e3d76a3570ff15a5027babbf16de810a8449507bf6d`;
- `build/semantic_report.json`:
  `567a1b9083971ed831182f1b6c05071c65677d85fbe76151614cb0634613da86`.

The direct report records `progress_bar.obj` at 16/54 matched functions and
1,011/7,280 meaningful code bytes. The new function contributes 62 meaningful
bytes and 64 padded exact bytes. It does not complete the object and receives
no data credit.

Campaign progress after semantic review is 377/833 matched objects,
4,299/11,060 accepted functions, 516,440/2,198,102 linked code bytes, and
1,836,780/4,176,062 linked data bytes. The semantic audit reports 470 units,
4,414 functions evaluated, 4,266 semantic exact, 115 hidden exact / 64,889
hidden code bytes, 4,326 accepted exact functions, and zero unit errors.

The remaining gates pass:

- object admission: zero candidates, contradictions, and revocations;
- parked-function validation: three active, zero stale, zero invalid;
- test suite: 179 passed (only the known unwritable pytest-cache warning);
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden-construct scan of the edited translation unit: no findings.

The unit remains `NonMatching` in `config/config.json`, as required.
