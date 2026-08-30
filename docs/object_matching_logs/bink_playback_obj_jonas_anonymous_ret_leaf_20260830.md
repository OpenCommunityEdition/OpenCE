# `bink_playback.obj` Jonas anonymous return-leaf follow-up

## Result

This bounded source-only follow-up starts from authoritative integration commit
`6ca42f4e7cf8fffe9adcb6118f3254fe36135846`. It adds the one anonymous Bink
playback helper selected by the inlined-helper census. The sole natural C89
candidate is independently strict, and all eight previously accepted Bink
owners remain independently strict. `bink_playback.obj` advances from 8/24 to
9/24 exact functions, from 349/3,803 to 350/3,803 meaningful code bytes, and
from 400/3,968 to 416/3,968 exact padded code bytes. It remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_001b5850` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

The target and candidate both contain one `ret` byte followed by fifteen
padding bytes. The hardened COFF comparator proves identical padded bytes,
section flags, and empty relocation lists. The target and candidate symbols
are both external function symbols (storage class 2, type 32), so the retained
source does not invent a private or SDK-wrapper identity.

## Provenance, census, and signatures

- Baseline `source/bink/bink_playback.c` blob / payload SHA-256:
  `8b4d2948423243f98ce2a0a838a96276661d59b5` /
  `ebe1a20409dfb982008baed5fcc6a2508f66985d9ea47a35cc60030424a7585a`.
- Retained source blob / working payload SHA-256:
  `49cab467abc784d99393527ead11737c13700020` /
  `92eb7bfc8d73ea490e7bc0fc41ddbd7a7178378f8c2ccbe61a588432017db071`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `6066221c0dd9bde77e2163643a063c7ff1ecd7d4225fea496d99d3d2274bf153`.
- Compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  It is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`, using the
  repository's unchanged XDK 3911 command and flags.

The registered-worktree census covered 1,565 worktrees and found five unique
`bink_playback.c` payloads. None defined `code_001b5850`. The wider artifact
census found 1,834 `bink_playback.obj` paths. Its 886 objects that exposed the
helper were target/split derivatives with the same strict target packet; no
base artifact supplied a pre-existing production candidate. The complete Git
history for this path contains only three historical source blobs, none with
the helper definition. There was no parked record or earlier helper ledger.

The preserved public, Claude, and Fable donor trees and artifacts were also
checked. Stian and Pastudan preserve the corresponding 16-byte gap before the
Bink render-frame region but provide no body or name. HCEA exposes only the
public playback stubs. The available Halo restoration, `xboxrecomp`, Baboon,
demon, nimbus, halocea, OpenSauce, and `punpckhdq` materials do not authenticate
an identity for this helper. The authentic January PDB likewise has no public
symbol at the helper RVA. XDK `D3D8.h` contains several no-op inline wrappers,
but no address/name evidence associates any of them with this owner. Therefore
the source retains the repository's address-derived external name and the
target-authenticated signature:

```c
void code_001b5850(
	void);
```

## One-shot record and whole-TU ownership

The one and only production candidate spelling was:

```c
void code_001b5850(
	void)
{
	return;
}
```

No alternate name, declaration, return spelling, control-flow spelling, SDK
alias, or compiler-control experiment was compiled. The candidate is ordinary
typed C89 with the house-style explicit terminal `return;`. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, pointer or union pun, inactive-union access, undefined
behavior, volatile or register scheduling device, pragma, intrinsic,
force-inline attribute, optimizer barrier, synthetic anchor, or byte forcing.

A detached clean baseline snapshot was compared with the candidate whole
translation unit. There are no changed non-exact owners. The only newly exact
owner is `_code_001b5850`; all eight inherited exact owners remain exact:
`_bink_playback_active`, `_bink_playback_ui_rendering_inhibited`,
`_bink_playback_in_progress`, `_bink_playback_initialize`,
`_bink_playback_stop`, `_bink_playback_dispose`, `_bink_playback_render`, and
`_bink_playback_update`. The manifest reports only the expected `.debug$F`,
`.debug$S`, and symbol-set growth caused by adding this target-owned external
function. There is no inherited code, non-debug data, or relocation loss.

Do not repeat this leaf without new authenticated identity evidence. In
particular, do not rename it to a guessed BeginScene/EndScene or other D3D/XDK
wrapper, add a call from an accepted owner, reorder accepted Bink definitions,
or try compiler controls. The remaining fifteen Bink owners were not edited or
claimed in this wave. The Observer fallback was not opened because the primary
target succeeded strictly.

## Validation

Only `source/bink/bink_playback.c` and this Jonas-owned ledger change. No
shared header, storage definition, data section, configuration, semantic
exception, parked record, pre-existing Markdown, Claude-owned file, or
Observer owner is edited.

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass.
- Direct hardened comparison: the new helper and all eight inherited exact
  Bink owners pass; the helper has zero relocations.
- Semantic audit: 470 units, 4,955 functions evaluated, 4,838 semantic exact,
  4,848 accepted exact, 131 hidden exact / 78,940 hidden bytes, and zero unit
  errors.
- Campaign progress: 384/833 complete objects, 4,809/11,060 exact functions,
  610,371/2,198,102 meaningful code bytes, and 1,856,050/4,176,062 matched
  data bytes. The Halo subset is 282/468 objects, 4,642/7,574 functions,
  597,457/1,770,166 meaningful code bytes, and 1,850,858/3,923,451 data bytes.
- Admission audit: zero candidates, zero contradictions, and zero revocations.
- Parked validation: twelve active, zero stale, and zero invalid entries.
- Protected `Units` sentinel: target/base 1,920 padded bytes, 87 relocations,
  normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
  all fields equal.
- Complete tooling suite: 212/212 tests pass; `git diff --check` passes.

A clean committed-state deletion and forced rebuild of only the generated
candidate `bink_playback.obj` reproduced the same nine strict Bink owners.
The protected Units sentinel also remained strict in that final same-path
check. Raw whole-object hashes are intentionally not used as acceptance
identities because the expected CodeView/debug metadata changes across clean
compiler invocations; the normalized owner packet above is stable and
authoritative. No push is performed.
