# Playlist display-name leaf: genuine owner packet held

Baseline and restored canonical: `adc1f36a2a95cff0ae99ebbf3b3bc1ee026e1932`.
Donor: `C:/halo-worktrees/opus-30k-ui-script-profiles-20260906`, tip
`08d6bea92b1b7da925bf3687b44310b6203953dd`.

## Disposition

**No source admitted; zero new canonical bytes.** The natural display-name
leaf matches January, but its proper public header declaration regresses an
inherited exact UI function. The complete source/header packet was restored,
including the original physical line endings. All frozen source/config/toolchain
inputs and all 572 rebuilt / 833 target objects match the baseline again;
only object timestamps are excluded. No donor work was changed or discarded.

Canonical remains **930,363 meaningful exact code bytes, 6,283 credited
functions, 391/833 Matching objects, 2,061,024 matched data bytes**. The strict
sweep remains **6,329/8,245**. This object remains five exact / nine unwritten
listed functions, not Matching. Its inherited data/BSS ownership holds remain.

## Natural candidate and owner contract

The packet added only `playlist_profile_get_display_name` plus its proper
prototype in `source/saved games/playlist_profile.h`. Its C file included that
owner, existing `saved_game_files.h`, `text/unicode.h`, and `<xtl.h>`, removing
the two copied public delete/enumeration prototypes. `MAX_GAMENAME` comes from
the genuine XDK header and equals 128. No shared type, prefix facade, global,
private helper, semantic waiver, compiler option or inline was added.

```c
boolean playlist_profile_get_display_name(
	long playlist_profile_index,
	wchar_t *display_name)
{
	wchar_t *name = saved_game_file_get_display_name(playlist_profile_index);

	if (name)
	{
		ustrncpy(display_name, name, MAX_GAMENAME - 1);
		display_name[MAX_GAMENAME - 1] = 0;

		return TRUE;
	}

	return FALSE;
}
```

January calls the getter, then conditionally saves ESI and copies up to 127
wide characters, explicitly terminates output element 127, and returns AL=1.
The null branch leaves output untouched and returns AL=0. This supports the
ordinary success-first topology without authenticating exact original spelling.
The donor records one initial success-first compile. The older canonical
bounded-pair attempt used an early-false topology and was rejected; root read
that history before the current genuine-owner trial. No topology/declaration
permutation or second source-shape trial was performed here.

First root scratch compile: six exact / eight unwritten, forbidden
`_point_from_line3d` guard passed, no warning. New leaf: **53 meaningful / 64
padded bytes**, two ordered REL32 relocations at +0x08 (getter) and +0x1C
(`ustrncpy`), normalized SHA-256
`b52d949230e98323168a50c0af6cc57ed4d7367094548c78700b5ee11687ce90`.
The public owner matches January at value zero, type 32, storage class two.

## Full-tree failure, not a leaf failure

Before editing, actual VALID Ninja dependency records identified four existing
header consumers: SavedGameFiles, PlayerUI, UIWidget and
UIWidgetGameDataInputFunctions. The self-include made PlaylistProfile the fifth.
All five were rebuilt with the pinned XDK 3911 VC7 flags unchanged.

The complete stable sweep found one 64-padded gain and one loss:
`source/interface/ui_widget_game_data_input_functions::`
`_solo_level_select_list_update_displayed_items`.

That unedited function stays 704 padded bytes / 45 relocations, but its
normalized SHA changes from
`085a6479228260634b66faf32f9a24a26b26970e7d1e186bdeb793fd3f44e828`
to `f227fef9eb2b8f59b53286bf3eafbd6773e5804d66051f03a43d20a5879e0804`.
Its 693 meaningful exact bytes are lost in the trial, yielding 929,723 total
code bytes despite the 53-byte new leaf. The full build command completes,
but that is not a passing regression gate. Nothing from this trial was pushed.

The only changed input shared with this UI consumer is the owning playlist
header. Restoring the packet returns every object to its pretrial bytes beyond
timestamps. Do not move the prototype between positions, duplicate it in a
consumer, omit the owner header, or waive the UI loss. Reopen with a genuinely
necessary owner/schema/caller packet that independently passes full regression
checks, not by declaration-position search. The now-stopped Opus UI lane is
available for evidence and reconciliation; it does not by itself waive this hold.

## Behavioral evidence and preservation

An independent test executes actual January and freshly compiled trial x86,
after fail-closed byte and ordered-relocation equality checks. **20 tests
passed**: lengths 0/1/126/127/128/255, non-ASCII 16-bit units and embedded NUL,
null-name failures with five full-width indices and null/non-null output,
complete source/output guards, exact API calls, AL return, stack and callee-saved
registers. Removed-terminator and wrong-copy-count mutants are rejected.
Getter and `ustrncpy` are explicit contract stubs, not a filesystem/profile
implementation. The actual January Unicode wrapper corroborates its pointer/
count checks and cdecl `wcsncpy` call; valid non-overlapping buffers are required.

Root read the complete test. After rejecting the integration packet, the harness
was moved out of the collected tooling suite to
`scratch/test_playlist_profile_display_name_runtime_held_20260907.py`, with a
hold notice. It requires the preserved trial in an isolated tree; it must not be
reported as a passing test of the restored, absent canonical implementation.
Original test SHA: `4ddbb22f7fcb265529a20831ffda8b6edc1d212b28c500b6c8b3d64f15d78353`.
Held-notice version SHA: `f27efa84b66d6c90b22119f4a22e9e6bf65e541c39c0d86d7e878a619049c65f`.
Its verified candidate remains at
`scratch/opus2-playlist-display-runtime-20260907-a/playlist-display-name0/rebuilt.obj`,
SHA `a7b08d0a42ca1d3ec1b6302c74b3dbb493c9c755bb58bbba8e0107e361f4b86c`.

Full restored build passes. Final sweep: zero gains, zero regressions against
the original checkpoint. Full restored tooling suite: **1,019 passed, two
skipped, 26 subtests passed**. All 297 parks validate. No config/admission
record or whole-object label is changed.

## Frozen evidence

Each capture pins 2,303 files, including complete source/header/config inputs,
toolchain, all rebuilt and target objects, and reports. The trial capture
preserves both complete source files beneath its directory, not just prose.

| Artifact under canonical | SHA-256 |
| --- | --- |
| `scratch/playlist-display-before-20260907.json` | `fd50bc69546ffbec4c79ddf0a63946ea31581d7e4b6002b2f779200a2ac3d331` |
| `scratch/playlist-display-owner-trial-20260907.json` | `4b70199f8a990584a3fdf7bd9fee4f0b8f42a1f83e2556d712cc9bebc3db7f6a` |
| `scratch/playlist-display-restored-20260907.json` | `25d52079a7e9ca1bea98e80c50c5705ee279cbc5b13607e7124511933d18a5c8` |
| `scratch/playlist_display_restoration_audit_20260907.py` | `6c4bcdd0e1498da9f9687d695b3943597ed01875369fafb342f17b92b33efe0e` |
| `scratch/playlist-display-restoration-audit-20260907.json` | `3621a2dcc2b4a54eba564e65c9762adf9504220cc091190d5c29ea508a429acb` |

Restoration audit verdict: **PASS**. All three frozen captures were hash checked;
source/config/toolchain contents are restored exactly, and every rebuilt/target
object is equal to baseline with only COFF TimeDateStamp excluded. No runtime,
debug section, symbol, relocation, COMDAT, data/BSS or COMMON loss is hidden by
a broad metadata normalization.

This packet used January COFF/hex/disassembly, dependency evidence and runtime
execution. It does not claim IDA or Ghidra analysis was performed for this leaf.
