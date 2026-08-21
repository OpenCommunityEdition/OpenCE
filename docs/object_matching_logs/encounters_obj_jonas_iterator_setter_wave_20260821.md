# `encounters.obj` Jonas iterator/setter wave

## Result

This bounded source-only wave starts from authoritative integration commit
`8de9760a24e3109243b4fd4626c0d1feed876725`. Six HCEA-backed encounter
iterator and flag-setter bodies were emitted together in one natural
code-producing compile. All six are independently strict byte-and-relocation
exact and are retained without a source-shape retry or code-generation tune.
The three pre-existing exact lifecycle functions remain strict.

`encounters.obj` advances from 3/61 to 9/61 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_encounter_iterator_new` | 44 | 48 | 3 | `2ec35abaea35990677c45a00c3d3657672fcd52dab12c447235d14e15a49c100` |
| `_encounter_iterator_next` | 58 | 64 | 2 | `43c27ca345e17e3d3c2cf583f2296b37aa8ddeea821cc663872d56f5c80fbd88` |
| `_encounter_actor_iterator_new` | 75 | 80 | 4 | `14f3ba82cbb16c569042a7a908b36f9a1bfaf62d7ec3d1b15e1b7575ea31f97a` |
| `_encounter_actor_iterator_next` | 58 | 64 | 3 | `ba1f455f134ca342f8c3b804435a9fce76fcfc8ec193e9aeeb49277edfbabc98` |
| `_encounter_set_blind` | 42 | 48 | 3 | `020e07b18632bdbf5763587ed024667df3432bbc54433bd0be505560e063a9b0` |
| `_encounter_set_deaf` | 42 | 48 | 3 | `de46e7bff41b70c763ea6dcc8f7d2e1e21e0066bbf65a2de7216d9445e8589e2` |
| **Wave gain** | **319** | **352** | **18** | |

The hardened comparator reports identical normalized bytes, padded size,
relocation type, relocation offset, symbolic destination, and addend for all
six functions. In particular, `_encounter_actor_iterator_new` naturally
retains both target `_ai_globals` reloads at `+0x04` and `+0x26`.

## Provenance and signatures

- Baseline `source/ai/encounters.c` blob:
  `f6edf1738512b75d0850f4af5fc2e31ae5e479f9`.
- Retained `source/ai/encounters.c` blob:
  `2b3db22a5bb84f98b6483acd76223a13ddb11044`.
- Retained source SHA-256:
  `4e4a63535210c4f016a50aaf07f5ee748ae4d3b9d850c93f0b7c3963c3639bbc`.
- Unchanged typed layout owners are `encounters.h` blob
  `dd66d0b2f4faaf5f10e4f883cfeb9d0e31e9edd5`, `actors.h` blob
  `e061b529b05c782b1220cff8a05dd89e6e2f3ed2`, and `memory/data.h` blob
  `4c1f445a0358c3962c4dc96341c1873af1ebd135`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `encounters.obj` SHA-256:
  `85be81b7244e668533791f748d379a49720c67fa215fae4c8ca7bfb4e4ee7c80`.
- Extracted January PDB-symbol JSON SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
- Clean HCEA source oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The six donor blobs are
  `48cf43fadd5a7353d11d32165304a737b522117a`
  (`encounter_iterator_new.c`),
  `ab48ee869a5655c99989c660f926d93304b9cbb8`
  (`encounter_iterator_next.c`),
  `fabccab044179f0bacab0912a7df3af3315b5642`
  (`encounter_actor_iterator_new.c`),
  `8235ceb7f0e0fa68661c86e66640bf7b84be748a`
  (`encounter_actor_iterator_next.c`),
  `7e940df19310d5ac27faccd78340cdc3b49935a6`
  (`encounter_set_blind.c`), and
  `0c04e4c8dffccfac544244b1df027e578e63c6a8`
  (`encounter_set_deaf.c`). HCEA iterator and AI-layout header blobs are
  `9a3f3325a7f22c1fe363ee9313822cec0dee6502`,
  `3bdd8024ab3ff831f0e582a78071f9f6ee3c8ed9`, and
  `c5c3693f1d4cef82db9b2aa91ad2bc90471a6af1`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

HCEA supplies named typed control flow, while January PC COFF, disassembly,
call widths, and the PDB remain authoritative for signatures and exact code:

```c
void encounter_iterator_new(
    struct encounter_iterator *iterator,
    boolean active_only);
