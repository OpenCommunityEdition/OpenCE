# Player camera-effect matrix: bounded coupled-source negative

Production unchanged. Canonical base fdf76bd0. Target function:
`_player_effect_get_camera_effect_matrix`, 1312 bytes / 49 relocations,
normalized SHA `34ba1689676f40ae08fd01a8f705c1ab0e81fba92ad6c4eac74e6565e02c324a`.

Current source is 1280/49, SHA
`36cc5e8fb04363d6e87bd5c48a971b5b17f6b318fcc37bdc8255024718cded67`.

The old cm2 packet recovers two distinct matrix lifetimes plus real point/vector
helpers. Re-measured on current headers it is 1312/49, still fuzzy. The old
whole-TU copy predates the accepted realcmp camera-impulse closure. The final
scratch source restores that current realcmp code before the full-TU checks;
no prior exact landing was lost or overwritten in production.

## Independent source leads and probes

Raw later unoptimized executable, read as data only: 0x57bce7..0x57be7a nests
six random-range calls in their consuming rotation/translation arguments.
It does not use canonical's six named random-value locals. It calls the real
local-random helper. Later arithmetic groups random * intensity * maximum;
January's own extent/hash remains authoritative for whether this transfers.

| Probe | Padded / relocs | Result |
| --- | --- | --- |
| historical cm2 | 1312 / 49 | nonexact |
| nested seed calls | 1296 / 49 | nonexact |
| nested genuine real_local_random_range | 1312 / 49 | same target-function hash as cm2 |
| later multiplication grouping | 1312 / 49 | same hash as cm2 |
| attested game-time result local | 1312 / 49 | same hash as cm2 |
| /Od impulse/shake pointer bindings plus nested helper | 1296 / 49 | nonexact; worse |

The game-time local is evidenced by the later call/store at 0x57bae1..0x57bae6;
its descriptive spelling is inferred and nonuse is disclosed. Its inert result
does not license adding further locals. Pointer bindings are observed at
0x57be97..0x57bea9. This was a coupled source test, not a declaration-count sweep.

The 1312-byte family has SHA
`d67d3e6b20bbb55cc77563e95e79d8e8a6cff27c63b35ba339ef17eb29cef709`.
It has 390 decoded instructions like January, but cross-product operand order
at +0x2c0..+0x2e2 still differs. Thus this source lead does **not** explain the
remaining compiler-state difference. Pointer-binding variant SHA:
`a541f6ec096e76b9b7b207c005a7a7af2a11090fcdf76eaad6600cc7b981df46`.

Both final current-context whole-TU gates preserve all 27 exact functions and
leave two residuals. The camera matrix candidate contains an attested view cast
and newly emitted real helper copies; their exact-caller admission condition is
not satisfied, so no candidate is proposed for production or strict credit.

Source/objects/edits and pre-probe reasoning are preserved in canonical
`scratch/astra_five_functions_20260926/player_effects/`. Use `nested.c` with
`local_helper.json` to reproduce the complete current-context 1312-byte family.
`cm2_historical.c` is a historical control, not an integration source.

No compiler flags, headers, asm, aliases, scorer or ownership rules changed.
