# `lightning.obj` complete-donor atomic one-shot (2026-08-28)

## Result

This lane tested the only remaining two code owners in
`source/objects/widgets/lightning.obj` as one inseparable same-TU cluster:

- private `_code_00124c70`;
- public `_lightning_submit`.

The first and only code-producing candidate compile was not strict-exact, so
the complete pair and all typed support were pruned without tuning. Production
`lightning.c` and `lightning.h` are byte-for-byte back at pushed base
`83883cc2e9dc7619b76068d6c86eb01b8ba735d6`. The object remains **7/9 exact**;
no code, data, semantic, parked, or object-completion credit is claimed.

| owner | target meaningful / padded | target relocs | candidate padded | candidate relocs | candidate normalized SHA-256 | result |
|---|---:|---:|---:|---:|---|---|
| `_code_00124c70` | 237 / 240 | 22 | 240 | 22 | `f31a3152a95637d3f7e9f1bbada7d9c01bbcb9a88193ccf7c741c3fde5e3dbaa` | residual; target `2fe97798519acba821968a88f76c5a3ac64a62c86a9919327b6ed89c8599f17d` |
| `_lightning_submit` | 2571 / 2576 | 80 | 2512 | 81 | `e03280899a3ed771bf2ada305b4729aedd25a945285852312eda06ae795cdf2b` | residual; 2,285 normalized byte positions differ over the padded comparison |

The helper reproduced the target's size, all 22 relocation addresses, types,
destinations, and addends. It returned exactly to the previously measured
two-independent-scheduler-swap fixed point. The complete caller therefore did
not naturally alter either private-ABI scheduling choice.

The submit body was structurally close enough to reproduce every intended
subsystem call, the 0x24024-byte point clear, the 0x24144 stack frame family,
the private ESI/EBX/EDI helper calls, and the target assertion sites. It was
still 64 padded bytes short and carried one extra relocation. This is a broad
code-generation residual, not an admissible compiler tie.

## Frozen base and authenticated objects

- branch: `jonas/lightning-complete-donor-20260828`;
- base: `83883cc2e9dc7619b76068d6c86eb01b8ba735d6`;
- January target `build/split/source/objects/widgets/lightning.obj`:
  SHA-256 `85AB5029070191A1C1B22FA398E56EC71932EB7D96E34B099C68DEB50EA3058F`,
  8,435 bytes;
- final restored base object:
  SHA-256 `F7344FB6E5B2872EA1564EB2D45B32B6B75C1A72BD44CECEF18C237697C99401`,
  3,068 bytes;
- compiler `xbox/bin/vc7/CL.Exe` 13.00.9254.1:
  SHA-256 `483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`;
- restored source Git blobs:
  `source/objects/widgets/lightning.c` `025a878044bceba191170fa6fc2d2828705785e5`,
  `source/objects/widgets/lightning.h` `e48069ad6d41d7c8850b99ca10a5ea5e88d1b3cb`.

## Evidence read before emission

Campaign and ownership rules were read in full:

- `docs/matching_methodology.md`;
- `docs/exact_match_acceleration_playbook.md`;
- `docs/shape_transfer_executor_strategy.md`;
- `docs/object_matching_logs/README.md`;
- `tools/campaign/README.md`;
- `HALO_DECOMP_MIGRATION_HANDOFF.md`;
- `KIMI_3_1_HALO_DECOMP_MASTER_PROMPT.md`;
- `PROJECT_INTEGRITY_BLOCKERS.md`.

The complete lightning history and previous fixed-point ledger were read,
including every patch touching `lightning.c` and
`docs/object_matching_logs/lightning_obj_jonas_offset_helper_fixed_point_20260826.md`.
The preserved wave-1 and wave-2 helper sources/objects under
`outputs/lightning_offset_helper_evidence_20260826/` were inspected rather
than reconstructed from memory.

Complete later-build bodies were read from:

- HaloCEA `lightning_submit.c`, `lightning_offset_marker_position.c`, and all
  referenced tag/runtime headers in
  `research-cache/halocea-cseries-20260820`;
- Stian full-history `src/halo/objects/objects.c` in
  `research-cache/stian-halo-full-history-20260828`;
