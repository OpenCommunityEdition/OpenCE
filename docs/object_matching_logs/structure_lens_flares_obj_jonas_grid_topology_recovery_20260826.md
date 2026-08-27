# `structure_lens_flares.obj` grid-topology recovery

## Scope and result

This wave restores the strongest previously measured
`_build_structure_lens_flares` source/config baseline and then incorporates one
new ordinary-C topology recovered from the user-supplied
`stianeklund/halo` disassembly lift. The object remains `NonMatching`; three of
four functions and all target-owned runtime data remain strict-exact.

The retained residual advances from 4000 padded bytes / 157 relocations to
4192 padded bytes / 156 relocations. The January target is 4336 / 156. The
complete relocation destination/type sequence is now equal and the remaining
addresses differ only because the body is still 144 bytes short.

No target object, comparator, semantic allowlist, parked entry, protected
Units source, or shared header changed.

## Recovered historical baseline

The integrated tree had retained the older source blob
`e25c9086649f62b762eb664a8acc4394565250c0` and lost the file-specific
`/QIfist` option. Historical closeout commit `b7a25968` and
`docs/object_matching_logs/structure_lens_flares_obj.md` on that commit prove
the stronger source blob
`eba2291e8389f042e85a6c3747756ec93d0b35f0` together with `/QIfist`.

That prior evidence is target-observable: January has zero `__ftol2`
relocations at the seven float-to-integer sites, while the default XDK edge
emits seven. `/QIfist` removes them and already has campaign precedent. The
historical source also restores the target's duplicated cluster lookups,
unrolled three-vertex geometry access, cold success/failure topology, and
temporary-marker copy order.

The one replay compile reproduced the documented baseline exactly:

- target/candidate padded bytes: 4336 / 4192;
- relocations: 156 / 156;
- candidate normalized SHA-256:
  `e36becefdfb9ac094555b749ca307c3a40b7541e913998d1fb87589f79932263`;
- normalized differing-byte distance, counting the 144-byte length gap:
  3843.

## New source oracle and bounded candidate

The user-supplied repository
`https://github.com/stianeklund/halo` contains a disassembly-backed readable
lift at `src/halo/structures/structures.c`, function
`build_structure_lens_flares`. Its target notes identify the grid walk as:

- long `u_grid` / `v_grid` induction values;
- unsigned-short-derived `u_count` / `v_count` down-counters;
- an outer-row binormal offset computed once before the inner loop; and
- explicit component-order point construction.

Only that ordinary-C topology was adapted. The repository's separate
`x87_round_to_int` inline-assembly helper was not imported or used.

The first compiler attempt stopped on a transcription brace mismatch and
emitted no object. After repairing only that syntax error, exactly one normal
code-producing compile ran. The retained result remains 4192 / 156 but reduces
the normalized differing-byte distance from 3843 to 3807, a 36-byte structural
improvement. Its normalized SHA-256 is
`74ceb6c2e9342e437708e6deaee8738241bffb4e16c1ee9dc66fbe12f9cf64d7`.
The relocation destination/type sequence remains equal to January.

## Strict sibling evidence

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_00183bb0` | 32 | 0 | `eb9585d53a8f4c0f6971e075a24c20a637237729fe5c0aebc4f4fc13513dcdaf` |
| `_cluster_index_from_point` | 96 | 3 | `a96c1883d3ff5a5c4dcaf4a99ae17bb8eba00022ce2088c3c9ee3801f21f0ae8` |
| `_structure_lens_flares_place` | 16 | 2 | `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c` |

All three pass the hardened comparator after the retained edit.

## Remaining boundary

The principal proven gap remains January's seven locally narrowed float
results: it stores each through a 32-bit float home before the `/QIfist`
conversion, while VC7 propagates the x87 value through every legal-C spelling
previously measured. Whole-TU `/Op`, helper calls, representation tricks, and
inline x87 code are rejected. Reopen that micro-pattern only with authentic
source provenance or a new ordinary-C dependency/lifetime topology.

## Gates

- Full Halo and libcmt builds: pass.
- Semantic report: 470 units, 4333 functions evaluated, 4191 semantic exact,
  4252 accepted exact, zero unit errors.
- Campaign: 510604 / 2198102 code bytes, 4226 / 11060 functions, 377 / 833
  complete objects.
- Halo: 497690 / 1770166 code bytes, 4059 / 7574 functions, 275 / 468 objects.
- Admission: zero candidates, contradictions, and revocations.
- Parked validation: 3 active, 0 stale, 0 invalid.
- Tooling tests: 179 / 179 pass.
- `git diff --check`: pass.

