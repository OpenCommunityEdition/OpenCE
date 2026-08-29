# `rasterizer_cinematics.obj` near-clip getter recovery (2026-08-29)

## Scope and result

This bounded wave started from published integration commit
`e6a5d81c71f202d40f79f743267a6ff7b1257a14` and recovered
`_rasterizer_get_near_clip_distance` in
`source/rasterizer/rasterizer_cinematics.c`. The natural typed-C body was
strict exact on its first compile. No spelling, scheduling, or code-generation
variants were attempted.

The object advances from 7/16 to 8/16 strict-exact functions. The retained
function contributes 38 meaningful / 48 padded code bytes and three `DIR32`
relocations. The final strict gate reports 8 exact, 0 residual, and 8 unwritten
of 16 listed functions. No object-completion, storage, semantic-exception, or
parked credit is claimed.

| Function | Target padded bytes | Target relocations | Target normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_rasterizer_get_near_clip_distance` | 48 | 3 | `b90d165e4a7fff416df6130e11d4a41ec314fbc818dab4296b673bd282e48613` | **strict exact** |

The relocations match at `+0x02`, `+0x08`, and `+0x15`, respectively naming
`_bss_004662f4`, `_rasterizer_global_defaults`, and `__real@00000000`.

## Evidence and source boundary

The current tree supplies the layout evidence that an earlier leaf wave had
explicitly deferred:

- `source/rasterizer/rasterizer.c` defines the separately-owned
  `rasterizer_global_defaults` object with `near_clip_distance` as its first
  field, at offset zero;
- the existing translation-unit-local cinematic state model proves the
  override at offset `0x74` with a compile-time `offsetof` assertion; and
- the January target disassembly requires the default load, an optional
  positive cinematic override, and exactly the three relocations listed above.

The later HCEA donor
`build/audit/refs/halocea/src/rasterizer_get_near_clip_distance.c` was used as
semantic corroboration, while January COFF bytes and relocations remained the
acceptance authority. The prior object records
`rasterizer_cinematics_obj_jonas_public_setter_pivot_20260828.md`,
`rasterizer_cinematics_obj_jonas_setter_abi_repair_20260825.md`, and
`rasterizer_cinematics_obj_jonas_leaf_wave_20260821.md` were read before the
candidate was emitted. Claude branches were also inspected; they retained the
getter stub and supplied no competing completed implementation.

The retained addition is a translation-unit-local one-field typed prefix for
the externally owned defaults object plus the public getter body. No shared
header or storage owner was changed.

## Policy and verification

The retained source is ordinary typed C with an explicit terminal return. It
contains no assembly, `volatile`, `register`, pragma, intrinsic, barrier,
force-inline directive, attribute, union/pointer pun, undefined behavior, raw
address, object-byte access, synthetic anchor, byte forcing, or private ABI.

- January split-object SHA-256:
  `257b38976236eb2fb5913f4bda88186c6b4de1d498c5677eddded99b5b7e6400`.
- Final rebuilt object SHA-256:
  `18c1702729368be601e9a45488d6d7924fe8e0848c710371be5e4be8ea1f3aca`.
- Retained source SHA-256:
  `b9828d48a1fb9447fc3078b2950474703236a05563c4bab6700a0e785dbead40`.
- Retained source Git blob:
  `621f248f0a1af09e8de435ffaefba40c36e58498`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,785 functions,
  4,661 semantic exact, 4,671 accepted exact, and zero unit errors.
- Tool tests: 205 passed.
- Object-admission audit: 0 candidates, 0 contradicted, 0 revoked.
- Parked ledger: 13 active, 0 stale, 0 invalid.
- Strict campaign board: 275/619 objects, 636,919/1,922,669 padded code
  bytes, and 4,642/8,246 functions.
- `git diff --check` and the prohibited-construct scan pass.

No configuration, semantic ledger, parked entry, tool, Units, Vehicles,
Matrix Math, Claude-active source, or other object was modified.