- Pastudan full-history `src/halo/objects/objects.c` in
  `research-cache/pastudan-halo-full-history-20260828`.

Applicable donor `CLAUDE.md`/`AGENTS.md` files were read in full. The current
checkout has no such file. The Stian and Pastudan guidance confirms that later
build source is a hypothesis generator only, C89/source clarity is required,
and January COFF remains authoritative.

The current tree, the previous lightning helper tree, the Fable cross-build
bridge, and active Claude work were inspected read-only. No tracked alternate
complete lightning body existed and no Claude-active file was changed.

## January authority and typed packet

The January target has nine code COMDATs, eight target-owned string COMDATs,
and one four-byte BSS owner. The two residual code sections are:

- `_code_00124c70`: 240 padded bytes, 22 relocations;
- `_lightning_submit`: 2,576 padded bytes, 80 relocations.

The target submit owns a 0x24144-byte frame and clears 0x24024 bytes, proving
4,097 elements of a typed 0x24-byte intermediate point. Target marker and tag
layouts prove:

- marker definition 0xE4;
- random-position bounds at +0x74;
- jitter/thickness/tint at +0x80/+0x84/+0x88;
- lightning marker block at +0x98;
- shader block at +0xA4;
- object marker matrix at +0x38 and position at +0x60;
- dynamic-unlit vertex size 0x18.

The frozen candidate used only ordinary typed C and public declarations. It
implemented the complete marker walk, recursive midpoint subdivision,
camera-facing cross products, random jitter, point-valid assertions, animation
value/color modulation, dynamic vertex emission, bounds centroid, shader
selection, draw, and cleanup. The private helper remained `static` and was
called naturally twice from the same full submit body. No asm, `volatile`,
register declaration, pragma, intrinsic, barrier, raw address, pointer/integer
conversion, aliasing trick, UB, synthetic anchor, or comparator exception was
used.

## Data and ownership result

The candidate naturally emitted all intended assertion literals, but its
stringified point assertions preserved source whitespace and therefore became
49/30/49-byte COMDATs instead of January's 47/28/47-byte literals. That alone
prevents strict target-owned data admission. The shared `"vertices"`,
`"position"`, and `"matrix"` literals remained compiler COMDAT/shared-symbol
forms rather than new runtime ownership claims. No new `.data`, `.bss`, COMMON,
or persistent runtime owner was introduced.

Because both code owners were residual, the atomic rule required pruning even
before any data-owner adjudication. No expression-whitespace correction or
other second candidate was compiled.

## One-shot and final verification

An XDK `/Zs` parse-only pass succeeded with no diagnostics before emission.
Exactly one candidate object edge was then executed:

`CL build\\base\\source\\objects\\widgets\\lightning.obj`

The strict comparison above was performed immediately. After rejection, the
entire pair and support types were removed, and the ordinary production edge
rebuilt the restored baseline once. Final verification established:

- all seven inherited public owners strict-exact with zero regression;
- restored source Git hashes exactly equal `HEAD`;
- `git diff --check` clean;
- 205 tooling unit tests passed;
- semantic audit: 470 units, 4,699 functions, 4,570 semantic exact, 130 hidden
  exact, 0 unit errors, 0 ordinary rejected;
- admission audit: 0 candidates, 0 contradicted, 0 revoked;
- parked audit: 13 active, 0 stale, 0 invalid.

The semantic/admission/parked whole-tree audits used the already-built clean
`83883cc2` integration report in the read-only coordinator worktree. The
fresh lane independently rebuilt and compared `lightning.obj` itself.

## Do not repeat

Do not repeat this complete typed Stian/HaloCEA adaptation, the 4,097-point
scratch topology, compound-versus-explicit helper addition, or a partial/full
caller context solely to perturb the helper. The full caller left the helper at
the exact prior hash and did not cure its two scheduler swaps.

A future attempt needs genuinely new January-specific provenance for the
remaining submit control-flow/scheduling gap. Correcting the three stringified
assert spellings is known bookkeeping, but it cannot justify another compile
by itself because the code body is still broadly residual.

## Handoff

The isolated worktree is intentionally uncommitted and unpushed. Its only
tracked change is this ledger; production source is clean at the pushed base.
