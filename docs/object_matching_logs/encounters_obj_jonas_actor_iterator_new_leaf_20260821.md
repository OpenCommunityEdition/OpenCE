# `encounters.obj` Jonas actor-iterator initialization leaf

## Result

This bounded wave starts from clean canonical commit
`c8ab755c312be6e9c4eb3cd0d9e6c6aba643c45e`. Four ordinary-cdecl,
typed-C encounter/actor iterator candidates were emitted together in exactly
one natural code-producing compile. `_actor_iterator_new` was independently
strict on that first object and is retained. The other three bodies were
removed unchanged, together with support used only by those misses; none was
tuned, reshaped, or compiled a second time.

`source/ai/encounters.obj` advances from 9/61 to 10/61 strict functions and
remains `NonMatching`. The retained gain is 57 meaningful / 64 padded code
bytes and three relocations.

| Candidate | Target meaningful / padded | Target relocs | Target normalized SHA-256 | First natural candidate | Disposition |
| --- | ---: | ---: | --- | --- | --- |
| `_encounter_get_by_name` | 92 / 96 | 3 | `e6dc02120d2b8adc36d35a02d4a4f89a6404ee6680c0c18ffa0f38b26898bb8f` | 128 padded, 5 relocs, hash `0cc084ecf9ae0b202745a62d5e8f2f4f2bcbcd5b2c56373c0d560e98b096a4f6` | removed, zero credit |
| `_encounter_actor_iterator_prev` | 113 / 128 | 5 | `1d676294a91ac35c0d44f3d080f1140f3d27e8ac2efaf7b6fca95d29f90b360e` | 128 padded, 5 relocs, hash `9d49afc7019825499633d75759dc5718e3abfd9967deb74b726546628883d196` | removed, zero credit |
| `_actor_iterator_new` | 57 / 64 | 3 | `0e6a94236e8ba4f91948179a6715e4393f437b0a409ebbf2c7595201d9582d09` | strict 64-byte COMDAT, 3/3 relocations, exact hash | retained |
| `_actor_iterator_next` | 158 / 160 | 5 | `9a45666a613a0dc39819ef1db6f11b3afa5684d9dba2c1a9a4146252fb40a29b` | 192 padded, 6 relocs, hash `091ba1d2edc850c38105f99ebb3879d2dd2a57eabab16f0be184ea6455b1f97b` | removed, zero credit |

The target relocation packets are independently frozen:

- `_encounter_get_by_name`: `+0x0A` `REL32 _global_scenario_get`,
  `+0x2B` `REL32 _tag_block_get_element_with_size`, and `+0x34`
  `REL32 _csstrncmp`.
- `_encounter_actor_iterator_prev`: `+0x04` `DIR32 _ai_globals`,
  `+0x1B` `DIR32 _encounter_data`, `+0x22` `REL32 _datum_get`,
  `+0x3F` `DIR32 _actor_data`, and `+0x45` `REL32 _datum_get`.
- `_actor_iterator_new`: `+0x04` `DIR32 _ai_globals`, `+0x11`
  `DIR32 _encounter_data`, and `+0x1C` `REL32 _data_iterator_new`.
- `_actor_iterator_next`: `+0x04` `DIR32 _ai_globals`, `+0x22`
  `REL32 _data_iterator_next`, `+0x52` `DIR32 _ai_globals`, `+0x6D`
  `DIR32 _actor_data`, and `+0x74` `REL32 _datum_get`.

## Immutable provenance and signatures

- Baseline `source/ai/encounters.c` blob / payload SHA-256:
  `2b3db22a5bb84f98b6483acd76223a13ddb11044` /
  `5f8f1a13f7b55f7fc56d792705993ca5f50fd29a5a2f48ac632168deebaf6524`.
- Retained source blob / payload SHA-256:
  `5335791d253c6538c0e7d4b624c06726d11033f9` /
  `653586017ad736bdf4de432e5c755dd5b6eb6df36a45af6b5f2186fecb92356c`.
  Both payload digests are over `git cat-file blob` bytes, never checkout
  bytes or their platform line endings.
