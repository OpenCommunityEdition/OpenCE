# tools/campaign — the working harnesses

The original five small tools carried the 2026-08-16 session; the directory now
also contains later campaign diagnostics. Unless a tool says otherwise, run it
from the worktree root after `ninja` has produced `build/base/**` and
`build/split/**`.

**Hard-coded path warning:** `units_hunt_all.py` and `tools/c2dbg32/*.py`
embed the absolute path
`C:\halo-worktrees\claude-finish-hs-20260816\xbox\bin\vc7\CL.Exe`.
If you work in a different worktree, edit the `CL` constant in each. `gate.py`
uses `HALO_CL` when set, then the current worktree's `xbox/bin/vc7/CL.Exe`,
and retains the old absolute path only as a compatibility fallback. Everything
else is path-relative.

## board.py — the scoreboard

```
python tools/campaign/board.py                 # totals + selected small partial objects
python tools/campaign/board.py source/hs/hs    # one unit, listing its non-exact functions
```

Walks `objdiff.json` and gates the eligible `_`-prefixed `.text` function-start
owners with `coff_compare.section_infos_equal`. Only `EXACT` rows contribute to
its done totals. Ordinary fuzzy `PARKED` rows do not count; `asm-implemented`
and `vendored-assembly` rows are displayed separately as `ASM` and do not count
in this C-reconstruction board even when their bytes are equal.

Board byte sizes include target section padding. Its selected function-owner
population and function-only "fully exact" objects are not the canonical
meaningful-byte or whole-object completion totals. Use the semantic-adjusted
production progress report for meaningful credited bytes, and require the
data/BSS/linkage/extra-owner and source-authenticity admission checks before
marking an object `Matching`.

The default per-object listing is filtered to partial objects with some exact
work and at most 8,000 remaining padded bytes. It is not an exhaustive census
and must not be diffed as a regression proof; use `stable_verdicts.py` for the
complete built-owner set. To inspect objects within N functions of completing
their listed function set:

```
python tools/campaign/board.py | grep -E 'remaining +[0-9]+ B / [123] fn'
```

## gate.py — compile-and-gate one unit, optionally with edits

```
python tools/campaign/gate.py source/units/units                       # whole unit
python tools/campaign/gate.py source/units/units --fn _unit_place      # one function
python tools/campaign/gate.py source/units/units --edits e.json --fn _unit_place --disas _unit_place
python tools/campaign/gate.py source/units/units --cflag /Ob1 --all    # diagnostic flag override
python tools/campaign/gate.py source/ai/encounters --alias old_name=new_name --all
python tools/campaign/gate.py source/ai/encounters --out scratch/encounters-probe.obj
python tools/campaign/gate.py source/physics/collision_bsp --forbid-emitted-symbol _point_from_line3d --all
```

`--edits` takes a JSON list of `[find, replace, tag]` triples applied to the real
source in memory (the file on disk is never touched), so you can sweep spellings
without dirtying the tree. `--disas` prints a side-by-side target/ours
disassembly with relocation annotations, but pairs instructions by index rather
than aligning insertions/deletions. Use `alndiff.py` for actual instruction
alignment. Spelling sweeps are not source-authenticity evidence and are not
authorized by the current house rules merely because the tool supports them.
Reads per-unit cflags out of
`build.ninja`, and always prepends `/nologo /c` (libcmt units omit them).
`--cflag` appends one diagnostic compiler option after those unit flags and may
be repeated. It is an audit aid only: a flag improvement is not source-match
evidence and must not be admitted without independent build provenance.
`--alias` applies a whole-identifier source rename in memory for symbol-mapping
probes; like `--edits`, it never changes the source file and is not admission
evidence by itself. `--out` preserves the PID-isolated probe object at the
specified path for independent COFF or disassembly inspection. `--all` prints
exact functions as well as residual and unwritten functions.

