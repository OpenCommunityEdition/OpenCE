# tools/campaign — the working harnesses

Five small tools that carried the 2026-08-16 session. All are run from the
worktree root and assume `ninja` has produced `build/base/**` and
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
python tools/campaign/board.py                 # totals + every partially-done object
python tools/campaign/board.py source/hs/hs    # one unit, listing its non-exact functions
```

Walks `objdiff.json`, gates every `_`-prefixed `.text` function-start symbol with
`coff_compare.section_infos_equal`, counts `config/parked.json` as done. This is
the only number that counts. Objects within N functions of complete:

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
```

`--edits` takes a JSON list of `[find, replace, tag]` triples applied to the real
source in memory (the file on disk is never touched), so you can sweep spellings
without dirtying the tree. `--disas` prints a side-by-side target/ours
disassembly with relocation annotations. Reads per-unit cflags out of
`build.ninja`, and always prepends `/nologo /c` (libcmt units omit them).
`--cflag` appends one diagnostic compiler option after those unit flags and may
be repeated. It is an audit aid only: a flag improvement is not source-match
evidence and must not be admitted without independent build provenance.
`--alias` applies a whole-identifier source rename in memory for symbol-mapping
probes; like `--edits`, it never changes the source file and is not admission
evidence by itself. `--out` preserves the PID-isolated probe object at the
specified path for independent COFF or disassembly inspection. `--all` prints
exact functions as well as residual and unwritten functions.

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