- January split `source/ai/encounters.obj` SHA-256:
  `85be81b7244e668533791f748d379a49720c67fa215fae4c8ca7bfb4e4ee7c80`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Extracted January PDB2.00 symbol payload SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It labels `_encounter_get_by_name` at RVA `0x47FA0`,
  `_encounter_actor_iterator_prev` at `0x48100`, `_actor_iterator_new` at
  `0x48180`, and `_actor_iterator_next` at `0x481C0`.
- Clean HCEA semantic-source commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name source blobs
  and authenticated payload SHA-256 values are:

  | Function | HCEA blob | Payload SHA-256 |
  | --- | --- | --- |
  | `encounter_get_by_name` | `96fab8c90d4c9263375acb61ff47d8f1a695187b` | `896bc197aca2291953756edda40fbb2bc2413fca289b3541768e168389be943e` |
  | `encounter_actor_iterator_prev` | `5cd9bb5f0dd01392d62af6b7cf701b8ff6e1073a` | `f8dbce22234ed00bc4a00fa0039b34afd61f1d940c6e1e9f20efd67c775edd0c` |
  | `actor_iterator_new` | `fbeef8f19c53c566853ae1e8504f1768f0d96062` | `73eb001b18619d98084633fea65b7b06730ba473f900b17c8f110ae9264dc8c6` |
  | `actor_iterator_next` | `d134cb3e23d1bcd6ccdcd8cea7aad5bc716a09f0` | `3d7e5fc440a0b795221baba8245a3e70c8eb0c771918ab8593c7cadb7333600c` |

  The HCEA `actor_iterator.h` layout blob is
  `679cb406c72b1b7f794d64ff37d8bfed89ee6797` (payload SHA-256
  `90756b8ad7369c396fe0b90b995d867e38d9e0061dd6d41efbecb060c08748be`),
  and its `data_iterator.h` blob is
  `3b4d5470f5ed7f419d89347df91091af74171ec8` (payload SHA-256
  `04eca64bdd50ab19ddf4c960f96bfb7662e3fa92d464b7f8483effa648a89114`).
- Clean independent January-PC reconstruction commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`. Its
  `src/halo/ai/encounters.c` blob / payload SHA-256 is
  `a0165ebbb9a4f140a7c603a368c1975de7455714` /
  `eb4cfd24f46a0fd11febf6c5f8b516e4b34bcbc743776f0804dbde16acabff47`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The repository's natural `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` command,
  include order, and XDK 3911 environment are unchanged.

The fail-closed signature packet is:

```c
long encounter_get_by_name(char const *encounter_name);
struct actor_datum *encounter_actor_iterator_prev(
    struct encounter_actor_iterator *iterator);
void actor_iterator_new(
    struct actor_iterator *iterator,
    boolean active_only);
struct actor_datum *actor_iterator_next(
    struct actor_iterator *iterator);
