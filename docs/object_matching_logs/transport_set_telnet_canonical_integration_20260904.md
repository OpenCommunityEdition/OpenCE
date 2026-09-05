# Endpoint-set operations and Telnet owner integration, 2026-09-04

## Scope and accounting

This batch starts from published canonical
`416c57f21e19d46c740b7e198bc4c43260bc014e`. It integrates the genuine
five-function endpoint-set operation cluster and reconciles Telnet's public
API and global naming. No active Fable worktree or assigned implementation is
inspected or changed.

Transport Endpoint Set advances from **12 exact / 1 residual / 13 unwritten**
to **13 exact / 5 residual / 8 unwritten**. Its real next-index helper adds
**56 meaningful / 64 padded exact bytes**. The four other newly implemented
owners span **1,184 target padded bytes** and remain measured fuzzy parks;
they receive no exact credit. Telnet remains **5/5 exact** and retains its
existing Matching classification. This batch completes no additional object.

Fresh canonical accounting is **900,076 / 2,198,102 meaningful credited code
bytes**, **6,150 / 11,060 credited functions**, **2,028,607 / 4,176,062
credited data bytes**, and **391 / 833 Matching objects**. The meaningful
code/function delta is **+56 / +1**; reported data and object counts do not
change. The new authentic `ep && set` literal is 10 physical bytes but does
not finish a report data section and is not claimed as new credited data.

## Transport-set source and ownership

The production C contains the actual typed bodies, not scratch includes.
The common `transport_endpoint.h` from the preceding published batch supplies
the complete endpoint type, semantic flag bits and public API. The XDK owner
supplies the real `fd_set`, `timeval`, socket and `FD_*` definitions.

Private functions have honest inferred names and ordinary private prototypes:

- January offset 459,824 (`0x70430`):
  `transport_endpoint_set_get_next_index`;
- January offset 461,584 (`0x70B10`):
  `transport_endpoint_set_compare_entries`.

The corresponding `config/symbols.json` entries name the real private owners;
there are no production comparison aliases or synthetic exports. The genuine
add caller naturally activates the next-index helper. No forced inline,
retention anchor or invented caller is involved. All 12 inherited strict
owners and the existing nonce park are unchanged.

The no-alias production gate is preserved as
`scratch/transport-set-operations-canonical-20260904.obj`. Complete source,
January ABI/control-flow evidence, hashes, sizes and the ownership census are
in `transport_endpoint_set_winsock_obj_set_operations_cluster_20260904.md`.
The four validated parks are:

| Owner | Target/base padded bytes | Objdiff similarity |
| --- | ---: | ---: |
| `transport_endpoint_set_compare_entries` | 48 / 48 | 97.61905% |
| `poll_endpoint_set` | 560 / 512 | 72.26966% |
| `add_endpoint_to_set` | 336 / 240 | 55.7094% |
| `remove_endpoint_from_set` | 240 / 240 | 94.02273% |

The established short public return ABI is not widened merely because some
January paths materialize EAX. The word timeout is January-authenticated.
The natural add path does not manufacture January's semantically inert
duplicate `FD_SET` branches. Two actual January boundary defects (one-past
capacity and unguarded all-NULL compaction) are preserved and explicitly
commented; they are not presented as safe API invariants or invented to steer
code generation. The observed server caller avoids these edges in its normal
lifecycle but does not prove general safety.

All 18 emitted code owners correspond to target owners; no new code, COMMON,
data or BSS owner is synthesized. Three inherited XDK encode tables and the
pooled transport-initialized assertion remain separate owner debts. This is
not whole-object admission or an ownership waiver. The hard emitted-symbol
guard finds no `_point_from_line3d`.

## Telnet: authentic API declarations and semantic BSS name

`source/networking/telnet_console.c` now includes the genuine narrow
`bungie_net/network/transport_endpoint.h` instead of eight caller-local
endpoint declarations. This removes the stale long-timeout declaration;
the actual owner declares the January word timeout. Calls already pass zero,
so all target call bytes remain unchanged.

