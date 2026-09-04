# Devices group-creation caller cluster — 2026-09-04

## Result and admission boundary

From canonical HEAD `96086c9c4`, scratch-only verification advances
`source/devices/devices.obj` from **19 exact / 0 residual / 11 unwritten** to
**20 exact / 2 residual / 8 unwritten**, after the two semantic names are
mapped to January's current anonymous labels. The new strict function is
`device_group_new`: **112 padded / 103 meaningful bytes**. All 19 inherited
exact owners remain exact; no existing exact function is traded for this gain.

The two reconstructed callers cover another 512 padded target bytes but are
honest residuals, not exact credit. No full build, configuration edit, commit,
push, object-completion claim, or parked-manifest edit was performed by this
worker. The orchestrator must regenerate the renamed target, run the whole-tree
build and stable regression sweep, then measure fresh parked entries.

Tracked scope is this ledger and `source/devices/devices.c` only. No shared
header edits are required. The source is frozen.

## Documentation and provenance

The matching methodology, Claude's campaign-throughput lessons, previous
Devices group/new-map/actual-value/touch/accessor ledgers, current source and
owning headers were inspected before reconstruction. The current root contains
no `AGENTS.md` or `CLAUDE.md`. January split COFF is the sole byte/ABI authority.

The frozen Opus tree `C:\halo-worktrees\opus-small-families-30k-20260902`
contains no production bodies for these three functions, so this packet is
fresh reconstruction rather than previously adjudicated donor credit.

Semantic corroboration comes from the read-only local Git repository
`scratch/halocea-full-audit`, commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`:

| Path | Blob |
| --- | --- |
| `src/blam/devices/device_group_new.c` | `5fc912913a8f886f7d326b1a5bcf42276bfc5738` |
| `src/blam/devices/create_initial_device_groups.c` | `f42ce3a284a23e1eb1c189aac6b687daec5e23e3` |
| `src/blam/devices/device_add_scenario_information.c` | `12d35c8800c418661df9111db74e9e32b8a74fc0` |

The older flat HCEA scenario-assignment file incorrectly passes `device_index`
as the new group's flags. The pinned newer donor corrects that error, and
January independently proves runtime flag 4 plus changes-only-once flag 1 for
the position group. HCEA's later allocator silently skips exhaustion; January
instead asserts at line `0x311`. The map initializer additionally asserts that
the new short datum index equals the scenario index at line `0x339`. Neither
later-build behavior nor a donor prototype overrides January.

Marathon's `scratch/marathon-source/marathon2/platforms.c::new_platform` was
consulted as a contemporary source-style analogue for allocating runtime state
from authored platform flags and copying initial state. No Marathon layout or
behavior was transplanted.

## Names, layouts, and natural callers

The orchestrator's intended `config/symbols.json` replacements are:

```json
{ "file_offset": 544448, "flags": 32, "name": "_device_group_new", "static": true },
{ "file_offset": 544624, "flags": 32, "name": "_create_initial_device_groups", "static": true }
```

Current anonymous labels are `_code_00084ec0` and `_code_00084f70`.
The image relocation census finds only two calls to the allocator, both in
scenario assignment (file offsets 546620 and 546683), and one tail call to the
initial-group helper from `devices_initialize_for_new_map` (544863). Both
helpers have genuine file-static declarations and definitions. No artificial
reference or public-linkage fiction is used to keep either emitted.

VC7 naturally inlines `device_group_new` into `create_initial_device_groups`
while keeping the out-of-line allocator called twice by scenario assignment,
as January does. No `inline`, forced-inline, or compiler-control annotation was
added. The inherited exact map initializer retains its separate tail call.

Existing device/scenario-instance/group datum types are reused. A TU-local
`scenario_device_group` matches the known 52-byte tag record: name[32], real
initial value at `+0x20`, flags at `+0x24`, and the schema's three unused longs.
January explicitly passes size `0x34` to `TAG_BLOCK_GET_ELEMENT` and reads those
two fields. The existing full scenario definition locates `device_groups` at
`+0x288`; compile-time assertions verify these boundaries. This is an actual
tag record, not padding introduced to influence code generation.

The source includes `scenario.h` and `scenario_definitions.h` for their real
owners, uses `device_get`, `device_definition_get`, `TAG_BLOCK_GET_ELEMENT`,
`datum_get`, `real`, `word`, named enum bits, `FLAG`, and `TEST_FLAG`. Public
scenario assignment already has its owner declaration in `devices.h`.
No data/BSS ownership or data exactness is claimed.

## Semantic fidelity and bounded source corrections

Scenario assignment resolves existing or newly allocated power/position groups,
copies their current real values, and sets the authored reversed/not-usable
flags. The existing typed getter's real-return boundary introduced x87
loads/stores absent from January, so the retained source uses direct typed
datum-field copies. A natural conditional selection of existing/new short
indices reproduces January's integer promotions, without width-forcing casts.

January makes two additional `datum_get` calls after copying both values,
discarding their returns at function relocations `+0xE6` and `+0xFA`.
The source explicitly preserves those checked lookups with a factual comment.
It does not invent unused variables, extra assertions, or fake callees to force
the calls. The initial tag validation is likewise genuinely present in January.

Three complete formulations were compiled: the first typed reconstruction;
direct-member copies with long index locals; and the retained ordinary
conditional index expressions. No declaration/allocation/lifetime search,
optimizer steering, shared-header permutation, raw offset, representation pun,
assembly, fabricated branch, or undefined behavior was used.

## Strict measurements and residual boundaries

Measurements below use the alias-gated final scratch candidate:

| January owner | Target padded / relocations | Candidate padded / relocations | Result |
| --- | ---: | ---: | --- |
| `_code_00084ec0` | 112 / 8 | 112 / 8 | strict exact |
| `_code_00084f70` | 224 / 14 | 240 / 14 | residual |
| `_device_add_scenario_information` | 288 / 12 | 288 / 12 | residual |

Allocator target/candidate normalized SHA-256:
`d14521a23ae8cf5b04d11f14b4d9eb11c1ad57acf2c3a1892c9b34f14c981b74`.

Initial-group target SHA-256:
`3599a03ce7c3c87fe5261d8f67e0865ee14eda302453f947ee3682499be3bb95`;
candidate:
`28358cf0131d290b01174a2e2d24ae4efa49a825a7885389699ea41460081cc0`.
The subobject-base/zero-initialization prefix differs, adding ten meaningful
bytes including loop alignment. After masking relocation fields, January's
`[0x26,0xDC)` loop/epilogue equals candidate `[0x30,0xE6)` byte-for-byte. All
14 ordered relocation identities agree, but their positions shift after the
prefix. Do not distort the ordinary tag-block pointer expression to force its
base-materialization schedule; retain an honest `unclassified` park.

Scenario-assignment target SHA-256:
`6a508ab57f807278d3115da228ff0f772290ec852b455d8ef8595112af1ce6da`;
candidate:
`44aeccc35c9004fb6a37926da5f38083cc0b7c6bfdee96d06e7edca02f5c06b8`.
All 12 relocations match completely. The only instruction-byte difference is
the nine-byte flags expression at `[0x7C,0x85)`: January lowers it as
`and eax,4; or eax,16; shr eax,2`, while the readable macro expression produces
`shr eax,2; and eax,1; or eax,4`. Both compute `4 | ((flags >> 2) & 1)` for every
unsigned input. A 1,024-case low-byte/high-bit check also passes. Every byte
outside that range agrees. Do not hand-author an unusual shifted mask solely
to buy exactness. Retain an honest expression-lowering residual.

## Verification and reproducibility

```text
python tools/campaign/gate.py source/devices/devices --all \
  --alias device_group_new=code_00084ec0 \
  --alias create_initial_device_groups=code_00084f70 \
  --out scratch/devices-group-conditional-20260904.obj \
  --forbid-emitted-symbol _point_from_line3d
