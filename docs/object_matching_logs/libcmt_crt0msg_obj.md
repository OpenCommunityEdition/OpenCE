# `libs/libcmt/crt0msg.obj` exact-match ledger

This atomic lane starts from campaign commit `b831acac`. At lane start the
object was `MISSING`, absent from `config/parked.json`, and had no production
source or object ledger. The January split target contains three routines,
one 144-byte message lookup table, one 4-byte callback variable, and seventeen
owned message strings.

## Plain-English behavior

This object is the small Xbox runtime-error reporter. `_NMSG_WRITE` searches a
table for an error number and sends the corresponding text to the Xbox debug
output. `_GET_RTERRMSG` performs the same lookup and returns the message
pointer. `_FF_MSGBANNER` prints a leading line break, invokes an optional
Fortran debug callback, and prints the `runtime error ` banner.

The table maps the familiar Microsoft runtime errors R6002 through R6028, the
domain/singularity/loss-of-significance math errors, CRLF, and the banner to
their exact text. Nothing here is fabricated from machine bytes: the readable
table and routines come from authenticated Microsoft CRT C source.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its actual `obj\i386\crt0msg.obj` member identifies ordinary C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\startup\crt0msg.c`, compiler
identifier `0x001c2426`, and is independently strict-equal to the January
runtime sections. The authenticated Microsoft source blob used for the
reconstruction is `4733dd8a46ee4c5342befdd8ade4f537d90fd0f6`.

The exact code inventory is:

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__NMSG_WRITE` | 44 | 4 | `c257a5cc8a643403707a25458d355ab70874507b7dd27466f2c421321c1bb839` |
| `__GET_RTERRMSG` | 42 | 3 | `10d3c4cb7a669f3771ff88ef0c28cfbc365210fa69f436a0d59ca06fef8b5f53` |
| `__FF_MSGBANNER` | 34 | 3 | `1fcbe9acf0bb677418f0680e0d122a3acfb3554b045b5938f6e7da30bbba0ebf` |

The exact non-code inventory is:

| Owner | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_rterrs` (private; correct original name restored 2026-09-23) | 144 | 18 | `25298d4d1ea85dfa6a4696577658216429876d7b7b59ed245667db872d9d79d2` |
| `__adbgmsg` | 4 | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

All seventeen target-owned string COMDATs compare strict-equal individually,
including their sizes, payload hashes, external storage, and section
ownership. Their total payload is 727 bytes. The shared three-byte CRLF
literal is undefined in the split target and defined by the rebuilt TU; the
table relocation resolves to the same external COMDAT and grants no extra
target-owned data credit.

## Experiment matrix

| ID | One changed factor | Result | Decision |
|---|---|---|---|
| E00 | Compile the authenticated source with its private Microsoft includes unchanged | The repository does not contain `cruntime.h`, `internal.h`, `rterr.h`, or the private startup headers | Rejected as unavailable build context, not a source-shape failure |
| E01 | Replace only unavailable private includes with standard types, declarations, and the named runtime-message constants | The host/desktop branch remained selected and referenced APIs absent from the Xbox object | Rejected; the original TU's Xbox preprocessor context was still missing |
| E02 | Restore the source's `_XBOX` context while retaining the natural `rterrs` table name | All code and string payloads reached the January shape, but the table owner was `_rterrs` rather than the split target's `_data_00319950` | Near result; ownership spelling remained strict-nonexact |
| E03 | Name the table `data_00319950` and retain readable source references through `#define rterrs data_00319950` | Matched against the earlier incomplete symbol atlas | Superseded: the fake public name violates the project's naming rule |
| E04 (2026-09-23) | Restore authentic `static struct rterrmsgs rterrs[]` and add the XDK-confirmed `_rterrs` private symbol at executable file offset 3250512 before re-splitting | All three functions, 144-byte table, BSS, and target-owned strings strict exact without semantic waiver | Accepted |

The former `data_00319950` spelling was a csplit placeholder, not Microsoft's
original name. The authentic XDK object names the table `_rterrs`, storage
class 3 (private), at `.data+0`. January's original symbol atlas lacked that
private record. The 2026-09-23 correction restored `static rterrs` in source,
added the one authenticated private symbol to `config/symbols.json`, and
re-split. This recovers proper ownership rather than steering code generation.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiles the unit under the unchanged libcmt flags
`/O1 /Gy /I"xbox/include"`. A single hardened comparison over all three
functions, both named data owners, and all seventeen target-owned strings
reports `all_equal: true`. This includes exact normalized bytes, relocation
addresses/types/destinations/addends, section flags, and owner storage.
The fresh 2026-09-23 direct comparison reports `section_infos_equal` true
for all three code functions, `_rterrs` (144 bytes, 18 relocations), and
`__adbgmsg` (4 BSS bytes); the gate reports 3/3 exact functions. There is no
semantic-data exception for this unit.
The two `void` functions now end with explicit `return;` under the project
house rule; a fresh gate still reports all three functions exact.

The candidate's compiler directives and debug records are discardable
metadata, not runtime ownership. The final forced object rebuild, full Halo
and libcmt builds, semantic/progress audits, clean-commit regression gate,
tool tests, and `git diff --check` are recorded after admission below.

## House/Berth and original-bug audit

The production unit is authenticated readable Microsoft C with named
constants, typed parameters, multiline declarations, and explicit ownership.
It uses no assembly, `volatile`, forced inline, undefined aliasing, byte patch,
opaque field offset, alignment trick, compiler-flag exception, or comparator
waiver. Defining `_XBOX` inside this recovered private runtime source restores
the original source configuration; it does not alter the project compiler
command.

Both lookup routines preserve a proven original defect: an unknown runtime
error number leaves the index equal to the table count and the next condition
reads one record beyond the array. The authenticated source and January
machine control flow independently prove that behavior. The production source
marks it with the required `BUG (preserved for exact matching)` comments and
states the safe corrected form: guard the array access with
`tblindx < _RTERRCNT`. The exact build deliberately does not silently repair
the shipped behavior.

## Historical gates and current recheck

The following older figures record an earlier lane. The 2026-09-23
private-name correction passed a fresh full campaign build and test run; the
current batch-level measurements are in
`library_five_object_batch_20260923.md`.

- Forced `crt0msg.obj` rebuild: passed under unchanged `/O1 /Gy`.
- Hardened whole-object comparison: all three functions, the 144-byte table,
  4-byte BSS, and all seventeen target-owned strings are strict-exact.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Semantic audit after rebasing onto the current campaign: 458 units, 3,575
  functions evaluated, 3,491 accepted exact,
  and zero unit errors.
- Progress/admission: `crt0msg.obj` is eligible for one complete object; its
  three functions contribute 120 code bytes and its table contributes 144
  exact data bytes by direct comparison after the symbol-atlas correction.
- Tooling unit tests: 179/179 passed.
- Configuration JSON parsing and `git diff --check`: passed.
- Current-canonical full rebuild/progress revalidation passed: 357/833 objects
  overall and 90/212 libcmt objects, with 3,486/11,060 functions and
  415,447/2,198,102 code bytes overall.
- The clean-commit fail-closed regression snapshot/check reports all three
  functions as `still_exact`, with no changed-nonexact/newly-exact functions,
  failures, or warnings.
