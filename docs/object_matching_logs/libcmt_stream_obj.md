# `libs/libcmt/stream.obj` — `__getstream` reconstruction

**Status: PARKED — near-complete, NOT matching.**
**Lane:** `claude/libcmt-stream` · **Object status in `config/config.json`:** `NonMatching`

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1
**Flags:** `/O1 /Gy /I"xbox/include"` (CRT favour-size settings; **not** the game's `/O2 /Oy- /DDEBUG /Dxbox`)
**Start state:** 0.00% — object `MISSING`, no source; none of the 212 `libcmt` units had ever been built.
**End state:** `.text` 290/290 bytes, byte-identical once relocation fields are masked; `.rdata` scope table likewise. **Strict comparator returns `False`: 21 relocations vs the target's 22.**

---

## 1. Verdict first

This object is **not accepted as matching** and must not be credited as such.

`section_infos_equal` returns `False`. The relocation **count** differs (21 vs 22),
which is a structural difference in the object file, not a naming or display
artifact. This project does not accept fuzzy matches, so the object stays
`NonMatching` and is parked here pending either a source-level lever that
produces the 22nd relocation, or provenance showing the original was built by a
compiler that emits it.

An earlier draft of this document recommended crediting this through
`config/semantic_matches.json`. **That recommendation was wrong and has been
withdrawn** — see §5 for why it does not qualify even on the project's own
tooling.

## 2. What the function is

`__getstream()` from the VC7 CRT `stream.c`: scans the `__piob[]` FILE-pool under
`_lock(_IOB_SCAN_LOCK)`, reusing a free slot or allocating a new `_FILEX`, wrapped
in `__try/__finally`. Confirmed by the target's imports: `__SEH_prolog`,
`__SEH_epilog`, `__lock`, `__unlock`, `__nstream`, `___piob`, `__mtinitlocknum`,
`__lock_file2`, `__unlock_file2`, `_malloc`, `_free`,
`___crtInitCritSecAndSpinCount`, `__imp__RtlEnterCriticalSection@4`.

Constants recovered from the disassembly, independently corroborated by the XDK
headers:

| constant | value | evidence |
|---|--:|---|
| `_IOB_SCAN_LOCK` | 1 | `push 1; call __lock` |
| `sizeof(_FILEX)` | 0x3c | `push 0x3c; call _malloc`; `0x20` FILE + `0x1c` Xbox `CRITICAL_SECTION` (`WinNT.h:4536`) |
| `_CRT_SPINCOUNT` | 4000 | `push 0xfa0` |
| `_STREAM_LOCKS` | 16 | `lea eax,[esi+0x10]` |
| `_IOB_ENTRIES` | 20 | `cmp esi,0x14` |
| `inuse` mask | 0x83 | `test byte ptr [eax+0xc],0x83` = `_IOREAD\|_IOWRT\|_IORW` |

## 3. Progress ladder

| step | ndiff | size | note |
|---|--:|--:|---|
| first draft (`== NULL` first, separate field stores) | 152 | 295/290 | wrong branch nesting |
| corrected: `!= NULL` first, alloc in `else`, chained field init | 17 | **290/290** | size exact |
| **+ inner test written as `!= 0` with `else`** | **1** | **290/290** | only the relocation-form delta remains |

Three source facts were required, all read off the target disassembly:

1. **`/O1 /Gy`, not `/O2`.** Favour-size emits the shared
   `__SEH_prolog`/`__SEH_epilog` helpers the target calls; `/O2` inlines the SEH
   frame instead and never matches.
2. **Branch nesting.** The target tests `__piob[i] != NULL` first
   (`cmp eax,ebx; je <alloc>`) and puts the allocation in the `else`.
3. **Chained field initialisation + inner branch polarity.** Store order +4, +0xc,
   then +8, +0, +0x1c, then +0x10 = `-1` corresponds to
   `retval->_flag = retval->_cnt = 0;`,
   `retval->_tmpfname = retval->_ptr = retval->_base = NULL;`,
   `retval->_file = -1;`. Writing the post-lock recheck as
   `if (inuse(...) != 0) { _unlock_file2(...); } else { retval = ...; break; }`
   reproduces January's block layout (the `retval`/`break` block placed
   out-of-line at 0x87); the natural `== 0` form inverts it. This step alone took
   the residual from 17 diffs to 1.

## 4. The residual

```
target  .text+0x109:  e8 19 01 00 00   + REL32 relocation vs __getstream (addend 0x119)
ours    .text+0x109:  e8 0b 00 00 00   direct displacement, no relocation entry
```

