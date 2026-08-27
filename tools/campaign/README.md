# tools/campaign — the working harnesses

Five small tools that carried the 2026-08-16 session. All are run from the
worktree root and assume `ninja` has produced `build/base/**` and
`build/split/**`.

**Hard-coded path warning:** `gate.py`, `units_hunt_all.py` and
`tools/c2dbg32/*.py` embed the absolute path
`C:\halo-worktrees\claude-finish-hs-20260816\xbox\bin\vc7\CL.Exe`.
If you work in a different worktree, edit the `CL` constant in each (or export
your own copy). Everything else is path-relative.

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
```

`--edits` takes a JSON list of `[find, replace, tag]` triples applied to the real
source in memory (the file on disk is never touched), so you can sweep spellings
without dirtying the tree. `--disas` prints a side-by-side target/ours
disassembly with relocation annotations. Reads per-unit cflags out of
`build.ninja`, and always prepends `/nologo /c` (libcmt units omit them).

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
