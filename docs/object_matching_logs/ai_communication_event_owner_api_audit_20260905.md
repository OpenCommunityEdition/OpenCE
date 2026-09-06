# AI communication event owner-API audit (2026-09-05)

This began as a read-only prerequisite audit and now records the verified
canonical owner-API correction. It claims no code or data progress. Its
purpose is to replace the misleading shared declaration before the private
Actor Combat burst caller cluster is resumed.

## Conclusive signature evidence

The declaration before this correction in `source/ai/ai_communication.h` was:

```c
void ai_communication_event(
	short type,
	long unit_index,
	long prop_index,
	long object_index,
	long position_index,
	long structure_index,
	boolean allow_reply);
```

This declaration was introduced by commit `0bcb14ce9` while consolidating the
Actions caller's former local declaration. It recovered the right owner but
not the actual API. Four other action files still carry the same Boolean-tail
shadow, while `ai.c` and `actor_perception.c` instead carry a seven-argument
shadow ending in `void const *` and still overstate arguments four through six
as `long`.

The primary type query was:

```powershell
& '..\..\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' `
  -sym ai_communication_event `
  '..\..\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
```

The PDB's parameter records are unambiguous:

```text
short                         communication_type
long                          subject_unit_index
long                          cause_unit_index
short                         hostility
short                         damage_type
short                         information_type
struct ai_information_data *  information_data
```

The same record exposes a public symbol for `ai_communication_event`.
January's cross-object relocations independently require external linkage.
The PDB SHA-256 is
`f55cfe957da8079a62a26a6753f1ddbb700b067a66de0bd5db5aac182d985ff1`.

January code independently confirms the ABI rather than relying on the later
PDB alone. In `_ai_communication_event`:

- argument 1 is consumed as a signed semantic word (for example the word
  checks at `+0x27` and `+0x274`);
- arguments 2 and 3 are unit handles read as dwords at `+0xC9` and `+0x199`;
- arguments 4 and 5 are read as words at `+0xAF` and `+0xB8`, with argument 4
  later sign-extended at `+0x42D`;
- argument 6 feeds the word `information_type` field of the packet around
  `+0x1BA3`; and
- argument 7 is loaded at `+0x1BB6`, tested as a pointer, and either supplies
  two copied dwords or causes the eight-byte information union to be zeroed.

The target `ai_information_packet` layout agrees with the PDB: its
`damage_category` is a short at `+0x08`, `information_type` is a short at
`+0x14`, and the eight-byte `ai_information_data` begins at `+0x18`.

One target call makes the pointer distinction executable, not nominal:
`actor_perception_update+0xEFE` takes the address of a stack
`ai_information_data`, pushes that address as argument 7, then pushes
information type 1. Target death and damage handlers likewise pass runtime
damage-category values in argument 5. A complete pre-freeze scan of all 833
January target objects found 43 direct calls from 31 code owners in 14 target
translation units:

| Target unit | Calls |
| --- | ---: |
| `source/units/bipeds` | 3 |
| `source/ai/ai` | 5 |
| `source/ai/actor_stimulus` | 5 |
| `source/ai/actor_perception` | 5 |
| `source/ai/actor_moving` | 2 |
| `source/ai/actor_combat` | 3 |
| `source/ai/actions` | 8 |
| `source/ai/action_wait` | 2 |
| `source/ai/action_uncover` | 1 |
| `source/ai/action_search` | 2 |
| `source/ai/action_obey` | 1 |
| `source/ai/action_guard` | 3 |
| `source/ai/action_flee` | 2 |
| `source/ai/action_charge` | 1 |

The reproducer is
`scratch/ai_communication_event_api_census_20260905.py`, SHA-256
`6c9efb2e33298dbfc83632d4cc640bc68a0357f324b68bdc9942edb65833852b`.
It records instruction context around every relocation and does not modify the
tree.

## Minimal authentic owner packet

The true owner remains `source/ai/ai_communication.h`. The smallest coherent
correction is:

```c
struct ai_information_data;

void ai_communication_event(
	short communication_type,
	long subject_unit_index,
	long cause_unit_index,
	short hostility,
	short damage_type,
	short information_type,
	struct ai_information_data *information_data);
```

