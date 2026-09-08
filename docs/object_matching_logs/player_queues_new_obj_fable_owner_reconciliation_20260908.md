# `player_queues_new.obj` Fable owner reconciliation (2026-09-08)

## Verdict

This is an **admitted** owner packet based on canonical commit
`7f7b72cc3819f05e3e85956101f314b544825e08` and donor commit
`740bbc33aeaa7c2c6ee86304b7a52279df44a930`. It advances the focused unit
from 9/23 to 20/23 strict functions without changing any of the nine inherited
exact owners. The gain is **11 functions, 2,396 meaningful / 2,496 padded
exact code bytes**. The three remaining complete bodies are recorded as
zero-credit fuzzy parks.

The reconciled source SHA-256 is
`63b06fb4ddd784df67cbd93744d8e6ef732d2ad94ee37617da82ce69f7e5266f`.
The untouched donor source SHA-256 is
`af55d91b3d7e5edb3b1f34a2145d7f4f1042d6e7feb7fb1c53fe7d7c08ad9a92`.

## Exact accounting

| January owner | Source owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---|---:|---:|---:|---|
| `_update_server_get_update` | same | 83 | 96 | 7 | `212a56fe3418f46e42fd144a0b12acf969541359132c9f97a838f204ce84798a` |
| `_update_client_get_update` | same | 38 | 48 | 2 | `7b6261a7fd66044136eb3cc8c6284bfec737da592688cb23266efda29315cb7a` |
| `_update_server_new` | same | 127 | 128 | 15 | `3fe8955eb36b046709e1ae9cda8fb4ebb957f1abcc7729f2e92cfd40df20ae29` |
| `_update_client_new` | same | 138 | 144 | 16 | `c731eb28b35a8984100ec676f6fc1e4c2fee9f24ed90445407966a70138f9c49` |
| `_update_client_start` | same | 177 | 192 | 19 | `d1a5cee7395d4a4f699e435e5e7bb2238270724faf6e6148f5315e3fa265da96` |
| `_update_server_start` | same | 182 | 192 | 20 | `adede2b085186622e0b1abd0ac2332d3ef6618b146435ba7a828acd85be8ded9` |
| `_update_server_build_server_update` | same | 205 | 208 | 15 | `00ac68608441dd200fc313baf4cb7418cff1d36d1b45e7e8ce9734dd61b040b6` |
| `_update_client_dequeue` | same | 705 | 720 | 37 | `8687a17a6d4bc2a2698f46aebc03a7b220cee5af5e6b1ef58b726ffd94bf80bf` |
| `_update_server_handle_client_update` | same | 327 | 336 | 22 | `d3d7342c9a2d6333864ef1a692b13a7f4ac99c8c5f2cf168fb530981f33c2479` |
| `_update_client_handle_server_update` | same | 196 | 208 | 17 | `e32ac20126e754dd7d2c564600cab08a5233cf203df3aee34de13c70ca9d4fef` |
| `_update_server_next_update` | same | 218 | 224 | 17 | `32dd64cc421dca45a4829b40a83636d95e0c093915c4200fe2f245ffafef2c94` |

The canonical baseline has 496 meaningful / 544 padded exact bytes.  This
packet has 2,892 meaningful / 3,040 padded exact bytes.  The complete January
unit contains 3,404 meaningful / 3,584 padded code bytes.

## Name and owner reconciliation

The Halo CE symbol atlas authenticates `update_server_get_update` at January's
corresponding object offset.  `update_client_get_update` is not atlas-named;
it is the unique strict-section name-gap pairing and the semantic client twin
of the authenticated server helper.  It is retained as a descriptive
project-convention name, not represented as first-party name proof.  Both are
private `static` functions in their owning translation unit.

The old address placeholder `bss_0043ee60` is split into the two genuine
semantic owners used by January diagnostics and corroborated by HCEA:
`update_server_globals` at 0x43ee60 and `update_client_globals` at 0x442f70.
The 0x410c-byte server aggregate is followed by the observed four-byte
alignment gap, then the 0x10494-byte client aggregate.  Both are ordinary
explicit-zero external definitions owned by this translation unit.  The
candidate COFF places them at offsets 0 and 0x4110 in one 0x145a4-byte `.bss`
section with flags 0xc0400080, matching the resplit target's offsets, size and
flags.  There is no synthetic grouped aggregate or address-arithmetic facade
in source.  Compile-time size and field-offset assertions guard the
reconstructed layouts.  This packet claims no data exactness for the renamed
target storage.

