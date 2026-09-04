# `files_windows.obj` small-family wave — 2026-09-04

> Integrated verification: after the orchestrator added the independently
> proven `_find_files_globals` symbol at 3,230,952 and renamed `_file_error`,
> the ordinary regenerated gate reports **23 exact / 4 residual / 0 unwritten**.
> All twenty prior exact functions survive the full 8,245-owner sweep. The
> three new strict functions add **944 padded bytes**. The four remaining
> bodies are valid measured parks, not exact credit. The original physical
> DATA contribution and unresolved eight-byte prefix are unchanged; the object
> remains NonMatching. See `small_family_integration_batch_20260904.md` for
> full-build/publication validation. Historical pre-symbol measurements below
> are retained to explain why the boundary correction was necessary.

## Result and admission boundary

From canonical starting HEAD `9a1a99f80`, the PID-isolated focused gate moves
`source/tag_files/files_windows.obj` from **20 exact / 0 residual / 7
unwritten** to **21 exact / 6 residual / 0 unwritten**. The newly recovered
`_file_path_split` is strict exact and adds **224 padded exact code bytes**.
All twenty inherited exact functions remain exact.

Two more complete reconstructions, `_find_files_start` and
`_find_files_next`, have the target's exact padded size, relocation count,
and normalized code hash. They are deliberately reported as relocation-
identity residuals, not strict credit: January places the authenticated
616-byte search state eight bytes into a raw 624-byte DATA owner, whereas the
candidate emits the naturally separate state at the start of its own section.
The remaining four bodies are ordinary typed-C fuzzy reconstructions. No
park, symbol configuration, full build, commit, or push is performed here.

## Scope and provenance

Changed files in this lane are:

- `source/tag_files/files_windows.c`;
- `source/tag_files/files.h`, limited to the public find-files flags and a
  compatibility spelling for the independently authenticated filename bit;
- this ledger.

`source/tag_files/files.c` has no textual change. The broad working tree also
contains changes owned by other workers; they were not edited or evaluated by
this lane.

The January split COFF is the sole byte and ABI authority. Read-only
corroboration came from the cached Stian `tag_files/files.c`, Pastudan's
equivalent file helpers, and the HCEA reconstruction under
`research-cache/halocea-full-blobs-20260830`, especially:

- `src/headers/find_files_globals.h`;
- `src/data/find_files_globals.c`;
- `src/headers/find_files_flags.h`;
- `src/blam/game/find_files_start.c`;
- `src/blam/game/find_files_next.c`.

HCEA establishes the coherent traversal algorithm and 616-byte state layout,
but is not treated as byte authority. Its `find_files_next` commentary itself
marks a later-build deviation around the dot-name comparisons; January calls
the project `csstrcmp` function, which is what the retained source uses.

## Recovered code

| Function | Before | Candidate / target padded | Relocations | Candidate SHA-256 | Target SHA-256 | Verdict |
| --- | --- | ---: | ---: | --- | --- | --- |
| `_file_path_split` | unwritten | `224 / 224` | `2 / 2` | `9575fecb9ec463bc462df3449826cdaaed8caa5b5a8064165bb2aaece098676f` | same | strict exact |
| `_find_files_start` | unwritten | `208 / 208` | `17 / 17` | `25a08a5eca92870f4f59dd3e42f5caa0962c5c91b45abc28060d3983440ee97e` | same | code exact; DATA identity blocked |
| `_find_files_next` | unwritten | `512 / 512` | `49 / 49` | `f49675e5d0fbde7ea79f062eba88720b4b430c05dd38fa934226d91848c10d62` | same | code exact; DATA identity blocked |
| `_file_open` | unwritten | `384 / 400` | `23 / 23` | `7a5172a1a82ec4c9d83a0577970c4a341bdd52ab76907b0d330a5f537f3ef0eb` | `b91ff127948eb458e70ae51cf1efc5158a353749d27b2e29a03ad3abcb7b930b` | fuzzy |
| `_file_get_size` | unwritten | `224 / 224` | `13 / 13` | `d26fde60720be6198f891f7f8e4688b9a5617339aa8b8f4d1d314522ece62a27` | `3e237cfe3e76e06d034d1b64effb416e37dfb3694bfa56ca1d16cd16cfba958f` | fuzzy |
| `_file_read` | unwritten | `176 / 176` | `13 / 13` | `51d43b44745309d3e8c9033422b29f4444fc8f19b492702bf0936aeae50a4a06` | `8442e02641914ce558f9947431dc6e60171d38dd9ba1c9882defba9f2e39773a` | fuzzy |
| `_file_write` | unwritten | `176 / 176` | `12 / 12` | `44d258415364d089b03af100e28a101e14bda1aace15f786f95f0f764ad0b7a3` | `f1fb6f5c8738ecea224c025c70e077c7175200d705df410ce5773d166733e101` | fuzzy |