`struct ai_information_data` is already genuinely defined in `source/ai/ai.h`.
Only a forward declaration belongs in the communication header because the API
uses a pointer. Pulling the complete AI header into this widely consumed owner
would create unnecessary dependency breadth. The short parameters are the
actual PDB types; inventing enum-typed ABI parameters is not necessary. The
event, hostility, damage-category, and information-type enums can remain a
separate ownership cleanup where canonical declarations are incomplete.

The following stale caller-local declarations should be removed:

- `source/ai/ai.c`;
- `source/ai/action_guard.c`;
- `source/ai/action_search.c`;
- `source/ai/action_uncover.c`;
- `source/ai/action_wait.c`; and
- the unused shadow in `source/ai/actor_perception.c`.

The first five files have actual reconstructed callers and should include the
owner header. `actor_perception.c` currently has no reconstructed call, so its
unused shadow can simply be deleted without adding an unused include.
`ai.c` must also remove its private two-value vehicle-entry/vehicle-exit enum
when it includes the owner, because those exact names and values already exist
in `ai_communication.h`.

Canonical `FALSE` is the integer constant zero and therefore was already a
valid C null pointer constant. Before any candidate was compiled or measured,
root nevertheless chose the clearer source-authentic spelling: replace
`FALSE` with `NULL` only where it is argument 7 (`information_data`) of an
existing `ai_communication_event` call in `action_guard.c`, `action_search.c`,
`action_uncover.c`, `action_wait.c`, `actions.c`, and `actor_stimulus.c`.
There are 21 such calls. This single semantic migration documents the newly
recovered pointer role; it is not a compiler-result variant. The two current
`ai.c` calls already use `NULL`. No event value, enum value, predicate, local
width, argument order, or other call expression is changed. Future non-null
information data must use the typed structure pointer.

## Complete canonical blast boundary

Eight C translation units currently include `ai_communication.h` directly:

- `source/ai/action_converse.c`
- `source/ai/actions.c`
- `source/ai/actor_stimulus.c`
- `source/ai/actors.c`
- `source/ai/ai_communication.c`
- `source/ai/ai_debug.c`
- `source/ai/ai_script.c`
- `source/units/unit_dialogue.c`

Adding the owner include to `ai.c`, `action_guard.c`, `action_search.c`,
`action_uncover.c`, and `action_wait.c` makes 13 direct header consumers.
The already-owning `actions.c` and `actor_stimulus.c` are also source-touched
only for the seventh-argument `NULL` clarification. `actor_perception.c` is a
fourteenth required focused comparison because its shadow declaration is
removed even though it need not include the header. There are no
header-to-header consumers in the current source census.

The two direct consumers reserved to the active Fable lane, `ai_script.c` and
`unit_dialogue.c`, need only be rebuilt and compared; this audit did not inspect
or alter their bodies. A safe root trial therefore captures all 14 units,
applies the one owner declaration plus the six shadow removals, five genuine
includes, and the preselected 21 pointer-spelling corrections, runs focused
owner/property comparisons, then runs the ordinary full Ninja, strict-owner
sweep, park validation, and Units sentinel. Any exact regression is a real
header-context result and must be reported rather than repaired through include
ordering, prototype variants, or reverting selected `NULL` sites after seeing
the result.

Prior ledgers are consistent with this disposition:

- `actor_stimulus_obj_jonas_public_wave2_20260821.md` already records the
  pointer-sized seventh slot and explicitly leaves the Boolean-tail drift for
  later shared-header work;
- `actor_stimulus_obj_small_exact_followup_20260902.md` proves the owner-header
  include itself is natural for Actor Stimulus;
- `actions_obj_jonas_large_natural_pair_no_fake_20260830.md` proves the owner
  location for Actions, but its then-new declaration is the mismatch corrected
  here; and
- `ai_obj_jonas_vehicle_event_pair_20260821.md` authenticates the two current
  `ai.c` callers and their null information arguments.

This is an actionable owning-API packet, not code admission. Its production
fitness remains conditional on the complete focused and full build blast above.

## Root-conditional post-application frozen-object audit

Root subsequently applied the exact 35-hunk manifest and reported a successful
full Ninja plus a stable `6217 / 8245` strict-owner result with no gain or loss.
The following independent audit is conditional on the two root-captured object
sets being the actual immediately-before and immediately-after products of that
application; it does not substitute for root's full-build or semantic checks.
No live build object was used.

Frozen inputs:

- `scratch/communication-api-before-20260905.json`, SHA-256
  `A77EAB7D86AD62BF57187DB6EF4E87F6C689799056AB9B0D78CEB09BA526339B`;
