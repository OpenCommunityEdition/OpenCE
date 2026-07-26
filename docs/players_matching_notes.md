# Players.obj matching notes

## `code_000ab820` powerup expiry

The authentic reconstruction iterates the two `player.powerup_durations`,
decrements active timers, and calls `code_000aa560` when a timer expires. A
compile-time-dead reference to `code_000ab820` retained both private bodies.

- January and candidate were both `0x70` padded bytes with five identical
  relocations.
- The first normalized divergence was the inlined powerup-index test: January
  emitted the long `test edi, edi`; the exact standalone `code_000aa560` source
  requires `(short)powerup_type` and inlined as `test di, di`.
- Adding an explicit long outer guard allowed the inner short test to fold, but
  VC7 expanded `code_000ab820` to `0x80` bytes.
- Calling `code_000aa4f0`/`code_000aa530` from the already-exact public powerup
  handlers changed their machine code, so those call-form experiments were
  reverted.

The `code_000ab820` foundation was not admitted because it added no exact
function credit. Its source and retention experiment may be revisited when
`players_update_after_game` is reconstructed.

## `players_update_before_game`

The dependency-complete action/update loop reconstructs to January's exact
`0x6E0` padded size with all 101 relocation identities and addresses aligned.
After restoring the proper `main_respawn(boolean)` declaration, only four
normalized instruction bytes differ:

- January keeps the player pointer in `ECX`, loads the unit index into `EDX`,
  and later forms `&iterator` in `EAX`.
- VC7 instead coalesces the unit index into `ECX` and forms `&iterator` in
  `EDX`.
- Passing `player->unit_index` directly increased the residual to ten bytes.
- Removing the compiler barrier produced the same worse schedule.

The proven four-byte baseline retains the local `player_unit_index` and
`_ReadWriteBarrier()`. The function remains dormant and uncredited until its
register-allocation tie is resolved.

## `players_update_after_game`

The authentic post-game loop and its expiry dependency reconstruct to
January's exact `0x330` padded size with all 52 relocations. The first CFG
divergence occurs when forming the scenario BSP-switch block pointer:

- January emits `lea ecx, [eax+0x39C]` and keeps the loop index in `EDI`.
- VC7 emits a base copy, offset load, and offset add, then keeps the loop index
  in `EBX`; this creates a nonuniform register/branch-layout drift through the
  loop.
- The historically grounded `byte * + offsetof(...)` expression compiled
  identically to the named-field expression and did not improve the residual.

The semantic lift remains uncredited and is not retained in `players.c`.

## `code_000a9ff0`

The semantic PVS reconstruction has January's exact `0x170` padded size and
all 22 relocation identities. Only one instruction-scheduling window differs:

- January emits `lea ecx, [ebp-0x14]`, stores the activation-call result at
  `[ebp-4]`, then loads `player_data`.
- VC7 stores the result, loads and pushes `player_data`, and only then forms
  `&iterator`; the `player_data` relocation is therefore at `+0x32` instead of
  January's `+0x35`.
- The normalized residual is 11 bytes at offsets `0x2E` through `0x39`; every
  instruction after that window matches.
- An explicit iterator pointer, a result temporary plus compiler barrier, and
  an aggregate exposing the iterator/result stack adjacency all compiled to
  the same baseline schedule. Argument-expression and premature-pointer
  variants either retained that schedule or worsened it.

The function remains dormant and uncredited as a scheduler/lifetime tie.

## `code_000aa9e0`

The independent-pointer reconstruction retains January's exact `0x510`
padded size and all 61 relocation identities. Its residual begins with a
stack-home and register-allocation difference and expands into a block-layout
tie:

- January homes `player`, `player_unit_index`, `biped`, and
  `source_root_object` at `[ebp-8]`, `[ebp-0x14]`, `[ebp-0x10]`, and
  `[ebp-0xC]`; VC7 uses a `0x78` frame, omits the source-root home, and maps the
  other three to `[ebp-4]`, `[ebp-0xC]`, and `[ebp-0x14]`.
- January keeps the initial `biped_get` result in `ESI` before homing it and
  later uses a stack-homed source-root pointer. VC7 instead homes the biped
  immediately and keeps the source-root pointer in `ESI`.
- Splitting the two-pointer aggregate into independent locals restored the
  exact `0x510` size after a failed CFG experiment, but did not resolve the
  opening register allocation and is not retained in production source.
- Whole-variable and targeted volatile access produced the `0x7C` frame but
  inflated the body to `0x520` and mapped the homes incorrectly.
- Hoisting/reordering scalar declarations was byte-identical to the best form.
- Late-label and single-fallback CFG rewrites shortened the body to `0x500`;
  VC7 either placed the fallback block early or over-merged the source paths.

The aggregate baseline remains dormant and uncredited pending a compiler
register-allocation/block-layout solution.
