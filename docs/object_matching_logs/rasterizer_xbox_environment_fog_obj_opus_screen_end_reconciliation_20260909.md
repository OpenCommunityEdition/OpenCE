# `rasterizer_xbox_environment_fog.obj`: Opus screen-end reconciliation (2026-09-09)

## Scope and result

This packet selectively reconciles the environment-fog work from Opus lane
`C:\halo-worktrees\opus-30k-rasterizer-widgets-20260905`, source commit
`74e015673`, onto the current published source boundary.  It does not copy the
donor file wholesale.  Only the semantically supported screen-fog activity
test and final compositing pass are retained, together with the minimum
translation-unit-private views needed to express their real fields.

After the four authenticated target-label corrections listed below, the packet
adds four strict owners and **1,506 meaningful / 1,520 padded exact code bytes**:

| January owner | Source owner | Meaningful | Padded | Kind |
|---|---|---:|---:|---|
| `__rasterizer_environment_fog_screen_end` | same | 1,472 | 1,472 | Halo C |
| `_code_001556c0` | `_IDirect3DDevice8_SetVertexData2s@16` | 13 | 16 | stock XDK wrapper |
| `_code_001556d0` | `_IDirect3DDevice8_Begin@8` | 11 | 16 | stock XDK wrapper |
| `_code_001556e0` | `_IDirect3DDevice8_End@4` | 10 | 16 | stock XDK wrapper |

All fifteen inherited strict owners remain exact.  The object then has 19
strict owners, one honest residual, and seven still-unwritten January owners.
It is not a whole-object completion and receives no data/BSS credit.

## Target-label corrections required at integration

The source deliberately contains no address-derived C names.  The parent
integration must apply these independently authenticated edits to
`config/symbols.json`, then regenerate the split target:

| File offset | Old target label | Semantic label |
|---:|---|---|
| 1,398,464 | `_code_001556c0` | `_IDirect3DDevice8_SetVertexData2s@16` |
| 1,398,480 | `_code_001556d0` | `_IDirect3DDevice8_Begin@8` |
| 1,398,496 | `_code_001556e0` | `_IDirect3DDevice8_End@4` |
| 1,398,880 | `_code_00155860` | `_rasterizer_environment_fog_screen_active` (`static: true`) |

The three XDK identifications are direct hardened whole-section matches against
the ordinary wrappers emitted by the stock January `<xtl.h>` path: padded
size, normalized SHA-256, relocation count, and relocation identity all agree.
They are emitted by calls genuinely required by the compositing body; no dummy
call was added to summon a COMDAT.

The private helper name is reconstructed rather than PDB-recovered.  Its one
purpose and four January callers make `rasterizer_environment_fog_screen_active`
the narrow semantic project-convention name: it validates the current window,
caches the per-window activity flag by frame, validates the screen definition,
and computes `local_fog_eye_density`.  Renaming the target helper removes the
only relocation-identity difference in the otherwise identical screen-end
owner.

## Exact proof

Before the helper target label is corrected,
`__rasterizer_environment_fog_screen_end` already has identical 1,472-byte
normalized code, 122 ordered relocations, and normalized SHA-256
`376c75b94c1ace234151e9ba53a510748481605603f32dfaa682bd45d924fe43`.
The only differing relocation is the call at `+0x32`:

- January: `_code_00155860`;
- candidate: `_rasterizer_environment_fog_screen_active`.

A PID-safe gate with the helper alias proves the body strict exact.  The three
wrapper identities are:

| Source wrapper | Normalized SHA-256 |
|---|---|
| `_IDirect3DDevice8_SetVertexData2s@16` | `da3274ee297466e512d4f578a729fd2a12e86cc08fec77580ea60253bc65e8a8` |
| `_IDirect3DDevice8_Begin@8` | `db1d1b9bf1eb9c529b3d3187b7a89d8d35944820bb41637558be9520fed5cfa4` |
| `_IDirect3DDevice8_End@4` | `c9740afccf5e7e01a66dfdb669d22a8462ac214622a1e8eff90b803ddb2d68c5` |

## Honest fuzzy frontier

`_rasterizer_environment_fog_screen_active` is retained as the best reviewed
semantic reconstruction but receives **zero exact credit**.  Target and
candidate are both 512 padded bytes, 42 relocations, and 138 instructions.
The target SHA is
`ea96544f961a628945353c8e51d4b6f6d23d0a311d2822cb7180c1ced9d5da3d`;
the candidate SHA is
`19b4d27dcd140be10cbcea9e2eb0ba9d9238c5e10eedc7011f47c5af924ded7f`.
The production report measures 97.25564 percent similarity.
Aligned disassembly shows the remaining differences are independent scheduling
of the initial activity-byte clear versus the frame-index comparison, and the
final activity-byte load versus density stores.  No declaration or statement
permutation search was used to force those choices.  The parent integration
must register this owner as an instruction-scheduling park after applying its
semantic target label.

## Donor code deliberately rejected

- The donor's screen-draw and model-begin bodies require screen-fog statistics
  fields not yet owned by `rasterizer_frame_statistics.h`.  Redefining that
  shared global locally, casting it to a duplicate layout, or addressing its
  reserved bytes would violate the ownership and anti-punning rules, so neither
  body is admitted here.
- The 3,904-byte screen-begin body, model-submit body, and wind-update helper do
  not belong to this clean exact subset.  They retain structural or instruction
  mismatches and need separate evidence-led reconciliation.
- Unused donor layout fields, hand-written CRT declarations, duplicate profile
  prototypes, and provisional transparent-geometry layouts are not copied.

## House-rule and tool verification

- no-argument functions use `void` on its own line; function parameters are one
  per line and every retained function has an explicit final return;
- project `real`, vector/color, `TEST_FLAG`, `VALID_INDEX`, and named rasterizer,
  vertex-type, XDK-state, XDK-register, and primitive constants are used;
- the screen view follows the HCEA `fog_screen` type and field names through the
  last accessed member; unknown TU-private storage remains reserved rather than
  receiving invented names;
- no inline assembly, force-inline directive, representation cast, raw address,
  dummy dependency, or `point_from_line3d` owner is introduced;
- `tools/fake_match_scan.py --fail-on-findings` reports zero review leads;
- `git diff --check` passes;
- PID-safe `gate.py` passes the forbidden-symbol guard, preserves all 15
  inherited exact owners, and proves screen-end exact under the reviewed helper
  alias.

The parent lane must run its whole-tree build, stable-owner sweep, park
validation, and production accounting after applying the target-label changes.