```

All four names use ordinary leading-underscore C decoration, external
storage class 2, function type 32, and caller-cleanup cdecl. HCEA's `int`
return for `encounter_get_by_name` is the same target-width 32-bit C type as
this repository's `long`; HCEA's `uint8_t active_only` is the same unsigned
byte representation as `boolean`. No private register ABI is inferred.

## January correction and typed layout

HCEA supplies the readable named behavior and the complete 0x1C iterator
layout. The independent January-PC reconstruction corrects one important
source detail before the one-shot experiment: its disassembly-backed store
table is `+0x10 = 0`, `+0x18 = -1`, `+0x14 = -1`, then `+0x11 = flag`.
That file accidentally labels the initializer `encounter_iterator_next`, but
its RVA commentary, 0x1C layout, call to `data_iterator_new`, and exact store
table identify the actor initializer; the January PDB resolves the real name
as `_actor_iterator_new`. The retained natural source follows that
target-independent January order and is strict without a target-driven retry.

The translation-unit-local typed iterator contains the existing 0x10-byte
`struct data_iterator`, `iterated_encounterless_list` at `+0x10`,
`active_only` at `+0x11`, two explicit padding bytes, `index` at `+0x14`, and
`next_index` at `+0x18`; compile-time assertions freeze every offset and the
0x1C total size. The previously accepted typed AI-globals prefix remains
unchanged and asserts `ai_initialized_for_map` at `+0x01`.

When AI is not initialized the retained function leaves the caller's cursor
untouched. Otherwise it initializes the embedded encounter-data iterator,
clears the encounterless phase byte, sets both handles to `NONE`, and copies
the caller's active-only byte. It uses the repository's existing typed
`data_iterator_new`, `encounter_data`, and `ai_globals` declarations directly.
There is no cast, raw address, pointer arithmetic, byte-offset access, union
pun, inactive-union read, uninitialized read, signed-overflow dependency,
aliasing violation, assembly, `volatile` scheduling device, optimizer pragma,
forced-inline annotation, synthetic anchor, or byte-forcing expression.

The local struct tag creates no cross-translation-unit C type collision. The
repository has no conflicting header prototype, live source declaration, or
second compiled definition. January COFF has the unique definition in
`encounters.obj`; `_actor_iterator_new` and `_actor_iterator_next` are
undefined in `actors.obj`, `ai.obj`, `ai_communication.obj`, `ai_debug.obj`,
`ai_profile.obj`, and `ai_script.obj`, while the other two quartet names are
undefined only in `ai_debug.obj`. The pointer and byte argument ABI agrees
with HCEA's shared iterator header and those callers. Reachable history has
the earlier six encounter iterator/setter bodies and symbol-list comments,
but no prior retained implementation of this exact initializer sequence.

## Fail-closed omissions and ownership boundary

The three nonexact quartet symbols are absent from the retained object. The
nine inherited strict bodies remain exact: the three encounter lifecycle
functions, encounter iterator new/next, encounter-actor iterator new/next,
and the blind/deaf setters. Source placement keeps every inherited accepted
COMDAT fingerprint unchanged; only the new function-owned code/debug/symbol
records appear.

Additional candidates were explicitly rejected before this wave:
`encounter_force_activate`, `encounter_force_deactivate`, and
`encounter_set_respawn` depend on private EAX helper ABIs;
`actors_freeze` carries an HCEA-only phantom second argument;
`actor_move_halt_at_firing_position` uses raw layout casts; and the perceived-
enemy helper depends on a private register convention. None is implemented,
declared, credited, or used to widen this wave.

Only `source/ai/encounters.c` and this new Jonas-owned ledger change. No
shared header, configuration, semantic exception, parked record, completion
label, pre-existing Markdown, Claude-owned file, protected-five source, or
tracked-path deletion is in scope. The protected five are `vehicles.c`,
`ai_debug.c`, `ai_script.c`, `actions.c`, and `units.c`.

This wave adds no `.bss`, writable `.data`, runtime `.rdata`, COMMON owner,
literal owner, or data credit. `ai_globals` remains a typed undefined external;
the pre-existing encounter-data storage owner is unchanged. Direct section
and symbol-set comparison confirms that the only non-code delta is compiler
debug information associated with the new COMDAT.

## Validation

- Complete Halo plus libcmt graph: pass. The retained-subset graph is clean;
  the final incremental invocation reports no work.
- Direct hardened COFF comparison: all ten accepted functions have exact
  padded bytes, normalized hashes, relocation offsets/types/destinations, and
  addends. All three rejected quartet symbols are absent.
- `ninja progress`: pass. The semantic audit scans 470 units and 4,113
  functions, reports 3,973 semantic exact, 101 hidden exact / 61,876 hidden
  code bytes, 4,034 accepted exact, 36 ordinary-only, 35 structurally
  accepted, one rejected, and zero unit errors.
- Campaign progress: 375/833 matched objects, 4,021/11,060 functions,
  479,605/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 273/468 objects,
  3,854/7,574 functions, and 466,691/1,770,166 code bytes; libcmt contributes
  102/212 objects, 167/476 functions, and 12,914/55,015 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The frozen pre-wave one-unit gate identifies exactly
  `_actor_iterator_new` as `NEWLY_EXACT`, retains all nine inherited bodies as
  `still_exact`, and reports `changed_nonexact: []`. Its only structural
  findings are the expected `.debug$S` and symbol-set additions for the new
  COMDAT; no runtime code/data drift or adjudication is present.
- A clean committed-state one-unit snapshot/check, deletion of the committed
  `encounters.obj`, same-path forced rebuild, second check, and direct hardened
  comparison reproduce all ten accepted functions with the three misses still
  absent and no runtime storage delta.

Nothing is pushed.
