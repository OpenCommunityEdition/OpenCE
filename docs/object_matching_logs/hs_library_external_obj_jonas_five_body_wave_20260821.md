# `hs_library_external.obj` Jonas five-body wave

## Result

This bounded body-only wave starts from authoritative integration commit
`1a95bf095652af7ddb5c86a10f21a2bf74d86bf3`. Five public ordinary-cdecl
bodies were emitted together in exactly one natural code-producing candidate
compile. Four were independently strict byte-and-relocation exact. The lone
miss, `hs_object_create_anew`, was removed wholesale with its support-only
declaration and was not retried or tuned.

`hs_library_external.obj` advances from 0/36 to 4/36 exact functions and
remains `NonMatching`.

| Retained function | RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_hs_not` | `0x000B8960` | 15 / 16 | 0 | `32991d2f9537a456b5b8acde1b67a6c0fbba8921ea6dbd527b463385d1c99541` |
| `_hs_print` | `0x000B8970` | 24 / 32 | 2 | `cc93a70dfca252fd3df5197977186fd70762b9ddb4a73e2d4aaf4bb7fc249eaa` |
| `_hs_object_destroy_by_name` | `0x000B8DC0` | 37 / 48 | 2 | `6810276425d581c023b1cbc723b900b325d7c49469b4b76bf3d3dff01e76277e` |
| `_hs_trigger_volume_test_objects_any` | `0x000B9490` | 23 / 32 | 1 | `29490629b9c921ca512955474a6c9f56c1717a9c5c44ae2ca2682a93bdb42d8c` |
| **Wave gain** | | **99 / 128** | **5** | |

The hardened comparator reports identical normalized bytes, padded extent,
relocation address, type, symbolic destination, and addend for every retained
body.

## Provenance and Git payload authentication

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `hs_library_external.obj` SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 and include paths.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.

Every Git identity below was read as its committed raw blob payload; the byte
count and SHA-256 therefore authenticate the payload independently of the Git
object ID.

| Evidence | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| baseline `source/hs/hs_library_external.c` | `8152e0d7e76d89a451c343593b0001adb7a0982f` | 2,349 | `59506047de8ea3a2c2c4a41298372398bbff9a39712bb501555f10650fcb1523` |
| retained `source/hs/hs_library_external.c` | `82b2dc5d3ccdd9e1c043ad0a142ae9ac5c4bbcdc` | 3,285 | `04877d914fcfc8ac4b7918c80f68689d08909f0afc56dcefec49fa951b60b3d9` |
| canonical `source/hs/hs.c` caller/prototype evidence | `cd69d9c9911b6ae144042388627df5cfaa40e45e` | 200,483 | `2a7241541eb4a2b7f6195122be41865ab3f327965be28c4001a2178890468403` |
| canonical `source/cseries/cseries.h` width/type evidence | `3c91ac46ab275894a18ac4e839b38ae13022d91b` | 10,070 | `e994e965cd3f48cd47cebf0a39f41a1aac460e8b25544452a1fc4b302a0b7043` |
| HCEA `hs_not.c` | `87157e1b9e51fe888f24624b924fee1797304d7d` | 160 | `7b41c0dac5ca7bdcf904fcef13ac49e98d1321551f525f0dffc10cfc15c376ab` |
| HCEA `hs_print.c` | `123aeb538f8305924b14bca2da25aeb0de0cdcf3` | 352 | `fca32db09fb21cdbd26f34c61515fc966c50044e1433d262baae4ecdba41706e` |
| HCEA `hs_object_destroy_by_name.c` | `863bbe02fa51979204501b16252775d4b65d1a65` | 693 | `878a0764b0198be941fb10884da9972ce489df3ce026014c0a5888791fee9de6` |
| HCEA `hs_trigger_volume_test_objects_any.c` | `8e147f86831587b6f6d40f3c911b61143e5b876c` | 442 | `c4aee87f39b2e98a2df0d48e87f0f8cb91443f309ab096fe7a5a834298d5bb2c` |
| HCEA screened `hs_object_create_anew.c` | `22b376fa44cc8dcd210d763d74a39eb6dd9e634a` | 792 | `eb19fcd475cd024b2e065bc89683366fc7ff0484f65b1be53372687766e3a407` |
| HCEA `hs_object_destroy_containing.c` caller evidence | `eaf59fdb485da7e75b04b185120c392fc9abc992` | 445 | `c6addcd2fa5e54fe436f9f0fcf04ede3004497334caf608711fd7d0ff406a040` |

The canonical translation-unit blob is unchanged on every pre-existing
HS-named branch and has only the initial repository commit in its all-history
path log. No existing branch, worktree body, or object-matching ledger claims
these functions.

## Signature, caller, and relocation proof

The screened signatures were:

```c
boolean hs_not(boolean value);
void hs_print(char const *message);
void hs_object_destroy_by_name(short object_name_index);
boolean hs_trigger_volume_test_objects_any(
	short trigger_volume_index,
	long object_list_index);
