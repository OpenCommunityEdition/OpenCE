# Main startup connection: natural reconstruction boundary (2026-09-05)

## Result

One natural scratch candidate restores the genuinely unwritten
`main_setup_connection` with no compiler barrier. Frozen Main changes from
**73 exact / 2 residual / 20 unwritten** to **73 exact / 3 residual / 19
unwritten**. All 75 inherited code owners and 53 inherited data owners retain
their bytes, ordered relocations, and runtime ownership properties. The only
new owners are the setup function and its strict/property-exact 25-byte January
diagnostic string. There is **zero new exact code credit** and no whole-object
Matching claim.

Target is **182 meaningful / 192 padded bytes, 17 relocations**, normalized SHA
`c5411956292ae880665d080ef10f13e76665ff55bb11fdb8983c5209a4260c72`.
Candidate is **173 meaningful / 176 padded bytes, 16 relocations**, normalized
SHA `7337c8f95e5e76c6be2f564871682de0ddd5916cff77a9eb825f02a3664ec2d6`.
Ordinary pinned objdiff displays **90.42%**. This is a genuine residual; no
semantic allowlist or comparator exception is requested.

## Exact source scope

The minimal production manifest is
`scratch/main_setup_production_20260905.manifest.json`. It changes only
`source/main/main.c`:

- Name the existing one-byte `__unknown115` field
  `boolean playback_last_recording`, preserving its 0x65 offset.
- Rename its genuine existing use in
  `main_set_game_connection_to_film_playback`; that function remains exact.
- Format the existing owner-TU `main_setup_connection` forward declaration.
- Add the complete ordinary setup body immediately before time initialization.

No header, config, compiler flag, alias, helper, aggregate extent, storage
definition, runtime guard, intrinsic, barrier, volatile qualifier, or inline
schedule was changed. No optional layout typedefs were added to production
source. The inherited `boolean` is a typedef of `byte`, both unsigned char;
the field change is semantic naming, not representation tuning.

The existing source asserts `_main_globals` size 0x620 and `game_options` size
0x10c. Machine diagnostics independently retain the flag's byte load/store at
0x65, short connection at 0x2c, menu byte at 0x63, solo map at 0x75,
0x10c-byte options stack frame, difficulty word at options+6, and map at+12.
No second or prefix-only options facade was introduced.

## January semantic and ABI proof

January target owner is `_main_setup_connection`, file offset/RVA 0xf1c20
(990240), section88, external storage2, function type32, `.text` alignment16,
selection1. The candidate preserves those properties other than its honest
code size. It consumes no arguments, returns void, and uses normal cdecl calls.

The complete target has three reachable branches:

1. With playback flag set, clear the menu-request flag, store film connection
   enum3, call `error(_error_silent, "error opening saved film")`, set the menu
   request, load the menu, and return. The flag stores straddle a genuine
   external call and are observable, not inert codegen anchors. January does
   not attempt a film-open or playback API in this path.
2. Otherwise, a menu request loads the menu and returns.
3. Otherwise, select local connection enum0; initialize a genuine options
   object; `csstrncpy` 255 bytes from the solo map; terminate map[255]; assign
   the short global difficulty; precache blocking; dispose the old map; and
   call `main_new_map(&options)`.

All actual APIs already have proper included owners: `main.h` for menu load,
`game.h` for enums/options/precache/disposal, `cseries.h` for string copy and
NUMBEROF, and `errors.h` for error priority and variadic error API. The existing
`main_new_map` prototype is in its own Main TU. Target reads its pointer from
[ebp+8], agreeing with that existing cdecl signature. Its separate 208-byte
body is not implemented or dragged into this packet.

The complete incoming census scanned **all 833 target objects and every
relocation-bearing section**, not only recognized code functions. Setup has
only `main_loop+0x65` as incoming reference; both target and baseline already
contain the identical call. The flag has exactly two references: its named
setter and setup. The setter is genuinely called by the HS evaluation wrapper
in a different TU. New-map has three genuine Main callers: change-map,
load-UI, and setup. No data-pointer or external-TU setup reference was found.

