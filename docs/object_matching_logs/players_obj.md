# `players.obj` strict-closeout strategy log

## Baseline

- Lane: `codex/players-closeout-root` at `caa820ccd` (`0892e393b`,
  `86a73bc55`, `caa820ccd`).
- Toolchain and flags: repository XDK 3911 CL, unchanged `/O2 /Oy- /DDEBUG
  /Dxbox`.
- Strict comparator: `tools/coff_compare.section_infos_equal` against
  `build/split/source/game/players.obj`.
- Current result: **106/110 sections exact** and 16,924/21,164 padded section
  bytes exact.
  - Functions: **66/70**, 11,664/15,904 padded bytes.
  - Data: **40/40**, 5,260/5,260 padded bytes.
- `config/config.json` remains `NonMatching`.

| Residual | Target size/relocs | Target normalized SHA-256 | Current normalized SHA-256 | First meaningful divergence |
|---|---:|---|---|---|
| `_code_000a9ff0` | `0x170` / 22 | `9e3ab79a5087c8d11b97f52c30afc3f596faa4aa6122480dc9f16d247be8ca9f` | `d9ca587214fd10308b7a16a797d8a154102f2095fbf28324cb9e3e2da8d8483e` | `+0x2E`: January forms `&iterator` before homing the activation result; current CL homes EAX first. |
| `_code_000aa9e0` | `0x510` / 61 | `8bb48b429649a3afcd41cf238ca9b1d5dba0ff61e15f2205f72512918cad7c43` | `740c065b9b622d07827e6ae83ac2d79aeb3813a54bbf7b8a43c7c0c8f0e8b36f` | `+0x5`: January uses a `0x7C` frame; current CL uses `0x78`, then chooses a different local/register topology. |
| `_players_update_before_game` | `0x6E0` / 101 | `8c92baaaebf7177d7bc54239c21b0923703a50f3ede19dca3a81a457f2860947` | `51c2c7b42d12188a024fac2a2c5d030c194d6a91be0b4e3d57bdd5efe1dd1c8c` | `+0x62F`: four register-encoding bytes; January uses EDX for the unit index and EAX for the next iterator address. |
| `_players_update_after_game` | `0x330` / 52 | `af64f40efdb1be0cd2754ada85b34acecdd4480a39c73b5a9ef2938c2c1d315b` | `6c0d80f1ab3c18aa8447682e0e9e7365486c42df02072c1a24c729a54306d7b7` | `+0x74` branch displacement; the causal difference begins at `+0x18C`, where January assigns EDI to the BSP loop index and EBX to the element pointer, opposite current CL. |

## Bounded source experiments

All experiments retained the ABI and flags and were rebuilt as the single
`build/base/source/game/players.obj` target. Unless marked accepted, the source
was reverted after measurement.