`--forbid-emitted-symbol NAME` checks the complete candidate COFF symbol table
for a code definition with that exact decorated name and exits nonzero if one
is present. Repeat the option to forbid additional names. The check includes
candidate-only functions absent from the target, regardless of `--fn`, as well
as static definitions, named code subsections, and nonzero symbol offsets.
Undefined references, COMMON symbols, and data definitions do not fail this
code-emission check. Failures identify the object, symbol-table index, section,
offset, and section size for inspection. A clean check prints an explicit pass;
without this option the existing gate output and exit behavior are unchanged.
Use this guard to verify the January `point_from_line3d` inline schedule: the
ordinary target-function listing cannot detect an extra candidate-only helper.

## apply_edits.py — land an edits JSON on the real file

```
python tools/campaign/apply_edits.py e.json source/units/units.c
```

Normalizes LF patterns to the file's CRLF. **Write edit JSON with a script, not
a bash heredoc** — heredocs eat a backslash level and silently corrupt
`"c:\\halo\\SOURCE\\..."` assert paths (cost real time this session; use
`chr(92)` if you must build such a string in Python).

## alias_scan.py — the naming-gap scanner

```
python tools/campaign/alias_scan.py
```

Finds target `_code_XXXXXXXX` sections whose size + normalized sha equal a
section our base object already emits under a real name. Rename the
`config/symbols.json` entry at that `file_offset`, run `ninja` (csplit
regenerates the split), and the pair gates EXACT. Closed two whole objects this
session. **Skip candidates that match multiple our-names** — ambiguous.

## stable_verdicts.py — rename-stable whole-tree regression snapshots

```
python -m tools.campaign.stable_verdicts snapshot scratch/before.json
python -m tools.campaign.stable_verdicts snapshot scratch/after.json
python -m tools.campaign.stable_verdicts diff scratch/before.json scratch/after.json
```

Keys each function by unit plus January target section index, so a reviewed
semantic symbol rename does not appear as one lost function and one new
function. The diff exits nonzero for every exact-to-residual/unwritten loss and
reports the padded bytes gained.

## alndiff.py — aligned normalized disassembly differences

```
python tools/campaign/alndiff.py source/objects/damage _object_damage_update
python tools/campaign/alndiff.py source/ai/actors _code_00123456 --ours-function _actor_helper
```

Disassembles exactly one target and candidate function section with Capstone,
masks relocated dwords, preserves relocation identities as annotations, and
aligns the normalized instruction streams with `difflib.SequenceMatcher`.
Unlike an index-aligned zip, an inserted block does not make the rest of the
function look different. `--target-object` and `--ours-object` inspect explicit
objects; relative overrides are resolved from `--root`.

## tinfo.py — bounded read-only COFF inspection

```
python tools/campaign/tinfo.py source/objects/damage
python tools/campaign/tinfo.py source/objects/damage --fn _object_damage_update
python tools/campaign/tinfo.py source/objects/damage --data --preview-bytes 128
python tools/campaign/tinfo.py --object scratch/probe.obj --json
```

Lists function section sizes, normalized hashes, and relocation identities, or
prints bounded previews of non-code sections. It opens objects read-only and
never materializes uninitialized/BSS contents. The default object is the
January split target; pass `--base` for the built candidate.

## namegap.py — proposal-only symbol-name gaps

```
python tools/campaign/namegap.py source/objects/damage
python tools/campaign/namegap.py --json
```

Finds unmatched target/candidate function sections that are strictly equal
under `coff_compare.section_infos_equal` and uniquely paired in both
directions. Results are **non-authoritative proposals**, not original-name
proof. The tool never edits source or config, rejects ambiguous pairings, and
requires independent January/PDB/source provenance review before a human makes
any rename.

## units_hunt_all.py — cross-branch splice hunter

```
python tools/campaign/units_hunt_all.py _unit_update _unit_died
```

Collects every unique version of the named functions across all ~1,100 local
branches' `source/units/units.c`, splices each into the current file, compiles,
and gates. Finds work already done in another lane. Generalize by editing the
unit path constants.

## Related tools (already in tools/)

- `tools/bijection_walk.py` — register-bijection lockstep walker; the instrument
  that separates real structural divergence from allocation permutation.
  `python tools/bijection_walk.py --unit source/units/units _unit_enter_seat -v`
- `tools/coff_compare.py` — the comparator; `section_infos_equal` is the gate.
- `tools/c2dbg32/` — the 32-bit C2.Dll debugger (see its own README).
