# `network_client_manager.obj`: lifecycle wave (2026-08-27)

## Outcome

Four additional functions and the complete two-byte BSS section are recovered
strict exact in `source/networking/network_client_manager.obj`:

- `_code_001141c0`;
- `_network_game_client_dispose`;
- `_network_game_client_switch_to_pregame`;
- `_network_game_client_game_shutdown`;
- `_allow_out_of_sync` and
  `_network_game_client_dont_use_directly_in_use`, in that BSS order.

The object advances from 18/52 to 22/52 exact functions, from 1,010 to 1,423
matched meaningful code bytes, and from zero to two matched data bytes. It
remains honestly `NonMatching`; no completion, semantic-exception, parked,
ownership, comparator, or admission credit is added.

## Evidence and frozen wave

The starting checkpoint is commit
`8fffaeeba7f453aaec34c137e5766a48aa8e9a0e`. January COFF fixes every branch,
field width/offset, assertion expression and source line, call order, literal,
tail call, relocation owner, and function extent in the four bodies. It also
fixes a two-byte, alignment-one `.bss` with `allow_out_of_sync` at offset zero
and `network_game_client_dont_use_directly_in_use` at offset one. Stian's
independently reconstructed later-build source corroborates the connectivity
guard, dispose topology, pregame state transition and field stores, duplicate
shutdown assertions, and shutdown tail call. Current networking/UI/error
headers independently corroborate the external interfaces.

The frozen baseline is:

- `build/audit/network_client_manager_lifecycle_baseline_20260827.obj`;
- raw SHA-256
  `c2aa4e13fe672d537f3512bdc28ae0161844cfd3aeadf5d2bd9dfea1639a8d35`;
- pre-wave source SHA-256
  `7c8e71ba7601449300a9ac1cf1dcbb4ec4a4ea31431be0d9624728ca8a0978b0`;
- pre-wave source Git blob
  `4c23be213e5e8dec697f666e0c328ed866df1a72`.

The candidate parsed under the pinned VC7 compiler before the only ordinary
code candidate. The frozen first-shot candidate is:

- `build/audit/network_client_manager_lifecycle_first_shot_20260827.obj`;
- raw SHA-256
  `817fecb00aba067ef6100a7fbd759fe5f142e107672c05c758c17d28c7072e23`.

All four functions were first-shot strict exact. That object exposed one new,
separate data-placement fact: declarations without explicit initializers were
emitted as COFF commons, while January owns a real BSS section. Existing
pinned-VC7 repository donors prove that explicit `= FALSE` zero initializers
produce owned BSS without runtime code. The evidence-separated data refinement
therefore changes only storage ownership; all 22 function measurements remain
identical. It is preserved as:

- `build/audit/network_client_manager_lifecycle_data_refined_20260827.obj`;
- raw SHA-256
  `21f5a82637a742934b81d3433d0cd125c0d09c1e590ffc9f29af4568de9ea742`.

## Strict results

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_001141c0` | 49 / 64 | 5 | `e8853839acc4f806c353ac5f3cb83806ceacd68ba81e43707bcb0f0f81b292c7` |
| `_network_game_client_dispose` | 92 / 96 | 9 | `9aed9cd5bdbb95616d092ebd2a4b70fa81786d198bcf95af9b20557eb3451207` |
| `_network_game_client_switch_to_pregame` | 161 / 176 | 10 | `fed35c85bad778b6d5b13347ad68269b80da3692d3854b4e0c72a1ef378f40d9` |
| `_network_game_client_game_shutdown` | 111 / 112 | 11 | `15892344fc2a70095bb7a35de74a88562d87f9d9017895aafee225c8e819793e` |

The BSS is 2/2 bytes strict exact, has zero relocations, alignment one, and
normalized SHA-256
`96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7`.
Direct hardened comparison reports `all_equal: true` for both data owners, all
four new functions, and all eighteen inherited exact functions. The functions
together cover 1,632 padded bytes and 100 ordered relocations, including every
symbolic destination/addend.

The retained object is preserved as
`build/audit/network_client_manager_lifecycle_exact_20260827.obj`, raw SHA-256
`21f5a82637a742934b81d3433d0cd125c0d09c1e590ffc9f29af4568de9ea742`.
The retained source SHA-256 is
`98a940a7cce38ba04a6cf66a546baa1eee8e9bdd528987c11b890726fe4d49b6`;
its Git blob is `98d83ff9d74743e030d96f8613bff657ffc6f8d1`.

## Campaign gates

- full `halobetacache_build` and `libcmt_build`: pass;
- semantic audit: 470 units, 4,395 functions evaluated, 4,250 semantic exact,
  4,310 accepted exact, zero unit errors;
- campaign: 377/833 complete objects, 4,284/11,060 exact functions, and
  514,589/2,198,102 exact code bytes;
- Halo: 275/468 complete objects, 4,117/7,574 exact functions, and
  501,675/1,770,166 exact code bytes;
- matched data increases by two bytes to 1,836,780 campaign-wide and 1,831,588
  in Halo;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass, with only the known unwritable pytest-cache
  warning;
- `git diff --check`: pass;
- no protected source, configuration, semantic ledger, comparator, completion
  label, or parked manifest changed; no push.

## House-rule audit

Every parameter is on its own line. The no-argument helper uses `void` on its
own line, and every value/void function has an explicit terminal return. All
client/game access goes through named typed fields guarded by zero-emission
layout assertions. The retained C89 source contains no assembly, `register`,
`volatile`, barrier, pragma, intrinsic, attribute, raw-address expression,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, byte forcing, object patch, alternate compiler, or acceptance
exception.
