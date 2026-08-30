# `hud_messaging.obj` time-code visibility first shot

## Result

The sole natural typed C89 candidate for `_scripted_hud_time_code_show` is
strict exact on its first and only code-producing compile. It contributes 39
meaningful bytes, 48 padded bytes, one function, and four ordered relocations.
`hud_messaging.obj` advances from 14/36 to 15/36 strict functions and remains
honestly `NonMatching`, with three documented residuals and eighteen unwritten
functions.

Target and candidate have normalized SHA-256
`006f9e4759cb29a3ab429115bbbbaf4c2289a959b9fa4538e5df7b6eb6f5b375`.
The immutable production artifact is
`build/audit/hud_messaging_time_code_show_first_shot_20260830.obj`, 5,759
bytes, with whole-object SHA-256
`a0a920d4753947033031917037f289d5e80e9cd1289864186ff235fca504bbd1`.

The relocation stream is identical in address, type, destination identity,
and addend:

| Function offset | Type | Destination |
| ---: | --- | --- |
| `+0x0B` | `IMAGE_REL_I386_REL32` | `_game_time_get` |
| `+0x10` | `IMAGE_REL_I386_DIR32` | `_time_code_time` |
| `+0x15` | `IMAGE_REL_I386_DIR32` | `_time_code_stop_time` |
| `+0x1D` | `IMAGE_REL_I386_DIR32` | `_time_code_time` |

## Binary behavior and source evidence

January loads the promoted Boolean argument as one byte. When true it samples
`game_time_get` once, stores the returned tick directly into both typed time
code globals, and returns from that arm. When false it stores `NONE` into
`time_code_time` and returns. The retained source expresses that topology
directly; VC7 emits the target instruction stream and padding byte-for-byte.

The independent January-PC reconstruction at commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` corroborates the same early-return
shape and state transition. The Claude HUD-messaging worktree at
`C:/halo-worktrees/claude-hud-messaging-3` independently records the same
`char` ABI and raw-address lift at commit `4e09cbab`; it was inspected
read-only. Its applicable `CLAUDE.md` requirements—binary authority, C89,
stable ABI, small changes, and no inline assembly—agree with this recovery.
Its separate clang patching and `/lift` workflow does not replace this
campaign's stricter January-PC COFF acceptance rules. No raw address from
either functional donor enters the retained source: the already authenticated
`long` externs own the state.

The existing HaloScript caller declaration independently fixes the public
signature as `void (boolean)`. Target disassembly confirms ordinary cdecl and
the byte-width argument read. No shared header or caller repair is necessary.

## Census, one-shot discipline, and gates

All prior HUD-messaging object ledgers, the relevant project and Claude
instructions, registered Claude/interface trees, current source history,
target disassembly, caller declaration, strict comparison tooling, and both
independent functional reconstructions were reviewed before editing. History
contains only the initial symbol inventory for this function, and the object
ledgers contain no earlier candidate or frozen boundary for it.

The pre-shot source blob was
`8bbfc076a7c2b681b4a89c14a676a7575b3d3447`; the retained source blob is
`dcab0fae97bfcb9b09b7c2d619f07f948bd81357`. The pre-shot focused gate was
14 exact, three residual, and nineteen unwritten functions. The first-shot
gate is 15 exact, three residual, and eighteen unwritten, proving all fourteen
inherited exact functions remain exact. The complete four-relocation strict
comparison reports `all_equal: true`.

The retained body uses no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or byte offset, pointer or union pun, undefined
behavior, synthetic anchor, or byte forcing. Units, Vehicles, Matrix Math, AI
Debug, configuration, semantic exceptions, parks, completion labels, and
build rules are untouched.
