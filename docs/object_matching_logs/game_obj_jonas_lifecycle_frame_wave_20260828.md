# `game.obj` lifecycle/frame recovery (2026-08-28)

## Scope and provenance

This frozen natural-C wave adds only `_game_dispose`,
`_game_dispose_from_old_map`, and `_game_frame` in `source/game/game.c`.
`game.obj` advances from 8/27 to 11/27 strict-exact functions and remains
`NonMatching`; the wave contributes 544 padded target bytes. No residual body
was retained.

Before editing, the canonical repository `CLAUDE.md`, `AGENTS.md`, all four
existing `game.obj` ledgers, and the campaign tooling documentation were read.
The later HCEA reconstructions of `game_dispose.c`,
`game_dispose_from_old_map.c`, and `game_frame.c` supplied public typed source
evidence. The January split object remained authoritative for call membership,
ordering, assertions, relocations, bytes, and padding. In particular, January
uses `rumble_dispose`, includes `telnet_console_dispose` and
`transport_dispose`, omits the later save-film disposal calls, and passes one
double-speed-adjusted delta through the frame update chain.

The source base is commit `c76f7a2872374b7cea6c879895ab43ea4a610087`.
The pre-edit source blob was `bc81043cbf95809c4c55411b00125be0f256a25f`;
the retained source blob is `02c36f0242adbe22a84961b161557c0d39cdf947`.

## Frozen first shot

The only code-producing compile succeeded without source iteration. The strict
target-owner COFF gate reports 11 exact, 0 residual, and 16 unwritten functions.
Direct `tools/coff_compare.py` comparison reports `all_equal: true` for all
three candidates:

- `_game_dispose`: 192 padded bytes, 36 relocations, normalized SHA-256
  `639d29862ec448ba82411e6767dba464e4430597800a1ccbd3b6f1fbecac37ad`;
- `_game_dispose_from_old_map`: 192 padded bytes, 36 relocations, normalized
  SHA-256
  `26f6a082200b27db689f8e976a7d4c521a90914c23d73afabaeb855b8e1516c4`;
- `_game_frame`: 160 padded bytes, 16 relocations, normalized SHA-256
  `c31dc7dc4ac2e5a347184da2290e930053b35627a0c1aa0d833b4bb5b3dbfe52`.

The January target object SHA-256 is
`FD703BA666F13A5F1311DFBE1383EBA452CF49A0A9A7FAB47B1CFBFD06C1CFD0`;
the retained base object SHA-256 is
`4D26D46C792A557F30B2CFEADCD02906FA1C6442E1EF4FD231FE76ECFF154F9D`.

## Policy

The retained functions use typed runtime state, ordinary C89 source,
one-parameter-per-line formatting, explicit `void`, and terminal returns. The
runtime-state alias preserves the target assertion text while naming the
attested BSS owner. No protected source, assembly, intrinsic, raw address,
pointer pun, barrier, compiler substitution, object patch, or byte-forcing
construct was used.
