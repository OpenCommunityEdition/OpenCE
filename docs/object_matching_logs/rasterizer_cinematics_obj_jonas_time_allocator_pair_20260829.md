# `rasterizer_cinematics.obj` time/allocator pair (2026-08-29)

## Scope and result

This bounded continuation starts from canonical commit
`cd2b8f1fe5d26a0f36bb434b9abeb8e3704a3642`, immediately after the exact
near-clip getter recovery. It retains two additional ordinary typed-C owners
in `source/rasterizer/rasterizer_cinematics.c`:

| Function | Target padded bytes | Relocations | Target normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_code_0016d140` | 32 | 2 | `c973d0be268b7ab62bba97aa84c7c939b42ace7d8dbc2bb00561b17569a3e778` | **strict exact on first compile** |
| `_rasterizer_screen_effects_initialize` | 64 | 7 | `36bb4ad77684d402da3632095f2f932c2f49019f24eeb8cee5d972c416523278` | **strict exact on first compile** |

The object advances from 8/16 to 10/16 strict-exact functions, adding 96
padded code bytes and nine relocations. The retained-only strict gate reports
10 exact, 0 residual, and 6 unwritten functions. No object-completion,
semantic-exception, parked, or target-owned storage credit is claimed.

## Evidence and reconstruction boundary

January disassembly identifies `_code_0016d140` as a call to
`game_time_get`, conversion of the returned integer to `real`, and
multiplication by the exact single-precision seconds-per-tick constant
`0x3D088889`. The authenticated HCEA donor
`build/audit/refs/halocea/src/rasterizer_screen_effects_time.c` independently
names and corroborates that operation. The retained body uses the existing
`TICKS_PER_SECOND` constant and emits the exact two relocations at `+0x05`
and `+0x11` to `_game_time_get` and `__real@3d088889`.

The allocator body is supported by the authenticated HCEA
`rasterizer_screen_effects_initialize.c` donor, January's complete 64-byte
body, the already-proven 0x78 local state view, and established
`game_state_malloc`/`match_assert` source patterns in sibling Halo translation
units. It allocates 0x78 bytes under the target string `"screen effect filth"`,
publishes the pointer to the existing external `_bss_004662f4` owner, and
asserts the semantic expression `cinematic_screen_effect_globals` at January
source line 54. All seven relocation positions, types, names, and addends are
exact, including the two target-owned assertion strings. The semantic-name
macro changes source spelling only; it neither creates nor rehomes the BSS
owner.

Before this wave, the complete prior and Claude evidence available for this
translation unit was screened, including:

- `rasterizer_cinematics_obj_jonas_leaf_wave_20260821.md`;
- `rasterizer_cinematics_obj_jonas_setter_abi_repair_20260825.md`;
- `rasterizer_cinematics_obj_jonas_public_setter_pivot_20260828.md`;
- `rasterizer_cinematics_obj_jonas_near_clip_getter_20260829.md`; and
- the current Claude branches, which retain stubs for these owners.

The earlier leaf wave had excluded the allocator because the state/storage and
assertion boundaries were outside that wave. Those boundaries are now proven
by the integrated local state, external BSS ownership, donor, sibling source
patterns, and exact January relocation oracle.

## Rejected map-reset candidate

The adjacent `_rasterizer_screen_effects_initialize_for_new_map` donor was
revisited only because the earlier candidate had been 64 bytes with six
relocations and repeatedly reloaded the external pointer. A distinct
post-`csmemset` local snapshot reduced it to the exact 48-byte size and exact
three relocation addresses/identities. Its sole remaining difference was
instruction scheduling:

```text
January:  three script-value stores; add esp, 0xC; fourth store
candidate: add esp, 0xC; four script-value stores
```

A right-to-left chained assignment, the only natural source topology that
directly encodes January's store order, canonicalized to the same candidate
hash `aefa986ca6e38326688e349fd4c7592575ef8ccb312f80881b6a17da2dae42e8`.
The residual body was removed. Reopen only with original statement/scope
evidence or a lawful compiler scheduling control; do not retry the two
measured spellings or introduce a barrier.

## Policy and verification

Both retained functions follow the house format: `void` occupies its own line
inside no-argument parameter lists, every parameter would occupy its own line,
and every function has an explicit terminal return. The source uses no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
attribute, pointer/union pun, undefined behavior, raw address/offset access,
object-byte access, synthetic anchor, byte forcing, new compiler flag,
comparator exception, or object patch.

- January split-object SHA-256:
  `257b38976236eb2fb5913f4bda88186c6b4de1d498c5677eddded99b5b7e6400`.
- Final rebuilt object SHA-256:
  `558aa20a11b4793dd49bd2927049304fa388302f15e8bf3963ff7d210d6563e2`.
- Retained source SHA-256:
  `660e92331c6b6d2c41e9c760982fc56acd473c32ed0111a52629eec7052c06e2`.
- Retained source Git blob:
  `4152d202ed7b31f6b013879ed1504e0858ef8bef`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,787 functions,
  4,663 semantic exact, 4,673 accepted exact, and zero unit errors.
- Tool tests: 205 passed; the only warning is the sandbox denying pytest's
  optional cache directory.
- Object-admission audit: 0 candidates, 0 contradicted, 0 revoked.
- Parked ledger: 13 active, 0 stale, 0 invalid.
- Strict campaign board: 275/619 objects, 637,015/1,922,669 padded code
  bytes, and 4,644/8,246 functions.
- `git diff --check` and the prohibited-construct scan pass.

No shared header, configuration, semantic ledger, parked entry, tool, Units,
Vehicles, Matrix Math, protected source, target object, or other translation
unit was modified.
