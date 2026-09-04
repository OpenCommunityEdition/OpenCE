# `player_effects.obj` interrupted-Opus private-helper reconciliation — 2026-09-04

## Outcome

The interrupted Opus reconstruction was reconciled with the current canonical
typed Player Effects translation unit rather than copied wholesale. Two small
same-TU helpers are strict exact on the first retained natural source shape:

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_effect_scale_value` | `33 / 48` | 2 | `abb05e6bbab65e966e7011c2a927dfeac9b992d0246a90957badf6c278517dd8` |
| `_get_shake_matrix` | `132 / 144` | 7 | `d6b5faded9493854bf53abcdd982eea0a770de0730ad04b9022b355003c4feb3` |

The packet gains 165 meaningful and 192 padded strict code bytes and two
functions, with zero data change and zero rename-stable regressions. The
natural owning function, `_player_effect_get_camera_effect_matrix`, is retained
as a complete typed 91.115486% reconstruction and honestly fuzzy-parked. The
object advances from `22/29` to `24/29` strict functions and from `1984/6096`
to `2176/6096` strict padded bytes.

## Scope and authority

- Isolated branch: `agent/player-effects-private-leaves-20260904`.
- Authoritative base: `84144bedfeea5de722e439d4e3718cf8f6928bb0`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Original January split `player_effects.obj` SHA-256, recorded by the prior
  authenticated ledger:
  `9a4ab25f9b58407433ecc9b04f235e5f3f3ae28bbeaf0023f3212bd5b789daeb`.
- Current canonical-label January split SHA-256:
  `0be29d1bc38199f6b3b3339245a1e731d1bd2df5e5377f2514168e4641ca7c6e`.
- Retained rebuilt object SHA-256:
  `34c46129b5499df7b6618e254bd9fdc6333fc3e25b736018c9fc446967a8ece5`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with unchanged `/O2 /Oy- /DDEBUG /Dxbox` and XDK 3911 inputs.
- Reconciled donor: `opus/small-families-30k-20260902`, commit
  `608c66380dca2d841b1e95150d1f88abe41a06d8`, whose message records a
  partially interrupted Opus campaign and whole-tree strict gating.
- Semantic corroborant: clean HCEA reconstruction commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`; its independently separated
  `effect_scale_value.c`, `get_shake_matrix.c`, and owner reconstruction
  agree on the helper bodies, `global_up3d` cross-product order, axis/value
  mapping, and the target's deliberate first shake call whose result is
  overwritten. It is corroboration only, not January byte authority.

All existing Player Effects ledgers were read first. The 2026-08-28 public
donor fixed point covers different functions; the 2026-08-30 initializer
owner boundary and the 2026-08-21 damage-indicator experiment likewise do not
attempt these two helpers or their owning camera-effect function. January COFF
is the byte, relocation, ownership, assertion, and call-topology authority.
The Opus/HCEA-style body is semantic evidence only. The latest paired
historical `CLAUDE.md`/`AGENTS.md` guidance was also checked; its applicable
binary-authority, ABI/layout, x87-order, evidence, and small-commit rules are
followed here, while its obsolete repository-specific commands do not apply
to this split-object tree.

## Reconciliation boundary

The donor translation unit could not be admitted wholesale. It replaced
already-exact canonical aggregate names and enum constants, used a raw
`tag_get` cast, emitted a numeric screen-flash map, and placed random-math
prototypes in `player_effects.c`. Those changes violate the current house
rules or needlessly disturb accepted exact source and were discarded.

The retained packet instead keeps canonical's typed and asserted January-PC
layouts:

- `player_effect_globals_definition` remains the typed `0x3EC` owner;
- four `0xEC` `player_effect_datum` records remain in the canonical layout;
- the existing screen-fade, scripted-effect, continuous-effect, timer, and
  flag fields are used by name at their already-proved offsets;
- the public `player_effect_get_camera_effect_matrix` declaration is placed in
  `effects/player_effects.h` rather than at a caller;
- random and matrix interfaces come from their owning existing headers;
- the existing semantic private name `effect_scale_value` is preserved; it
  follows the neighboring `contrail_scale_value`/`sound_scale_value`
  convention and is also the name used by the independent HCEA corroborant.