All public declarations move to `player_queues_new.h`; the private helpers
remain declared in their `.c` owner.  `machine_get_player_list` is declared in
its true owner, `players.h`, rather than as a foreign local prototype.  The
signed `long update_number` contract is compelled by January's signed `jle`
and `jge` comparisons and closes the otherwise two-opcode residual in
`update_client_handle_server_update`.

## Credibility review

The source uses named constants, project `real`/vector types, typed queue
records, `FLAG(...)` for the latched control mask and ordinary public APIs.
It contains no inline assembly, volatile reinterpretation, raw byte emission,
address-named function/global definitions, synthetic grouped storage, copied
foreign shadow prototype, or emitted `point_from_line3d` COMDAT.  The
fake-match scanner reports zero review leads.

The apparently unused `system_milliseconds()` result in
`update_server_build_server_update` is not synthetic pressure: January has the
call at that exact point and the HCEA cross-build reconstruction independently
records the discarded timer call.  The local-ticks body rejects the earlier
fake `(long *)&ticks` parameter-slot cast and keeps an ordinary
`long update_number` local even though that honest spelling makes VC7 inline
the exact callee in this TU context.

HCEA is used only as a cross-build semantic map.  January object bytes,
relocations, diagnostics and target control flow remain authoritative.

## Focused verification

The packet candidate was compiled with the canonical VC7 flags and proposed
headers.  A target tree was regenerated into scratch with the packet's symbol
map; neither canonical source nor `build/split` was changed:

```text
build/tools/csplit.exe -i cachebeta.exe \
  -p scratch/player-queues-owner-packet-20260908/_split-config \
  -o scratch/player-queues-owner-packet-20260908/split-target

== emitted-symbol guard passed (1 forbidden names checked)
== exact 20  residual 3  unwritten 0  (of 23 listed)
```

The regenerated target object SHA-256 is
`96334deda60a202d1f18e375750459e418fab5b1fb1f511f8b04bd35eaff3d03`;
the semantic two-global candidate object SHA-256 is
`4d76b90a80aac11ea575f2a885fd32a4c3a6e224d377852f91a3d055d1537dfe`.
All 20 strict comparisons include relocation identity, proving that the
two-global split closes rather than merely moving the former BSS-name gap.
The same candidate inspection proves the target-shaped external `.bss`
storage owners described above; neither is left as an undefined reference or
tentative COMMON.

The semantic-name object also yields exactly two unique `namegap.py`
proposals: the 96-byte server helper and 48-byte client helper.  That tool is
non-authoritative; the naming basis is stated above.

Focused before/after gates also preserved the exact/residual/unwritten counts
of every directly edited caller: `game_time` 20/0/0, `players` 54/15/1,
`player_control` 45/4/0, `game` 22/2/3, `main` 73/3/19, and `game_state`
24/0/0.  These are local checks, not a substitute for the global sweep.

The enclosing reconciliation batch ran the full Ninja build, complete
stable-owner sweep and tracked pytest suite. All pass; the stable sweep reports
zero inherited regressions across the broad `players.h` include surface and
the four callers that now include the genuine `player_queues_new.h` owner.

## Honest zero-credit parks

| Function | Meaningful | Target/candidate padded | Target/candidate relocs | Fuzzy | Reason |
|---|---:|---:|---:|---:|---|
| `_update_client_get_maximum_possible_server_time` | 74 | 80/80 | 3/3 | 92.11539% | One independent compare/load schedule transposition at +0x0d. |
| `_update_queues_reset_and_fill_with_lies` | 260 | 272/272 | 24/24 | 97.297295% | One store is scheduled on the opposite side of an independent push. |
| `_update_client_local_ticks` | 178 | 192/352 | 15/31 | 1.6181818% | January calls the exact builder; this TU context inlines it. The honest complete body is retained. |

These bodies receive zero exact bytes and zero credited functions. Their
target and candidate hashes are recorded in `config/parked.json`.
