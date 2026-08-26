# `ui_widget.obj` Jonas `widget_free` first shot

## Result

This bounded source-only wave starts from clean cumulative commit
`0da30100e9b0ab965765d48367852fc55866436b`. `_widget_free` matches in the
first and only production candidate compile. `ui_widget.obj` advances from
11/102 to 12/102 strict functions and from 113/23,042 to 137/23,042
meaningful code bytes. It remains `NonMatching` and claims none of the
target's 26,804 data bytes.

| New retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_widget_free` | 24 | 32 | 2 | `cc93a70dfca252fd3df5197977186fd70762b9ddb4a73e2d4aaf4bb7fc249eaa` |

The function has a DIR32 relocation at `+8` to `_widget_memory_pool` and a
REL32 relocation at `+15` to `_dispose_pointer`. January proves the external
cdecl signature `void(void *)`, the unconditional call, the argument order,
and the complete 32-byte padded extent.

## Provenance and documentation audit

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/interface/ui_widget.obj`: 111,969 bytes, SHA-256
  `783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. The 81,920-byte `CL.Exe` has SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- The clean exact HCEA working repository at commit
  `c89106c4964f2df3a98ef7b4035d6750372797cd` contains
  `src/widget_free.c` blob `eee605622d12025e807c74e591547f2493a2d908`.
  Its 303-byte physical source has SHA-256
  `e1edaa76c264d8247cbf65e671043c5d71add55d7f6a2758a2225a80bf6632fa`.
  Supporting `blam_data_globals.h` and `stack_memory_pool.h` have SHA-256
  `119bd7c3cb948be79e4f49ad2ac230d235b45985a81a9ff0cbb2ab4149a96639`
  and `c428a4be774eb0a8c1b7862ace351506f675bb3667d6894c4b9cb0742d4351fd`.
- The clean address-comment HCEA cache at commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains independently
  corresponding blob `1e193699f8d8e000a15b45243a65dc4dd7eceed6`, physical SHA-256
  `56a8589f0acee4b466d256bb2a7fa562b671cacf358a560c6e8d76702cb4761a`.
- Current Claude/HCEA working commit
  `afc14e5b1d016560ac5808a7da5aa9f2adb2b885` has interface source blob
  `9fad3cb54b67b2536d69c3139c275a5416009103`, physical SHA-256
  `ebcb2ee52f914d48d68a93ec2ac33f7c0c9e1c2cdd55d486450b5bf319a08fd0`.
  The Stian research cache at commit
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7` has blob
  `3ebf5a20059a2dcea0db3591d9cb4eead9bd1572`, physical SHA-256
  `3440efe75ac1255ff25520d964254ec3877c3d7aff939e9627eebcfb846ad9cd`.
  Neither corresponding aggregate source contains a transplantable body.

The exact HCEA body independently preserves the same semantic wrapper:

```c
void widget_free(void *ptr)
{
    dispose_pointer(widget_memory_pool, ptr);
}
```

January COFF remains authoritative for the January ABI, code extent, bytes,
padding, relocation types, and relocation destinations. HCEA authenticates
source intent and the typed pool/callee relationship; its includes, duplicate
include, formatting, and donor-build ownership are not production source.

All available Claude documentation and machine-readable evidence relevant
to this object and its toolchain was reviewed before mutation. The canonical
`work/halo/CLAUDE.md`, interface-lane copy, and direct Claude repository copy
are 47,280 bytes with SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`.
The research-cache `CLAUDE.md` is 37,778 bytes with SHA-256
`97d4c63b9dd9d642cbe6774c5f3a578c60f2de002fce3ac888cd4ee635cc4c3d`.
Their binary-first, C89, ABI, prototype, and narrow-validation guidance was
applied as research discipline. Donor-only commands, hooks, raw-address
techniques, configuration, and completion labels were not treated as
production rules.

The direct Claude repository was audited at commit
`0e1fc785b7f5fb8c8d6fec8cae667005aa663957`; its only dirty path was the
reviewed working `tools/equivalence/leaf_cache.json`, and no dirty production
source was consumed. Relevant documentation and tooling identities are:

- `AGENTS.md`: `b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`;
- `CLAUDE.md`: `c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
- `docs/references/prototype-inference.md`:
  `ebd2a3e72206a2b727a70adf269fa5932f1392ced43a303167b969e6042be5b2`;
- `docs/references/kb-update-policy.md`:
  `4c9e2a4ae5b521f14ea45c6debe01c73e88258b35f6b51f50e53354528e0ea42`;
- `docs/ghidra-live-delinker.md`:
  `87069190158c3f6a05bf542aa5626b488539d8890df8f508b6d5d537ed6ddbae`;
