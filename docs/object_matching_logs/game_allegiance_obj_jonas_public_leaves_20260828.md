# `game_allegiance.obj` Jonas public-leaf wave

This 2026-08-28 Jonas/Codex ledger records one frozen natural typed-C batch
for `source/game/game_allegiance.c`. All five attempted public leaves were
strict on the first compile and are retained without a source-shape or
code-generation tuning pass. The object advances from 3/15 to 8/15 strict
functions and remains `NonMatching` with seven symbols unwritten.

## Scope, instructions, and provenance

- Clean pushed integration base:
  `d77b090a375439e0dce06640feea4b27d25047d8`.
- Isolated branch: `jonas/game-allegiance-public-leaves-20260828`.
- Isolated worktree:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/game-allegiance-public-leaves-20260828`.
- Baseline source blob:
  `e18fd7a9451d8b5f12815e7864b5d5812554cfaa`.
- Retained source blob:
  `9509ddb334cf5c321d79de6ab9e6720363233aec`.
- The complete prior lifecycle ledger was read before editing; its blob is
  `2c087a613410d2a9467826c1cc8b6737921706e8`.
- The applicable Stian instruction corpus was read in full in the current
  campaign context. `AGENTS.md` and `CLAUDE.md` have SHA-256 fingerprints
  `a5188f5a573ac3dcf077f50f49ffb99cfb69b1df138f414242207e53dc975b3d`
  and
  `116eda38343bf1a290f0b04797a74ab92c97dfa12eeb2a17f813480f5ec70bbb`.
  The authenticated Pastudan copies are byte-identical to each other at
  SHA-256
  `39736825cebf6edd2d2d8e8f9481d36b36954227f00370fcd9c089b6e045c561`.
- The exact local tools used were `tools/campaign/gate.py`,
  `tools/coff_compare.py`, and `tools/audit/dump_coff_disasm.py`, with blobs
  `37880af12887a6b4eb455ef800d23dc13a80c500`,
  `54fa2025044da29225bef633f33c0a9ec0ea603f`, and
  `2a3e0bd1b1406d966600e94271a00e511fa12b87`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `005dc3acdefcc3bbb2c041087494c54531400a18babab990fb405a27172ed9cf`.
- XDK compiler:
  `C:/halo-worktrees/claude-finish-hs-20260816/xbox/bin/vc7/CL.Exe`,
  SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The object uses the pinned build flags `/O2 /Oy- /DDEBUG /Dxbox` and the
  inherited project include set.
- Only `source/game/game_allegiance.c` and this additive ledger are changed.
  Units, Vehicles, all Claude-active sources, headers, build configuration,
  frozen objects, and every other object are untouched.

## Donor audit and ownership mapping

The clean local HCEA semantic oracle is pinned at
`c168af2e747d3095d9a29418ae401f3a39544863`, with authenticated origin
`https://github.com/surreptitiousresearch/halocea.git`. Every
`src/game_allegiance_*.c` donor was read before the batch:

| HCEA donor | Blob |
| --- | --- |
| `game_allegiance_broken.c` | `77978862e0a2ecfd4bd38af1c5dd9d8ede4808d0` |
| `game_allegiance_create.c` | `3de97ad70758ff0cf57a8627699cf109e5dc0cd9` |
| `game_allegiance_dispose_from_old_map.c` | `fc4f896121300dd6ec886eb838539a307d51c5ce` |
| `game_allegiance_dispose.c` | `83cd6ec7838a9a065889313ab4efbee5eaafd01a` |
| `game_allegiance_get_incidents.c` | `ef46fb81a33424f35c8135026412e78b5d3b1856` |
| `game_allegiance_incident.c` | `2ee88a1c159b0d496a7f63e95a011ecbd487f52d` |
| `game_allegiance_initialize_for_new_map.c` | `3e7dd6d1a89e42faf85c998071deb0da75059eb9` |
| `game_allegiance_initialize.c` | `079be9b4c6a998d9dd94dc54c4f0da5002e2be09` |
| `game_allegiance_notify_change.c` | `7df7980f1509bff6c773c3403e2a06f0a9c39d2b` |
| `game_allegiance_provoke.c` | `7da0d9a20c05ae55debe22cbc26cb69392aa7b35` |
| `game_allegiance_remove.c` | `814148ff755aa7cef5060d6342e726cc4415bf78` |
| `game_allegiance_update.c` | `4a7ed1058f4ff34c4f406eb7da96b95c5d1907fb` |

The typed HCEA record, globals-layout, and pointer-storage donor blobs are
`62981f25f2c82ab2d7ef305ea486ac60b45241fe` and
`7d085987fbdc4415795ee76c7ec943d0c3f2db1a`, and
`7278b7c547036e32a4fada2fac2a8eea5ec904e6`. They authenticate the 0x12-byte
record, the eight-record array at `+0x02`, the ally bitvector at `+0x94`, the
friendly bitvector at `+0xA4` inside the 0xB4-byte globals block, and the
single globals-pointer storage model.

