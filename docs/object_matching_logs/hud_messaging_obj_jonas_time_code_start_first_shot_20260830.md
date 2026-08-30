# `hud_messaging.obj` time-code start first shot

## Result

The sole natural typed C89 candidate for `_scripted_hud_time_code_start` is
strict exact on its first and only code-producing compile. It contributes 61
meaningful bytes, 64 padded bytes, one function, and seven ordered relocations.
`hud_messaging.obj` advances from 15/36 to 16/36 strict functions and remains
`NonMatching`, with three documented residuals and seventeen unwritten
functions.

Target and candidate have normalized SHA-256
`caf33a29dec535773e17c1c3e1227308a5dcb615ed6f079e3cfa2b98f9628675`.
The immutable first-shot production object is
`build/audit/hud_messaging_time_code_start_first_shot_20260830.obj`, with
whole-object SHA-256
`e471c64a489b6e2fe4ddaf1778fdfd1271eed2b1eb880f4ccb1fe1030e61bb2b`.

The relocation stream agrees exactly:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x0B` | `IMAGE_REL_I386_REL32` | `_game_time_get` |
| `+0x11` | `IMAGE_REL_I386_DIR32` | `_time_code_stop_time` |
| `+0x17` | `IMAGE_REL_I386_DIR32` | `_time_code_time` |
| `+0x21` | `IMAGE_REL_I386_DIR32` | `_time_code_time` |
| `+0x27` | `IMAGE_REL_I386_DIR32` | `_time_code_stop_time` |
| `+0x32` | `IMAGE_REL_I386_REL32` | `_game_time_get` |
| `+0x37` | `IMAGE_REL_I386_DIR32` | `_time_code_stop_time` |

## Evidence and topology

January loads the promoted Boolean argument as a byte. In the true arm it
samples `game_time_get`, adds the elapsed interval since
`time_code_stop_time` to `time_code_time`, writes `NONE` to the stop time, and
returns. The false arm samples the game time into `time_code_stop_time` and
returns. The retained compound assignment makes VC7 reproduce January's
EDX/ECX global-load order and complete instruction stream naturally.

The independent January-PC reconstruction at commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` and the clean Claude HUD branch
at `C:/halo-worktrees/claude-hud-messaging-3` corroborate the same `char` ABI,
branch topology, state transition, and call count. They were read only. Raw
addresses in those functional lifts were used only as evidence; the retained
source uses the canonical authenticated typed globals. The existing
HaloScript caller declaration fixes the public `void (boolean)` signature.

## Census, discipline, and gates

The full relevant HUD object ledgers, project and Claude instructions,
registered interface/Claude trees, history, target disassembly, caller, and
strict COFF tooling had already been audited for the immediately preceding
independent time-code leaf. A focused history and ledger census found no prior
candidate for this function; the two later history hits modify only its
HaloScript wrapper, not this owner.

The pre-shot source blob was
`dcab0fae97bfcb9b09b7c2d619f07f948bd81357`; the retained source blob is
`8a4bffa85d9811964ea79449c2166f1cd76c43fa`. The focused gate advances from
15 exact / three residual / eighteen unwritten to 16 exact / three residual /
seventeen unwritten. Direct comparison reports `all_equal: true` and proves
the newly retained 64-byte COMDAT plus all fifteen inherited exact functions.

The retained body uses no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or byte offset, pointer or union pun, undefined
behavior, synthetic anchor, or byte forcing. Units, Vehicles, Matrix Math, AI
Debug, configuration, semantic exceptions, parks, completion labels, and
build rules are untouched.
