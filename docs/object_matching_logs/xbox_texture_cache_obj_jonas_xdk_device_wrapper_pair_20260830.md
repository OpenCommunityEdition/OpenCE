# `xbox_texture_cache.obj`: XDK device-wrapper pair

## Result

This isolated one-shot wave starts from canonical commit
`8e581510ae4acdb4c7b9110a32c94f8fac770d65` and admits two previously
anonymous stock-XDK inline helpers.  The strict unit gate advances from 9/25
to 11/25 exact code owners and from 656/3,648 to 688/3,648 padded code bytes,
with zero residual owners.

| January owner | Evidence-backed owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_code_001ae7e0` | `_IDirect3DDevice8_IsBusy@4` | 16 | 1 | `6d46436cb945dbb1b016f222c634a74f3596ac3dee003ade6338c25801aa8edb` |
| `_code_001ae7f0` | `_IDirect3DDevice8_KickPushBuffer@4` | 16 | 1 | `6d46436cb945dbb1b016f222c634a74f3596ac3dee003ade6338c25801aa8edb` |

The IsBusy relocation is `REL32 +0x01 -> _D3DDevice_IsBusy@0`; the
KickPushBuffer relocation is `REL32 +0x01 ->
_D3DDevice_KickPushBuffer@0`.  The final source uses the ordinary XDK 3911
calls with `&global_d3d_device`.  VC7 removes the unused receiver while
inlining those calls into the already-exact flush/close bodies and naturally
retains both exact wrapper COMDATs.  No wrapper body or emission anchor is
hand-authored.

## Census and identity proof

The local census covered 2,333 `xbox_texture_cache.obj` artifacts with 1,138
distinct raw hashes.  Every pre-existing exact hit for the fixed helpers was
another copy of the immutable January split object; no compiled candidate or
source donor existed.  All reachable repository revisions of this source
were also inspected.  They contained only the earlier lifecycle,
flush/close, and public-quintet work.

The XDK 3911 definitions in `xbox/include/D3D8.h` were then compiled with the
production VC7 `/O2 /Oy- /DDEBUG /Dxbox` toolchain in an ignored research
probe.  The resulting `_IDirect3DDevice8_IsBusy@4` and
`_IDirect3DDevice8_KickPushBuffer@4` sections match the January sections in
padded bytes, normalized bytes, relocation address/type/destination/addend,
and decorated identity.  Their natural source calls also preserve the exact
`_texture_cache_flush` and inlined `_texture_cache_close` packets.

The evidence review included every prior Xbox texture-cache ledger, the
campaign and matching doctrine, relevant Claude tooling documentation and
`docs/bugs/texture-cache-corruption.md`, and the authenticated HCEA,
Stian, and Pastudan texture-cache donors.  Those donors corroborate the GPU
busy/flush topology but do not override January bytes or layouts.

## Resource-helper boundary and one-shot rejection

The first and only production candidate also used a typed 0x20-byte texture
entry and the natural XDK BaseTexture busy call in the real
`code_001ae840` locked-block callback.  That compile produced all three fixed
16-byte helpers strictly exact, including the January `_code_001ae800` shape
(`push eax; call _D3DResource_IsBusy@4; ret`, normalized SHA-256
`43d508dca4211f96527b769632b4c2daf03f76d0dd594c76496b6c2c32c5c27a`).

The callback itself rejected: candidate and target were both 64 padded bytes
with the same three relocation identities and addresses, but normalized
SHA-256 was
`ef0debbd170c841a4c89b83eb0363369278f09c33897019f50199a9b888efbf4`
instead of target
`9f883aeaa9d5a930f38b79b2960d634b647d971b5fcd92675174941d340b1f1d`.
The source-level `boolean` contract emitted byte returns (`xor al,al` and
`mov al,1`), whereas January returns the IsBusy zero directly and uses
`mov eax,1`.  This is direct evidence that the current callback declaration
does not preserve January's full-width return contract, but the one-shot rule
forbids a return-type/body retry.  The callback definition, typed entry, and
resource-wrapper alias were therefore pruned immediately.  January
`_code_001ae800` is restored to its anonymous name and remains unwritten.
There was no second candidate spelling, synthetic caller, forced emission, or
fallback-object attempt.

## Whole-TU ownership

All nine inherited exact code owners remain strict exact after the prune.
The rebuilt object has no candidate-only code owner: its complete defined
code set is exactly those nine inherited owners plus the two admitted XDK
wrappers.  All nine common runtime literal COMDATs compare strictly equal to
January.

Including the official XDK header also instantiates its three known
`DECLSPEC_SELECTANY` lookup tables:

| Discardable compiler support | Bytes |
| --- | ---: |
| `_D3DTEXTUREDIRECTENCODE` | 16 |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 |

They are unreferenced discardable header support, receive no target ownership
or matched-data credit, and are the only candidate-only runtime-named
sections.  This is the same audited boundary used by the completed Detail
Objects XDK-wrapper closure.  There is no candidate-owned `.bss` or writable
`.data`, and every January-only runtime owner remains unclaimed.

The retained production source is readable typed C.  It adds no assembly,
`volatile`, `register`, user-written pragma, intrinsic, attribute,
force-inline annotation, optimizer barrier, raw address/offset access,
representation pun, undefined behavior, byte forcing, or dummy anchor.

## Reproducibility and validation

- Final source Git blob: `8836e5efb424acb7ec76738c63a02a31fa119434`.
- Final source payload SHA-256:
  `5ea81d8c18db7cd13104275b4e6fd1b90135f877198ddf6ffdb0a950b922aa5b`.
- Final rebuilt object SHA-256:
  `734be2ca88ba32c5f2370dda4ef5443ec0459bc0e5c9572a0fab4f79d43579ec`.
- Immutable pre-alias January split-object SHA-256:
  `457d6eb9f4e79e816f6539c4bdbe2555c957267e75fefef156fa48076f03dc0f`.
- VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Full Halo/libcmt build and ordinary/semantic/progress reports: pass;
  semantic audit scanned 470 units / 4,956 functions, found 4,839 semantic
  exact and 4,849 accepted exact, with zero unit errors.
- Strict campaign board: 277/619 exact objects, 4,821/8,246 exact functions,
  and 667,575/1,922,669 padded code bytes.
- Object admission: zero candidates, contradictions, or revocations.
- Park validation: 12 active, zero stale, zero invalid.
- Complete tooling suite: 212 tests passed.
- Protected Units: 189/189 functions exact; sentinel
  `_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `git diff --check`, changed/deleted-path review, prohibited-construct scan,
  strict eleven-owner comparison, and common-runtime-owner comparison: pass.

Tracked scope is exactly the texture-cache source, the two symbol identity
corrections, and this new ledger.  No shared header, semantic exception,
parked record, completion label, protected source, or unrelated object is
changed.  Nothing is pushed from this isolated lane.
