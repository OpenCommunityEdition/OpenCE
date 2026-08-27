# `rasterizer_xbox.obj`: typed stencil-cache getter

## Scope and result

This bounded wave retains one newly strict-exact function and its exact writable
state owner in `source/rasterizer/xbox/rasterizer_xbox.c`:

- `_rasterizer_get_stencil_mode`: 7 meaningful / 16 padded bytes, one
  `IMAGE_REL_I386_DIR32` relocation;
- `_data_0030ceec`: six exact initialized `.data` bytes, no relocation.

The implementation commit is `34547c22f5026374585885f1ea690539a69f0508`
(`Match rasterizer stencil cache getter`). The prior cumulative commit is
`396ab90c0c621d54af8d0bf670f0db8c0d8e190f`. The source blob changes from
`8ece4b731294f1deaf3725245c50c00b2c95184c` to
`abfec76012f3d29bde4e85d35e10c64c819edb63`; the retained file SHA-256 is
`2fc86bc47aa7d1239f198446bba95c1a3671e49201c6ea69236a5e3652eb18ae`.

No header, configuration content, semantic exception, adjudication, parked
entry, completion label, protected Units/game-engine/AI source, or existing
Claude-authored document is changed. `config/symbols.json` remained
content-identical; a transient worktree stat/line-ending indication was cleared
without staging a diff. Nothing is pushed.

The object remains `NonMatching`: it now has 5/95 ordinary exact functions,
24/15,574 meaningful code bytes, and 6/9,048 data bytes. This wave adds one
function, seven meaningful code bytes, and six data bytes; it does not complete
an object.

## Evidence authority and research audit

January split COFF is authoritative for bytes, padding, ABI, relocation type,
relocation destination/addend, linkage, storage, and data ownership. The
compiler is Microsoft VC7 13.00.9254.1, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
on the unchanged `/O2 /Oy- /DDEBUG /Dxbox` production edge.

Before source mutation, every reachable Claude tip (27 refs) was searched for
the unit, getter, stencil cache, and `_data_0030ceec`. The only applicable
Claude material was target symbol/frontier evidence; no candidate body or
compiler recipe existed. The following historical Claude branches and files
were also reviewed where they intersected the current frontier:

- `claude/lra-cache-17` at `f0e5a48f...`;
- `claude/finish-hs-20260816` research packets
  `research/reloc_identity_residuals.md`,
  `research/single_function_residual_diagnoses.md`,
  `tools/c2dbg32/IR_LAYOUT.md`, `research/PROMPT_finish_hs.md`,
  `research/PROMPT_next_session.md`, and
  `research/residual_frontier.json`;
- the current `docs/matching_methodology.md`, SHA-256
  `3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.

The preserved HCEA checkout at
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halocea-reference`
corroborates the later-build semantic model: rasterizer stencil state is cached
and initialized to `-1`, and the setter tracks the previous stencil mode. HCEA
does not supply this January getter body and is not used as byte authority.
The user's wider Xbox-recomp, Baboon, demon, nimbus, Halo-PC archive,
OpenSauce, reverse-skill, and golden-test references remain useful contextual
research/tooling leads, but none supersedes this pinned January COFF or yielded
a more direct getter definition for this wave.

The current shape-atlas entry for this getter was deliberately rejected as
stale generic-shape evidence. Direct target inspection proves a distinct
load-and-return section; future work must not retry the stale atlas hash as a
transfer recipe.

## January packet

The complete January getter section is:

```text
66 a1 04 00 00 00 c3 90 90 90 90 90 90 90 90 90
```

Its meaningful instructions are `mov ax, word ptr [_data_0030ceec+4]` and
`ret`. The relocation is `IMAGE_REL_I386_DIR32` at section offset `+2`, with
resolved destination `_data_0030ceec+4`. Its strict normalized SHA-256 is
`1f82cf78b0df84e5b0bf619acb9ebfc8c165ff2558679479ef137123faa560a2`.

The owner `_data_0030ceec` is an external storage-class-2 six-byte `.data`
object with bytes:

```text
ff ff 00 00 ff ff
```

It has no relocations and normalized SHA-256
`b4e07c90bb40a057a2b1092a772022a9cd785345085a50cd438e6a75df02b72a`.
January references establish three signed-short fields: the first and third
are initialized to `-1`, the middle field is zero, getter/setter traffic uses
the third field at `+4`, and vertex-shader traffic uses the first field.

## Retained natural C

The source uses a typed three-short `rasterizer_hardware_state_cache`, a named
`INVALID_RASTERIZER_HARDWARE_STATE = -1` constant, and a semantic
`rasterizer_state_cache` alias. The anonymous January owner spelling
`data_0030ceec` is retained only to preserve the authoritative external
relocation identity; typed consumers do not perform raw byte-offset access.

The getter follows the house format:

```c
short rasterizer_get_stencil_mode(
	void)
{
	return rasterizer_state_cache.stencil_mode;
}
```

The code is C89-compatible. It adds no assembly, `register`, `volatile`,
barrier, pragma, intrinsic, attribute, raw address, pointer/integer conversion,
undefined behavior, synthetic caller, or byte-forcing construct.

