# `ima_adpcm.obj` Jonas complete-object wave (2026-08-25)

## Scope and frozen provenance

This bounded source-only wave starts from cumulative integration commit
`eeade5e51b0886b16ad83713df7cc63d16a28552` in the isolated worktree/branch
`jonas/ima-adpcm-complete-20260825`. Its only permitted production source is
`source/sound/ima_adpcm.c`; the only other permitted tracked file is this new
ledger. The fixed candidate set contains exactly the compressor initialization
order E01 and decompressor result-lifetime order E02 below. There is one normal
code-producing compile for the pair, with no spelling, tuning, retry, compiler
switch, or alternate candidate after that emission.

The immutable January executable is `cachebeta.exe`, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
The split target produced by pinned csplit has phase-local raw SHA-256
`2d0ce87b2d0342a01359022b4719f9bb8825375083919c53c0b671dbe4099fa1`.
Strict authority is the section fingerprint, not a whole-object digest. The
compiler is XDK 3911 `CL.Exe` 13.00.9254.1, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
using the unchanged repository edge `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` and
the generated include set. The pinned tools are csplit SHA-256
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
and objdiff-cli SHA-256
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

The baseline source blob is
`66244dc35af5020848642afb014d8f7939305d85`. The source-identical cumulative
baseline object has phase-local raw SHA-256
`fc4a75898d925f5a9f5e8b582957d7df885ff82227ce8da39a5b678cced500dc`.
That digest is recorded only as an artifact sentinel because compiler debug
paths and timestamps make whole-object hashes non-portable across worktrees.

## Validated baseline

The translation unit owns three functions. `_byte_swap_bungie_ima_adpcm_header`
is inherited strict exact; the compressor and decompressor are nonexact. All
runtime non-code ownership is already strict: the 24-byte header-name string,
the combined 420-byte step tables, and the 44-byte writable byte-swap
definition are equal in size, normalized bytes, flags, ownership, and ordered
relocations. Compiler-only `.drectve` and `.debug$S` sections are not runtime
ownership.

| Function | Target meaningful / padded | Baseline meaningful / padded | Relocs target / baseline | Target normalized SHA-256 | Baseline normalized SHA-256 | First true divergence |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `_compress_ima_adpcm_audio_data` | `346 / 352` | `346 / 352` | `2 / 2` | `97e166836a7a6f05810477e6d722d209a4c07f5fb2e604756915f8b3de58fff3` | `9ef202fbd4db1aced4543636bb9f85efdcaca79b0edbb7b8d912535d3fc6eaf6` | In the second reconstruction pass January emits `mov esi,edi; mov ecx,esi; sar ecx,3`; baseline emits `mov ecx,edi; sar ecx,3; mov esi,edi`. The only meaningful residual is this three-instruction/six-byte order. |
| `_decompress_ima_adpcm_audio_data` | `404 / 416` | `410 / 416` | `2 / 2` | `c308b60091d47921aeb30f63188ec5317c64aa9fa52b9e53721774ecd34176f9` | `92f27a5e493fd0e401a7ee0c7c823e3040bddfdd6100cccb5b4f673be630d882` | January stores decremented `input_data_size` before materializing `sample_count * 2`; baseline materializes the result first. This shifts the two table relocations from target `+178/+275` to baseline `+181/+277`. |
| `_byte_swap_bungie_ima_adpcm_header` | `24 / 32` | `24 / 32` | `2 / 2` | `0831428713702f318c38875175e23ba63701439c51add602977f7e890ca3a832` | same | none; inherited strict exact |

The compressor relocation addresses are already exact at `+108` and `+219`,
both `IMAGE_REL_I386_DIR32`, to `_step_size_table + 0` and
`_step_size_adjustment_table + 0`. The decompressor target uses the same
destinations at `+178` and `+275`. The byte-swap target/candidate relocations
are exact at `+10` to `_data_00316a7c + 24` and `+15` REL32 to
`_byte_swap_data + 0`.

## Claude evidence and its explicit limit

Claude's nearest-object documentation was read from
`claude/near-complete-objects-20260816` at
`74a085e90ae66511c22820a8f5256b3273342542`. In
`research/register_rename_census.md` it records exactly:
`sound/ima_adpcm compress_ima_adpcm_audio_data 3/141 (11 levers already inert)`.
The same 4.0%, 768-byte/two-function frontier is repeated in
`research/PROMPT_finish_hs.md`, `research/PROMPT_next_session.md`, and
`research/residual_frontier.json`; duplicate frontier copies exist on the
reachable Claude finish-HS, libcmt-stream, and untried-objects branches.
Generic register/scheduler guidance from
`research/register_tie_cumulative_findings.md` and
`research/register_tie_reachability.md` establishes that ordinary statement,
initializer, and live-range order can move allocation-stage choices and that
the real translation unit must be measured in situ.

