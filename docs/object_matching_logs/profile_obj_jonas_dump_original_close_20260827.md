# `profile.obj`: dump-file original close behavior (2026-08-27)

## Outcome

`_profile_dump_to_file` is strict-exact again. The sole current-wave normal
compile adds one exact function and 127 meaningful code bytes. `profile.obj`
advances from 16/44 to 17/44 exact functions. It remains incomplete; no
completion label, semantic exception, adjudication, or data credit was added.

This wave deliberately restores January's original behavior: `fclose(file)`
is called after the `if (file)` block. If `fopen` returns `NULL`, January passes
that null pointer to `fclose`. The risk is documented rather than silently
retaining a safer behavior that contradicts the binary.

## Frozen baseline and evidence

The clean starting point was commit
`ca0fa8ee3c1b3d4bffe7f51bc742896b1a1afb2b`. The source blob before the wave
was `ec79b6538903ae63a705c02148e70a319a0dda2a`.

The January function is 127 meaningful / 128 padded bytes with nine
relocations. Its normalized SHA-256 is
`fa7272097cf84a7cc0f8e896d86de43e8776c37f79454e3446f4c23324107d74`.
Target and baseline are instruction-identical through the `fprintf` call. The
only control-flow difference is the final close:

- January branches around only `profile_dump` and `fprintf`, then pushes the
  file value and calls `fclose` unconditionally;
- the safety-modified baseline puts the `fclose` call inside the successful
  open branch.

That difference moves the `fclose` REL32 relocation from `+0x6E` to January's
`+0x71` and changes the final branch and stack-cleanup schedule. All eight
other relocation identities and addresses already agree.

Repository history independently preserves the exact original C body:

- `8584923e` first reconstructed the unguarded close;
- `6fb0a3d8` measured it strict-exact at the profile checkpoint;
- `3b285f47` later moved the close under the guard as an intentional safety
  change, causing the present mismatch.

The archived Claude corpus confirms the public one-parameter ABI and caller
use but contains no later body that contradicts January. HCEA exposes only the
unrelated script evaluator stub. Stian confirms the `const char *` parameter
and HaloScript caller. The January object and prior strict-exact repository
artifact therefore remain the body authority.

Pinned artifacts:

- target split object SHA-256:
  `f40acee3f68569049519a0f2c71775232cba27c6578e2bfc2aebfc52eca40587`;
- frozen baseline object:
  `build/audit/profile_dump_to_file_baseline_20260827.obj`, SHA-256
  `78564c6067c0dcc66e8b54ca411d44fae8b56546ada16431819d0939ffa2d9f9`;
- sole first-shot object:
  `build/audit/profile_dump_to_file_first_shot_20260827.obj`, SHA-256
  `7ba24bf373404fe6de7b0cbc4c9f5307630b8e83051c4e0aa6ea363238163b38`;
- retained source blob:
  `522ceba3ac5f623fbb3c0fd1b21d4c807efed2a2`;
- retained source SHA-256:
  `68ba8f6bb2c60bc81eb4cd545c665bd9f0ae5ea0292444ba5069a3f3c06d9ebc`;
- VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

## Candidate discipline and exact comparison

The edit moves the existing `fclose(file);` statement one lexical level out
of the guard and changes nothing else. The exact production-flag `/Zs` pass
was clean. Exactly one normal object-producing VC7 compile followed.

The resulting function agrees with January on all 128 padded bytes, normalized
hash, and all nine relocation addresses, types, destinations, and addends. The
frozen exact-set comparison reports only `_profile_dump_to_file` as
`newly_exact`; all 16 inherited exact owners remain exact and `lost_exact` is
empty.

No alternative source shape, corrective compile, compiler or flag change,
allowlist, comparator exception, object patch, byte forcing, or undefined
code-generation technique was used. The original null-close behavior is a
measured source fact, not an optimizer trick.

## Full gates

The full Ninja graph and report regeneration pass. Campaign progress is:

- 377/833 complete objects overall and 275/468 for Halo;
- 511,277/2,198,102 meaningful code bytes overall;
- 4,242/11,060 exact functions overall;
- 498,363/1,770,166 Halo code bytes and 4,075/7,574 Halo functions;
- 1,836,762/4,176,062 data bytes overall.

The semantic report evaluates 470 units and 4,353 functions, finds 4,208
semantic-exact and 4,268 accepted-exact functions, and reports zero unit
errors. Object admission is 0/0/0, parked validation is 3 active / 0 stale /
0 invalid, and all 179 tests pass with only the inherited pytest cache warning.

## House-rule audit

No signature or formatting change was needed. The function retains one
parameter on its own line and an explicit terminal `return;`. The C89 body
contains no assembly, `register`, `volatile`, barrier, pragma, intrinsic,
attribute, raw address, pointer/integer reconstruction, type pun, undefined
code-generation lever, synthetic anchor, or byte-forcing form. Protected
Units, game-engine, vehicle, and AI paths were untouched.
