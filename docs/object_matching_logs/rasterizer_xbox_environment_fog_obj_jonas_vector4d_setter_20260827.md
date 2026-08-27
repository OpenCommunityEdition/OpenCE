# `rasterizer_xbox_environment_fog.obj`: vector4d setter (2026-08-27)

## Outcome

`_set_real_vector4d` is newly strict-exact on the sole normal candidate
compile. The wave adds 31 meaningful code bytes and one exact function. The
object advances from 4/27 to 5/27 exact functions and from 32 to 63 matched
meaningful code bytes. It remains incomplete; no completion label, semantic
exception, adjudication, or data credit was added.

## Frozen evidence

The clean starting point was commit
`b43ad87075efce379b19f5c79900190580831e51`. The source blob before the wave
was `c0ca51cd26839d2bdf0b3f99767e692b6a5e57fb`.

The January function is 31 meaningful / 32 padded bytes with no relocations.
Its normalized SHA-256 is
`9a46b00e8fe912ad0044759feb81101ce35e7914093416a4c31169719f10d199`.
The target instructions establish all ABI facts needed by the source:

- the first stack argument is returned unchanged in `EAX`;
- four 32-bit arguments are stored at offsets 0, 4, 8, and 12;
- the first scalar is handled through x87 and the remaining scalar bit patterns
  through integer registers;
- the function is ordinary caller-cleanup C and has no external dependencies.

The canonical tree defines `real_vector4d` as a 16-byte union with named
members `i`, `j`, `k`, and `l` at those offsets. Existing `real_math.c`
functions authenticate pointer-returning vector helpers and named member
access. The adjacent Xbox rasterizer sources authenticate including
`cseries.h` before `real_math.h` when the math header is used directly.

The archived Claude prototype guidance was applied narrowly: return-value use,
stack arguments, and repeated stores fixed the pointer plus four `real`
parameters, while no richer semantic type or behavior was invented. HCEA,
Stian, and Claude source searches contained no competing implementation of
`set_real_vector4d`; they supplied no evidence contrary to the January object
or canonical type layout.

## Candidate discipline

The retained C89 body assigns `i`, `j`, `k`, and `l` in offset order and
returns the destination pointer. A non-object-producing `/Zs` pass first
identified that `real_math.h` depends on foundational `byte` and `boolean`
typedefs. Adding the established `cseries.h` predecessor made the syntax gate
clean. This did not consume a candidate object compile.

Exactly one normal VC7 object-producing compile was then performed. It matched
on first shot. No corrective compile, source permutation, compiler or flag
change, allowlist, comparator exception, object patch, byte forcing, or
undefined behavior was used.

Pinned artifacts:

- target split object SHA-256:
  `d12ea6f0ba18bc4e6e25921090f448b27adad35f5563477ea0cd64b6305e021c`;
- frozen baseline object:
  `build/audit/rasterizer_xbox_environment_fog_set_real_vector4d_baseline_20260827.obj`,
  SHA-256
  `a1193d7bd40a75cf2feb64e8109734ee132c72f30e6deff6275773ab3f7df203`;
- sole first-shot object:
  `build/audit/rasterizer_xbox_environment_fog_set_real_vector4d_first_shot_20260827.obj`,
  SHA-256
  `04600d05d57e775817b786bd656e4656524c8a6e30bdee0b678c66f973e6dfa5`;
- retained source blob:
  `16ef3946d543dba5a7d090a5fac02f50bdbdf999`;
- retained source SHA-256:
  `59dd37d3d5c777f8a63b74685ca99c652246d7178e07ae037f5b89496381fbfc`;
- VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

## Strict comparison

Target and candidate agree on all 32 padded bytes, zero relocations, and the
normalized hash above. The frozen exact-set comparison reports only
`_set_real_vector4d` as `newly_exact`, with no lost exact owners. These four
inherited functions remain strict-exact:

- `__rasterizer_environment_fog_end`;
- `_rasterizer_environment_fog_screen_model_end`;
- `_rasterizer_environment_fog_screen_window_begin`;
- `_rasterizer_environment_fog_screen_window_end`.

## Full gates

The full Ninja graph and report regeneration pass. Campaign progress is:

- 377/833 complete objects overall and 275/468 for Halo;
- 511,150/2,198,102 meaningful code bytes overall;
- 4,241/11,060 exact functions overall;
- 498,236/1,770,166 Halo code bytes and 4,074/7,574 Halo functions;
- 1,836,762/4,176,062 data bytes overall.

The semantic report evaluates 470 units and 4,353 functions, finds 4,207
semantic-exact and 4,267 accepted-exact functions, and reports zero unit
errors. Object admission is 0/0/0, parked validation is 3 active / 0 stale /
0 invalid, and all 179 tests pass with only the inherited pytest cache warning.

## House-rule audit

Every parameter is on its own line and the function has an explicit terminal
return. The body is ordinary C89 and uses the canonical typed aggregate. It
contains no assembly, `register`, `volatile`, barrier, pragma, intrinsic,
attribute, raw address, pointer/integer reconstruction, type pun, undefined
behavior, synthetic anchor, or byte-forcing form. Protected Units, game engine,
vehicle, and AI paths were untouched.