The target symbol at file offset `594656` remains `_effect_scale_value`.
Target and candidate have identical 48-byte normalized code, the same two
ordered relocations, and the natural body is called twice by the
target-authenticated owner. No speculative symbol-map edit is needed.

## Exact helper packets

`effect_scale_value` uses the complete natural expression
`transition_function_evaluate(function, 1.0f - elapsed / duration) *
zero_scale_factor`. When called from the real same-TU owner, VC7 naturally
passes the transition selector in EAX and emits the exact January 48-byte
private packet. No calling-convention annotation, assembly, forced emission,
or synthetic caller is used.

`get_shake_matrix` conditionally selects seeded random directions, constructs
the rotation from the axis plus sine/cosine, and writes the translated matrix
position from the named vector components. Its 144 bytes and all seven
ordered relocations are exact. The source uses `real`, `real_vector3d`, and
`real_matrix4x3` throughout.

All 22 inherited exact Player Effects functions remain exact. The two
pre-existing residuals, `_player_effect_add_continuous_effect` and
`_player_effect_get_screen_flash`, are textually unchanged and remain outside
this experiment's retry scope.

## Parked owner

The retained `_player_effect_get_camera_effect_matrix` reconstructs the full
visible behavior: assertion and local-player guard, scripted effect timing and
rumble, randomized scripted orientation/translation, impulse falloff and
matrix construction, periodic and continuous shake, rumble refresh/reset,
timer decay, and final `matrix4x3_multiply` composition.

| Measurement | January target | Retained candidate |
| --- | ---: | ---: |
| Padded bytes | 1312 | 1280 |
| Relocations | 49 | 49 |
| Normalized SHA-256 | `34ba1689676f40ae08fd01a8f705c1ab0e81fba92ad6c4eac74e6565e02c324a` | `36cc5e8fb04363d6e87bd5c48a971b5b17f6b318fcc37bdc8255024718cded67` |
| Objdiff | 100% | 91.115486% |

All 49 relocation types and identities are in the same order and both sides
use a `0x48`-byte frame. The first substantive mismatch is x87 temporary
storage/order in the scripted translation block; later differences are x87
stack and local scheduling in the periodic shake expression. The fail-closed
classifier returns `UNKNOWN` / `unclassified`. The complete honest source is
therefore parked without inferring a compiler recipe or tuning declarations,
statement order, aliases, or control flow around the disassembly.

Reopen only for authoritative January source/local records or a natural
same-compiler donor that explains the x87 temporary lifetimes and expression
schedule. Do not retry through volatile/register hints, barriers, pragmas,
forced inline/noinline, fake dependencies, raw offsets, representation puns,
undefined behavior, synthetic callers, gameplay assembly, or semantically
meaningless branches.

## Validation

- Focused gate: `22 exact / 2 residual / 5 unwritten` before;
  `24 exact / 3 residual / 2 unwritten` after. The added residual is the
  complete parked owner and the two remaining unwritten functions are
  `_player_effect_update_camera_impulse` and `_player_effect_start`.
- Full 576-edge Halo/libcmt build and report generation: pass.
- Semantic report: 473 units, 6,502 functions evaluated, 6,132 semantic
  exact, 6,148 accepted exact, and zero unit errors.
- Campaign progress after the strict helper gain: 884,762/2,198,102
  meaningful code bytes and 6,075/11,060 functions; data remains 2,018,310.
- Rename-stable sweep: 6,121 to 6,123 exact sections, +192 padded bytes, zero
  regressions.
- Park manifest: 227 active, zero stale, zero invalid after adding the owner.
- The header-triggered full rebuild preserves every unrelated accepted exact
  function. No object-completion label or data owner is changed.
- `git diff --check`, fake-source scan, admission audit, Units sentinel, and
  `point_from_line3d` COMDAT check pass in the isolated lane.
- The complete tooling suite passes `261/261` with a worktree-local pytest
  base-temp directory. All gates will be repeated at canonical replay before
  push.

The retained source contains no raw tag/object cast, raw offset, address-named
private function or global, numeric switch-table substitute, gameplay
assembly, volatile/register forcing, optimizer barrier/pragma, forced inline,
undefined behavior, representation pun, fake dependency, or nonsensical
branch.
