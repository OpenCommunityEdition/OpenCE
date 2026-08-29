# `object_shadows.obj` ordinary-C closeout

Date: 2026-08-29

## Outcome

`source/objects/object_shadows.c` is fully exact under the strict campaign
comparator: 224/224 padded code bytes and 3/3 functions.

```
EXACT        16  _code_0012b870
EXACT        80  _code_0012b880
EXACT       128  _object_build_shadow
== exact 3  residual 0  unwritten 0
```

This is also a policy recovery. The previous source used a naked assembly
transcription for `_code_0012b870` and two `_ReadWriteBarrier` intrinsics to
hold the builder's return branches apart. The function was therefore excluded
from the strict campaign board as `asm-implemented`.

## Recovery

The 16-byte owner is the compiler's private in-TU calling convention for a
static typed wrapper with live call sites. Writing that ordinary helper and
calling it from `_code_0012b880` and `_object_build_shadow` reproduces the
target exactly:

```
push -1
push eax
call _object_get_and_verify_type
add esp, 8
ret
```

For `_object_build_shadow`, a readable `boolean result = FALSE` local followed
by an explicit positive-count assignment preserves the target's two return
paths naturally. Direct true/false returns collapse to `setg al` and miss the
target; the local-result topology compiles to the original 128-byte section
without barriers.

No assembly, `volatile`, `register`, pragma, intrinsic, attribute, raw address,
cast-based representation access, or byte forcing remains in the unit.

## Validation

- `python tools/campaign/gate.py source/objects/object_shadows --all`: 3/3 exact.
- `_code_0012b870`: 16 bytes, one relocation, normalized SHA-256
  `1603316925ff3734e5b16c1fdce605135a996f0f0a5a1094f4551c0c79b29988`.
- Rebuilt object SHA-256:
  `F41C42BC7C87994E40765C7C53C950FABF86AEA608C69FCBFEB3DD177661DFBB`.
- The obsolete `asm-implemented` entry was removed from
  `config/parked.json`, and the assembly audit was updated.

After unpark, the strict campaign board advances by one complete object, one
function, and 16 padded bytes from the preceding baseline:

```
objects 276/619 fully exact; bytes 651127/1922669; fns 4700/8246
```