- `scratch/communication-api-after-20260905.json`, SHA-256
  `E0B8B001451FAF5821A4285ADA5CD67AF51597E4924CDFC1EB82867D87F3EECD`;
- the 14 object trees named by those manifests; each object's bytes were hashed
  before and after parsing and checked against its manifest record; and
- `scratch/communication_api_independent_runtime_audit_20260905.py`, SHA-256
  `E700B3FFC4849D96F04F6327A4A3DA7CDF1AB6EECCBF706828DD81F00AEF320B`.

The audit reuses the hardened all-section fingerprint and symbol comparison
functions in `scratch/observer_dependents_runtime_audit_20260905.py`; its nine
synthetic invariance and negative tests passed first. The complete result is
`scratch/communication-api-independent-runtime-audit-20260905/summary.json`,
SHA-256
`C58A14FA1A1AAAFE2F8A1637EE7DDC82DBBADBC1814E41A3D57CBE5A35CFC9E4`.
Per-unit reports beneath that directory retain every original fingerprint,
section property, relocation target record, and program symbol record.

All 14 units pass the runtime/ownership comparison:

- all section identities and counts are preserved; there are 1,448 byte- and
  metadata-identical section records and eight records whose only difference
  is compiler-local `$L...` spelling;
- no debug section is added, lost, or changed;
- no code, RDATA, DATA, BSS, directive, or other runtime section is added or
  lost;
- normalized program-symbol and defined-runtime-symbol multisets have zero
  additions or removals; there is no genuine function/helper owner addition,
  definition transition, storage/type/value change, or relocation-target ABI
  change; the complete function-symbol count is 504 before and 504 after;
- COMMON inventories are identical in every unit. The only nonempty inventories
  are the same five owners in `ai_debug.obj`, the same two four-byte owners in
  `ai_script.obj`, and the same four-byte `_sequential_counter` in
  `unit_dialogue.obj`; no owner, extent, storage class, or type changes;
- every unit preserves its section count and symbol-record count; the only COFF
  header field change is the ordinary rebuild timestamp; and
- `_point_from_line3d` is not newly emitted or referenced. `ai_debug.obj` had
  one existing definition and two existing references both before and after.
  The definition remains a 48-byte, zero-relocation, alignment-shift-5,
  selection-2 COMDAT with normalized hash
  `9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`;
  both caller relocation records are also unchanged.

The eight compiler-local-only records are three functions in
`action_guard.obj`, three in `actions.obj`, and two in
`actor_perception.obj`. They account for 46 removed and 46 added raw `$L...`
names. Each paired label retains its owning section, offset, storage class,
type, relocation address/type/addend, and resolved destination; recursively
normalizing only the label name makes the complete section and symbol records
identical. These are compiler-local numbering changes, not new owners or an ABI
change. The other eleven objects have no runtime-section difference at all.

On the frozen evidence, the owner declaration, consumer migration, and
preselected 21 `NULL` arguments introduce no runtime or ownership regression.
This approves the API packet's object-level preservation only; it creates no
new matching-function or data credit and does not broaden approval to any
unwritten communication body.

## Root production verification and admission

Root confirms that both frozen inventories are the actual immediate products
of the single, preselected 35-hunk application. The complete production diff
matches the reviewed manifest, SHA-256
`8a49a55695d5d2173aa639d55d8c141886cfeb40f0ca95a3608e90704cdc3052`.
No source-shape, header-order, or compiler-option variant was tried.

The full Ninja build passes. The stable 8,245-owner sweep preserves all 6,217
strict owners with zero gains and zero regressions. All 281 parks validate;
admission reports zero candidates, zero contradictions, five recorded
rejections and zero revocations. All 288 tests plus 26 subtests pass. The
nine-file fake-match scan reports zero leads and `git diff --check` passes.
Protected Bitmap source hashes and the Units 189-exact sentinel are unchanged.

Root reviewed the complete independent adapter, its summary, and the final
source diff. The fourteen-object comparison approves this API migration with
no runtime or ownership regressions. Published accounting remains 906,391
meaningful matched code bytes, 6,171 credited functions, 2,031,312 matched
data bytes, and 391/833 Matching objects. This batch adds no exact credit.

The canonical copies of reserved Fable consumers were verification-only.
The active Fable worktree was neither inspected nor modified; its pending
changes still need reconciliation against this recovered owner signature
when the user releases that lane.
