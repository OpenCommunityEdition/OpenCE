# `hud_unit.obj` exact-but-unlinked state accessor and shield-tick boundary

## Result

The sole frozen paired production shot measures private `_code_000c6600`
byte-exact while its public caller remains residual. The helper is 99
meaningful / 112 padded code bytes with ten ordered relocations. Target and
candidate share normalized
SHA-256
`923d09ad9b67550b4747520338451c77bcfddc864dbe9de7f90c510a108ec50d`.
It is not admitted or retained: after the rejected caller is removed, VC7
correctly dead-strips the private static helper because no current exact caller
owns its SI convention. `source/interface/hud_unit.obj` therefore remains
13/22 strict and `NonMatching`.

The connected public `_hud_tick_shield` candidate is not retained. It matched
the target's 64 padded bytes, three relocation identities, helper call at
`+0x24`, FPU subtract/store sequence, and behavior, but not the complete
normalized hash or first call relocation address. The target normalized
SHA-256 is
`13c3cf982fa6906bd2a74a7be8ea087726438c98714ef0e2c46b5c2ac1930afe`;
the candidate is
`ecc29b3036b8829d7d642f51b02d9dd814debdbd2026e1547633a601bfe83b46`.
No alternate declaration, source spelling, or retry was attempted.

The immutable combined first-shot object is
`build/audit/hud_unit_tick_shield_pair_first_shot_20260829.obj`, 5,413 bytes,
whole-object SHA-256
`3f090f68d55495a066a3e226f9a6f042f406f7fd11abd4d9b148efbf99a7cba9`.

## Exact helper packet and link-closure boundary

January's `_code_000c6600` receives its private short argument in SI. It
validates `0 <= local_player_index < 4` with the source line `0x106` assert,
validates `unit_hud_globals` with line `0x107`, and returns the typed
`hud_states[local_player_index]` element using the authoritative 0x58 stride.
The frozen static C89 helper reproduces all bytes and these ten relocation
packets in the immutable first-shot object:

- source file and local-index assertion strings at `+0x13` and `+0x18`;
- `_display_assert` / `_system_exit` at `+0x1D` and `+0x24`;
- the first `_bss_00453ac0` read at `+0x2C`;
- source file and globals assertion strings at `+0x3C` and `+0x41`;
- `_display_assert` / `_system_exit` at `+0x46` and `+0x4D`;
- the returning `_bss_00453ac0` read at `+0x56`.

The independent HaloCEA typed `get_hud_state_0.c` donor authenticates the
0x58 typed-array access. January disassembly independently fixes both asserts,
their exact line identities, the private SI convention, and the return
arithmetic. The already-exact new-map initializer supplies the same typed
structure, source strings, assertions, and line numbers without adding raw
offsets or reconstructed data.

The exact packet alone is insufficient for admission. Its SI argument is a
private whole-TU convention selected only when a visible caller keeps the
static function live. Removing the residual caller removes the helper section;
making it externally callable would change that ABI, while a synthetic anchor
is forbidden. Reopen the helper only together with an independently
admissible exact caller.

## Shield-tick fixed boundary

The HaloCEA `hud_tick_shield.c` donor authenticates the player-to-local-HUD
mapping and cached shield subtraction. January disassembly fixes `player_get`,
the signed 16-bit local-player field, the private accessor call, and the x87
`last_shield_vitality -= amount` update.

The frozen natural candidate declared the local from its authoritative short
field type. VC7 therefore emitted `xor esi,esi; mov si,[eax+2]`, putting
`_datum_get` at `+0x10`. January emits a single
`movsx esi,word ptr [eax+2]` and places that relocation at `+0x0F`. All later
instructions and relocation identities converge, but complete ordered COFF
identity fails. This is a narrow extension/lifetime source-type boundary. The
public candidate, helper, and now-unused include were inverse-reverted exactly.

## Discipline and scope

All canonical HUD Unit ledgers, January target/helper disassembly, current
typed headers, registered interface/Claude worktree state, applicable Claude
object/tooling instructions, history, and both HaloCEA donors were inspected
before editing. Claude's current interface worktree had no competing body.

One readable typed C89 helper/caller pair was applied and the ordinary
production candidate edge compiled exactly once. The first focused gate
reported one exact helper packet, one public residual, and all 13 inherited
exact functions unchanged. After inverse reversion, one restoration build
confirmed 13 exact, zero residual, and nine unwritten functions. The source
blob is restored to `eb61af7b57d017e7f3ca00dfb1f2568c4ee8f131`.
No tuning or sweep followed. The retained source uses no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw address/offset access, pointer/integer reconstruction, pointer or
union pun, undefined behavior, synthetic anchor, or byte forcing. Units,
Vehicles, Matrix Math, AI Debug, Motion Sensor implementation, configuration,
semantic exceptions, parks, completion labels, and build rules are untouched.