| Function | Evidence-based source shape | Strict outcome | Status |
|---|---|---|---|
| `_code_000a9ff0` | Fold activation-result assignment into `data_iterator_new`'s second argument with a comma expression. | `0x170` / 22; canonicalized to current hash `d9ca5872...`; same `+0x2E` scheduling window. | Reverted |
| `_code_000aa9e0` | Split the unit-index/biped aggregate into independent typed locals. | `0x510` / 61; hash `0d30bbd7f96442016f38bdfc9c200999b37e61291f2c7d401786153f876f2b3b`; frame remained `0x78`, unit and biped homes moved to `-0x10`/`-0x14`, not January's `-0x14`/`-0x10`. | Reverted |
| `_code_000aa9e0` | Extend the placement aggregate with the source-root pointer to model January's three adjacent homes. | `0x510` / 61; hash `e79a85eafa5ab7123747724dc7b241a69307a6631c7a103edc09e4532161389a`; recovered the `0x7C` frame but placed the aggregate at `-0x30`, first differing home at `+0x24`. | Reverted |
| `_code_000aa9e0` | Wrap only the source-root pointer in a one-field struct, then in a one-element typed array. | Both scalarized to current `0x510` / 61 hash `740c065b...`; no stack home or code change. | Reverted |
| `_code_000aa9e0` | Move the three-field placement aggregate after the matrix/vector locals. | Canonicalized to the same `0x510` / 61 hash `e79a85e...`; declaration position did not move the `-0x30` aggregate home. | Reverted |
| `_code_000aa9e0` | Group the source-root and later source-biped pointers, then take a non-volatile typed address of the mutable source-unit parameter. | Both scalarized to current `0x510` / 61 hash `740c065b...`; neither forced January's `-0xC` home. | Reverted |
| `_code_000aa9e0` | Materialize the initial `biped_get` result through a typed temporary before copying it to the placement state. | Copy-before-result canonicalized to `740c065b...`; result-before-copy moved `xor bl,bl` ahead of the call and produced hash `11d6457990632220105232975d079705b646a90e8f14c0bcb8369377236a0fa8`. | Reverted |
| `_players_update_before_game` | Move `_ReadWriteBarrier()` before the local unit-index assignment. | `0x6E0` / 101; hash `af6add6c8b40f7ae93e5853584e2f075ce3a52c88707cddefbe94c5a6543cbf4`; regressed from four to ten normalized bytes, first at `+0x62B`. | Reverted |
| `_players_update_before_game` | Make the local unit index `const`. | Canonicalized to current `0x6E0` / 101 hash `51c2c7b4...`; same four bytes. | Reverted |
| `_players_update_after_game` | Move the loop index and element-pointer declarations across inner/function scopes and reorder declarations. | Every form canonicalized to current `0x330` / 52 hash `6c0d80f1...`; same EBX/EDI mirror. | Reverted |
| `_players_update_after_game` | Remove the barrier after `global_scenario_get`. | `0x330` / 52; hash `20ac0c082a00bbe619f3f3c868fbc6c6933177c5ba04d1ad46ae3613be37f18d`; regressed to the base-copy/add scenario-block expression and 373 normalized bytes. | Reverted |
| `_players_update_after_game` | Move that barrier after the scenario block-address assignment. | `0x330` / 52; hash `0f8682e9333c2a6c9cdf18fae3a4a77cc7d36868cd0badb9069bec177c635c2b`; retained a non-target copy/add sequence beginning at `+0x184`. | Reverted |

No source experiment improved a strict section in this closeout pass. The
production source is restored to `caa820ccd`.

## Park decisions and next leverage

- `_code_000a9ff0` is parked as methodology class D (scheduler/lifetime).
  Twenty-one of 22 relocation addresses are exact; only `player_data` moves
  three bytes inside the `+0x2E..+0x39` scheduling window, and all following
  instructions reconverge. Reopen only with provenance for the activation
  result/iterator statement shape.
- `_players_update_before_game` is parked as class C (register fixed point).
  All 101 relocation identities **and addresses** are exact. The only
  differences are four ModRM/register bytes at the final `unit_control` and
  next-iterator sequence. Reopen only from a caller/compiler forced-register
  anchor.
- `_players_update_after_game` is parked as class C/D. All 52 relocation
  identities agree; 33 addresses are exact and the remaining addresses form
  two uniform drift plateaus (`+1` for six, then `-5` for thirteen) after the
  EBX/EDI loop mirror. Reopen only with source provenance that changes the loop
  index/element live-range weights while retaining the required post-scenario
  barrier shape.
- `_code_000aa9e0` is parked as an unresolved class A/C boundary, **not**
  claimed as a proven compiler-only tie. All 61 relocation identities and the
  `0x510` padded size agree, but only 20 relocation addresses are exact and the
  target's opening `0x7C` frame/local homes remain unrecovered. January homes
  `player`, `player_unit_index`, `biped`, and `source_root_object` at `-0x8`,
  `-0x14`, `-0x10`, and `-0xC`; current CL uses a `0x78` frame and keeps the
  source-root pointer in ESI. Reopen only with new donor/PDB/local-variable
  provenance or a legal-C source form that recovers all four homes; more
  declaration/aggregate permutations are not evidence-based.
- A strict completion still requires all four current hashes to equal their
  target hashes plus a fresh 110-section sweep. Equal size/relocation count is
  not match credit.

## Do not repeat

- `_code_000a9ff0`: explicit iterator pointer, result temporary, result/iterator
  aggregate, comma-expression assignment, or compiler barrier around the
  activation call. These retain the same store/load/LEA schedule or regress it.
- `_players_update_before_game`: direct `player->unit_index`, barrier removal or
  relocation, `const`/`register`, pointer aliases, and declaration-order
  changes. The four-byte baseline is best.
- `_players_update_after_game`: barrier removal/relocation, declaration-scope
  permutations, `register` hints, and equivalent `for` declaration placement.
  Keep the barrier immediately after `global_scenario_get`.