- `docs/verification_policy.md`:
  `1a394e10a67b29926e6d82aa1303327e336053413f55a1b2ff545d8bebdf5922`;
- `tools/audit/check_arg_counts.py`:
  `3e5842ca9b8f7c16142a0b35fa4417a49405d51ff422ce879ea38dea22c8ef32`;
- `tools/verify/vc71_scores.json`:
  `93e71b2f0907266c259b4e9fdf823136d0e8d27409fbc52d8f1f37f68457e35c`;
- working `tools/equivalence/leaf_cache.json`:
  `e6ccf76b86864210331042cd1cd541b3de3319125b02d0d4483eb93b0eacaf6a`;
- `kb.json`: `6c5f1d3dc73686770b7888d656042cbbf43a9e3fa0289d4cc637e8cbf759ba52`.

Claude's `kb.json` declaration `void widget_free(void)` is stale and was
explicitly rejected. January reads `[ebp+8]`, the exact HCEA source takes one
pointer, the public campaign header already declares `void widget_free(void *)`,
current callers supply one argument, and the clean correspondence
repository classifies it as `exact_unique`, cdecl, one parameter. The
correspondence repository is at
`bf520b6938d256ecc9a55d93e994a9f5dd389ec1`; its correspondence JSON has
SHA-256 `c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`.
Its `EVIDENCE_MODEL.md`, `REPRODUCTION.md`, and `FINAL_REPORT.md` have
SHA-256 `2520a4a0b516752cc1e74bc92a2e494f1b425044de3eb124ad6b214cdff9c607`,
`6ed5dddd92e6c33f602ed911e5af328a1057c57186da41e135acb07536caf25`,
and `0bb6498b9ba55a28eaa47990a5cf9a1a1b090ba77d83606e3be48808d5ba13f1`.
Those documents preserve the rule that cross-build evidence cannot replace
January/XDK strict comparison.

Current campaign validation-tool identities are:

- `tools/coff_compare.py`:
  `411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`;
- `tools/regression_gate.py`:
  `d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`;
- `tools/semantic_progress.py`:
  `5cc1bedd907e277a603b47080d78ecd1fcd668834fd564b15257a1c5b4eefa67`;
- `tools/audit_semantic_matches.py`:
  `05b709e5899c5d43ff3aee7d4077b906cb80b60f4e8e8041c039ad7667e1a33b`;
- `tools/audit_object_admission.py`:
  `1e17c437a53433680fa01c3c558794bc1ee29246955e028bf5c9d816faf8fcc1`;
- `tools/parked_functions.py`:
  `deb8d6b78b3085ba896230242bf1c4629efd442beb9c8a1962d723cc063e4646`;
- `tools/transfer_executor.py`:
  `9a30ce16b29656235de647528616e9b462016611fb2de48aace1a20f6812d888`.

The current campaign's comparator, whole-TU regression, semantic-progress,
admission, park, and transfer tools were used as validation infrastructure,
never as matching authority, a code generator, or an exception. The complete
reachable campaign history and production-worktree census revealed no prior
retained `_widget_free` implementation attempt.

## Retained source and ownership

The baseline C blob is `9d6207c85a9531b5092ef4808bb65d121d44cda9`.
The retained C blob is `6e75ddbbbc76591f30444f88c5537cecd431d60a`.
Its canonical CRLF worktree form is 25,631 bytes with 815 CRLF endings, zero
bare LF endings, and SHA-256
`a01f35f004ea60d56680ba3968f5191f1fdedc46b1542243c129469864767cbf`.
The public header blob remains
`2155a5a4d1bfbb9a5e209557da01a2e189b2a207`; its existing one-argument
prototype is unchanged.

Only `source/interface/ui_widget.c` changes. It adds a private incomplete
type, the exact private callee prototype, an external declaration of the
pool-pointer owner, and the body after child counting and before the main-menu
leaves:

```c
struct stack_memory_pool;

void dispose_pointer(
	struct stack_memory_pool *pool,
	void *pointer);

extern struct stack_memory_pool *widget_memory_pool;

void widget_free(
	void *ptr)
{
	dispose_pointer(widget_memory_pool, ptr);

	return;
}
```

The target owns `_widget_memory_pool` inside composite
`.data:_data_00301da0+0x4034`; the candidate deliberately leaves it undefined
and defines no composite owner. The existing event-handler translation unit
has a stale local `void *widget_free(void *)` declaration whose three callers
discard the result. That declaration debt is recorded and deferred rather
than broadening this wave.

The retained source is readable C89 with each parameter on its own line and
an explicit terminal return. It contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, compiler control, barrier, raw address or raw
offset access, pointer/integer reconstruction, cast or union pun, undefined
behavior, synthetic anchor, byte forcing, storage definition, or comparator
exception.

