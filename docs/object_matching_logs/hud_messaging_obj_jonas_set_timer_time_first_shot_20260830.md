# `hud_messaging.obj` set-timer-time codegen boundary

## Result

`_scripted_hud_set_timer_time` remains unwritten after one natural typed C89
production candidate. Target and candidate are both 128 padded bytes, but the
complete normalized code and ordered relocation streams are not exact. No
alternate spelling, local-pointer retry, tuning, compiler switch, or sweep was
tried.

The January normalized SHA-256 is
`d2d65da4e3ebf60fece545a76b903e162cb46b127ff739eda3a31cb905cefb98`;
the candidate normalized SHA-256 is
`920aa07a2f5a3c1966937368a532b9aa4bd5fc8ce38fa245221545e9af8c0862`.
The immutable candidate object is
`build/audit/hud_messaging_set_timer_time_first_shot_20260830.obj`, with
whole-object SHA-256
`565a60e12196dda2388b68b0613d307573795dc830ac9b73691696fcf0e5c680`.

## Fixed boundary

Both forms compute 30 ticks per second from promoted `short minutes` and
`word seconds`, set the timer ticks, clear `paused`, set `enabled`, call
`_game_time_get`, store the reference time, and clamp the signed timer corner
to the inclusive range zero through four with explicit early returns.

January has three ordered relocations: `_bss_00453ab8` at `+0x0f`,
`_game_time_get` at `+0x2c`, and `_bss_00453ab8` again at `+0x32`. It loads one
timer-globals base for all three stores before the call, then deliberately
reloads the global base once after the call for the reference-time and corner
work. The natural canonical typed-global expression has six relocations: five
references to `_bss_00453ab8` at `+0x10`, `+0x20`, `+0x2c`, `+0x3e`, and
`+0x4a`, plus `_game_time_get` at `+0x38`. VC7 reloads the global for each of
the first three stores and again around the post-call work. That changes both
the instruction stream and relocation topology even though padded size and
semantics agree.

The independent Stian January-PC reconstruction and the clean Claude
HUD-messaging worktree corroborate the timer fields, arithmetic, clamp, and
the donor's local-base/reload topology. Their raw-address source was evidence
only; the rejected candidate used the canonical typed global and named timer
record. A local-pointer transcription would be a second spelling aimed at the
observed bytes, so the one-shot rule closes this function here. January COFF
alone decided the miss.

## Scope and restoration

Before the shot, the three canonical HUD ledgers, project and strict-tooling
instructions, the complete applicable Claude `CLAUDE.md` and `HANDOVER.md`,
registered trees, history, target disassembly, caller declaration, and the two
independent functional reconstructions were audited. A focused history and
ledger census found no earlier implementation candidate for this owner.

The sole candidate compiled once on the production object edge. Its focused
gate preserved all sixteen inherited exact functions and reported four
residuals and sixteen unwritten owners, with this function as the only new
residual. The candidate was inverse-reverted. The restored source blob is
exactly `8a4bffa85d9811964ea79449c2166f1cd76c43fa`; a normal restoration build
reports 16 exact, three residual, and 17 unwritten functions.

The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or byte offset, pointer or union pun, undefined
behavior, synthetic anchor, or byte forcing. Units, Vehicles, Matrix Math, AI
Debug, configuration, semantic exceptions, parks, completion labels, and
build rules were untouched.
