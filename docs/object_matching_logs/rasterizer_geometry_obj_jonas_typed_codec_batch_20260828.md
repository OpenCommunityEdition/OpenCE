# `rasterizer_geometry.obj` Jonas typed-codec batch (2026-08-28)

This Jonas-owned ledger records exactly one frozen, natural typed-C batch for
the six remaining large/helper bodies in
`source/rasterizer/rasterizer_geometry.c`. All six were measured from the same
first code-producing artifact. None passed the strict January owner/COFF gate,
so every candidate definition and every candidate-only type, declaration, and
data owner was removed. The accepted production source is byte-for-byte the
input source, and `rasterizer_geometry.obj` remains at 9/19 exact functions.

## Fixed scope and authority

- Integration base: `b1709807eb7aeda1f07e7e3c3f0d5e31e9b437d2`.
- Isolated branch: `jonas/rasterizer-geometry-typed-batch-20260828`.
- Input and final source blob:
  `4b8701acfc443808777a59cb27fb0ce1570e2f86`.
- January split-object SHA-256:
  `d6357486072be1c8fb98fed92bc36eb0c1d6c7e68386df5c9ce164f0627ebb49`.
- The three earlier Jonas ledgers for compressed accessors, scalar codecs,
  and stripify were read in full before the experiment.
- All `AGENTS.md` and `CLAUDE.md` copies in the Pastudan and Stian research
  caches were screened; byte-identical copies were de-duplicated by SHA-256
  and every unique instruction payload was read in full. No applicable
  instruction file exists in this checkout.

The semantic donors were the six HCEA c168 bodies under
`research-cache/halocea-cseries-20260820/src/blam`, namely the DX9
`uncompress_int32_to_real_vector3d.c`,
`rasterizer_geometry_get_vertex_size.c`,
`rasterizer_geometry_uncompress_vertices.c`,
`rasterizer_geometry_compress_vertices.c`, and
`compress_real_vector3d_to_int32_clamp.c` files plus interface
`compress_real_vector3d_to_int32.c`. Their related typed vertex-layout headers
were also read. The January-oriented reconstruction in
`research-cache/stian-halo-full-history-20260828/src/halo/rasterizer/rasterizer_text.c`
was the strongest repository corroboration. These sources establish readable
intent and type shape; January COFF alone decides admission.

The four already-screened scalar compressors
`_compress_real_to_int8`, `_compress_real_to_int8_clamp`,
`_compress_real_to_int16`, and `_compress_real_to_int16_clamp` were expressly
excluded. Their known natural VC7 lowering calls `__ftol2`, whereas January
uses inline x87 conversion.

## Frozen batch and first gate

The batch used ordinary C89-compatible structs, fields, loops, switches,
assertions, calls, and arithmetic. It contained no assembly, `volatile`,
`register`, pragma, intrinsic, optimizer barrier, undefined behavior, raw
address or offset, representation pun, synthetic owner/anchor, or byte-forcing
construct. A parse-only `/Zs` pass succeeded. Exactly one ordinary production
compile then emitted the six bodies together.

The measured first artifact was 14,872 bytes with whole-file SHA-256
`5b305734847456f38ad38f98a80593e76d2873b579c8697f91219c310ffa303e`.
The strict comparator froze this result before any source-shape change:

| Function | Target padded / relocs / normalized SHA-256 | Candidate padded / relocs / normalized SHA-256 | Result |
| --- | --- | --- | --- |
| `_uncompress_int32_to_real_vector3d` | `144 / 9 / aa4a69683e5f1148413f6029acfc993341346b93b86809dcb5450896dcc3944d` | `128 / 9 / bc37687b6cd7deae87b69d0e96746a0a088e0c7d018edc9bc76741ef5c81b2d1` | reject |
| `_rasterizer_geometry_get_vertex_size` | `80 / 5 / f9ecaf008b54cfdfd5e4f02516c96a643e7044a4dc67bc6426477e06531b48a7` | `80 / 6 / 9102bbeb7fbe6b53cef7bb65a6da5172e8e2b8f8c2a08eb6ce5e0a6815cb10bd` | reject |
| `_rasterizer_geometry_uncompress_vertices` | `1088 / 59 / ebc1346db0db059d85ad9f0950a5772bbd00c9513ccdad85015964016c97ee10` | `1088 / 59 / 380275791a3ace7f8764cbd23441d28944e8c48496c87003957e5ce882a9e4a6` | reject |
| `_compress_real_vector3d_to_int32` | `544 / 38 / 1bb3b5c9d4712b2fc2ec7e9dc9f3f8e791496a82595ca154ca76719067cd60a7` | `496 / 41 / 6b72fa1a07641054960cdb525151e026ab1ed16f2720a6ffe49877923c77e047` | reject |
| `_compress_real_vector3d_to_int32_clamp` | `512 / 38 / ba84758e70f380dff5a1f53fc34fd72500d77f38cdef7529edeff8c5ccdffe3c` | `496 / 41 / 174487dc4a93f9aea18657dfef70cae3b4cbefdd6ef518dddf139ac99b3853e5` | reject |
| `_rasterizer_geometry_compress_vertices` | `736 / 46 / 61853f8316b2f593c3780ef8b378db764cb69f0c12d04f3f647118cd638992cb` | `736 / 46 / 48ec6c9bfdfcb361a7866632b363ecbb9568a7edbd9844c09d30fa4b24b632b5` | reject |

The helper differed in extent, scheduling, normalized bytes, and relocation
addresses. The natural vertex-size table introduced six relocations and the
readable `_rasterizer_vertex_type_sizes` owner, while January has five and a
single access to its anonymous `_rdata_0029e344` owner. Naming that raw target
owner in source is outside policy.

Both vector compressors emitted three `__ftol2` relocations absent from the
January bodies. The non-clamping target also diagnoses both promoted vector
components and their raw representation; the safe candidate deliberately did
not reproduce the representation pun. The typed uncompress loop reached the
exact 1,088-byte extent and relocation count, but its normalized code differed
and its node assertions owned `source->nodes[...]` strings where January owns
`src->nodes[...]`. The typed compress loop reached the exact 736-byte extent
and all 46 relocation identities, but its normalized code hash still differed.
Strict equality, not closeness, controls retention; no rename, expression
rewrite, or second candidate compile was attempted.

## Prune and retained-only replay

All six rejected bodies and all candidate-only support were removed with no
surviving production-source change. The retained-only production object was
rebuilt once from the original source. It is 4,357 bytes with phase-specific
whole-file SHA-256
`3207d34877dbb87b131b5e527771b7667099b04d7de54a07fe3315d7f5320b1e`.
The complete gate reports:

```text
== exact 9  residual 0  unwritten 10  (of 19 listed)
```

The ten unwritten functions are precisely these six bodies plus the four
excluded scalar compressors. All nine inherited exact functions remain
strict. The final tracked diff is this additive ledger only; Units, Vehicles,
Claude-active work, headers, configuration, existing ledgers, completion
labels, parks, and tooling are untouched. No commit or push was performed.