`_file_path_split` uses the project character-stepping helper and the natural
directory/parent/name/extension partition. Its complete 213 meaningful / 224
padded packet and both call relocations are exact.

`_find_files_start` validates the named flags, closes every live search
handle, initializes the depth/location/path state, and preserves January's
two assertion literals. `_find_files_next` opens or advances a handle per
depth, pops exhausted directories, skips `.` and `..`, applies the named
recursive and enumerate-directories flags, builds typed file references, and
optionally copies the eight-byte last-write date. The result local and normal
control-flow labels explain January's observable false initialization and
shared return topology; no compiler directive or artificial operation is
present. After relocation masking, both functions are instruction-for-
instruction identical to January.

The directory attribute test intentionally uses the Win32 mask
`FILE_ATTRIBUTE_DIRECTORY` directly. `TEST_FLAG` takes a bit *index*, so
passing the Win32 mask to it would test bit 16 and be a semantic bug. This is a
mask API boundary, not manual reconstruction of a project flag field.

January also authenticates one surprising combined-flags behavior. When both
the enumerate-directories and recursive bits are set for a directory, the
function returns the directory and increments depth but skips appending that
directory name to the persistent path. This can revisit the same path and is
best understood as an original invalid-combination invariant or bug; ordinary
call sites use no flag or a single mode. The retained behavior is not a
cleanup invented by the reconstruction: the target instruction stream and
branches prove it exactly, so this lane preserves and discloses it.

### Private error helper name and source boundary

The inherited exact private function previously inventoried as
`code_00189ca0` is independently named `file_error` by both the Stian and
Pastudan lifts. Its typed body accepts a file reference and function name,
reports `GetLastError()` through the common file-error format, and clears the
Win32 error. Its January RVA is `0x00189CA0`; the semantic mapping proposed to
the canonical owner is `_file_error -> _code_00189ca0`.

January's emitted `file_open`, `file_read`, `file_write`, and `file_get_size`
packets contain the full info/error/clear sequence directly. The retained
source now expresses each as the natural `file_error(file, function_name)`
call corroborated by Stian and Pastudan. One bounded alias gate proves that
VC7 automatically inlines all four calls into exactly the same candidate
packets: sizes, relocation counts, hashes, and the 21/6/0 object result are
unchanged. No manual or forced inline is present. VC7 leaves the existing
`file_exists` call out of line, reproducing January's selective schedule and
keeping the private helper itself strict exact under the semantic alias.

## Search-state DATA ownership blocker

The retained TU-private structure is the independently corroborated natural
layout:

- flags at `+0x000`;
- signed depth at `+0x004`;
- location at `+0x006`;
- 256-byte path at `+0x008`;
- eight handles at `+0x108`;
- `WIN32_FIND_DATAA` at `+0x128`;
- total size **616 bytes**.

Its initializer is likewise authentic: flags and location zero, depth `NONE`,
empty path, all eight handles `INVALID_HANDLE_VALUE`, and zero find data.
January's raw `_data_00314ce0` section is **624 bytes**. Bytes `+0..+7` are
`3f 3a 5c 00 00 00 00 00` (`"?:\\"` followed by a zero dword); the proven
search state begins at `+8`. No target code in this object references that
prefix. Candidate `find_files_globals` is exactly the 616-byte state and does
not invent a wrapper, prefix member, or aggregate owner merely to move its
relocations.

Consequently, every state relocation in the two traversal functions differs
by the same eight-byte owner offset while code bytes and ordered relocation
types/counts agree. Examples are candidate/target flags `+0/+8`, depth
`+4/+12`, location `+6/+14`, path `+8/+16`, handles `+264/+272`, find data
`+296/+304`, last-write time `+316/+324`, and filename `+340/+348`.