## First-shot discipline and strict comparison

The clean cumulative baseline object is 3,190 bytes with SHA-256
`9868ed31b62b9c9318c3bfd70433aca8aaa09c62c4f00b5ddaa907d110219384`.
The pre-wave whole-TU manifest is 1,999,649 bytes with SHA-256
`3bd20e6694e7bd0a72d83285f0baf2128ef606a39b3cfa6e800a29c35f980bb4`.

The frozen source passed XDK `/Zs` with production include paths and flags;
the restored baseline object's size and hash remained unchanged. Its resolved
path was proved inside the isolated worktree, the literal object was deleted,
absence was verified, and the selected-object dry run named exactly one edge.
The ordinary selected-object edge then ran exactly once:

```text
[1/1] CL build\base\source\interface\ui_widget.obj
ui_widget.c
```

The preserved first artifact is 3,419 bytes with SHA-256
`4a34330391ccc7c0bcfe4cc88f90ca657825bcafa5de74d8f4541051150b1046`.
There was no declaration, spelling, control-flow, source-shape, or compiler-
tuning retry. CRLF normalization after preservation changes no preprocessing
token or Git blob and is not a candidate experiment.

Direct hardened comparison proves the new function and all eleven inherited
functions independently strict:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ui_widgets_safe_to_load` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_ui_widgets_set_fade_value` | 13 | 16 | 1 | `3307464d0a4c3704ea054046b1a71de6ed4ab1c8c397d82e01d96e9777ce0123` |
| `_ui_widget_debug_show_path` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_widget_instance_count_children` | 29 | 32 | 0 | `ae15477fb628a00d4f51a719438f430179585e7069e2e464dee76e2b0d39b60f` |
| `_widget_free` | 24 | 32 | 2 | `cc93a70dfca252fd3df5197977186fd70762b9ddb4a73e2d4aaf4bb7fc249eaa` |
| `_main_menu_active` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_main_menu_is_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_ui_widget_load_progress_widget` | 16 | 16 | 2 | `fe882d09c800f4644f0bc8697f08b2787ad6c0fa8f285098947f4cd38b34ed3b` |
| `_filesystem_check_thread_is_active` | 14 | 16 | 1 | `e3d713882962f29c50f9529269c70d85d4e93d911caff2d69af551668dff0246` |
| `_ui_main_menu_music_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_code_000d4680` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_code_000d4690` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| **Strict total** | **137** | **224** | **10** | |

The generic pre-wave gate reports `_widget_free` as `NEWLY_EXACT` and an
empty `changed_nonexact` array. It fails closed on expected accepted-function
evidence, debug-COMDAT, string-COMDAT, and symbol-set index changes caused by
inserting a new external function COMDAT. This is reviewed evidence, not a
waiver: direct hardened comparison proves all twelve complete padded sections,
normalized bytes, and ordered relocation address/type/destination/addend
tuples exact. The baseline and candidate runtime `.data` and `.rdata` payloads
remain identical. No adjudication, semantic exception, or comparator change
was added.

The first artifact defines exactly the twelve strict-table functions, the
inherited two-byte `_dashboard_abort_error`, and the inherited 72-byte
progress-string COMDAT. Undefined external records are `__fltused`,
`_bss_00454240`, `_error`, `_dispose_pointer`, and `_widget_memory_pool`.
It defines no BSS or COMMON owner. The wave adds no runtime data section and
receives no matched-data credit.

## Validation

The complete Halo and libcmt object graph passes and both it and the
`all_source` dry run report no pending work. Canonical outputs are:

- `build/report.json`: 1,574,650 bytes, SHA-256
  `dd7b2f223395fa50461afd27baace7897f1fc90787ea398cfbd6c37e5678e92f`;
- `build/semantic_report.json`: 2,983,640 bytes, SHA-256
  `02fd5d7aa025cb831aa5b01495ea618e03c79cee1eb73c63394ff8039199907d`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,155/11,060 exact functions,
  502,560/2,198,102 meaningful code bytes, and
  1,835,208/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,988/7,574 exact functions,
  489,646/1,770,166 meaningful code bytes, and
  1,830,016/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,262 functions:
4,120 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,467 missing,
4,181 accepted exact, and zero unit errors. Admission remains zero candidates
and zero revocations, with only the inherited unrelated
`source/shell/shell_xbox` completion-label contradiction. Parks remain three
active, zero stale, and zero invalid. All 179 tooling tests pass, and all 14
selected tracked/configuration/canonical-report JSON documents parse.

The tracked scope is exactly `source/interface/ui_widget.c` plus this new
ledger. There are no deletions. The complete `source/units/**` tree and all
eight protected paths have zero intersection with the wave. No push, amend,
rebase, history rewrite, or worktree removal is performed.
