# `game_allegiance.obj` private-cluster recovery

This 2026-08-29 Jonas/Codex ledger records the retained typed-C recovery of
the private allegiance state-transition cluster, two previously unwritten
public leaves, and the target-owned data in `source/game/game_allegiance.c`.
The object advances from 8/15 to **13/15 strict-exact functions**, totaling
**1,360/2,000 padded code bytes**, with zero unwritten functions. Its 40-byte
team-name table and four-byte globals-pointer BSS owner are also strict-exact.
The object deliberately remains `NonMatching`: `_code_00096300` and
`_game_allegiance_create` are equal-size/equal-relocation residuals, and no
semantic, parked, comparator, or configuration credit is claimed for them.

## Frozen inputs and provenance

- Integration base: `f7d0acb137c7bcd7bf1b11b86a2f7fb45102e819`.
- Retained source blob before this ledger:
  `954efb10e5e08e930d33cf550dd88d11ae4b6a58`.
- January split-object SHA-256:
  `005dc3acdefcc3bbb2c041087494c54531400a18babab990fb405a27172ed9cf`.
- Retained candidate object SHA-256:
  `093cecd74050a150bf73f7681f9480ef5af77718b334d91aef96edc16c712e66`.
- XDK `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The build uses `/O2 /Oy- /DDEBUG /Dxbox` and the pinned project includes.
- HCEA reference commit: `c168af2e747d3095d9a29418ae401f3a39544863`.
  Donor blobs are `4a7ed1058f4ff34c4f406eb7da96b95c5d1907fb`
  (`update`), `3de97ad70758ff0cf57a8627699cf109e5dc0cd9`
  (`create`), `814148ff755aa7cef5060d6342e726cc4415bf78`
  (`remove`), and `2ee88a1c159b0d496a7f63e95a011ecbd487f52d`
  (`incident`).
- Pastudan reference commit: `918af885935ec470a31256ecce9a977b12b01f80`;
  its named allegiance source and the January target independently establish
  the symmetric record walks and private helper semantics.
- Earlier object ledgers remain authoritative for the first eight exact
  leaves and donor audit:
  `game_allegiance_obj_jonas_lifecycle_wave_20260821.md`,
  `game_allegiance_obj_jonas_public_leaves_20260828.md`, and
  `game_allegiance_obj_jonas_public_continuation_20260828.md`.

## Retained strict functions

All rows below pass `coff_compare.section_infos_equal`, including normalized
bytes and relocation identities.

| Function | Padded bytes | Relocations |
| --- | ---: | ---: |
| `_game_allegiance_initialize` | 48 | 4 |
| `_game_allegiance_dispose` | 16 | 0 |
| `_game_allegiance_initialize_for_new_map` | 160 | 10 |
| `_game_allegiance_dispose_from_old_map` | 16 | 0 |
| `_game_team_is_enemy` | 96 | 1 |
| `_game_team_is_ally` | 80 | 1 |
| `_game_team_ally_status_changed` | 96 | 1 |
| `_game_allegiance_get_incidents` | 128 | 1 |
| `_game_allegiance_provoke` | 112 | 1 |
| `_game_allegiance_notify_change` | 96 | 1 |
| `_game_allegiance_update` | 128 | 7 |
| `_game_allegiance_remove` | 176 | 3 |
| `_game_allegiance_incident` | 208 | 2 |

The two new standalone leaves reproduce January's fatal-assert reload and
self-friendly-bit loop in `initialize_for_new_map`, and the symmetric
`status_changed` record search in `game_team_ally_status_changed`.

The private cluster was recovered atomically. VC7 naturally derives the
January private call surface for `code_00096300`: the record pointer arrives
in EAX, `currently_broken` in BL, and `permanently_broken` on the stack.
`update`, `remove`, and `incident` then close exactly with ordinary typed calls.
No private ABI was declared, forced, or exposed in source.

## Exact data ownership

The retained source defines the two data owners that the January split object
assigns to this translation unit:

| Owner | Size / relocations | Strict result |
| --- | ---: | --- |
| `_global_game_team_names` | 40 / 10 | exact bytes and ordered symbolic destinations (`default`, `player`, `human`, `covenant`, `flood`, `sentinel`, `unused6` through `unused9`) |
| `_bss_0043e490` | 4 / 0 | exact zero-filled `.bss` owner |

The explicit `= NULL` initializer is required for VC7 to emit the pointer as a
defined `.bss` owner rather than a tentative common symbol. The ten name
literals are normal discardable string COMDATs; the target split data records
them as external symbolic destinations, and the hardened comparator resolves
the candidate definitions to the same identities.

## Remaining measured residuals

| Function | Target / candidate | Target SHA-256 | Candidate SHA-256 | First stable divergence |
| --- | ---: | --- | --- | --- |
| `_code_00096300` | 416/2 vs 416/2 | `3eba7c13f76df6f625c5abf7165fe4e320ba92642bad6390de317f45dfcd0f9e` | `abbb6b20b7bb98348e048ae1252104ab55585fc1da07cdc14e12e699d681ef34` | January loads the stack boolean into DL and colors the two bounded team words as CX/SI; VC7 colors the same dependency graph as CL and SI/DI. The 142-instruction bit-vector body is otherwise structurally identical. |
| `_game_allegiance_create` | 224/5 vs 224/5 | `47961c6c1d4c59a901156d907b22388d0b0a39a06f367b8403cd9e97ada8ed1f` | `be3a5bd445aa935a156513c29538f662a0d940b83315fab36696cf7603821b0d` | January keeps the globals base in EDI and uses DX for the hot record field; VC7 keeps the base in EDX and shrink-wraps EDI around the loop. Both bodies contain 82 instructions and the same private call/relocation topology. |

Objdiff reports 94.296875% for the helper and 83.69334% for `create`, but
those percentages grant no credit. The fail-closed codegen classifier returns
`UNKNOWN` for both, so no parked entry is added. Reopen with an independently
authenticated source topology that changes the relevant live-range graph, or
with a mapped VC7 callee-saved-register allocation decision; do not repeat
local spelling permutations.

## Bounded source-shape search

The retained forms follow the HCEA/Pastudan semantics and January instruction
order. Rejected or codegen-neutral controls included:

- `short` versus donor-style 32-bit search index with explicit truncations;
- combined versus separate symmetric match predicates and reversed equality
  operands;
- block- versus function-scope team-field aliases and declaration-order
  permutations;
- direct-global versus typed-local count uses, redundant globals reloads, and
  separate typed search/fill aliases;
- pointer increment versus indexed typed record recomputation;
- reused versus block-local target record pointers;
- HCEA versus Pastudan field-store order;
- helper-local team aliases, explicit first-bit indices, direct-global bit
  accesses, and outer versus nested globals lifetimes;
- a redundant private forward declaration versus definition-before-use.

The indexed record recomputation introduced a stack spill and was reverted.
Direct-global helper accesses multiplied relocations and shortened the body;
the explicit bit-index form collapsed the helper to 368 bytes. None of the
remaining legal typed-C controls produced a classifier-supported close.

## Policy audit and validation

Retained source contains no assembly, `volatile`, `register`, pragma,
intrinsic/barrier, attribute, raw address or offset, representation pun,
undefined behavior, object patch, compiler flag exception, or comparator
exception. It uses typed record/global access, C89 declarations, one parameter
per line, `void` on its own line for no-argument functions, and explicit
terminal returns.

Validation at the retained head:

- exact gate: 13 exact, 2 residual, 0 unwritten;
- strict board: 275/619 complete objects, 636,775/1,922,669 code bytes,
  4,639/8,246 functions (parked and assembly-implemented functions excluded);
- consolidated Ninja build: pass;
- semantic report: 470 units, 4,782 functions evaluated, 4,658 semantic exact,
  4,668 accepted exact, zero unit errors;
- object admission: 0 candidates, 0 contradictions, 0 revocations;
- parked validation: 13 active, 0 stale, 0 invalid;
- tool tests: 205 passed;
- object remains `NonMatching`; no configuration, semantic ledger, data
  exception, parked manifest, comparator, compiler, or protected source was
  changed.