The evidence-backed semantic correction is to identify the January state at
target address/raw-owner offset `_data_00314ce0+8` as
`find_files_globals`. A plain source-name alias cannot repair the addends; the
target DATA boundary must be split or mapped at the subobject offset by the
canonical symbol owner. The unreferenced eight-byte prefix remains separately
unresolved until first-party type/name or reference evidence identifies it.
No name or type is proposed for that prefix.

## Public enum spellings

The owning `files.h` now defines the already corroborated public find flags
`_find_files_recursive_bit`, `_find_files_enumerate_directories_bit`, and
`NUMBER_OF_FIND_FILES_FLAGS`. It also exposes both equal-value filename-bit
spellings:

```c
has_filename_bit = 0,
_has_filename_bit = has_filename_bit,
```

This is a source-API compatibility alias, not codegen control. January
`files_windows.obj` authenticates the assertion text
`!TEST_FLAG(info->flags, has_filename_bit)`, while January `files.obj`
authenticates `!TEST_FLAG(info->flags, _has_filename_bit)`. Keeping both lets
each owning source spell its original assertion without regressing the exact
`files.obj` function. The alias does not alter
`NUMBER_OF_REFERENCE_INFO_FLAGS`, which remains one.

## Honest fuzzy parks

No `config/parked.json` entry is changed by this worker. The following source
is suitable for later evidence-based parking if it survives the orchestrator's
full header blast and stable exact-set sweep:

- `_file_open`: complete Win32 open/append/error semantics and all 23 target
  relocations are present, but the candidate is 16 padded bytes short. The
  residue is return/control topology, not missing behavior. Earlier ledgers
  already reject the obvious donor-shaped early-return spelling; this natural
  result form is retained as better fuzzy source, not exact credit.
- `_file_get_size`: target and candidate are both 224 padded bytes with 13
  relocations. The remaining difference is register/lifetime allocation
  around the typed info pointer and error path.
- `_file_read`: both are 176 padded bytes with 13 relocations. January and the
  candidate implement the same exact-count/EOF/error behavior; the remaining
  difference is register selection and reload placement.
- `_file_write`: both are 176 padded bytes with 12 relocations and the same
  write/count/error behavior; the remaining difference is likewise register
  selection and reload placement.

Only one ordinary declaration/assignment variant of the three same-size
leaves was tried after the direct transcription, with no metric change. No
register keyword, volatile lifetime device, declaration-order search,
assembly, pragma, intrinsic, forced inline, dummy branch, undefined behavior,
or other fake-match technique was used. Reopen these parks only for authentic
January source, same-compiler source-shape evidence, or a family-level API/type
correction.

## Focused validation and handoff

The final isolated gate reports **21 exact / 6 residual / 0 unwritten** for all
27 target functions. The complete candidate symbol scan passes the forbidden
`_point_from_line3d` emitted-symbol guard. The retained bodies use semantic
names, public flag constants, typed file structures and APIs, parameter-per-
line definitions, explicit void returns, and normal C control flow. There is
no raw address/offset access, representation pun, inline assembly, optimizer
attribute, or fabricated source operation.

`fake_match_scan.py --fail-on-findings` over the changed C/header pair reports
zero review leads, and focused `git diff --check` passes. Final hashes are:

- `source/tag_files/files_windows.c`:
  `1107bee44bf6eb848b735ddbfaaf00b34d03bd1d82337bf15bbde2c83fb7b12d`;
- `source/tag_files/files.h`:
  `62bea38a66f6286e3b219d91fa495a710cad3533ba484faf3116e206326d55b5`;
- focused alias object:
  `833b13cea32bf257c8942288d09ab3ae31f01feb4f60b450466e9349cde5004e`.

Focused semantic-alias object artifact:
`scratch/files_windows_obj_natural_file_error_20260904.obj`, compiled with
`--alias file_error=code_00189ca0` until the canonical symbol mapping is
updated. Canonical admission
still requires the orchestrator's full Ninja build, shared-header blast-radius
check, stable exact-set regression sweep, data-owner adjudication, accounting
refresh, and tool suite. Until those pass, this ledger makes no canonical or
GitHub publication claim and gives strict code credit only to the 224-byte
`_file_path_split` closure.
