# `hud_unit.obj` Jonas scripted-HUD flag recovery (2026-08-27)

This ledger records the evidence-bounded recovery of the six scripted HUD
health, shield, and motion-sensor show/blink functions. All six are strict
exact with the retained canonical `SET_FLAG` topology. `hud_unit.obj` advances
from five to eleven accepted functions but remains `NonMatching`; no
whole-object or data completion is claimed.

## Authority and scope

- Integration base: `526798189aaa312e6d756c15efc005f1298ae9bf`.
- Baseline source blob: `6795588eb27b83f92a5cfe4071510c5b8f57d9be`.
- Baseline production object SHA-256:
  `45d16af646c476274c3d9f03316e6314040145519d70041f73da68d548ce6e71`.
- January split object SHA-256:
  `8e675992cb876c291c4ee4c18ae69a0be143f61f5161bf87cdde5f6633df5cfd`.
- Compiler: pinned Microsoft VC7/XDK `xbox/bin/vc7/CL.Exe` through the
  unchanged ordinary Ninja production edge.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The only tracked implementation change is
`source/interface/hud_unit.c`; this ledger is the only other tracked file. No
configuration, completion status, semantic allowlist, parked entry, compiler
flag, comparator, or admission rule changes.

## Provenance and layout reconciliation

HCEA supplies independent behavior and naming provenance for all six members:

- `build/audit/refs/halocea/src/blam/sound/scripted_hud_show_health.c`,
  SHA-256
  `f10f1e25a1d109d267daaae14a6954717d42f2101cc7289eee5fc0e4c3e2b7bc`;
- `build/audit/refs/halocea/src/blam/sound/scripted_hud_blink_health.c`,
  SHA-256
  `c7e022b0cc41d7bb20a388c7772d8637985c65ffd1c4266c5154d6bcb5f67f01`;
- `build/audit/refs/halocea/src/blam/sound/scripted_hud_show_shield.c`,
  SHA-256
  `713d167321b680ddced5079d91ab54dc657fc4ef189f5430efcf9b0a9bb0e495`;
- `build/audit/refs/halocea/src/blam/sound/scripted_hud_blink_shield.c`,
  SHA-256
  `920391d417608f2afb84ff89c223d0a91d007e72c889283480954ee25fc543b2`;
- `build/audit/refs/halocea/src/blam/sound/scripted_hud_show_motion_sensor.c`,
  SHA-256
  `70ba0d9cc9f15e0ec28697697ac0ded2ad9e59888b05f59da905c69f2df3a069`;
- `build/audit/refs/halocea/src/blam/sound/scripted_hud_blink_motion_sensor.c`,
  SHA-256
  `0e35aa63d3befec4ce0b58be0d096966b0623bdba1a91c9c971b3f7ab2ed4c9c`.

`source/hs/hs.c` independently establishes every public signature as
`void(boolean)` and binds the evaluator arguments as booleans. January data
and disassembly establish a 0x164-byte global allocation referenced through
`_bss_00453ac0`, with `script_flags` at offset 0x160. Bits zero through five
are, in order: health don't-show, health blink, shield don't-show, shield
blink, motion-sensor don't-show, and motion-sensor blink. The show functions
invert their boolean because a set bit suppresses the panel; blink functions
store the boolean directly.

## January packets

Every target function owns 43 meaningful bytes, 48 padded bytes, and one
`IMAGE_REL_I386_DIR32` relocation at offset `0x09` to `_bss_00453ac0`. The
target sequence loads the incoming boolean into `AL`, tests it, loads the
global pointer into `EAX`, reads the flags into `ECX`, conditionally ORs or
ANDs the relevant bit, and stores the result.

