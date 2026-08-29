# `punpckhdq/main` AI-debug harvest

This ledger records the admissible progress harvested from
`punpckhdq/halo` commit `fc47f9a15a0aea7f0081394c4d66db5c68ab5323`.
The authoritative integration base was
`6a1c13bb6010d567986af01bdc4749a1e86bf888`.

## Retained result

`source/ai/ai_debug.obj` advances from 11 to 23 strict-exact functions. The
12 newly exact functions total 1,600 padded code bytes. Equality here means
equal section size, normalized bytes, relocation count, relocation addresses,
and strict relocation destinations; no comparator exception is involved.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_ai_debug_get_last_path` | 96 | 1 | `5d2f7e783baa1414472892577ef332370b9cfee457b1ffa00677648caa89ddf5` |
| `_ai_debug_get_path_storage` | 256 | 9 | `6e5a52ef2d8fec8e52e31249db86265ef77ebc1717ca5a0f60856c81006ff9e7` |
| `_ai_debug_select_encounter` | 80 | 8 | `a3a265121321b1044af4ebaf7a3e7d02517add2266c5ac7e6e80f9889f0c83cd` |
| `_ai_debug_sound_point_set` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_ai_debug_lineoffire_new` | 96 | 9 | `ee77d42c20d1c10640c7c9fd3abcf00a6ff4aac44a69f448888c6e72c288724e` |
| `_ai_debug_lineoffire_addpill` | 160 | 15 | `58f769fb2da93176a21e2588d3705fa7df2d9c5d554c4c8b26c01ddd44283c6a` |
| `_ai_debug_lineoffire_success` | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_ai_debug_lineofsight_reset` | 16 | 2 | `5fa6b7a179d62ac56920f83faa7cc0c64e610df71b53d63b820b2afe60a10ac7` |
| `_ai_debug_describe_actor` | 432 | 25 | `c0cfefddd21d25eee68399b3d736bff0314798cdfbfee8ac916567dae5869d48` |
| `_ai_debug_vocalize` | 208 | 10 | `09e8e64be69ea66f9cc149597aac57d47520c7aa9115a845c1da77906ea13d4d` |
| `_ai_debug_speak` | 112 | 10 | `1eb4410740e5df5d5e6e21b968ff5e54bbcf8b3d910b84d8f17b6bf9fc97c4fc` |
| `_ai_debug_select_actor` | 112 | 9 | `dc346bcd54fcefba3128b1bb185c11b6c466fd3cee6a106b1d7316b8e5f48773` |

The donor was adapted rather than copied wholesale:

- The canonical `selected_squad_index` member name is preserved so the
  inherited exact `_ai_debug_highlight_cluster` keeps its relocation identity.
- The donor's missing 16 KiB debug-state span is represented locally in
  `ai_debug_state`. The shared `path_state` and `path_debug_storage` layouts are
  unchanged, preserving every non-debug AI consumer and its layout assertions.
- The existing early `ai_debug_state` field names and raw compatibility span
  remain in place for `actions`, `actor_looking`, `ai_communication`, and the
  scripting globals.
- Typed dialogue prototypes and the dialogue-definition declaration are added
  for the two speech functions. `unit_speak` is const-correct with the existing
  declaration in `ai_communication.c`.

## Audited but not retained

- `breakable_surface_effect`: the upstream 4,032-byte near match relies on an
  inline-assembly `fast_ftol`. An ordinary-C replacement retained all 11 sibling
  exact functions but left 2,526 normalized byte differences, so the entire
  candidate was removed.
- `collision_log_end_time`: the public function gates exact, but its required
  256-byte private user-selection helper cannot prove strict relocation
  identity because the split target aliases the common user-array address to a
  different data owner. The comparator fails closed, as intended, so the pair
  was removed.
- The donor's unsafe `real_point3d` cast in `ai_debug_render_actor`, raw-address
  comments, placeholder render bodies, and unrelated declaration changes were
  not imported.

## Policy and validation

The retained diff is ordinary readable typed C. Its added lines contain no
assembly, `volatile`, `register`, pragmas, attributes, intrinsics, raw
addresses, pointer-punning casts, byte forcing, or comparison exceptions.
No-argument prototypes use explicit `void`, parameters are one per line, C89
declarations precede statements, subsystem tag-block access stays typed, and
all retained functions have explicit terminal returns.

- Strict object gate: `23 exact / 2 residual / 35 unwritten` of 60 listed.
- Full Ninja graph: passes; 470 units scanned and zero unit errors.
- Semantic report: 4,806 functions evaluated, 4,682 semantic exact, 4,692
  accepted exact.
- Object admission: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, zero invalid.
- Campaign attestation scan: passes; reported aliases belong only to existing
  residual functions and grant no match credit.
- Tool tests: 205 passed.
- `git diff --check` and added-line policy scan: clean.

Reproducibility identities before the harvest commit:

- Retained `ai_debug.c` SHA-256:
  `dbf078447c1c4a992d85b52ae1e0232c9a65007cd0a76d26422b127273953722`.
- Rebuilt `ai_debug.obj` SHA-256:
  `704a86d6688a80945804ee8ecef29388fb3b27add939823f1f5a70ce454397bc`.
- January split `ai_debug.obj` SHA-256:
  `46285debcdecd5e3d5306b3bae6f944694728495afb17900edfe507eb642db78`.
