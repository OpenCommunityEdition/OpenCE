# `source/scenario/wind.obj` — defined-C fuzzy closeout

## Result

`wind.obj` remains **7 / 9 strict exact**, with no unwritten functions. The
two residuals now have fail-closed fuzzy-parking evidence:

| Function | Target / candidate padded bytes | Relocations | Objdiff |
| --- | ---: | ---: | ---: |
| `_wind_variance_get` | 224 / 208 | 8 / 8 | 93.333336% |
| `_wind_variance_initialize` | 384 / 400 | 9 / 9 | 60.99206% |

This lane adds the park for `_wind_variance_get`; the initializer park is
unchanged. No production C or header change is retained. The isolated gate
still reports seven exact functions, two residuals, and zero unwritten
functions.

## Retained source and evidence

The retained helper is ordinary typed C. It uses the project `real`,
`real_point3d`, and `real_vector3d` types, preserves the target-backed
axis-index plus three-axis-countdown loop, and reads the low byte of the
bias-added `real` through `byte const *`. `byte` is the repository's unsigned
character typedef, so this is defined object-representation access rather than
an inactive-union read.

The HCEA C-series reconstruction at
`research-cache/halocea-cseries-20260820/src/blam/scenario/wind_variance_get.c`
independently corroborates the algorithm: derive one phase per axis from
position, time, and the per-axis scale; take its magnitude; add the `2^23`
bias; select the low six representation bits; accumulate a three-axis noise
table; and scale by one third of the requested magnitude. The HCEA PowerPC
source uses a fixed-size `memcpy` for its representation read, but VC7 lowers
that spelling to calls in this tree, so the lawful unsigned-character form is
retained.

The open Marathon source supplies no direct wind donor. Its authenticated
`cseries.lib/cseries.h` defines `ABS(x)` as a conditional expression:

```c
#define ABS(x) ((x>=0) ? (x) : -(x))
```

That is useful naming/style evidence, but it does not explain January's
full-width integer sign clear.

## Fixed code-generation boundary

January and the candidate have the same eight relocation destinations. Their
principal difference is the magnitude operation inside the loop:

- January spills the phase, emits `and dword ptr [...], 0x7fffffff`, reloads
  it, then adds `8388608.f`;
- lawful VC7 `fabs` spellings emit x87 `fabs` and remain 208 bytes.

A split assignment had already been measured and produces the same x87 form.
The machine-wide freshness audit found one exact historical candidate, but it
reads an inactive union member to reinterpret the float; it remains rejected.

This lane also tested the smallest assembly hypothesis: leave the surrounding
math as C and replace only the absolute-value operation with
`__asm and sample_key, 0x7FFFFFFF`. The instruction itself matched, but the
assembly boundary forced VC7 to spill the live one-third magnitude, grew the
frame from `0x0C` to `0x10`, and left the function nonexact. Expanding the
assembly to manually preserve the compiler's x87 lifetime would be source
steering without original-source provenance, so the experiment was reverted.

The residual is therefore parked as `unclassified`, not assigned an invented
compiler recipe. Reopen it only for authoritative January source or a defined,
same-compiler full-width sign-clear idiom with credible provenance.

## Validation

The PID-safe campaign gate was run from the isolated worktree with the target
split copied locally and the Xbox include/toolchain linked read-only:

```text
residual    224  _wind_variance_get  [size 208!=224, sha]
residual    384  _wind_variance_initialize  [size 400!=384, sha]
== exact 7  residual 2  unwritten 0  (of 9 listed)
```

Strict measurements for the newly parked helper:

- target: 224 bytes, 8 relocations,
  `4df611f5e5a98c97ca86e2ea03d29c3057296fb1382b36fc687c31f0e58db1a6`;
- candidate: 208 bytes, 8 relocations,
  `951ffdb4c1b81a50bceb55a1b2ac7167c51c20a3bdb8542b61524c88e89c37d6`;
- objdiff: 93.333336%.

No union punning, volatile access, assembly, fake dependency, address-derived
name, manual object-byte forcing, or nonsensical control flow is retained.