Claude records that eleven levers were inert, but does not preserve their
source spellings or fingerprints; this ledger does not manufacture that
matrix. The frozen candidates are new against every reachable
source/blob/artifact, not proof against erased undocumented experiments.
All-history `-S`/`-G` searches for the exact compressor chain and uninitialized
`long result;` form are empty. The wider workspace census examined 1,317 source
copies in 12 content clusters and found no exact frozen-form hit.

## Fixed one-shot candidate matrix

| ID | Evidence-backed source shape | Expected target | Production result | Decision |
| --- | --- | --- | --- | --- |
| E01 | In the compressor's second reconstruction pass, assign `temporary_step_size = step_size;`, then `sample_difference = temporary_step_size;`, then shift `sample_difference`, before `mask = 4;`. | `346 / 352`, two relocations at `+108/+219`, normalized SHA-256 `97e16683...` | `339 / 352`, two relocations at `+108/+212`, normalized SHA-256 `0aab3da271db0e4ba7ab2e396dc3a497fefad58d71922dacb73abe22e7086bc3` | **rejected and reverted** |
| E02 | Declare `long result;` and assign `result = sample_count * sizeof(short);` immediately after subtracting the eight-byte header from `input_data_size`. | `404 / 416`, two relocations at `+178/+275`, normalized SHA-256 `c308b600...` | `410 / 416`, two relocations at `+181/+277`, normalized SHA-256 `fca3e783c192a308481636f6f8c3e0028de819cdd8e47bf86b240ed0175cea1a` | **rejected and reverted** |

Both are readable defined C and preserve behavior. E01 gives the temporary the
same value used by the following shift, exposing January's dependency chain
without a scheduling device. E02 begins the result's live range at its first
semantic need, after the independent input-size update. No function signature,
header, table, storage owner, branch, arithmetic width, or call changes.

## Admission and do-not-repeat boundary

Acceptance requires exact padded bytes, normalized bytes, relocation count,
address, type, destination and addend for all three functions, plus exact
runtime data/storage ownership. A complete result may change
`source/sound/ima_adpcm.c` from `NonMatching` to `Matching` only after those
checks, the whole-TU regression gate, full Halo/libcmt builds, semantic and
progress reports, object-admission and parked audits, and all 179 tooling tests
pass. If only one body is exact, retention is allowed only when its independent
function COMDAT plus unchanged data/sibling evidence proves the post-revert
state without recompilation; otherwise the packet fails closed.

Do not repeat compiler-equivalent spelling changes, manufacture Claude's
missing eleven-row matrix, or use assembly, volatility, `register`, pragmas,
intrinsics/barriers, attributes, raw offsets/addresses, pointer or union puns,
undefined behavior, synthetic anchors, flag/tool substitution, object-byte
patches, comparator exceptions, or fuzzy-only credit. Reopen a failed form only
with preserved original-source provenance or a genuinely new ordinary-C
topology supported by the target's first divergence.

## One-shot result and disposition

The syntax-only XDK pass succeeded and emitted no object. Exactly one normal
Ninja production edge was then invoked for the complete frozen pair. Its
first-shot object was preserved unchanged at the ignored audit path with raw
SHA-256
`e832ca8aae3f5710aff6c3a72e102d36b42cf04e0a3d5e2fc329fc5166d9081f`.

Neither candidate reached strict equality:

- E01 shortened the compressor by seven meaningful bytes and moved its second
  table relocation seven bytes early. The first relocation stayed at `+108`,
  but the candidate hash and `+212` second address both fail the target.
- E02 changed the decompressor's normalized body but did not move either
  downstream table relocation or reduce its meaningful size. It therefore
  remains structurally different from the target at `+181/+277`.
- `_byte_swap_bungie_ima_adpcm_header` stayed strict exact at `24 / 32`, two
  relocations, and normalized SHA-256 `08314287...`.
- All three runtime non-code sections stayed strict exact: 24-byte header-name
  string SHA-256 `2da27a13...`; 420-byte combined step tables SHA-256
  `d397f34f...`; and 44-byte writable byte-swap definition SHA-256
  `69aafaeb...`, including both data relocations.

Both source edits were reverted wholesale through the exact inverse patch,
with no second compile, alternate spelling, tuning, or partial retention. The
working source again hashes to baseline blob
`66244dc35af5020848642afb014d8f7939305d85`. Because no implementation or
metadata change survives, there is no implementation credit, no `Matching`
admission, no progress gain, and no committed-state implementation replay.
`ima_adpcm.obj` remains one inherited exact function out of three, with exact
runtime data and the two original residuals. This documentation-only closeout
is the complete record of the bounded wave.