Despite that same-TU scope, this packet deliberately preserves existing
external linkage. The January PDB public extract has no setup label, and Main
compiland241 has only112bytes of OBJNAME/compiler records, not an original
private-linkage/type record. No static metadata change is inferred here.

## Residual, provenance, and stop condition

January contains two physical `main_menu_load(); return;` tails. The natural
compiler output cross-jumps the direct-menu branch at+0x48 to the already
existing tail at+0x38, removing nine meaningful bytes and one call relocation.
The local-map suffix is otherwise the same instructions and relocation
identities displaced by nine bytes. The complete film branch is unchanged.
Aligned disassembly, not an index-zipped gate view, establishes the boundary.

Historical donor commit `f50be1825f6291c8837c7dcaba078d983bf9e8a5`
(`agent/main-strict-complete`) recorded the same first natural176/16 result.
Its later exact192/17 result used a prohibited `_ReadWriteBarrier` after the
direct menu call; that source is excluded. Donor source blob:
`4abe5197a1bdd3062e6715fb085c87f87ec6aa6e`; ledger blob:
`eef44838c72f1b1aed16416d55302e9a517aa95c`. The public-nine20260828 ledger
explicitly excluded this setup owner and its barrier-controlled form.

Semantic name and intent are corroborated by frozen HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`,
`src/blam/main/main_setup_connection.c`, in
`../../research-cache/halocea-cseries-20260820`. HCEA's actual film playback
branch is later-only and was not copied; January's immediate error is the
authority. The before/after flag behavior is explicitly preserved, not
rewritten into a plausible but historically different feature.

Park as `unclassified`: the observed cross-jump is established, but no
original source fact explains January's decision to retain both tails. Do not
repeat the old source-shape/barrier experiments. Reopen only with genuine
January source/local provenance or a demonstrated ordinary source topology;
no exactness tuning occurred in this pass.

## Complete runtime/data ownership audit

`scratch/main_setup_owner_audit_20260905.json` records all real runtime code
and data owners, including symbol offset/storage/type and section logical
size/flags/alignment/selection/raw presence, normalized bytes and ordered
relocations. It also records all local code-label owner properties.

- Target95code/74data; baseline75code/53data; candidate76code/54data.
- All inherited owners unchanged; the untouched PID baseline also equals the
  frozen production base for every runtime owner.
- Exactly one new code owner, `_main_setup_connection`.
- Exactly one new data owner,
  `??_C@_0BJ@CKCGDFO@error?5opening?5saved?5film?$AA@`, 25 bytes including NUL,
  strict normalized SHA
  `b6f15d9b412d96496bbf3dc6590541ac15851098a223f95b51186a769dbac332`.
  It matches target properties too: offset0, storage2/type0, align4,
  `.rdata`, selection2, flags0x40301040, zero relocations.
- No candidate-only code, COMMON, new global, or emitted point_from_line3d.
- All17 candidate-only data owners pre-exist unchanged: ten scenario-path
  strings, the scenario_paths table, `"w"`, two real constants, and three SDK
  D3D constants. This packet does not adjudicate or credit their inherited
  whole-object ownership differences.
- Inherited local code-label properties unchanged. Debug/source bookkeeping
  is not treated as runtime byte credit or whole-object completion.

The source credibility scanner reports one file scanned, zero findings.
Independent review has also reported a passing target-first three-branch
behavior harness, including at-call flags, options pointer/copy/layout, call
order, and ABI. Its final independent ledger is maintained by the reviewer;
root must include it with final production checks.

## Reproduction and frozen artifacts

Exactly one untouched PID gate and one code-producing natural candidate gate
were run. There were no source variants or diagnostic flag changes.

```text
python tools/campaign/gate.py source/main/main --out scratch/main-setup-baseline-20260905.obj --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/main/main --edits scratch/main_setup_first_natural_20260905.edits.json --out scratch/main-setup-first-natural-20260905.obj --forbid-emitted-symbol _point_from_line3d
python scratch/main_setup_owner_audit_20260905.py
python tools/campaign/alndiff.py source/main/main _main_setup_connection --target-object scratch/main-setup-target-20260905.obj --ours-object scratch/main-setup-first-natural-20260905.obj
build/tools/objdiff-cli.exe diff -1 scratch/main-setup-target-20260905.obj -2 scratch/main-setup-first-natural-20260905.obj _main_setup_connection
python tools/fake_match_scan.py scratch/main_setup_first_natural_20260905.c --format json
```

Paths are relative to canonical
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

| Frozen artifact | SHA-256 |
| --- | --- |
| `scratch/main-setup-target-20260905.obj` | `48dd640a170457b7d73c59c725b18d85f7eca91af350ce0403c0c1ba0470aa2f` |
| `scratch/main-setup-base-20260905.obj` | `1d29dbcb855889660c67de2adae127c6d1f4d8f72af28a3bb4fd3e9eb9937ea8` |
| `scratch/main-setup-baseline-20260905.obj` | `6a94d85937e2ecee6215c0c88cf429407919d0bf8255e7e39a05b57fb8d1996a` |
| `scratch/main-setup-first-natural-20260905.obj` | `da9f72b5a59ec3a4b5f86390d307a5be779fb6cc212df93a24ef3f662b83cb3d` |
| `scratch/main_setup_first_natural_20260905.c` | `72a43b68a945d6aa99ef434c591db1cb9ced12cec259f036f5ff6baf5c5381c6` |
| `scratch/main_setup_first_natural_20260905.edits.json` | `ec86d0b6c545b0ab660f152dc100b49bd771aec31dd59ed8fbe9985138565020` |
| `scratch/main_setup_owner_audit_20260905.json` | `039e0c34d65993c533dc696b37a4afeba7777495321f2bda7e2b973e682a8c49` |

No worker production edit, Ninja/configure, commit, push, or global park check
was performed. Root alone integrates the minimal manifest, adds the honest
park against this durable destination, runs full build/regression checks, and
decides publication.

## Canonical integration verification

Root applied the minimal source-only manifest and the park, then compiled the
actual production object. Its SHA-256 is
`cb38b3277767db3602617158e940913846c9bc78cff70319457822fed9d48cb9`.
The ordinary full-report percentage is **90.416664%**, recorded without
rounding in the park. The initial progress step correctly rejected the new
park until that actual full-report measurement was added; no source retry
occurred.

Root's independent complete actual-before/after audit preserves all75 code
and53 data owners. It adds only the intended function and25-byte target-exact
literal. The only inherited runtime-record transition is Main loop's unchanged
REL32 at+0x65: its target symbol becomes the now-defined external setup owner.
Every other byte, relocation field, owner property, anonymous data record, and
COMMON record is unchanged. This specific undefined-to-defined transition is
reviewed explicitly, not erased globally.

The unchanged independent behavior harness also passes all three branches
against the actual production object and January.

| Actual integration evidence | SHA-256 |
| --- | --- |
| `scratch/main_setup_canonical_owner_audit_20260905.json` | `b06599b251e98d09f6877d3bbea1ae72b080d4373e5f6de175939719b0b95ba2` |
| `scratch/main_setup_canonical_runtime_audit_20260905.json` | `b76c54ad49a7b24d44facab5f169688e4b29e213d6c8ef94862408d7c5f2e5e7` |
| `scratch/main_setup_canonical_behavior_20260905.json` | `dce664fce45fd908983bb7e262082469c4e8d99f3c890355c2bbd72b7b5b6961` |

Full Ninja passes. The stable sweep `scratch/main-texture-canonical-first-20260905.json`
retains6,217/8,245 strict owners with zero gains or regressions versus the
restored baseline. Units remains189/189 exact; both protected Bitmap hashes
are unchanged. All280 parks validate (zero stale/invalid); all288 tests and26
subtests pass. Fresh admission finds0 candidates/0 contradictions/5 recorded
rejections/0 revocations. The production meaningful totals remain906,391 code
bytes,6,171 functions,2,030,868 data bytes,391/833 Matching objects. No exact
code, data, or whole-object gain is claimed for this batch.
