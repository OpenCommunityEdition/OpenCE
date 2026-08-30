# `hs_runtime.obj` macro-evaluate private-ABI boundary (2026-08-29)

## Result

Starting from canonical commit
`69724391f3a9a33020eb01d0e69c98eb1d02e38b`, one previously unattempted,
three-donor typed wrapper shape was compiled exactly once for
`_hs_macro_function_evaluate`. Candidate and January both emit a 48-byte
padded COMDAT with two relocations to the same callees, but the normalized
bytes and second relocation address differ. The candidate was removed in
full without a spelling, declaration, scheduling, signature, or linkage
retry. No implementation or matching credit survives.

| Owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January `_hs_macro_function_evaluate` | 38 / 48 | 2 | `99a8835c1bb3ef4061297659003a5f2d0e7f1513031e450c45579fe8e2285bab` |
| Frozen candidate | 39 / 48 | 2 | `c32c9309383f631b977e2be4ccbbd4691e7c465812090911d3513ddd7850f1b0` |

The target relocation sequence is `_hs_function_get` at `+0x08` followed by
private `_code_000bb740` at `+0x1D`. The candidate retains the first record
at `+0x08`, but the second record is one byte late at `+0x1E`.

## Census and evidence

The complete existing `hs_runtime.obj` ledgers were read before emission:
the HS checkpoint, wake-staging rejection, short-to-real union-pair rejection,
and mask-index/operand-order rejection. None attempts or rejects this owner.
All reachable campaign commits for `source/hs/hs_runtime.c`, registered
campaign worktrees, and durable object ledgers were searched; none contains a
production definition of `hs_macro_function_evaluate`. The baseline source
blob is `73261b3dec4b32e61c12166c8d89ffb2879aea38`.

January COFF is the acceptance authority. It calls `hs_function_get`, reads
the signed parameter count and inline parameter-type array at typed descriptor
offsets `+0x18` and `+0x1A`, and forwards them with the initialize flag and
thread datum to `_code_000bb740`.

Three independent public reconstructions agree on that semantic topology:

- HCEA commit `aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`,
  `src/hs_macro_function_evaluate.c` blob
  `38efbc8f3d5f450da2fcc5983cab144c6e324115`, names the downstream routine
  `hs_arguments_evaluate` and supplies the typed descriptor fields.
- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`,
  `src/halo/hs/hs_runtime.c` blob
  `29b8888f7540d9d22c652ac2e8c1b1d47daab4e0`, independently reconstructs
  the target-version wrapper at `0xCC560`.
- Pastudan commit `918af885935ec470a31256ecce9a977b12b01f80`,
  `src/halo/hs/hs_runtime.c` blob
  `89e1360effdcf1ca5f095976f0817bb98da4ff98`, contains the same
  target-version source shape.

Those repositories are semantic and layout evidence only. Their target and
compiler differ from January PC, so none grants byte credit.

## Frozen production candidate and measured boundary

The candidate used a local typed `hs_function_definition` view whose parameter
count and flexible first parameter-type member land at the January offsets. It
called `hs_function_get(function_index)` once, then returned the result of
`code_000bb740(thread_index, function->parameter_count,
function->parameter_types, initialize)`. It was ordinary readable C89 with
one declaration at the top of the body and an explicit return.

The first call, descriptor accesses, padded extent, relocation destinations,
and public symbol shape all agree. The fixed boundary is the private first
argument ABI:

- January leaves the thread datum in `EAX`, pushes only initialize, parameter
  types, and parameter count, then cleans 16 bytes total, including the
  earlier `hs_function_get` argument.
- With `_code_000bb740` still unwritten, VC7 sees only an ordinary external
  four-parameter prototype. It pushes initialize, loads the thread datum,
  pushes types, count, and thread datum, then cleans 20 bytes.

The downstream target is a 448-byte private function. Its January body itself
calls the still-unwritten private `_code_000b9f40` and `_code_000bb570` through
translation-unit-derived register ABIs. A lawful reopen therefore requires
authentic source for that complete connected private cluster; declaration,
argument-order, linkage, or return-shape tuning of this wrapper alone cannot
teach VC7 January's `EAX` convention.

## Gate and disposition

- January split object raw SHA-256:
  `8ead2e539de118ecb284e82abe96fbb636b66800bbe623c2f8a306546a4b8ea6`.
- Frozen first-shot object: 15,687 bytes, raw SHA-256
  `71aff387080e690bb68fd7f08e9cb9a074b3f88762234af1b6e4c8589ef9fbed`.
- Compiler: XDK 3911 VC7 `13.00.9254.1`, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the unchanged `/O2 /Oy- /DDEBUG /Dxbox` command.
- Baseline gate: 24 strict exact, 7 residual, 35 unwritten.
- First-shot gate: the same 24 strict exact owners, 8 residual, 34 unwritten;
  all inherited exact siblings remain strict.
- Reverted replay: 24 strict exact, 7 residual, 35 unwritten, followed by a
  one-target Ninja no-work state.

The candidate additions contained no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, optimizer barrier, raw offset or integer
address, pointer or union pun, undefined behavior, synthetic anchor, byte
forcing, object patch, compiler exception, or tool exception. Units,
Vehicles, Matrix Math, AI Debug, configuration, headers, and all other source
files were untouched. Full builds, progress regeneration, and admission
closeout were not warranted after the strict miss. This additive ledger is the
only retained change, and no push is performed.
