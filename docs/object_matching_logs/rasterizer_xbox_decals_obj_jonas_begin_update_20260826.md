# `rasterizer_xbox_decals.obj`: begin-update leaf (2026-08-26)

## Outcome

`_rasterizer_decal_vertices_begin_update` is newly strict-exact.  The wave adds
one exact function and 13 meaningful code bytes.  This object now reports 5/29
exact functions and 79 matched meaningful code bytes.  It is not complete, and
no completion label, semantic exception, adjudication, or data credit was
added.

Source commit:

- `b310a7a3da42c084f30059011cb65da2034372b7`
  (`Match decal vertex update begin`)

## Frozen January evidence

The untouched translation unit was frozen at commit
`c37bf400654be3c6ab28af73590a3840c6e74ac9` in:

- `build/audit/rasterizer_decals_begin_update_baseline_20260826.json`
- manifest SHA-256
  `ab14d59390fd59afa46c201818ac605cdb7e7dd8cddec169546fcbd65305e0d7`

The January leaf has 13 meaningful / 16 padded bytes:

```text
A1 14 00 00 00 50 E8 00 00 00 00 59 C3 90 90 90
```

Its normalized SHA-256 is
`9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc`.
It has exactly two relocations:

- `IMAGE_REL_I386_DIR32` at byte 1 to the defined BSS owner
  `_bss_0045e8e8` + 20;
- `IMAGE_REL_I386_REL32` at byte 7 to `_lruv_idle`.

The instructions are therefore fixed as:

```text
mov eax, dword ptr [bss_0045e8e8 + 20]
push eax
call lruv_idle
pop ecx
ret
```

The existing typed prefix in this translation unit independently proves that
owner +20 is `struct lruv_cache *local_vertex_cache`.  The exact
`dispose_from_old_map` sibling already reads the same field and calls the
authenticated LRUV API, while `rasterizer_decal_vertices_end_update` is the
exact paired no-op leaf.  Exact-name web/source search found no conflicting
implementation; January's object bytes and relocation identities remain the
authority.

The candidate keeps the BSS owner external, as the existing exact functions
do.  The hardened comparator accepts the unique symbolic owner anchor while
preserving the target's proven defined-owner identity and +20 addend.  No
partial BSS section or data credit was introduced.

## Candidate chronology

1. The first ordinary C89 candidate emitted the exact new function bytes and
   both exact relocations.
2. Because it was placed before four accepted siblings, the frozen whole-TU
   gate reported their otherwise exact COMDAT/debug evidence as renumbered.
3. That gate result was genuine new placement evidence.  The unchanged body
   was moved after the accepted public functions and rebuilt once.
4. The second comparison retained the exact new leaf, restored all four prior
   siblings to `still_exact`, and left only the expected new debug record and
   symbol inventory.

No ungrounded source permutation, compiler flag change, semantic allowlist,
adjudication, comparator exception, object patch, or byte-forcing expression
was used.

## Exact comparison

The hardened comparator reports `all_equal: true` for the new leaf and all
four previously accepted siblings:

- `_rasterizer_decal_vertices_begin_update`;
- `_rasterizer_decal_vertices_end_update`;
- `__rasterizer_decals_initialize_for_new_map`;
- `__rasterizer_decals_dispose_from_old_map`;
- `__rasterizer_decal_vertices_unlock`.

For the new leaf, target and candidate agree on all 16 padded bytes, normalized
hash, relocation count, both relocation types and addresses, the BSS owner
anchor +20, and the `_lruv_idle` call destination.  The corrected frozen gate
reports the new leaf as `newly_exact`, the four siblings as `still_exact`, and
`changed_nonexact: []`.

## Clean committed state and replay

The clean committed manifest is:

- `build/audit/rasterizer_decals_begin_update_committed_20260826.json`
- commit `b310a7a3da42c084f30059011cb65da2034372b7`
- SHA-256
  `4bd43dfb03bd00e1db501ee0642ada347f6784514fe598fe669bb1d467c44f84`

Its pre-rebuild and post-rebuild checks both return `ok: true`, no failures,
no warnings, no changed nonexact functions, and all five functions above as
`still_exact`.

The generated object path was resolved and verified inside the worktree.  That
single generated file was then deleted and rebuilt exactly once.  Raw base
object hashes were:

- before replay:
  `b4e73cbfd6ee24f7093227175a29cff55c8c2e4f410704c04ddd1beba24ab765`;
- after replay:
  `6d1bffda46f6831ee4d1af9d7fe011103885107f338983a1de1d180ec75fe17b`.

VC7 debug/build records are nondeterministic, so raw object hashes are not the
equality oracle.  The post-replay committed manifest is green and the direct
hardened comparison remains `all_equal: true`.

Pinned artifacts:

- target split object:
  `bcbbf5e61571861c22ed4503fae05f8760652ba2d53f87e7e5d112c4db23c892`;
- source file:
  `d1a66fc817dc5264a1d291fe7a7bb96aa5816838d1b6119fe0f02c75013e7f2a`;
- source blob before/after:
  `e5f8f853ce689f0f8c3870b08c7aeff1c421b3d1` /
  `1d49fc195ea9289bb94f9c0b1ea4b6bdf8697a82`;
- canonical report:
  `1af9d5909d7c6a48423fd170262ddefb797c9851dd8955dbfcd38624a8d5dd9c`;
- semantic report:
  `c09eb27ab1c11eaeb8d91d271fbb7d89c9dd488c28910673e415bbf3c3839e39`;
- VC7 compiler:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- hardened comparator:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- regression gate:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`;
- matching methodology:
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.

## Canonical progress and gates

After regeneration:

- all categories: 23.13% matched and 14.21% linked, 377/833 complete files;
- code: 508,449/2,198,102 bytes and 4,202/11,060 functions;
- data: 1,836,762/4,176,062 bytes;
- Halo category: 4,035/7,574 functions;
- semantic audit: 470 units and 4,310 functions evaluated, 4,167 semantic
  exact, 114 hidden-exact functions / 64,705 hidden bytes, 4,228 accepted exact,
  and zero unit errors.

Repository gates:

- object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` completion-label contradiction remains;
- parked validation: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass;
- Halo and libcmt dry graphs: no work;
- protected-path touches: zero;
- deleted tracked paths: zero;
- added-source banned-construct matches: zero;
- no push.

## House-rule audit

The no-argument list places `void` on its own line, and the void function ends
with an explicit `return;`.  The call uses the typed `local_vertex_cache`
field, not a raw owner offset.  The source is C89 and contains no assembly,
`register`, `volatile`, barrier, pragma, intrinsic, attribute, raw address,
pointer/integer reconstruction, undefined behavior, synthetic anchor, or
byte-forcing form.
