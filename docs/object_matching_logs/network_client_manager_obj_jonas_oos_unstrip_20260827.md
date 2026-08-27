# `network_client_manager.obj`: OOS and player-index wave (2026-08-27)

## Outcome

Two additional functions are recovered strict exact in
`source/networking/network_client_manager.obj`:

- `_network_game_client_game_out_of_sync`;
- `_unstrip_player_index`.

The object advances from 22/52 to 24/52 exact functions and from 1,423 to
1,617 matched meaningful code bytes. Its already exact two-byte BSS remains
unchanged. The object remains honestly `NonMatching`; no completion,
semantic-exception, parked, ownership, comparator, or admission credit is
added.

`_network_game_client_address_matches_server` was also reconstructed in the
frozen wave, but it remained a normalized-code mismatch after one
evidence-separated refinement. The non-exact body and its private supporting
types were removed before retention.

## Evidence and frozen wave

The starting checkpoint is commit
`cd0d05e34ee3dc0303c2b86d1640089495dbbbe3`. January COFF fixes the exact
control flow, field widths and offsets, assertion expressions and source
lines, call order, literals, relocation owners, and function extents. Stian's
independently reconstructed later-build source corroborates the transport
address shape, `char` address-comparison signature, local-player OOS loop,
error arguments, iterator topology, and stripped-player-index comparison.
Current networking, player, error, and data-iterator interfaces corroborate
the external calls. The iterator is represented with named typed fields and
zero-emission size/offset assertions.

The frozen baseline is:

- `build/audit/network_client_manager_address_oos_unstrip_baseline_20260827.obj`;
- raw SHA-256
  `21f5a82637a742934b81d3433d0cd125c0d09c1e590ffc9f29af4568de9ea742`;
- pre-wave source SHA-256
  `98a940a7cce38ba04a6cf66a546baa1eee8e9bdd528987c11b890726fe4d49b6`;
- pre-wave source Git blob
  `98d83ff9d74743e030d96f8613bff657ffc6f8d1`.

The candidate parsed under the pinned VC7 compiler before the only ordinary
code candidate. The frozen first-shot candidate is:

- `build/audit/network_client_manager_address_oos_unstrip_first_shot_20260827.obj`;
- raw SHA-256
  `3e27e747e31d1f5dc64265af890069e20bb49e6bc7c390c47e0e298050c4ce9d`;
- candidate source SHA-256
  `1414c979eaed0b8f80cd97bae559c9cf9fea07a038755c580b78b01562b6d1d9`.

The OOS helper was first-shot strict exact. The first shot also gave exact
padded extents and every relocation identity for the two other candidates,
while exposing narrow normalized-code differences. The target keeps the
`NONE` result of `_unstrip_player_index` live in EDI across iterator calls;
expressing that real source-level result lifetime with a C89 local and a
terminal return produced an exact body. The target address helper returns
`char`, independently corroborated by the later source, so the sole justified
retry also corrected that signature. The frozen evidence-refined object is:

- `build/audit/network_client_manager_address_oos_unstrip_evidence_refined_20260827.obj`;
- raw SHA-256
  `72b21f421fdc67fe6907c6b822bf6c27c0f813950be2ff722014830be6942c98`.

The address signature refinement did not change its remaining register-choice
difference: target and candidate are both 202/208 meaningful/padded bytes with
17 identical ordered relocations, but their normalized SHA-256 values remain
`d3684cc6e53dbe2bbf948c85382867fb598248571b373c780aed7b1a6e4395e7`
and
`35e5a00880ffadaf14301dd34c38400520cffe8e8a9a52d15efdb001fac5f42d`.
It was therefore rejected without another codegen search.

## Strict retained results

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_client_game_out_of_sync` | 104 / 112 | 6 | `2c4b4a540f4706aab2cfabc0d8c32e5b4bd8f3051a04899345962f3874ed0bd8` |
| `_unstrip_player_index` | 90 / 96 | 4 | `9586474740ec363653a13a6f4eab0a0721a4cd3034431a9a35217c6cff9897f6` |

Direct hardened comparison reports `all_equal: true` for both new functions,
all 22 inherited exact functions, and both BSS owners. The 24 exact functions
together cover 1,840 padded bytes and 110 ordered relocations, including every
symbolic destination and addend. The BSS remains 2/2 bytes strict exact with
zero relocations, alignment one, the original two-symbol ownership order, and
normalized SHA-256
`96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7`.

The retained object is preserved as
`build/audit/network_client_manager_oos_unstrip_retained_20260827.obj`, raw
SHA-256
`0ab74402b8dec4f1b77bef33e5aa8e5deb1e2db69c337cd1385a989fa2b1a002`.
The retained source SHA-256 is
`431723ead77a69bb40118043a3e25a47a92c4ac3930ced41f995eb0f318a9cac`;
its Git blob is `a5cf789a4cbe19bfbb33a17051b2c4a99eaee8a8`.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,397 functions evaluated, 4,252 semantic exact,
  4,312 accepted exact, zero unit errors;
- campaign: 377/833 complete objects, 4,286/11,060 exact functions, and
  514,783/2,198,102 exact code bytes;
- Halo: 275/468 complete objects, 4,119/7,574 exact functions, and
  501,869/1,770,166 exact code bytes;
- matched data remains 1,836,780 campaign-wide and 1,831,588 in Halo;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, configuration, semantic ledger, comparator, completion
  label, or parked manifest changed; no push.

## House-rule audit

Every parameter is on its own line and every value/void function has an
explicit terminal return. The iterator and client accesses use named typed
fields guarded by zero-emission layout assertions. The retained C89 source
contains no assembly, `register`, `volatile`, barrier, pragma, intrinsic,
attribute, raw-address expression, pointer/integer reconstruction, pointer or
union pun, undefined behavior, synthetic anchor, byte forcing, object patch,
alternate compiler, or acceptance exception.
