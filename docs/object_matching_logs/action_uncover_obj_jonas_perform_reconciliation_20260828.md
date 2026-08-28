# `action_uncover.obj` perform ABI reconciliation and scheduler closeout

## Result

`source/ai/action_uncover.obj` remains honestly incomplete at 8/9 strict
functions and 2,112/2,656 padded code bytes.  `_action_uncover_perform` is
still absent from the production candidate object; no residual body, owned
string, completion label, park, semantic exception, compiler option, or
comparator exception is admitted.

This wave resolves the contradictory selector-width history and exhausts a
new, bounded set of ordinary-C copy topologies.  The best lawful candidate
has the exact 544-byte envelope, all 19 relocations, the same 164-instruction
count, and the complete January call/ABI schedule.  Its only remaining
difference is
the already isolated pursuit-point copy scheduler: 43 normalized byte
positions in one contiguous `+0xA2..+0xD3` window.

## Evidence read first

The current recovery record
`docs/object_matching_logs/action_uncover_obj_jonas_recovery.md`, the fuller
historical strategy log at commit
`1633ccff6d9baa3f01b8e4eb3648b5880932e8be`, and the preserved earlier
cross-build closeout log were read in full.  The historical log is decisive:
January callee disassembly proves all three related types are narrow:

- `actor_active_select_firing_position` returns `short`;
- the receiving `selected_firing_position_index` local is `short`;
- `actor_change_firing_position` receives that argument as `short` and returns
  `short`.

The 2026-08-26 follow-up in the current recovery record narrowed the selector
and local while retaining a `long` consumer parameter.  Its resulting
sign-extension was therefore an incomplete ABI experiment, not evidence of
January-width drift.  Repeating the complete three-part short topology makes
the call window strict: January and candidate both emit
`add esp, 0x18; mov ebx, eax; cmp bx, -1`, with no later sign extension.
HCEA independently corroborates the three short types, but January code is the
authority.

## Frozen best candidate

| Measurement | January | Best ordinary-C candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 533 / 544 | 533 / 544 |
| Relocations | 19 | 19 |
| Instructions | 164 | 164 |
| Normalized SHA-256 | `ddd3ed044dc0de47ecb6ba83ac683372ff87b3cb7d8fee318d20da5467c39ed8` | `f6c5eb3588f3efccd66611ba0c133b20dc3fb449fad4894f3022b8e9c6f04bd0` |

The immutable best scratch source snapshot is
`scratch/_gate_26956.c`, SHA-256
`d136beeae2cbe62fa65e46cfc3c16777cd8ae18ad70d2a51977d96feacf6199a`.
Its phase-local object is `scratch/_gate_26956.obj`, SHA-256
`0ae37faec0fc7446e23ffae6ad373a05a9dec640e0b486ae6c75de768afb41cd`.
The January split object is SHA-256
`82fd7ef88e212ebca0a7afe5e7eae37eef82054020988046090774865e5bb135`.
Raw candidate COFF identity is phase-local because debug records contain the
output path; the normalized function hashes are the reproducible identities.

Every relocation address, type, symbolic destination, and addend agrees.  Code
before and after the pursuit-copy window realigns exactly.  January uses one
compact source `lea`, loads all three point dwords, then schedules the stores
among the independent surface, cluster, and enable operations.  VC7 lowers
the best aggregate source with the same compact `lea` but interleaves each
point load and store.

## New compiler probes

Twenty-five production-neutral variants were compiled from the immutable
snapshot.  They cover ordinary inline pointer/value aggregate helpers,
`const` variants, by-value and return-by-value helpers, scalar/array/union
initializers, comma declarations and expressions, branch lifetimes, enclosing
aggregate layouts, and supported alias qualifiers.  They add the following
facts to the historical E01-E20 matrix:

- aggregate, value-identity, pointer-local, and `const` forms all canonicalize
  byte-for-byte to the `f6c5eb35...` best candidate;
- an inline helper taking a `real_point3d` by value and spelling scalar stores
  does create January's compact `lea; load x; load y; load z` prefix, but VC7
  also materializes a 12-byte argument temporary, shifts the frame, duplicates
  stores, and diverges globally.  It remains 544/19 but has normalized hash
  `3169a4b10565db618e9129673a3ac294cd6c0103cc23345879cde972f58eb6cf`
  and raw object SHA-256
  `3c7338bd4dfd682a40fe591a937580be7ddc81674981c570e010a003c65030bb`;
- scalar return-by-value helpers remain out of line at 576 bytes / 20
  relocations;
- a named enclosing target subobject rounds from 22 to 24 bytes, shifts every
  following offset, and yields a nonviable 544/19 object with raw SHA-256
  `dfde9276e1a93f833186af26bd492ff1e62f09fd7ffef490fc1c2fccc4e343d2`;
- XDK 3911 rejects `__restrict`, so this compiler supplies no supported
  non-alias qualifier to test;
- the January PDB module stream for `action_uncover.obj` is only 124 bytes of
  object/compiler records and contains no local-variable records.

The research harness is
`scratch/uncover_codegen_probe_subagent.py`, SHA-256
`176ae1e683a25e6850f75fb98fbae48b81fef7bb4dbee41afb010f371e19d66a`.
Scratch artifacts are evidence only and are not tracked or published.

## Public-source provenance

The user-supplied repositories were searched at their current public heads and
through relevant history.

- `punpckhdq/halo` is the same build-2342 project, but commit
  `fc47f9a15a0aea7f0081394c4d66db5c68ab5323` and all 141 reachable commits
  contain only the original `action_uncover.c` symbol/string stub.
- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains the strongest typed
  cross-build reconstruction.  Its own comments identify the body as a manual
  disassembly recovery after decompiler failure; it is not lost Bungie C.
- Stian/pastudan target retail Xbox `01.10.12.2276`, not January debug build
  2342.  Their bodies are Ghidra/LLM raw-pointer lifts.  The two historical
  enable/point statement orders are non-admissible and are already subsumed by
  the measured scalar and field-order families.
- Baboon, demon, nimbus, halopc-restored, and xboxrecomp contain no January
  function source or compiler-spelling record.  Xboxrecomp pull request 5 is
  useful runtime/recompilation infrastructure, but adds no source-provenance
  control for this scheduler.

No authentic January/Bungie source or new admissible defined-C spelling was
found.

## Disposition

The production source is intentionally unchanged: the eight established exact
functions remain, `_action_uncover_perform` remains declaration-only, and the
two perform-owned strings remain absent.  This preserves the atomic/fail-closed
boundary and all house rules.

Do not repeat the historical aggregate, scalar, alias, helper, field-order,
scope, barrier, representation-copy, or E01-E20 families, nor the 25 variants
above.  Reopen only for authentic January source/local records, a same-compiler
strict-exact C donor, or a genuinely new defined-C lifetime dependency that
retains the compact base `lea` and load-all/store-all schedule without a new
temporary, frame change, relocation, or forbidden compiler control.
