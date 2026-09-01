# `decals.obj`: `decal_new` deterministic bit-hash exception (2026-08-31)

## Result

`decal_new` is strict exact: 416/416 padded bytes and all 133 normalized
instructions agree with the January target. The source temporarily replaces
the local random seed for permanent decals by XORing the three raw IEEE-754
words of `origin` with `0xDEADC0DE`, then restores the prior seed.

## Representation access ruling

The retained expression is deliberately narrow:

```c
((unsigned long const *)origin)[2]
	^ ((unsigned long const *)origin)[1]
	^ ((unsigned long const *)origin)[0]
	^ 0xdeadc0de
```

HCEA independently reconstructs the same operation as integer reads of the
three `origin` components. The January disassembly likewise performs three
DWORD loads followed by XORs and the constant. No authenticated CSeries or
real-math macro converts a `real` bit pattern to an integer, and
`real_point3d` has no integer overlay. Inventing a union, calling a copy
routine, or adding a new shared macro would be less source-authentic and
would change the generated code.

This is therefore admitted as an original-era deterministic bit-pattern hash
under VC7/MSVC's non-strict-aliasing model. It is not precedent for geometric
representation punning, incompatible pointer call adapters, inactive-union
reads, or source changes made only to influence register allocation. Any
future exception requires its own semantic need, independent source evidence,
and strict target proof.

## Verification

- `python tools/campaign/gate.py source/effects/decals --fn _decal_new`
  reports `EXACT 416`.
- The gate's normalized target/candidate disassembly contains 133 identical
  instructions.
- The full `decals.obj` gate remains separately responsible for all sibling
  functions and honest parked residuals.