The C file also includes the existing `hs/hs.h` and removes its incompatible
local non-const `hs_compile_and_evaluate` declaration. The real owner takes
`char const *`. No HS header or implementation is changed.

Telnet's own initialize/dispose declarations are removed from its C file.
The missing public dispose declaration is placed in
`source/networking/telnet_console.h`, alongside its existing public API.
The stale duplicate dispose prototype is also removed from `game.c`, which
already includes Telnet's owner header. The header's print/process
declarations now follow one-parameter-per-line
and separate no-argument `void` formatting. Direct consumers are Game, Main,
Telnet and Terminal; the full dependent build and stable sweep preserve their
existing exact owners.

The external 140-byte BSS owner is renamed from the address-derived
`bss_00456d00` to **`telnet_console_globals`**, following the existing structure
name. This is an honest inferred name, not a claim of recovered PDB spelling.
`config/symbols.json` updates only the corresponding target name at offset
4,549,888. Its external storage class remains 2; it is not converted to static
or hidden behind a macro.

The final actual-source, actual-target-name artifact is
`scratch/telnet-final-noalias-20260904.obj`: **5 exact / 0 residual / 0
unwritten**, without aliases and without emitted `_point_from_line3d`.
Independent no-alias review confirms all 17 target owners are exact: five
code owners, 11 literals and the 140-byte external BSS. The inherited extra
3-byte CRLF literal has the same name, bytes, flags and select-any linkage as
January's selected Bitmap Extract owner. It is link-compatible, not a new
owner or newly granted exception. The existing semantic ledger correctly
handles objdiff's false negative for the private buffer processor; its 496
padded bytes and all 30 relocations pass the hardened strict comparator.

## Verification checkpoint

Full Ninja and the stable section-identity sweep have passed. Before/after
artifacts are:

```text
scratch/readiness-enumeration-after-20260904.json
scratch/transport-set-telnet-final-after-20260904.json
```

The sweep advances from **6,195 to 6,196 strict owners out of 8,245**, with
exactly the new 64-byte helper and **zero regressions**. The stable key
survives its semantic rename; no rename is miscounted as a loss/gain pair.
Units retains all **189 target exact functions**, with its separate inherited
helper-ownership veto unchanged.

All **265 parks validate**, with zero stale or invalid records. Whole-object
admission reports zero candidates, contradictions or revocations, and four
otherwise-complete existing rejections. The scoped source scanner has zero
review leads and `git diff --check` passes.

The repository test command is:

```text
python -m pytest tools -q --basetemp scratch/pytest-transport-set-telnet-20260904
```

It passes **288 tests plus 26 subtests**, with only the inherited optional
pytest-cache permission warning. An initial unscoped pytest invocation also
tried to collect unrelated scratch donor tests and inaccessible old scratch
directories; that collection failure is not a repository test result. No
test, dependency or permission rule was weakened to obtain the scoped pass.

The protected bitmap source hashes remain unchanged:

- `bitmap_utilities.c`:
  `ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140`;
- `bitmaps.h`:
  `b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`.

Only this batch's source, symbol/park metadata and evidence are staged for
publication. Inherited EOL-only markers, scratch files and incidental research
output remain unstaged. Publication remains root-owned and targets only the
two `bnunu/halo` integration refs, never the upstream `origin` remote.

Independent Transport Set review confirms the production scratch artifact
SHA-256 is
`daef8d683f7d63e4a470352a955fa8307724b3528300172338595e84c082ad0f`.
It is code/relocation-identical to the rebuilt production object for every
one of its 18 emitted owners. Both independent reviews pass without aliases,
owner waivers or changes to the strict comparator.

The independently audited final Telnet artifact SHA-256 is
`da3756fdb8da8d0ee436892eebc4d8204ef8349d84ffc8f89db2c751815d242e`.
The final full rebuild and snapshot were repeated after removing the Game
consumer's duplicate declaration; the same one exact gain and zero
regressions remain. All 265 parks and the admission audit still validate.
