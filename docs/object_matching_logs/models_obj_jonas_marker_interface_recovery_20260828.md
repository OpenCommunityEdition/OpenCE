# `models.obj` marker-interface recovery — 2026-08-28

## Result

This evidence-led recovery starts from integration commit
`a7aa669a24efcf2a97ca02ba1b8e5c459a2153d9`. A natural five-residual HCEA
batch identified one function whose machine code was already exact but whose C
library interface spelling gave the wrong relocation. A separately bounded
typed-interface correction recovers it:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_model_find_marker` | 160 | 3 | `665808659b324263ea5e816125b0a2ae156dc201f183ecb43d478ea310799b50` |

All seven inherited functions remain strict exact. `models.obj` advances from
7/12 and 1,168/5,136 padded bytes to 8/12 and 1,328/5,136 padded bytes. It
remains `NonMatching`.

The retained source Git blob is
`db7e7c380fff55498696085134841ea04ec4d121`. The January split-object
SHA-256 is `2f888351b09fb71c9042e11ff32a88975b7676a9d966d9a0eca6a82609eb6c65`.

## Source provenance

Both existing models ledgers, all reachable source history, the repository
guidance, and the five HCEA donors were read before emission. HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplies readable semantic
oracles:

| Donor | Git blob | Payload SHA-256 |
| --- | --- | --- |
| `src/blam/models/render_model_parts.c` | `a94f1fdab736ec9fac7d84b6954a749b2021dada` | `ffc2ac46ab016b769a93185ad21c1d0dcec51dff96cec2ed8a3381ab920e0b13` |
| `src/blam/models/model_get_node_matrices.c` | `f7ef971e00de7d30d5c63073bd40767e5202daa3` | `8819f876e6166b61099d3fcc0fd74b80b217c711b99a0003c8b08141f0d01468` |
| `src/blam/models/model_node_matrices_from_orientations.c` | `076e735045424b0c86833699312fe50781ab0fc0` | `43eb440492a6a04f8281ea400f2974615e5a6410c6b1fe9f9283ef06340ca35e` |
| `src/blam/models/model_find_marker.c` | `4cd3f6f398405a54c684b63e623539af8b9947a3` | `c7ca69aa765796a0db5915ca8a32fe011c7cb19604d03300d61ab9a2f35d689b` |
| `src/blam/models/render_model.c` | `d70ada4c5d3faa2b89ef380f9d334569cd9bd60d` | `3f6dfc904d91f0d644904626956dc86e673d2e9a090d376144b1abfc85b7d2cf` |

No preserved PC source body for these five functions exists in the audited
Pastudan, Stian, punpckhdq, or bnunu histories. January target bytes,
relocations, assertions, and layouts therefore remain authoritative.

The accepted body performs a typed model tag lookup, binary-searches the
typed marker block with `TAG_BLOCK_GET_ELEMENT`, and compares the requested
name through the repository's canonical `_stricmp` interface. Every parameter
is on its own line and the function ends in an explicit `return NONE;`.

## Frozen five-body batch

The sole code-producing donor batch was preserved as
`build/audit/models_first_batch.obj`, raw SHA-256
`b6f73d3b0827bc1b68933b36de06186d28e3095becb658f0c5587c25538932f4`.
Its compiler-input source has Git blob
`5660528bb038322098fcc00c1bbc0cb74128b92a` and SHA-256
`ae00565c821c6cd3aecca2074f8583eb05581ea79b0c07f2f0654bedb9340388`.

| Function | Target bytes/relocs/hash | Candidate bytes/relocs/hash | Decision |
| --- | --- | --- | --- |
| `_code_00112db0` | 1072 / 32 / `4b40dbfba3d6f83941f9ed1943892110c8eb44b70432089365716fc2fd257b92` | 1072 / 32 / `bccca213ae810352fc41dfca3fdb7d3d2d987caf0adac4842ee0d016103e679b` | source topology residual |
| `_model_get_node_matrices` | 320 / 12 / `157e991f6ed1593209119cae0cf3cbd59018ef79e0bc161aca7ab31c760063b0` | 320 / 12 / `3c5fd454b14e0fed0ef59804a250a93ed4d99b802304a74c376803a7f2700140` | source topology residual |
| `_model_node_matrices_from_orientations` | 272 / 4 / `e4827bacc6a2b0f77c8fc0397d4809954592c82bff4f019644d4720fc2ef7da2` | 272 / 4 / `0093dcfde1bb51aa23162f324c467e81d9b13b4adfe6a4cedcf0c06679a55466` | source topology residual |
| `_model_find_marker` | 160 / 3 / `665808659b324263ea5e816125b0a2ae156dc201f183ecb43d478ea310799b50` | 160 / 3 / same normalized hash | relocation spelling only |
| `_render_model` | 2144 / 98 / `bb666c876f1ce2d0f146ebb9c0cf099dca3fdc1361fddc1097daa8a95233d3e4` | 832 / 34 / `f4525e17b19fbf1a09ad02121eeeb7c764a9aeaf1d121b0f67eca5306b40140d` | later donor lacks January debug block |

All five donor bodies were removed after that gate. The four structural
rejections were not retuned. Reopen them only with authentic January
source/debug-scope evidence; in particular, the 1,312-byte render gap is not a
local compiler-scheduling tie.

## Typed-interface correction

The first candidate's marker body had identical padded bytes and the same three
relocation addresses and types, but called C identifier `stricmp`, producing
COFF destination `_stricmp`. January instead relocates to `__stricmp`. Current
repository source consistently uses the declared C identifier `_stricmp`, whose
ordinary C decoration is `__stricmp`.

A new isolated one-function wave changed only that interface call. Its first
code-producing compile and retained replay are strict exact. Ordered relocation
destinations are:

| Offset | Destination |
| ---: | --- |
| 33 | `_tag_get` |
| 103 | `_tag_block_get_element_with_size` |
| 113 | `__stricmp` |

The frozen first object SHA-256 is
`f14e9ea718bc07c6544f778946ea11b60451af573977487284f48974a566891c`.
The retained replay object SHA-256 is
`52fa6be75ba731ac7d0f1a4eadb4ff71352589cf31134cc28e49cf875eb7ed02`.

## Policy and validation

The retained source is ordinary typed C. It uses the subsystem tag-block macro
and no raw tag/object casts. It contains no assembly, `volatile`, `register`,
pragma, intrinsic, optimizer barrier, raw offset/address access, pointer
reconstruction, punning, undefined behavior, synthetic anchor, forced
emission, object-byte patch, comparator exception, configuration change, or
alternate compiler flag.

The isolated first gate and retained replay both reported 8 exact, zero
residual, and four unwritten functions. The cumulative normal Ninja build and
`python tools/campaign/gate.py source/models/models --all` reproduce that
result. `git diff --check` is clean. Only `source/models/models.c` and this
Jonas-owned ledger change; Units, Vehicles, Claude-active source, shared
headers, storage, configuration, policy, parked records, and object status are
untouched.
