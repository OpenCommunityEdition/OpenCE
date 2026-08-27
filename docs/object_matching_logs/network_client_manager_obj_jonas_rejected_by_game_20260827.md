# `network_client_manager.obj`: rejection-handler wave (2026-08-27)

## Outcome

`_network_game_client_rejected_by_game` is recovered first-shot strict exact.
`source/networking/network_client_manager.obj` advances from 24/52 to 25/52
exact functions and from 1,617 to 1,801 matched meaningful code bytes. Its
already exact two-byte BSS remains unchanged. The object remains honestly
`NonMatching`; no completion, semantic-exception, parked, ownership,
comparator, or admission credit is added.

`_network_game_client_create` was reconstructed in the frozen wave as well.
It reached the exact 144-byte padded extent and all 17 relocation identities,
but VC7 folded the singleton pointer rather than retaining it in ESI. Its
normalized code therefore remained non-exact and the body and private support
were removed before retention.

## Evidence and documented tooling

The starting checkpoint is commit
`d31c239a4efefb09a505ff32440aa9e77de8424b`. January COFF fixes the rejection
handler's cdecl ABI, unsigned 16-bit rejection code, assertion expression and
line, state-field width and offset, seven-entry bounded switch, string table,
call order, cleanup grouping, terminal return, function extent, jump-table
ownership, and all relocation destinations/addends.

Stian Eklund's independently reconstructed build-2276 source corroborates the
three-argument signature, default reason, code-to-string mapping, state reset,
diagnostic call, and final client reset. Its root `CLAUDE.md` and the relevant
Claude lift, offset-to-structure, tool-reference, verification, and VC71-match
documentation were treated as donor/tooling guidance: preserve C89, ABI,
field widths, branch shape, and source evidence; baseline before editing; and
measure the rebuilt object rather than trusting source similarity. That
project's behavior-oriented scoring is not an acceptance rule here: January
build 2342 plus this repository's hardened byte-and-relocation comparator
remain authoritative.

The retained source uses the named typed `network_game_client.state` field.
The source address is asserted but never dereferenced, so its pointee remains
explicitly unknown rather than being guessed. No tag or object datum access is
performed in this function.

The frozen baseline is:

- `build/audit/network_client_manager_create_rejected_baseline_20260827.obj`;
- raw SHA-256
  `0ab74402b8dec4f1b77bef33e5aa8e5deb1e2db69c337cd1385a989fa2b1a002`;
- pre-wave source SHA-256
  `431723ead77a69bb40118043a3e25a47a92c4ac3930ced41f995eb0f318a9cac`;
- pre-wave source Git blob
  `a5cf789a4cbe19bfbb33a17051b2c4a99eaee8a8`.

The candidate parsed under the pinned VC7 compiler before the sole ordinary
candidate compile. The frozen first shot is:

- `build/audit/network_client_manager_create_rejected_first_shot_20260827.obj`;
- raw SHA-256
  `f996b9357dba8eff3c65e9d5938541d9e82818ffd71c8ee735b37bae45da896c`.

The rejected creation helper's target/candidate normalized SHA-256 values are
`70ce26e5bb69bdf07ddaba7dfb5ead506dd34a7d6a8769f780fa8b52629802fb`
and
`5c389524808c91c36519bc807dbfa0ba08cad8e3c6e0df81ae9a01205671f1f3`.
Its precise first-shot difference is the singleton-pointer lifetime: January
pushes ESI and loads the singleton before the assertion, then reuses ESI for
the clear, field store, reset, return, and disposal; the candidate uses direct
symbol relocations and materializes the singleton only for the success
return. No ungrounded codegen search followed.

## Strict retained result

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_rejected_by_game` | 184 / 192 | 23 | `4d291d10d83ec6bd2950a2eede3d056dcacae714e9942063ce2743331378c194` |

Direct hardened comparison reports `all_equal: true` for the new function,
all 24 inherited exact functions, and the BSS. The 25 functions together
cover 2,032 padded bytes and 133 ordered relocations, including every
symbolic destination/addend. The BSS remains 2/2 bytes strict exact with zero
relocations, alignment one, the original two-symbol ownership order, and
normalized SHA-256
`96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7`.

The retained object is preserved as
`build/audit/network_client_manager_rejected_retained_20260827.obj`, raw
SHA-256
`1f5ed1cb70285e934ca1c1ec4cc72de5595157e828479aa8b14fc3d1b34034fd`.
The retained source SHA-256 is
`7f810f626d745941bbd004e110dc2d4905f8cb81d52713c3d0b063d6664aba33`;
its Git blob is `237c1650144dab8c929324e0028b71e248f64fe7`.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,398 functions evaluated, 4,253 semantic exact,
  4,313 accepted exact, zero unit errors;
- the semantic audit rises by exactly one function, but the configured
  headline remains 377/833 complete objects, 4,286/11,060 functions, and
  514,783/2,198,102 code bytes because objdiff reports this strict-exact
  jump-table function at 85.96491% and no semantic-manifest exception was
  added;
- Halo headline remains 275/468 complete objects, 4,119/7,574 functions, and
  501,869/1,770,166 code bytes for the same reason;
- matched data remains 1,836,780 campaign-wide and 1,831,588 in Halo;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, configuration, semantic ledger, comparator,
  completion label, or parked manifest changed; no push.

## House-rule audit

Every parameter is on its own line and the void function ends with an
explicit terminal return. Client access uses a named typed field guarded by
the translation unit's layout model. The retained C89 source contains no
assembly, `register`, `volatile`, barrier, pragma, intrinsic, attribute,
raw-address expression, pointer/integer reconstruction, pointer or union pun,
undefined behavior, synthetic anchor, byte forcing, object patch, alternate
compiler, or acceptance exception.
