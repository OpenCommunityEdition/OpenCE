# `network_server_manager.obj` countdown-ready first-shot boundary

## Result

The first and only natural production candidate for
`source/networking/network_server_manager.obj::_server_ok_to_countdown` did
not match January and was removed completely. The source translation unit is
restored; this additive ledger records the measured compiler boundary and
grants no code, data, semantic, object-completion, or admission credit.

| Packet | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January target | 73 | 80 | 3 | `a9bc36fd8cd52ab9f5dd022472a05ced1636f2e10b09a4cb9a2de94153c154c1` |
| Sole candidate | 76 | 80 | 3 | `fb7ad95686af5bb469d84cf7d4a211af3ca2ef6574b28f67f7d3dbe1d2396bae` |

Both packets have the same ordered zero-addend REL32 schedule: `+0x09` to
`_server_has_enough_machines`, `+0x16` to
`_server_has_a_player_on_each_machine`, and `+0x23` to
`_server_needs_more_teams`. Their call order, short-circuit branches, signed
field load, signed comparison, stack frame, saved ESI lifetime, and 80-byte
section envelope are otherwise the same.

The isolated difference is the returned truth value. January emits
`MOV AL,1` / `XOR AL,AL`; VC7.1 emits `MOV EAX,1` / `XOR EAX,EAX` for the
natural compound expression. The three extra meaningful bytes are the two
wide result forms and their adjusted branch displacements. This is a return
materialization/compiler-topology boundary, not a missing semantic fact.

## Evidence and sole candidate

January COFF is authoritative. The current typed server layout proves that
the signed byte at server `+0x115` is
`game.minimum_player_count` and the signed word at server `+0x22C` is
`game.player_count`. The authenticated Pastudan donor independently supplies
the same four predicates, though its raw-address spelling and two speculative
helper names are not retained. No HCEA definition was found. Repository
history, the existing Network Server Manager checkpoint, object ledgers, and
current source contained no earlier body or measured boundary for this leaf.

The sole readable C89 candidate was:

```c
boolean server_ok_to_countdown(
	struct network_game_server *server)
{
	return server_has_enough_machines(server) &&
		server_has_a_player_on_each_machine(server) &&
		!server_needs_more_teams(server) &&
		server->game.player_count >= server->game.minimum_player_count;
}
```

It required only the ordinary external typed declaration of
`server_has_a_player_on_each_machine`. The generated XDK 3911 edge compiled
once. Direct hardened comparison then reported the equal padded size and
relocation packet but unequal normalized bytes above. No local-result form,
alternate return type, cast, temporary, regrouping, condition rewrite,
compiler option, or second candidate was compiled.

The rejected source used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced inline, barrier, raw address or offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing,
comparator exception, or tool exception. Reopening this leaf requires
genuinely new source/prototype/compiler evidence rather than a spelling sweep.

## Scope

The retained exact Network Server Manager set is unchanged. Vehicles, Units,
Matrix Math, AI Debug, Motion Sensor, Path Smoothing, LRA Cache, Collision
Usage, and all other excluded or concurrently owned lanes are untouched.
The protected Units sentinel is rechecked by the cumulative integration gate.
