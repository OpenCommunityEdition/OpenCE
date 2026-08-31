# `rasterizer_xbox_active_camouflage.obj` visibility recovery

> Superseded state note (2026-08-30): the visibility implementation remains
> exact, but the four handwritten assembly wrappers described below were
> removed and their stock-XDK identities recovered. The BSS placeholder was
> also replaced by three authenticated global names. See
> `rasterizer_xbox_active_camouflage_obj_jonas_credibility_recovery_20260830.md`.

## Result

`_rasterizer_active_camouflage_set_visibility` is strict semantic-COFF exact,
and its six-byte BSS owner is exact in size, flags, symbol offset, storage
class, and zero contents. The whole-unit gate reports 5/12 exact functions;
four are pre-existing `asm-implemented` parks and remain excluded from strict
C credit. This wave adds one genuine C function, 32 meaningful/padded bytes,
three relocations, and six matched data bytes. Seven functions remain
unwritten, so the object remains `NonMatching`.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_active_camouflage_set_visibility` | 32 / 32 | 3 | `3c3d3a4c5367ce81d27babd47f164172f684935f32a1a090e36e4f9cac3d1b7a` |

January proves a six-byte private state. Its visibility byte is written at
offset zero on every call. Disabling visibility additionally clears the
16-bit state at offset four and the cached window byte at offset one. The
retained named aggregate states those semantics directly, and compile-time
checks hold its size and all accessed offsets. Explicit zero initialization
causes VC7 to emit January's true six-byte `.bss` owner rather than a COMMON
symbol. Direct function comparison reports `all_equal: true`; all relocation
addresses, types, base identity, and addends are identical.

January object SHA-256 is
`c54a305b551a8913d7eeb75b29e8ed1b7c097ed5fd9392c443f273dcec036a70`;
the final rebuilt raw object SHA-256 is
`fa472c69cc5ca79519d17392d745f96bdf4d452c3cfe48d8536cd4698ce401c8`.

The combined wave passed complete Halo/libcmt/all-source builds, fresh
progress and semantic generation, admission 0/0/0, park validation 13/0/0,
strict board generation, `git diff --check`, and 205/205 tests. The strict
board is now 274/619 objects, 627,783 / 1,922,669 bytes, and 4,581 / 8,246
functions after excluding parked and assembly-implemented credit.

The newly retained implementation and state model are readable typed C. No
new assembly, attribute, pragma, intrinsic, qualifier trick, raw address,
undefined behavior, compiler option, object patch, semantic exception, or
comparator exception is introduced. The four inherited custom-ABI assembly
parks are unchanged. `matrix_math.obj`, Vehicles, Units, and all protected
AI/game-engine sources are untouched.