void hs_object_create_anew(word object_name_index); /* screened, rejected */
```

`cseries.h` fixes `boolean` as an unsigned byte and `word` as an unsigned
short. The current `hs.c` evaluator declarations and macros independently fix
the arguments and return types of `hs_not`, `hs_print`,
`hs_trigger_volume_test_objects_any`, and the rejected
`hs_object_create_anew`. HCEA declares `hs_object_destroy_by_name` with a
signed 16-bit name index, while its containing-name caller passes that body as
the matching 16-bit callback. January confirms the contract by loading the
ordinary 32-bit stack slot, comparing `AX` with `-1`, and emitting a plain
`ret`. This corrects the preliminary rank packet's mistaken `word` spelling
for that one function.

All five target symbols use ordinary leading-underscore C decoration with no
stdcall suffix. January callees return with plain `ret`, and each target call
site is followed by caller stack cleanup. The exact relocation identities for
the retained bodies are:

- `_hs_print`: `_global_real_argb_green` at `+8`, `_terminal_printf` at
  `+15`;
- `_hs_object_destroy_by_name`: `_object_index_from_name_index` at `+14`,
  `_hs_object_destroy` at `+28`;
- `_hs_trigger_volume_test_objects_any`:
  `_hs_trigger_volume_test_objects` at `+14`.

The TU-local declarations use the undecorated C identifiers that produce
those target COFF spellings. No private or register-ABI helper is introduced.

## Readable behavior and fail-closed omission

`hs_not` returns logical negation. `hs_print` deliberately passes the message
as `terminal_printf`'s varargs format argument, matching both HCEA semantics
and January's exact two-relocation call sequence. The name-destroy wrapper
rejects `NONE`, resolves the object index, and delegates an extant object to
`hs_object_destroy`. The trigger-volume wrapper delegates with `FALSE` to
select any-object semantics. Every retained void body has an explicit terminal
return.

The one-shot `hs_object_create_anew` candidate preserved the January three
relocation destinations but emitted a 64-byte COMDAT with normalized SHA-256
`e240d276912ddf429800bd254e8675669606a7c400be4c6ee87437cdfdfed91c`.
Its relocation offsets were `17`, `31`, and `40`, versus target padded size
48, hash
`5e1d387f1cd88d67179915c6700612a8edd4c565bef5bbb4bd16ef1ae42b57e7`,
and offsets `15`, `29`, and `38`. The body and its support-only
`hs_object_create` declaration are absent from final source. No alternate
signature, branch spelling, local lifetime, or scheduling adjustment was
attempted.

## Ownership and scope boundary

Only `source/hs/hs_library_external.c` and this new Jonas ledger change. No
shared header, configuration, semantic exception, parked record, completion
label, protected-five source, pre-existing Markdown, or tracked-path deletion
is in scope. The protected five (`vehicles.c`, `ai_debug.c`, `ai_script.c`,
`actions.c`, and `units.c`) are untouched.

The final candidate object contains only the four accepted code COMDATs plus
compiler directive/debug sections. It defines no runtime `.rdata`, writable
`.data`, `.bss`, COMMON symbol, or aggregate owner. All runtime dependencies
remain typed undefined externals, and all 232 target non-code bytes remain
unclaimed. The source is readable defined C with no assembly, raw address,
pointer cast/overlay, type pun, inactive-union read, undefined behavior,
volatile scheduling device, register forcing, optimizer pragma/barrier,
synthetic anchor, or byte-forcing expression.

## Validation

- One natural five-body candidate compile: four strict accepts and one
  fail-closed removal, with no candidate retry.
- Complete Halo and libcmt builds plus generated ordinary and semantic
  reports: pass.
- Direct hardened comparison: all four retained functions exact; rejected
  `_hs_object_create_anew` absent; no extra runtime non-code or ownership.
- Semantic audit: 470 units, 4,134 functions evaluated, 3,994 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,055 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,042/11,060 functions,
  480,917/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,875/7,574 functions and
  468,003/1,770,166 code bytes; libcmt contributes 167/476 functions and
  12,914/55,015 code bytes.
- Admission: zero candidates and zero revocations; the one known
  `shell_xbox` completion-label contradiction remains explicit.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source-policy, deleted-path, protected-scope, and
  changed-path audits: pass.
- Clean committed-state snapshot, object deletion and forced same-path
  rebuild, regression check, and final direct COFF comparison: pass.

## Recorded committed-state replay

The implementation commit is
`43aec81f56c0fac5f6a7457701cfe7c7a6b989e0`. At that clean committed state,
`source/hs/hs_library_external.c` resolves to Git blob
`82b2dc5d3ccdd9e1c043ad0a142ae9ac5c4bbcdc`; its raw committed payload is
3,285 bytes with SHA-256
`04877d914fcfc8ac4b7918c80f68689d08909f0afc56dcefec49fa951b60b3d9`.
`git status --short --branch` reported only the branch header and no changed or
untracked path.

The clean regression snapshot was written to
`build/regression_hs_library_external_wave_20260821.json` with status
`SNAPSHOT_WRITTEN`, commit
`43aec81f56c0fac5f6a7457701cfe7c7a6b989e0`, and sole unit
`source/hs/hs_library_external`. The generated candidate object path was
resolved inside the worktree and its existing file was verified before
`build/base/source/hs/hs_library_external.obj` was deleted. The regression
check then invoked the normal repository Ninja/VC7 build edge for that unit.

The committed-state check returned `ok: true`, no failures, no warnings, no
newly exact functions, and no changed nonexact functions. Its `still_exact`
set is exactly `_hs_not`, `_hs_object_destroy_by_name`, `_hs_print`, and
`_hs_trigger_volume_test_objects_any`. A subsequent direct hardened comparison
reported strict equality for all four code COMDATs, including every relocation
address, type, destination, and addend. `_hs_object_create_anew` was absent.
The replay object SHA-256 was
`cf0b006f2ad30d73392c021bbf4253da8d63d1ed5c4b6431bf6964895ffae39d`;
its only defined external code owners were the four retained functions, with
no runtime `.rdata`, `.data`, `.bss`, or COMMON owner.

Nothing is pushed or history-rewritten.
