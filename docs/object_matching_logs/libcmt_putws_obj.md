# `libs/libcmt/putws.obj` provenance park

This isolated lane starts from authoritative campaign commit `3d2b81dd`.
Configuration index 719 remains `MISSING`. The unit was unparked, unowned,
and absent from prior source/history/logs. No production or configuration file
was edited.

## Plain-English behavior and provenance

`_putws` locks standard output, temporarily prepares its buffer, writes every
wide character followed by a newline, restores buffering, and always unlocks
the stream through a Microsoft SEH `finally` cleanup. It returns zero on
success and wide EOF on failure.

The exact `obj\i386\putws.obj` member was extracted from XDK 3911
`libcmt.lib`. CodeView identifies ordinary Microsoft C at
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\puts.c`, compiled as its
wide specialization. This is not assembly.

## Measured blocker

January and the authentic member both contain a 143-byte function and the
same 12-byte SEH scope table. The authentic compiler member has 11 text
relocations; January's split representation has 12. Two sites prove that the
difference is extraction metadata, not source shape:

| Site | January split identity | Authentic XDK identity |
|---|---|---|
| `+0x72` | synthetic internal REL32 to cleanup at `+0x84` | encoded local displacement `+0x0E`, no relocation |
| `+0x88` | REL32 labeled `_ai_debug + 216415` | REL32 to the real callee `__unlock_file2` |

The second identity is especially decisive: the disassembly and Microsoft
source both show `_unlock_file2(stdout)`. No honest reconstruction may replace
that call with an unrelated Halo AI routine merely to satisfy a splitter's
symbol guess. The target's effective linked instruction behavior corresponds
to the authentic member, but strict COFF relocation identity correctly refuses
the extracted representation.

## Decision and reopen criterion

No source experiment was run. Production/configuration remain pristine. This
is a docs-only park because source cannot manufacture the synthetic local
relocation or the incorrect `_ai_debug` identity without byte forcing and
semantic corruption.

Reopen only after a reviewed target-extraction fix restores `_unlock_file2`
and preserves the compiler's local finally displacement, or after a
fail-closed comparator adjudication specifically proves these csplit artifacts.

## House, Berth, bug, and integrity audit

No assembly, `volatile`, forced inline, undefined behavior, flag change, byte
patch, comparator exception, or prebuilt substitution was introduced. No
original implementation bug was found; the SEH cleanup is intentional. The
only repository change is this plain-English evidence log. Tooling tests passed
179/179 and `git diff --check` is clean.
