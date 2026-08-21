# `action_obey.obj` Jonas public-lifecycle salvage

## Result

This bounded source-only wave starts from canonical commit
`3f2ba63dfb1258f7e36baaec69fe495bcefdc20f`. Six ordinary public,
one-argument stack-ABI bodies were emitted together in one natural XDK compile
and measured independently. Five were strict in that first complete batch and
are retained. `_action_obey_flush_structure_indices` missed and was removed
without a second source shape or tuning pass. `action_obey.obj` advances from
0/27 to 5/27 exact functions and from 0/11,585 to 290/11,585 meaningful code
bytes. It remains `NonMatching` and receives no data credit.

| Function | Target meaningful / padded | Relocations | First candidate normalized SHA-256 | Decision |
| --- | ---: | ---: | --- | --- |
| `_action_obey_flush_command_indices` | `106 / 112` | 6 | `32a82f8ac1a4940e43f61ca190846d4dd6d6d2970adedde76ab0e42837ca3c03` | **retained: strict exact** |
| `_action_obey_flush_structure_indices` | `46 / 48` | 2 | `685bb16bd7a8c09586002344b247d5aa94fe50371bf33b398b82896932470495` | rejected: target hash `9f0facf2a5e64545b5b323119ea1ce6c45813a307bf0fb256308aab64fe0db82` |
| `_action_obey_advance_command_list` | `46 / 48` | 4 | `e1cc926aa1c23e5f6c1e9bb46b326736e53bff9e460248611ef98d43f48a8faf` | **retained: strict exact** |
| `_action_obey_begin` | `46 / 48` | 4 | `e1cc926aa1c23e5f6c1e9bb46b326736e53bff9e460248611ef98d43f48a8faf` | **retained: strict exact** |
| `_action_obey_end` | `46 / 48` | 4 | `e1cc926aa1c23e5f6c1e9bb46b326736e53bff9e460248611ef98d43f48a8faf` | **retained: strict exact** |
| `_action_obey_update` | `46 / 48` | 4 | `e1cc926aa1c23e5f6c1e9bb46b326736e53bff9e460248611ef98d43f48a8faf` | **retained: strict exact** |

The five retained functions total 290 meaningful bytes, 304 padded bytes, and
22 relocations. Each passes the hardened comparator in padded size, every
normalized byte, and every relocation address, type, symbolic destination,
and addend. Explicit terminal `return;` statements were added to satisfy the
house source form; final validation replays prove that these style-only
statements do not change any retained section.

## Provenance and one-shot boundary

- Baseline and restored atomic `action_obey.c` Git blob:
  `febbd96e39c7519dc21dd86a35bbe6fe7b6a4579`.
- Retained `action_obey.c` Git blob / stable committed-payload SHA-256:
  `e0bb4361e9e4a4bf353e5cbf58037fee9b9e0169` /
  `e9160d896d224885bc51bee44329aca58cee420efb52c45875be1cee6d76e4ca`.
  The SHA is over `git cat-file blob e0bb4361...`, not checkout bytes.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `action_obey.obj` SHA-256:
  `0e4f889a35b97c44f6ae91e4906b547bd64f2ff12b9f399e02b46f02b69633fb`.
- The preserved atomic base object is only the restored 476-byte shell,
  SHA-256
  `0c7ed4643c2075fe416eacfa9a464d49c59b8d32241755cc743e953793824454`;
  it is not a candidate oracle and preserves none of the historical bodies.
- The supplied historical commit `677defb9d9b4dd0ac4f20bc2c5dbd53bf3e8d7f5`
  is the action-charge frontier. The independently authenticated action-obey
  atomic ledger is `f164672e22f55ade17493b2030d561d806bab2ec`, whose parent is
  `983f99f6ea9de2d43f4c17297017982ca42ac883`.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The six screened body blobs are
  `7c6b1802eda90af54c7c0c81a0b3136aeac3fdff` (flush command),
  `bf1a73ee05723cd4d0fe9a63a6945f3fa1995d8b` (flush structure),
  `6599ec2fbe3b660229e331b71b789bd9d79149c8` (advance),
  `b9c9eef8510f14c386742e4405e2d7e7e52bf7ce` (begin),
  `80fb2698ca87abb6b554f6bf4a8cc0e633c838fd` (end), and
  `7ac747c1b0688fc4a38dc0a99d4ef63a84e9aaa7` (update).
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.

HCEA was used only for semantics. January's symbols, stack access,
disassembly, literals, exact byte counts, and relocations supplied the PC
source shape. Every selected target symbol is a COFF external function of type
`0x20`, reads its sole `long` argument from `[ebp+8]`, and returns with the
ordinary C stack ABI. Exactly one new exploratory six-body object was emitted.
The rejected body was then deleted; later builds are validation replays of the
retained source, not alternate candidate searches.

## Retained behavior and typed state

`action_obey_flush_command_indices` obtains the typed actor and its embedded
obey state, validates the command-list index against the current scenario, and
either marks the state finished and transitions to no action or invokes the
private individual iterator with the command-index flush callback. The
advance, begin, end, and update wrappers obtain the same typed state and invoke
that iterator with their respective private callbacks.

The translation unit locally declares the callback function type and ordinary
typed undefined functions. Compile-time proofs preserve the January layouts:
simple control `0x24`, complex control `0x58`, obey state `0x84`, and the obey
state at actor offset `0x9C`. It uses named fields and existing typed accessors.
There is no raw address, byte-offset dereference, pointer or union pun,
inactive-union access, volatile scheduling device, force-inline annotation,
optimizer pragma/barrier, assembly, synthetic anchor, or object-byte forcing.

## Fail-closed scope, signatures, and ownership

The final candidate defines exactly the five retained public functions. The
rejected `_action_obey_flush_structure_indices` definition is absent. The
other 21 January functions are also absent, including every private
`code_*` helper, perform/control/setup/describe, and generic inline emission.
The six private callbacks/iterator used by the retained bodies are ordinary
typed undefined relocations, never definitions.

The final object consists only of `.drectve`, `.debug$S`, and five `.text`
COMDATs. It defines no `.rdata`, `.data`, `.bss`, COMMON symbol, or other
storage owner; January's 1,748 data bytes remain wholly unclaimed. Only
`source/ai/action_obey.c` and this ledger change. No header, caller,
configuration, semantic exception, parked record, existing matching log, or
protected source changes, and no path is deleted.

The current model is consumer-safe without a shared declaration. The only
textual begin/update/end references are in the disabled action table in
`actions.c`, while `actions.h` already models the relevant action callbacks as
ordinary `void(long)` functions and declares no conflicting cross-TU
signature. The private callback declarations likewise have no definitions or
conflicting declarations elsewhere in production source.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass (569/569 actions).
- Direct hardened final comparison: all five retained functions remain strict;
  the rejected wrapper, the other 21 target functions, and all data owners are
  absent.
- Semantic audit: 470 units, 4,077 functions evaluated, 3,937 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,998 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,985/11,060 exact functions,
  476,985/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. Halo contributes 3,818/7,574
  functions and 464,071/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, deleted-path, and banned-policy scans are clean.

A clean committed-state regression snapshot/check followed by deletion and
forced rebuilding of `action_obey.obj`, another regression check, and a direct
hardened replay is the final same-path reproducibility proof. No push is
performed.
