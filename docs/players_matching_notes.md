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