Both encode the same destination: `__getstream + 0x119 − (0x10a + 4) = 0x0b`,
which is exactly our stored displacement. The same pattern appears in the
`.rdata` SEH scope table:

```
target  ff ff ff ff 00 00 00 00 16 01 00 00    handler = __getstream + 0x116
ours    ff ff ff ff 00 00 00 00 00 00 00 00    handler = $L3640, addend 0
```

Our CL emits **local label symbols** for the SEH funclet — `$L3640` @0x116,
`$L3642` @0x119, `$L3641` @0x121, `$L3643` @0x10e, `$T3647` for the scope table —
and references them with addend 0. January's CL had no local labels and expressed
the same addresses as `__getstream + addend`.

Verified inert against 16 flag variants: `/Gy`, `/Gy-`, no `/Gy`, `/GX`, `/Gs`,
`/Gz`, `/G5`, `/G6`, `/GB`, `/GF`, `/Zl`, `/Ob0`, `/Oy-`, `/Os`, `/Oxs`, `/Gh`.
No source construct controls symbol emission for compiler-generated SEH funclets.

## 5. Why this is NOT a semantic-ledger candidate

The existing ledger precedents are **same-count** relocation cases where objdiff
merely displays the wrong label:

* `_cache_files_map_directory` — *"objdiff reports a local-label/relocation
  encoding false negative"*
* `_object_compute_node_matrices` — *"all 318 semantic relocations exact"*
* `_network_game_client_start_frame` — *"all 57 semantic relocations exact"*

In every one of those the relocation **count matches** and only the symbol a
relocation names differs. The project's own validator, `tools/audit_semantic_matches.py`,
encodes exactly that rule in `relocation_shape_matches`:

```python
if len(target_relocations) != len(base_relocations):
    return False
```

followed by a positional address+type check. Our case fails that first test —
the target has a relocation entry at `.text+0x10a` that our object **does not
have at all**. That is a different kind of difference from the precedents: not a
renamed relocation, but a missing one. Running the audit tool would reject it.

So the object is parked, not credited.

## 6. Reopen conditions

* A legal-C or flag lever that makes our CL emit the funclet reference as
  `__getstream + addend` instead of a local `$L` label (16 flag variants already
  ruled out; no source construct is known to reach compiler-internal SEH label
  emission).
* Provenance for the CL build that produced the shipped `libcmt.lib`. Note that
  this is now much weaker as a lead: all three distinct local VC7 suites emit the
  same bytes here (see the compiler A/B section below), so a fourth image would
  have to differ from all of them.

## 7. Build integration (this lane's other contribution)

`config/config.json` already declared `libs/libcmt/stream.c` (index 714). This
lane:

* adds `libs/libcmt/stream.c`;
* uses canonical's existing `libcmt` options — `cflags: ["/O1","/Gy"]`,
  `include_dirs: ["xbox/include"]`, `defines: []`;
* flips that one object `MISSING` → `NonMatching`.

The other 211 libcmt units stay `MISSING`, so no other build rules are emitted.
The `/O1 /Gy` recipe established here is the prerequisite for any further libcmt
work.

Canonical integration revalidates this object through the normal Ninja build,
the focused campaign gate, and the parked-evidence validator; no direct-CL-only
exception remains.

## Compiler A/B — all three local VC7 suites produce identical bytes

Prompted by the `units.obj` compiler-provenance work
(`units-campaign-current-20260802/docs/object_matching_logs/units_obj_compiler_provenance.md`
and `units_obj_compiler_qfe_hunt.md`), the local worktrees were re-censused for
distinct compiler binaries. **Three** distinct `CL.Exe` images exist, not one:

| suite | copies | `CL.Exe` SHA-256 (16) | note |
|---|--:|---|---|
| 13.00.9254.1 | 830 | `483e00c47bb08d69` | project standard |
| 13.00.9210 | 4 | `38955691c987c64d` | genuinely different `C1`/`C2` back end |
| vsnet-beta2-rc4-9254 | 1 | `cd6b42c2e4f14140` | reports 9254 but is a distinct image |

This object was rebuilt with all three complete suites (each with its matching
`C1.dll` / `C1xx.dll` / `C2.dll`), same flags, same source. **All three produce
byte-identical output**, with the identical residual.

That is a stronger park than "only one compiler is available": the residual is
now shown not to be compiler-version variance at all. It also corrects an
earlier claim in this log that only a single CL binary existed locally — that
was wrong, and the conclusion it supported (a QFE-level codegen difference)
is not supported by this test.
