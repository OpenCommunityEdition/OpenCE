# Main vblank callback: genuine schema boundary (2026-09-05)

## Decision

Starting at published canonical `d00a3788d551d7187b213ab757f77494beeb40d3`,
the first natural two-file prerequisite packet was **withheld and fully
restored**. It regressed Decals' `decals_delete_permanent_from_cluster`:
392 meaningful / 400 padded bytes. Main's callback was never compiled.
No qualifier, source-order, header-position or layout variants were tried.

Canonical remains 907,050 meaningful matched code bytes, 6,174 credited
functions, 2,031,312 matched data bytes and 391/833 Matching objects.
The restored whole-tree sweep retains all 6,220 strict owners out of 8,245.
This research adds no exact/fuzzy credit and does not certify Main complete.

## Why the old body can be reopened, but not copied wholesale

The source-bearing historical donor is
`f50be1825f6291c8837c7dcaba078d983bf9e8a5:source/main/main.c`,
blob `4abe5197a1bdd3062e6715fb085c87f87ec6aa6e`.
The public-nine salvage ledger records its callback as 192 padded bytes /
26 relocations / normalized SHA-256
`a521a165a21cccf94e131328067d26ea586ba9d1a3297a59ded61f7d72dd31c8`.
The old supporting anonymous union/split-state facade was not acceptable.

The earlier genuine vblank64 schema packet already established one volatile
signed qword `vertical_blank_index` at rasterizer+0x28, not two counters.
Current Main also already has the genuine callback declaration, registration
through `rasterizer_set_vblank_callback`, volatile flip-count pointee,
short ring index at +0x3FC and fifteen short entries at +0x3FE.
However, current rasterizer schema still reserves +0x20 and +0x30, which
the callback actually needs. This is not an existing-layout-ready body.

Root and independent review read the entire January callback. It increments
the qword at +0x28; a null flip pointer copies it to +0x30. A changed flip
records the low-16-bit counter difference, advances the ring modulo15,
copies the qword snapshot and saves the pointed-to flip count at +0x20.
Every branch finishes through the genuine input-vblank API.

The full833-target direct-relocation census finds exactly two +0x20 references,
and three each at +0x30/+0x34, all in this callback. This census describes
direct symbol references, not a proof that aliases can never access storage.
Later HCEA release PDB `rasterizer_globals_struct` authenticates
`unsigned long flip_index` at +0x20. Its platform-dependent layout does not
authenticate the January qword at +0x30.

`previous_vertical_blank_index` is a descriptive recovered name. The paired
January copies establish qword storage. Volatility is an explicit inference:
+0x4A reads the otherwise discarded high dword at +0x34 before narrowing
the snapshot to a short, mirroring the authenticated volatile current
counter's high read at +0x58. ISR context alone does not prove volatility.
No raw half-word facade, representation pun or compiler barrier was proposed.

## Exact prerequisite attempted once

In the real `source/rasterizer/rasterizer.h` owner:

- replace `byte reserved20[8]` by `unsigned long flip_index` and
  `byte reserved24[4]`;
- retain the existing volatile signed qword at +0x28;
- replace `byte reserved30[13]` by
  `volatile __int64 previous_vertical_blank_index` and `byte reserved38[5]`.

The corresponding initializer in `source/rasterizer/rasterizer.c` used
ordinary zeros for the recovered scalar fields and retained byte3 at +0x39.
No typedef/assert, other declaration, function body or compiler flag changed.

All46 valid Ninja dependencies were frozen before, after and restored.
The two-file patch preserved the entire244-byte Rasterizer initialized
section, all data owners and zero relocations; globals still occupy0x68
and debug-options still begin at0x68. Existing layout assertions passed.
The dependent runtime audit nevertheless found:

- 3,503 unchanged sections;
- 18 compiler-local-label-spelling-only sections;
- one real instruction-order change in Decals.

The Decals function retains400 padded bytes and all29 ordered relocations.
At +0x13D/+0x140, the loads `mov esi,[ebp-4]` and `mov ebx,[ebp+8]`
exchange order, changing four bytes. The before function is January-exact.
This is an inherited strict regression, not an accounting exception.

Root reversed the exact packet. All3,522 frozen sections across all46
consumers are now unchanged, with no owner, COMMON or new
`point_from_line3d` definition. The full stable sweep reports zero gains
and zero regressions. Git-normalized content is restored; no claim of
raw source newline identity is made.

## Target-first behavior evidence retained

The independently written harness executes the genuine public cdecl
callback and models only its real input-vblank outside API. Root read its
complete375 lines and reran the target. All74 scenarios /98 callback
invocations pass: null/unchanged/changed flip, short sign/truncation
boundaries, low32 carry, valid near-signed64-limit counters, flip wrap,
ring0/14/full cycles and pointer transitions.

It checks complete guarded global pages, allowed reads/writes, one input
call after updates, cdecl stack behavior, nonvolatile registers and DF.
Valid domain excludes hardware races, invalid ring indices and signed
counter overflow. It does not require candidate tail-jump shape.
No candidate result exists and this is not candidate acceptance.

## Reproduction artifacts and next boundary

Scratch evidence remains local, not published target binaries:

- `scratch/main_vblank_snapshot_schema_capture_20260905.py`;
- `scratch/main-vblank-snapshot-schema-{before,after,restored}-20260905.json`;
- `scratch/main-vblank-schema-root-runtime-audit-20260905/`;
- `scratch/main-vblank-schema-restored-root-runtime-audit-20260905/`;
- `scratch/main-vblank-schema-{stable,restored-stable}-20260905.json`;
- `scratch/main_vblank_rasterizer_globals_xrefs_20260905.{py,json}`;
- `scratch/main_vertical_blank_behavior_20260905.py`, SHA-256
  `6d9d1473a906b7d5db451be3484394070dd6a4cd575e9b7077e0d7df6a18f5eb`;
- `scratch/main_vertical_blank_target_behavior_review_20260905.md`;
- root rerun `scratch/main-vertical-blank-root-target-behavior-20260905.json`.

The generic comparator performs all-section, symbol, COMDAT, ordered
relocation and storage checks, including nine synthetic negative checks.
Future work must resolve the genuine dependency boundary; do not replay
this same failed packet or pursue declaration-order matching.
The new active Fable tree and implementations remain unread and unmodified.
Reconciliation continued to the older RenderDebug cache/string caller graph.
