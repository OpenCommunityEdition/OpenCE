# hud_nav_points: canonical reconciliation ledger

## 2026-10-01: custom waypoint renderer exact

Reconciled against canonical `6bfd95673719d54b92d0b23f01dc9df4c0679768`.
The natural same-compiler donor is punpckhdq/halo commit
`2e5b1da` (HUD nav-points reconstruction), inspected at upstream
`6a23ce979ac09f7ca032449ca205ceb82eac4b81`. This is a reconstruction,
not recovered Bungie source text. Names not separately authenticated are
descriptive donor names.

Only `custom_render_nav_point` changes. Canonical types, API signature,
constant names and assertions remain authoritative. The donor's pointless
multiply by 1.f is removed; exactness survives. All alpha shifts use
unsigned `pixel32`. The existing bitmap const-API adaptation is retained,
and the known long argument uses integer zero. No header, prototype,
helper body, compiler option or held single-axis parenthesis patch changes.

Strict extent, bytes and all relocation identities match January:
1,618 meaningful / 1,632 padded bytes, 95 relocations, normalized SHA-256
`287a20cd338648964303d1f60b46e9cdf94753f43718b48f8c648a0dd9e60306`.
The five new shared-inline copies (`distance3d`, `distance_squared3d`,
`magnitude_squared3d`, `power`, `vector_from_points3d`) match January and
current providers in code and relocations. No duplicate credit is added.

Six new /W3 C4244 warnings are deliberately disclosed: clamped long-to-byte
alpha; two real-to-short pixel offsets; two long-to-short number arguments;
one double-to-real fmod result. These are existing narrowing boundaries
previously written with explicit casts, not new ABI mismatches. Alpha is
bounded by PIN to 0..255; offset/number range assumptions are not newly
claimed proven. No warning-silencing casts are introduced.

The unit is 32/32 strict exact and its ownership audit passes. It remains
NonMatching pending a separate object-admission decision; this landing
does not grant object credit. The renderer park is retired after fresh
verification. Historical negative probes and holds remain in the
date-stamped hud-nav-points ledgers. See
[the full reconciliation receipt](upstream_two_reconcile_20261001.md).
