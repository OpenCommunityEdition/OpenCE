# `hud_messaging.obj` timer-ticks codegen boundary

## Result

`_scripted_hud_get_timer_ticks` remains unwritten after one natural typed C89
production candidate. Target and candidate are both 80 padded bytes with two
matching relocation identities, but the complete normalized code and ordered
relocation addresses are not exact. No alternate spelling, tuning, compiler
switch, or sweep was tried.

The January normalized SHA-256 is
`c28cd50e0150845564921ef4ab5bb445c5a9cb58ac319a84461460fa53686468`;
the candidate normalized SHA-256 is
`79a172ded3b4745a41ba9d1b6408525b4181782d37e1497cb049dfcb1c1e3491`.
The immutable candidate object is
`build/audit/hud_messaging_get_timer_ticks_first_shot_20260830.obj`, with
whole-object SHA-256
`878d9a0a42a6c11618317bd3fdb4627a1a2d23b475bc9d4ae5535e6d7b6651bb`.

## Fixed boundary

Both forms load `_bss_00453ab8` at relocation offset `+0x03`, form the typed
timer pointer, test `enabled`, recognize `NONE`, skip elapsed-time adjustment
while paused, and call `_game_time_get` only on the active running path. The
target call relocation is at `+0x32`; the candidate's is at `+0x30`.

January materializes the `NONE` result with a five-byte `mov eax,-1`. The
natural short-return candidate preserves the already loaded low word through
a three-byte `or ax,ax`, shifting the following block by two bytes. After the
call, January zeroes ECX, loads and adds the two timer low words in CX,
subtracts the game-time result in 32-bit ECX, and copies CX into AX. VC7 emits
the readable typed expression as `mov ecx,eax`, a tick load into AX, then
16-bit subtract and add operations. This is a fixed return-materialization and
narrow-arithmetic scheduling boundary.

The independent Stian January-PC reconstruction and the clean Claude
HUD-messaging branch corroborate the field roles, `short` return ABI, branch
topology, and state arithmetic. Their raw-address forms were evidence only;
the rejected candidate used the canonical typed timer record. January COFF
alone decided the miss.

## Scope and one-shot discipline

The relevant HUD ledgers, project and Claude instructions, registered trees,
history, target disassembly, caller declaration, strict tools, and independent
functional reconstructions were already audited for the adjacent retained
time-code leaves. A focused census found no earlier candidate for this owner;
the one later history hit changes only its HaloScript evaluator wrapper.

The sole candidate compiled once on the production object edge. Its focused
gate preserved all sixteen inherited exact functions and reported this
function as the only new residual. The candidate was inverse-reverted; the
retained source blob is again
`8a4bffa85d9811964ea79449c2166f1cd76c43fa`.

The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or byte offset, pointer or union pun, undefined
behavior, synthetic anchor, or byte forcing. Units, Vehicles, Matrix Math, AI
Debug, configuration, semantic exceptions, parks, completion labels, and
build rules were untouched.