## Frozen compile waves

The clean pre-wave manifest is
`build/audit/rasterizer_stencil_getter_baseline_20260826.json` at
`396ab90c...`.

The first evidence wave initially gave the typed owner a new external spelling.
That one normal VC7 emission proved the getter and data payload exact, but the
strict regression gate exposed a relocation-owner identity mismatch. This was
genuine new evidence, not a code-shape preference.

The one corrective wave restored `_data_0030ceec`, placed the new declarations
after the four inherited exact bodies so their source topology stayed fixed,
and retained the semantic typed alias. A `/Zs` parse-only check passed before
the corrective production emission. The corrective dry run contained exactly
one `CL` edge and the required split edge; no later candidate tuning occurred.

The pre-wave fail-closed check then reported exactly:

- `_rasterizer_get_stencil_mode` as `NEWLY_EXACT`;
- `changed_nonexact: []`;
- evidence changes for the four inherited exact functions;
- the new `.data|owners=_data_0030ceec` section;
- four associative `.debug$F` records, three inherited XDK select-any `.rdata`
  sections, and the symbol inventory.

These failures were reviewed, not waived. Adding the target-owned `.data`
section renumbers candidate COFF section indices and therefore changes the
gate's physical candidate evidence records. Direct owner-based comparison,
which removes only object-local numeric indices while preserving all runtime
bytes, padding, relocation addresses/types/destinations/addends, linkage, and
storage, proves there is no inherited runtime regression. This is the same
documented additive-ownership workflow used by prior campaign waves. No
adjudication or exception was added.

## Strict retained census

After the corrective emission and again after the committed forced rebuild,
the hardened comparator reports `all_equal: true` for every emitted runtime
owner:

| Owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `__rasterizer_reset_state` | 1 / 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_spin_begin` | 1 / 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_spin_end` | 1 / 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_set_vblank_callback` | 14 / 16 | 1 | `63bf78a6f07971df04769bae2ba18a6b90615e87a71be92f83beb002afa345bf` |
| `_rasterizer_get_stencil_mode` | 7 / 16 | 1 | `1f82cf78b0df84e5b0bf619acb9ebfc8c165ff2558679479ef137123faa560a2` |
| `_data_0030ceec` | 6 / 6 | 0 | `b4e07c90bb40a057a2b1092a772022a9cd785345085a50cd438e6a75df02b72a` |

The vblank callback's relocation remains REL32 at `+8` to
`_D3DDevice_SetVerticalBlankCallback@4`. The getter relocation remains DIR32
at `+2` to the defined `.data` owner `_data_0030ceec+4`.

## Validation and committed replay

Canonical regeneration ran only `csplit`, report, and semantic-report edges;
it performed no extra candidate compile. Current campaign progress is 378/833
complete objects, 4,108/11,060 ordinary exact functions,
454,166/2,198,102 meaningful code bytes, and
1,811,284/4,176,062 data bytes. The semantic audit scans 470 units, evaluates
4,303 functions, reports 4,160 semantic exact, 114 hidden exact / 64,705 hidden
bytes, 4,221 accepted exact, and zero unit errors.

The clean committed-state manifest is
`build/audit/rasterizer_stencil_getter_committed_20260826.json`, SHA-256
`99b59634eede8ce1796d2a9c6acf09fb328c69d0af17ac6f254ee7f0143ed886`.
Its immediate no-build check has no failures or warnings, no newly exact or
changed-nonexact entry, and all five functions above under `still_exact`.

The generated base object was then deleted only after its resolved absolute
path was verified inside the cumulative worktree. Exactly one pinned VC7 `CL`
edge rebuilt `build/base/source/rasterizer/xbox/rasterizer_xbox.obj`. The
same committed manifest replay is again completely green, and both hardened
code and data comparisons again report `all_equal: true`.

The replayed raw base-object SHA-256 is
`91adbb3d0b94bf658632b0c5490b6c6dce9ec3eb99b6d2a0bcf2f81708e12c8b`;
the unchanged January split object is
`039e8296db80d1391e59f6d8ce59fefc49618513926edbbe4e9621d8b7202182`.
Raw candidate-object hashes are phase-specific COFF artifacts and are not used
to replace the strict normalized section/relocation proof.

The canonical report SHA-256 is
`5b366c297c86873b3c81ec9db38091187e4c54b6a59b1f853623c5336536effa`;
the semantic report SHA-256 is
`a68ea00cd89e5a75614a24fd3aff25406b1a470554b539a24ea2d9f773ae52f5`.

Final gates:

- `python -m unittest discover -s tools -p 'test_*.py' -v`: 179/179 pass;
- object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` contradiction remains;
- parked validation: three active, zero stale, zero invalid;
- `halobetacache_build` and `libcmt_build` dry runs: no work;
- added-source banned-construct count: zero;
- protected-path count: zero;
- `git diff --check`: clean.

No completion label is changed, no semantic credit is manufactured, and no
push, amend, rebase, or history rewrite is performed.