| Function | Bit | Input mapping | Normalized SHA-256 |
| --- | ---: | --- | --- |
| `_scripted_hud_show_health` | 0 | invert | `89aad93f59e7ff19dbeb1eaa548cd79a8b69694ebf90f0a8e74dc6447ccd6290` |
| `_scripted_hud_blink_health` | 1 | direct | `abf7edb657a5cd6841c1f5821b55b4b59d8f133b9bf5466126c1dc77a3d04150` |
| `_scripted_hud_show_shield` | 2 | invert | `b822b735b48bf262da5741a9d7ba9b1c92adaece9c9c5b731ef0d72becfb5776` |
| `_scripted_hud_blink_shield` | 3 | direct | `d94feb4c6c8578bf9803a93ee61d37512af196088698539ed43fab179524c880` |
| `_scripted_hud_show_motion_sensor` | 4 | invert | `e6c4edb290df0fa2bcebacf42f8aacbf98a2c6d7a1cb284bdf827e389f1ae075` |
| `_scripted_hud_blink_motion_sensor` | 5 | direct | `056a15933dd23893cde79fbe0b72e2fe29b9e0afe092dac9a7b6d6396e22123c` |

## Frozen experiments and strict result

The pre-edit manifest is
`build/audit/hud_unit_script_flags_baseline_20260827.json`; its preserved
object is `build/audit/hud_unit_script_flags_baseline_20260827.obj`.

The first production topology used an explicit local flags value and repeated
typed global expression. Its preserved object is
`build/audit/hud_unit_script_flags_first_shot_20260827.obj`, SHA-256
`185e2609271ea2dd706a2fdde17fd7d8b3160ccc2c96401681202c4aba81a9e8`.
All six had the right size and relocation destination, but the compiler loaded
the pointer with `8B 0D` into `ECX`, placed flags in `EAX`, and emitted the
relocation at offset `0x0a`; the topology was rejected.

A genuinely new second topology used an explicit typed pointer local plus a
flags local. Its preserved object is
`build/audit/hud_unit_script_flags_pointer_local_20260827.obj`, SHA-256
`377726750d4c8ef7f257d0d765c00cddea954154c7806759c6f53f41fc7c12d7`.
It optimized to the same nonexact register allocation and was rejected.

The retained topology passes the typed `script_flags` lvalue directly through
the canonical `SET_FLAG` macro. Its preserved production object is
`build/audit/hud_unit_script_flags_set_flag_20260827.obj`, SHA-256
`232b0c3db9790b4eaf5d8c702388416eb2a9d5410263be4215f7c4f009e004e3`.
The retained source blob is `3c5ec6c74ccb7ed0712b1e457ac3690451ff9e30` and
its physical SHA-256 is
`2684748722dd1c812857014c39ff48e538b9c0042facace2bb0ebd138bda3a92`.

The hardened comparator reports all six additions `equal: true`: all 288
padded bytes, all normalized hashes, and all relocation addresses, types,
destinations, addends, and ownership are identical. It directly reconfirms
the five inherited exact functions and reports `all_equal: true` across the
eleven-function set.

The fail-closed regression check names precisely the six scripted-HUD
functions as `NEWLY_EXACT`, all five inherited functions as `still_exact`, and
`changed_nonexact: []`. Its symbol-set finding is the expected six new
external COMDATs and compiler debug ownership. The finding is reviewed, not
waived; no exception or matching-credit mechanism was changed.

## Validation and progress

The full Halo-plus-libcmt graph and both reports build. Their SHA-256 hashes
are:

- `build/report.json`:
  `d612fce553b2cc704f00117f0b0383e826936faea3fcbccb12c17015b462f325`;
- `build/semantic_report.json`:
  `dedf850b2d58c7e5842e3a5792c0aa02ce9b066148fe6945a5748d04ec1603eb`.

The direct report records `hud_unit.obj` at 11/22 functions and 263/6,176
meaningful code bytes. Hardened padded progress is 368/6,368 bytes. This wave
contributes six functions, 258 meaningful bytes, 288 padded bytes, and six
relocations. The strict campaign board becomes 260/619 complete objects,
4,276/8,246 functions, and 554,871/1,922,669 padded code bytes; the complete
object count is unchanged.

All remaining gates pass:

- semantic audit: 470 units, 4,433 functions evaluated, 4,285 semantic exact,
  115 hidden exact / 64,889 hidden code bytes, 4,345 accepted exact, and zero
  unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- direct eleven-function comparison: `all_equal: true`;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

No assembly, register/volatile qualifier, barrier, pragma, intrinsic,
attribute, raw address, raw tag/object access, aliasing trick, or byte-forcing
construct is present. The typed structure has compile-time offset and size
assertions, and the unit remains `NonMatching` in `config/config.json`.
