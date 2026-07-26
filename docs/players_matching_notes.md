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
