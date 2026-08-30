# `game.obj` `game_safe_to_save` frozen first shot (2026-08-29)

## Result

This fresh lane starts from canonical commit
`f6ad9861386686ff913c66511b344785f9cfe051` and tests exactly one natural
C89 body for `_game_safe_to_save`. The body reaches the January function's
full 320-byte padded envelope and all 32 semantic relocation destinations,
but is not byte-exact. It is withdrawn without a source-shape retry.
`game.obj` therefore remains at 13 strict-exact functions; no code or data
credit is added.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `315 / 320` | `32` | `663d7e30967f2dc94c247932ed1cd9e78c2c3bbc99ce27b9ff2017bcc637607b` |
| Frozen natural candidate | `313 / 320` | `32` | `60e578107d0069d569419d778291ae5bf633bb6f66ff8bde2786ba2ffecc7a3d` |

The target relocation addresses are
`4,13,26,31,45,54,67,72,86,95,108,113,127,136,149,154,168,177,186,191,205,214,223,228,242,251,260,265,279,288,297,302`.
The candidate addresses are
`4,13,22,27,41,50,59,64,78,87,96,101,115,124,133,138,152,161,174,179,193,202,215,220,234,243,256,261,275,288,297,302`.
Order, relocation type, destination, addend, and string identity agree for all
32 records. Address equality does not.

## Scope and provenance

Before selection, all six existing `game.obj` ledgers, the campaign playbook,
the object-ledger README, campaign/audit tooling documentation, and the
applicable Stian `CLAUDE.md`/`AGENTS.md` were read. Repository-local
`CLAUDE.md` and `AGENTS.md` do not exist at this commit. All-ref history,
campaign ledgers, and registered worktrees contain no earlier production body
or rejection for this function.

Two authenticated public reconstructions independently fix the eight safety
predicates, their order, the `debug_game_save` guard, and every warning text:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, source blob
  `afadbef3feb43197d88e3dc14be5f4e27c350c12`, SHA-256
  `c2ab76edad6d1cc6b0f97e327fd87413cd86bc1b68288c475167f9bc5c8fe1e4`;
- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, containing `game.c` blob
  `b0808931bc9367a9c09a666f4ede9e03c9828ee2`, SHA-256
  `c8bbb7041f6461720fb904a7f5b794c064dfe15a51dff47338d653a4b59409f0`.

HCEA expresses a result variable initialized true, guarded later tests, and a
direct false return from the terminal vehicle test. Stian expresses immediate
false returns. January independently proves an unsigned-byte Boolean result
held in `BL`. The frozen body therefore uses the HCEA result/short-circuit
shape with the canonical `boolean` type, while preserving HCEA's terminal
direct false return. All callees and the debug flag use ordinary typed
declarations; no raw layout or address operation is involved.

Immutable artifacts:

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- January `game.obj` SHA-256:
  `fd703ba666f13a5f1311dfbe1383eba452cf49a0a9a7fab47b1cffd06c1cfd0`;
- pre-shot baseline `game.obj` SHA-256:
  `f4470a4409231edbdecb378e6cc3f2590e3ab4dcd72f01a83268a50de2cf00c9`;
- frozen first-shot object SHA-256:
  `183b68c31d4e31dedbe6be8da7ae5bde111352dd11e4095e31227b04e5d6c548`;
- XDK 3911 CL 13.00.9254.1 SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The retained production source is restored exactly to Git blob
`9f845c0774f40b2b02186d6d7cd72aa87be65d0a`; the candidate symbol and its
temporary declarations are absent.

## Boundary and do-not-repeat rule

The first raw divergence is the first condition's `JE` displacement at
function offset `+0x0B`: January branches from `+0x0A` to `+0x2C`, while the
candidate branches to `+0x28`. More importantly, January's debug-disabled path
at `+0x13` is a six-byte near branch to a common false-result tail at
`+0x135`. The donor-based candidate uses a two-byte short branch to a local
false-result tail at `+0x22`. Later candidate blocks either jump backward to
that first tail or duplicate their own return. January instead jumps forward
to the terminal false tail and duplicates the post-warning returns. The
candidate's terminal direct-false source also materializes the final result in
`AL`; January clears `BL` and then moves `BL` to `AL`.

These are source-level CFG/lifetime differences, not missing semantics or
relocation identities. Do not retry the HCEA guarded-result body, the Stian
early-return body, declaration changes, predicate spelling, or branch/byte
tuning. Reopen only with original January source, a same-compiler natural C
donor that independently proves the common terminal `BL` assignment, or new
compiler provenance that explains the branch-tail placement.

No assembly, `volatile`, `register`, pragma, intrinsic, attribute, barrier,
raw address, byte offset, pointer/integer reconstruction, cast or union pun,
undefined behavior, synthetic anchor, compiler substitution, object patch,
or comparator exception was used. The production candidate was emitted once;
the subsequent compile only restored the body-free canonical object.

## Validation

The frozen artifact preserves all thirteen inherited exact `game.obj`
functions, and the restored baseline reproduces all thirteen. The focused
campaign gate reports `13 exact / 0 residual / 14 unwritten`; the withdrawn
function is again `UNWRITTEN` at 320 padded bytes.

- Complete `halobetacache_build` and `libcmt_build`: pass, with no pending
  work after the restored object replay.
- Direct hardened comparison: all thirteen inherited `game.obj` functions
  remain exact; no new or lost exact owner exists.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden code bytes, 4,810 accepted exact, and zero
  unit errors.
- Campaign progress: 384/833 matched objects, 4,771/11,060 functions,
  606,187/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes. Halo is 282/468 objects,
  4,604/7,574 functions, and 593,273/1,770,166 code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, and zero invalid records.
- Protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Complete tooling suite: 212/212 tests pass.
- `git diff --check` and the final source-policy scan: clean.

The final tracked change is this additive ledger only. No production source,
header, configuration, semantic record, parked record, protected Units file,
or target artifact is changed. No push is performed.
