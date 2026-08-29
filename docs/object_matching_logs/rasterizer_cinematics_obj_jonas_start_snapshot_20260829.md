# `rasterizer_cinematics.obj` start snapshot recovery (2026-08-29)

## Scope and result

This one-function continuation starts from canonical commit
`023471de05473dc168ebe339819348de89b82923` and recovers
`_rasterizer_screen_effect_start` in ordinary typed C. The object advances
from 10/16 to 11/16 strict-exact functions:

| Function | Target/base padded bytes | Target/base relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_rasterizer_screen_effect_start` | 64/64 | 3/3 | `aa4476e7b4eeaa5041c93eed35837d057f7de66b048b83b84e9aad5a92ea4adc` | **strict exact** |

The retained-only object gate reports 11 exact, 0 residual, and 5 unwritten
of 16 functions. This wave adds 64 padded code bytes and three relocations.
No object-completion, storage, semantic-exception, or parked credit is
claimed.

## Evidence and lifetime control

The authenticated HCEA state declaration proves the `initialized` boolean at
offset `+0x39`, adjacent to the already-proven `has_control` byte at `+0x38`.
The local state view now names that field and adds a compile-time offset
assertion while preserving its established 0x78 size and the script/near-clip
offset assertions.

January and the authenticated HCEA donor
`build/audit/refs/halocea/src/rasterizer_screen_effect_start.c` agree on the
semantic topology: if the external state exists, optionally clear the 0x38
parameter prefix when requested or not yet initialized, mark it initialized,
then take control.

The earlier natural donor adaptation recorded in
`rasterizer_cinematics_obj_jonas_leaf_wave_20260821.md` was already equal in
padded size but emitted a fourth relocation: VC7 reloaded the external pointer
before the final control-byte store. The distinct retained lifetime uses a
typed local snapshot on entry and explicitly refreshes that snapshot from the
external global only after `csmemset`. This directly models January's two
paths:

- the no-clear path keeps the entry snapshot in EAX through the control store;
- the clear path reloads `_bss_004662f4` once after the call, then uses that
  one pointer for both the initialized and control stores.

The result is instruction-for-instruction exact. Relocations match at `+0x04`
to `_bss_004662f4`, `+0x20` to `_csmemset`, and `+0x25` to
`_bss_004662f4` again.

## Policy and verification

The body has one parameter on its own line and an explicit terminal return.
It uses named typed fields and no assembly, `volatile`, `register`, pragma,
intrinsic, optimizer barrier, attribute, pointer/union pun, undefined
behavior, raw address/offset access, object-byte access, synthetic anchor,
byte forcing, compiler-flag change, comparator exception, or object patch.

- January split-object SHA-256:
  `257b38976236eb2fb5913f4bda88186c6b4de1d498c5677eddded99b5b7e6400`.
- Final rebuilt object SHA-256:
  `f25fb118bdf759da4a4f2d809b51ec7d96acb8ad8b2329bd6dbba8e8918842bd`.
- Retained source SHA-256:
  `ed00317c4b084935f323d7bc1987f6800e54aa19658a5fb1599d26b540510632`.
- Retained source Git blob:
  `d19d4bb5490f3b54bb1ca7e983cdb90aa67066dc`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,788 functions,
  4,664 semantic exact, 4,674 accepted exact, and zero unit errors.
- Tool tests: 205 passed; pytest's optional cache-directory warning is
  sandbox-only and does not affect test results.
- Object-admission audit: 0 candidates, 0 contradicted, 0 revoked.
- Parked ledger: 13 active, 0 stale, 0 invalid.
- Strict campaign board: 275/619 objects, 637,079/1,922,669 padded code
  bytes, and 4,645/8,246 functions.
- `git diff --check` and the prohibited-construct scan pass.

No shared header, configuration, semantic ledger, parked entry, tool, Units,
Vehicles, Matrix Math, protected source, target object, or other translation
unit was modified.