```

- Baseline: 19 exact; final alias-gated candidate: 20 exact, 2 residual,
  8 unwritten. Strict owner-deduplicated comparison: `lost=[]`, gained only
  `_code_00084ec0`. All 19 inherited exact sections remain exact.
- Complete candidate code census: no candidate-only code owners after the
  documented names are mapped. Complete emitted-symbol guard passes, including
  the separately compiled semantic-name object.
- `fake_match_scan.py source/devices/devices.c --fail-on-findings`: zero leads.
- `git diff --check -- source/devices/devices.c`: no whitespace errors.
- No Ninja/configure/build-directory writes or full-suite claims by this worker.

Until the target symbol rename is applied, an unaliased gate necessarily reports
the inherited map initializer's relocation-name mismatch and cannot find the
two old anonymous names. That is not an admitted regression or a reason to
keep address-derived C names; the orchestrator must verify the renamed target.

Frozen SHA-256 evidence:

| File | SHA-256 |
| --- | --- |
| `source/devices/devices.c` | `943887bdc41943545d3d1157f218a74f098ff3dbfe6d35023ea65611698933f3` |
| `scratch/devices-group-baseline-20260904.obj` | `fcbbfaa5cbf4239853961cc77c9130be8eaef56a754e5663db5565a3ec05acec` |
| `scratch/devices-group-conditional-20260904.obj` | `4bc74cf07d7861a3fa7976b47bd17d910fd059d152e101450440f466ee49c36e` |
| `scratch/devices-group-final-semantic-20260904.obj` | `a24c9b9f3f05dbd3a291e393069e40098e2f8083aea8498e91b5afe28546fd66` |
