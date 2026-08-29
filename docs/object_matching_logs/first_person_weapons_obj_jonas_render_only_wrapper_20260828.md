# `first_person_weapons.obj` Jonas render-only wrapper (2026-08-28)

## Result

This bounded ordinary-C lane starts from pushed integration commit
`c8c40b2ceaaee631f11abbed03447257c83f9a36`. A direct January ABI audit
reduced the proposed public-wrapper batch to the sole owner whose complete
call boundary is ordinary cdecl. That one frozen candidate was strict on its
first code-producing compile and is retained without a spelling, declaration,
control-flow, scheduling, or compiler-control retry.

`source/interface/first_person_weapons.obj` advances from 4/34 to 5/34 exact
functions and remains `NonMatching`. The wave gains 27 meaningful and 32
padded code bytes, one exact relocation, and no data credit.

| Retained owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_animation_update_render_only` | 27 | 32 | 1 | `afd8b647ac6c7c34236688e364dfef509434838b3c220383f8a6c9e3f8973f45` |

The hardened COFF comparator proves equal padded bytes and exact relocation
address, type, destination, and addend.

## Provenance and ABI boundary

The clean HCEA oracle is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Its
`src/animation_update_render_only.c` donor is Git blob
`166afa7644d1d54911670e118f9b3491bcd4bd24`, 632 bytes. It supplies the
readable four-argument wrapper topology and render-only selector value.
January COFF remains the authority for the 32-bit types, cdecl ABI, emitted
bytes, and relocation.

The Stian history at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` and Pastudan history at
`918af885935ec470a31256ecce9a977b12b01f80` were searched directly. Neither
history supplies an independent named definition of this wrapper. Their
absence is recorded rather than converted into unsupported corroboration.

The January body loads the three incoming 32-bit arguments from ordinary
stack slots, pushes them right-to-left, pushes the render-only selector zero,
calls `_animation_update_internal`, performs caller cleanup, and returns the
callee's signed-short result. Its sole relocation is REL32 at `+0x12` to
`_animation_update_internal`, addend zero. The retained typed declaration is:

```c
short animation_update_internal(
	short animation_type,
	long animation_graph_index,
	struct animation_state *state,
	long *sound_index);
```

The retained wrapper has three ordinary 32-bit parameters and ends in an
explicit return of that call. No private or register convention is named or
simulated.

## Corrected candidate boundary

A read-only census covered 1,469 worktrees and found no dirty
`source/interface/first_person_weapons.c`. Applicable campaign, tree,
Claude-history, prior object-ledger, HCEA, Stian, Pastudan, and January
evidence was read before emission.

The other three initially screened wrappers were excluded before compilation:

- `_first_person_weapon_message_from_weapon` calls
  `_code_000cc290`, `_code_000cd670`, and `_code_000cbf00` through
  compiler-owned same-TU register argument placement;
- `_first_person_weapon_get_marker_by_name_render` calls
  `_code_000cc290` through the same private boundary before its ordinary
  marker call;
- `_first_person_weapon_message_from_unit` calls
  `_code_000cc2f0`, `_code_000cd670`, and `_code_000cbf00` through
  private register placement.

Their January disassemblies pass live arguments in EDI, EAX, or EBX without
cdecl pushes. Reconstructing or declaring those private contracts merely to
shape the wrappers would violate the campaign policy. They are pre-excluded,
not failed spellings, and produced no candidate code.

The prior allocation/indexing ledger's no-retry dispositions for
`_first_person_weapons_initialize_for_new_map` and
`_first_person_weapon_get_local_index` remain in force.

## One-shot proof and fingerprints

A production-flag `/Zs` parse-only check passed before code emission. The
baseline object SHA-256 remained
`c7a192b516146bab3ee9f9bd5e14c18eae59c2a15ddd90e657eeb818446c0166`
before and after that check. Exactly one normal Ninja compile then emitted
the wrapper, and its first gate was strict.

- Baseline source Git blob:
  `a26bf37e5434611b7736bbfc37a876ae7f519ee3`.
- Retained source Git blob:
  `0e848507d88d260a57d97bfae9de0fc9fe30e47f`.
- January split `first_person_weapons.obj` SHA-256:
  `b60642b1e92d0b35661172f7fd6c8df47934231f20e431b97b816e039a789e76`.
- First candidate object SHA-256:
  `fe2b447fe49fcc736e574eaa0f3fb3012cae74bb6b4f9f300da5d730e5ae300d`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
  SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 flags.
- Target/candidate section: 32 padded bytes, one relocation, and identical
  normalized SHA-256
  `afd8b647ac6c7c34236688e364dfef509434838b3c220383f8a6c9e3f8973f45`.
- `tools/campaign/gate.py source/interface/first_person_weapons --all`:
  5 exact, zero residual, 29 unwritten of 34.
- Full `halobetacache_build`, `libcmt_build`, semantic report, and progress
  graph: pass across 572 steps.
- Semantic audit: 470 units, 4,690 functions evaluated, 4,561 semantic exact,
  4,571 accepted exact, and zero unit errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 13 active, zero stale, and zero invalid.
- Complete tooling suite: 205/205 tests pass; source and ledger whitespace
  checks are clean.

Only `source/interface/first_person_weapons.c` and this Jonas-owned ledger
are changed. No header, storage owner, configuration, semantic exception,
parked entry, completion label, Units, Vehicles, Claude-active file, or other
object is modified. The retained source is ordinary C89-compatible typed C
with one parameter per line and an explicit terminal return. It contains no
assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier,
undefined behavior, raw address, byte-offset access, representation pun,
private ABI, artificial anchor, or byte/code-generation forcing.

No commit or push is performed in this isolated lane.