The Pastudan repository was clean and pinned at
`918af885935ec470a31256ecce9a977b12b01f80`; its complete allegiance source
blob `19602ba8282bd0156c2aa7872dde5f749d1885b2` independently corroborates all
five meanings and the same record traversal. It was used only as semantic
corroboration because that donor expresses storage with raw offsets. The
Stian sparse reference is pinned at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` and records allegiance source blob
`d2ac5fff90eb5d90018d678c33dd9abeef6e0ca3`; that promisor blob was not
materialized locally, so no claim or emitted source depends on it.

January names, addresses, sizes, field offsets, bounds behavior, and
relocation identity remain authoritative. The mapping is direct:

- `_game_team_is_enemy` at `0x000960A0` validates both team indices in
  `[0, 10)`, returns true for invalid input, and negates the `+0xA4`
  friendly-bit test.
- `_game_team_is_ally` at `0x00096100` validates the same bounds, returns
  false for invalid input, and tests the `+0x94` ally bit.
- `_game_allegiance_get_incidents` at `0x000961B0` walks 0x12-byte records in
  either team ordering and reads `incident_threshold` at `+0x04` and
  `current_incidents` at `+0x0E`.
- `_game_allegiance_provoke` at `0x00096230` accepts either ordering only
  when the corresponding suspicious byte at `+0x08` or `+0x09` is set, then
  rearms `current_incident_decay_time` at `+0x10` from `+0x06` when allowed.
- `_game_allegiance_notify_change` at `0x000962A0` walks the same symmetric
  pair topology and clears `status_changed` at `+0x0B`.

## Frozen batch and exact results

The baseline gate measured exactly 3 exact, 0 residual, and 12 unwritten
functions. Exactly the five mapped functions above were then compiled
together once as a natural typed-C batch. No alternate expression, local
lifetime, declaration order, loop form, cast, macro, or other residual-tuning
candidate was compiled.

All five were strict on that first compile:

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_game_team_is_enemy` | 83 | 96 | 1 | `ce0607d1aec6334726c37ccf28790c312fb7c24869080f63a31922abacbdad14` |
| `_game_team_is_ally` | 80 | 80 | 1 | `0d69bf507152323a8110f41a53fa64a7863ab27ffc7701fd48218ae5119a45bd` |
| `_game_allegiance_get_incidents` | 125 | 128 | 1 | `5d01487de94f9d049e7fe1f8c26a51d502a5bf9ab5a893a2c49530add3dd427b` |
| `_game_allegiance_provoke` | 111 | 112 | 1 | `649c9df5c39ebbe18ec0c896df3151e91fe943468039f46b63b1a6877b78d92e` |
| `_game_allegiance_notify_change` | 81 | 96 | 1 | `ca0733ec350a57049e9afdb664ca110e10adf31e77fef5440348bded66e930be` |

For every row, the hardened comparator proves identical padded bytes and
exact relocation address, type, destination, and addend. The single
`IMAGE_REL_I386_DIR32` relocation is to `_bss_0043e490 + 0`; its section
offset is `0x3D` in both predicates, `0x08` in `get_incidents`, and `0x05` in
`provoke` and `notify_change`.

First-gate candidate object:
`scratch/_gate_31228.obj`, 2,692 bytes, SHA-256
`d77f70737e584381c6fc5cc4ca2f5c5e0693e88b038e0f9d1257924020ce9d7e`.
Because every attempted body was exact, reject and support pruning was a
no-op. The unchanged retained-only replay again measured 8 exact, 0 residual,
and 7 unwritten functions. Replay object: `scratch/_gate_29268.obj`, 2,692
bytes, SHA-256
`c83d3e3e321e615e5d13b1fe9e388465a422d824bef333631aa530c6b82ffa76`.
The raw hashes differ only in COFF timestamp and scratch-source-name metadata;
all admitted section fingerprints and relocations replay exactly.

The five new functions add 480 meaningful and 512 padded code bytes. With
the inherited initializer and two disposer leaves, the object now has 521 of
1,868 meaningful code bytes exact. The remaining unwritten symbols are
`_game_allegiance_initialize_for_new_map`,
`_game_team_ally_status_changed`, `_code_00096300`,
`_game_allegiance_update`, `_game_allegiance_create`,
`_game_allegiance_remove`, and `_game_allegiance_incident`.

## Strict target-owner and policy gates

The replay object declares `_bss_0043e490` only as an undefined external:
section zero, value zero, type zero, external storage class. It defines no
`.bss` or `.data` bytes. Its only defined external non-code symbol is the
compiler-generated 24-byte `"game allegiance globals"` literal COMDAT already
required by the inherited exact initializer. Therefore none of the target's
232 non-code bytes or owners is claimed by this wave.

All retained additions use the existing typed globals aggregate and
`BIT_VECTOR_TEST_FLAG`; there is no raw offset or integer-address access. The
source is C89 declaration-safe, uses explicit `void` for no-argument
functions, places each parameter on its own line, and gives every function a
terminal return. It contains no assembly, volatile/register scheduling
device, pragma, intrinsic, barrier, undefined behavior, representation trick,
synthetic anchor, or byte/code-generation forcing.

Validation results:

- Baseline gate: 3 exact, 0 residual, 12 unwritten.
- Frozen first gate: 8 exact, 0 residual, 7 unwritten.
- Retained-only replay: 8 exact, 0 residual, 7 unwritten.
- Strict target-owner COFF inspection: pass, zero BSS/data ownership.
- Forbidden-token and raw-address audit: pass.
- `git diff --check`: pass.
- Worktree scope: one modified C source and this one additive ledger.

No commit or push is performed in this lane.