struct encounter_datum *encounter_iterator_next(
    struct encounter_iterator *iterator);
void encounter_actor_iterator_new(
    struct encounter_actor_iterator *iterator,
    long encounter_index);
struct actor_datum *encounter_actor_iterator_next(
    struct encounter_actor_iterator *iterator);
void encounter_set_blind(
    long encounter_index,
    boolean blind);
void encounter_set_deaf(
    long encounter_index,
    boolean deaf);
```

The January setter bodies load the complete first argument from `[ebp+8]`,
so their authoritative encounter-index formal is `long`; the narrower HCEA
header spelling is not imported.

## Typed layout and accepted behavior

The translation unit adds only local definitions for the 0x18 encounter
iterator, the 0x0C encounter-actor iterator, and the meaningful 0x0C prefix
of the target-owned AI globals. Compile-time assertions fix every consumed
layout boundary: iterator index `+0x10`, active-only byte `+0x14`, actor
iterator index `+0x04`, next index `+0x08`, AI initialized byte `+0x01`, and
first encounterless actor handle `+0x08`.

The existing complete typed owners are used directly. Additional assertions
fix encounter active `+0x0D`, first actor `+0x14`, blind `+0x40`, deaf
`+0x41`, and actor next-link `+0x2C`. `data_iterator_new`,
`data_iterator_next`, `encounter_get`, and `actor_get` remain their ordinary
typed repository accessors.

The two encounter-iterator functions initialize and advance the encounter
data cursor, optionally skipping inactive encounters, then publish the
current datum index. The actor-iterator functions select either the global
encounterless chain or an encounter's first actor and follow typed
`meta.next_actor_index` links. The setters update only the named Boolean
fields when AI is initialized. All void bodies have explicit returns.

## Fail-closed omissions and policy boundary

`encounter_force_activate`, `encounter_force_deactivate`, and
`encounter_set_respawn` were excluded before compilation. Their January code
moves an encounter handle into `EAX` and tail-jumps private helpers
`code_00048b50` or `code_00048cb0`; isolated ordinary C declarations would
not reproduce that private register ABI. No helper body or caller repair was
attempted.

Only `source/ai/encounters.c` and this new Jonas-owned ledger change. No
shared header, configuration, semantic exception, parked record, existing
Markdown, frozen source, or completion label changes. `ai_globals` is an
undefined typed external; the target retains storage ownership. The wave
adds no `.bss`, `.data`, COMMON owner, runtime `.rdata`, or data credit.

The retained source is readable typed defined C. It contains no assembly,
raw address or byte-offset access, pointer arithmetic, raw cast-based object
access, union pun, inactive-union read, undefined behavior, `volatile`
scheduling device, force-inline annotation, optimizer pragma or barrier,
synthetic anchor, or byte-forcing expression.

## Validation

- The one natural candidate compile retained all six bodies; there was no
  retry, declaration shuffle, field-order change, or code-generation tune.
- Complete 568-action `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all six additions and all three preserved
  lifecycle bodies have exact normalized bytes, padding, and relocations.
- `ninja progress`: pass. The semantic audit scans 470 units and 4,027
  functions, reports 3,887 semantic exact, 101 hidden exact / 61,876 hidden
  code bytes, 3,948 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,935/11,060 functions,
  474,687/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,768/7,574 functions and
  461,773/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The fail-closed pre-wave translation-unit check identifies exactly the six
  additions as `NEWLY_EXACT`, preserves all three lifecycle bodies as
  `still_exact`, and reports `changed_nonexact: []`. The only generic
  structural findings are the expected `.debug$S` and symbol-set changes
  caused by adding six COMDATs; no adjudication or configuration exception is
  added.
- Clean committed-state snapshot, deletion-plus-forced single-unit rebuild,
  and regression check: pass. All nine accepted functions are `still_exact`,
  with no changed nonexact sibling, failure, or warning.

Nothing is pushed.