- `_code_000aa9e0`: independent unit/biped locals, a three-field placement
  aggregate in either declaration region, one-/two-field source-pointer
  wrappers, a non-volatile source-parameter alias, an initial biped temporary,
  declaration reordering, and volatile pointer/parameter access. Volatile
  forms are also barred by current methodology.
- Do not change optimization flags, add inline assembly/object patches, accept
  fuzzy objdiff percentages, or mark the TU `Matching` before the hardened
  110-section gate passes.

## 2026-07-29 isolated follow-up

The collision-safe follow-up is preserved on `codex/players-final-four` as
source checkpoint `576d93f33` and full ledger commit `8dad10254`. The source
checkpoint is deliberately **not integrated here** because `players.obj`
remains incomplete; these measurements prevent later lanes from repeating the
same work.

- P01: rewriting the final iterator loop from `while` to explicit `for`
  produced no bytes and was reverted.
- P02: a Players-only `long` declaration for
  `objects_get_activating_cluster_index` modeled January's measured cross-TU
  return-ABI bug and reduced `_code_000a9ff0` from 287 to 11 normalized
  differing bytes. The lane labels the preserved bug and documents the
  corrected-build fix.
- P03: moving the activation-result declaration before the iterator had no
  effect and was reverted.
- P04: the HCEA-PDB-backed `player_teleport_internal` local topology retained
  `0x510`/61 and reduced `_code_000aa9e0` to 929 normalized differing bytes.
- P05: restoring the scenario barrier, two BSP counters, and measured loop
  topology retained `0x330`/52 and reduced
  `_players_update_after_game` to 222 normalized differing bytes.
- P06/P07: the historical 877-byte teleport candidate was reproduced, but it
  depended on an unproven `volatile` pointer-store expression used only as a
  codegen lever. It was rejected under campaign policy; the clean PDB-backed
  P04 form was restored.
- P08: recovered field/flag names and compile-time January layout assertions
  were byte-neutral across the whole TU.

Final isolated-lane gate: **106/110 sections**, **66/70 functions**, and
**40/40 data sections** exact. The four residuals retain equal padded size and
relocation count but receive no exact credit:

| Function | Size / relocs | Candidate SHA-256 | Differing bytes |
| --- | ---: | --- | ---: |
| `_code_000a9ff0` | `0x170` / 22 | `d9ca587214fd10308b7a16a797d8a154102f2095fbf28324cb9e3e2da8d8483e` | 11 |
| `_code_000aa9e0` | `0x510` / 61 | `d833e6d8f06cfdb556417d852c6a0717eecfdb2726f11a92d9241c15b363bbb0` | 929 |
| `_players_update_after_game` | `0x330` / 52 | `6c0d80f1ab3c18aa8447682e0e9e7365486c42df02072c1a24c729a54306d7b7` | 222 |
| `_players_update_before_game` | `0x6E0` / 101 | `51c2c7b42d12188a024fac2a2c5d030c194d6a91be0b4e3d57bdd5efe1dd1c8c` | 4 |

### P20 compiler-provenance and patch-level falsification

The January `cachebeta.pdb` module stream for
`\halo\objects\halobetacache\players.obj` contains a 60-byte
`S_COMPILE2_ST` record. Its front-end and back-end version triples are both
`13.0.9254`, followed by the producer string
`Microsoft (R) Optimizing Compiler`. This is target-authored evidence that the
TU was compiled with the same numbered VC7 build used by the campaign.

The unchanged retained source was then compiled once under the only two
distinct local VC7 compiler binaries:

| Compiler | `_code_000a9ff0` | `_code_000aa9e0` | `_players_update_before_game` | `_players_update_after_game` |
|---|---|---|---|---|
| `13.00.9254.1` | `d9ca5872...` | `d833e6d8...` | `51c2c7b4...` | `6c0d80f1...` |
| `13.00.9210` | `d9ca5872...` | `d833e6d8...` | `51c2c7b4...` | `6c0d80f1...` |

All four normalized candidate hashes were byte-identical across compiler
binaries. The older 9210 compiler therefore supplies no code-generation
control for this TU, while the target record materially weakens the hypothesis
that an unidentified compiler build explains the residuals.

Disposition: no source retained and no exact credit. Do not repeat a local
compiler census or download nominally different 3911 kits without first
proving that their `CL.Exe` hash is new. The remaining reopen conditions are
authentic January/cross-build statement topology or a focused legal-C control
that changes the measured register/lifetime window without moving any exact
relocation.
