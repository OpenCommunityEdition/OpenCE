# Player-facing changelog

The changelog uses short, authored highlights. It does not publish commit
subjects, infer changes from file names, or call an AI service. Internal
maintenance commits can remain unannotated. The generated JSON supplies the
menu; `notes.md` supplies release highlights from the same authored source.
Each published entry uses its exact GitHub release build number. Previous and
Next navigate these published builds; long notes scroll within a build's page.

The main menu also shows `gulch build N` directly below the original Xbox
version, using its orange font and style. `N` comes from the executable's
`HALO_BUILD_NUMBER`, the same GitHub Actions number used for its release;
the label does not infer a number from Git history or the notes archive.
Local and testing builds without a release number show `gulch build local`.
This label remains visible when the optional changelog is disabled.

## Writing a note

Add one or more lines to the commit message body:

```text
Player-Note: Added controller navigation to the changelog.
Player-Note: Fixed missing sounds on supported maps.
Player-Note: [INTERFACE] Improved menu navigation.
Player-Note: [QUALITY OF LIFE] Added controller shortcuts.
```

Each note must contain 1 to 100 printable ASCII characters. Keep it a short
sentence about a change players can observe. Backslashes, menu pipes (`|`),
tabs, newlines within a note, control characters and non-ASCII text are
rejected. Annotations start at the beginning of a line. The spelling
`Player-Note:` is case sensitive; incorrectly cased prefixes and empty notes
fail generation with the commit SHA and a corrective diagnostic. Ordinary
prose mentioning Player-Note and indented documentation examples are ignored.
XML punctuation is permitted; the menu generator must escape it when writing
XML. A note's surrounding spaces are removed.

An optional `[CATEGORY]` prefix assigns a blue section heading in the menu.
Its label contains 1 to 24 uppercase ASCII letters, digits and single spaces,
starting with a letter, such as `INTERFACE`, `QUALITY OF LIFE`, `GAMEPLAY`,
`GRAPHICS` or `MULTIPLAYER`. The note's 100-character limit excludes this
prefix. Categories are authored explicitly; the collector never classifies
notes from keywords or asks an AI service. Plain notes continue to work and
appear under `GENERAL`. Categories follow their first appearance in a build,
and notes retain their order within each category. Release Markdown retains
the original note order and plain bullet format.

Use `Player-Note: none` to explicitly omit a commit, including any catch-up
entry for it. Do not combine `none` with another note in the same commit.
The omission marker has no category.
Unannotated commits produce no automatic fallback. Do not add annotations
merely to satisfy a quota.

Before adding a highlight, inspect its actual diff and evidence. Never use
commit titles alone as evidence of a player-facing change.

## Generating the files

Run from a full checkout with Git and Python's standard library:

```sh
python tools/player_notes.py --head HEAD --output build/player-notes
python tools/player_notes.py --head <full-sha> --build-number 433 --base <previous-successful-release-sha> --previous previous-changelog.json --output build/player-notes
python tools/player_notes.py --head HEAD --preview --output build/player-notes-preview
```

`--head` accepts `HEAD` or a full 40-character commit SHA. `--base` accepts a
full commit SHA and must be an ancestor of the selected head. A base identifies
the previous successful release, rather than the previous commit: highlights
therefore include changes from failed or skipped builds. Without a base,
release highlights cover all history after the catch-up baseline. Invalid or
unavailable ranges fail instead of publishing incomplete notes. Shallow
checkouts fail with instructions to use `fetch-depth: 0` or fetch full history.

`--build-number` is the GitHub Actions run number, from 1 to 2147483647, used in the published
release's `Build N` title and `build-N` tag. Without it, local generation defaults
to an `UNRELEASED` preview; `--preview` states that choice explicitly. Preview and
a numbered build cannot be combined. Each run produces a current entry even
when its note list is empty, so releases without player-facing changes still
have their own build page. Rebuilding the same commit under a new build number
also creates a new entry.

`--previous` supplies the schema-2 `changelog.json` attached to the previous
published release. It requires `--base`, and the archive head must match that
base exactly. The first archive entry must name the same commit, each retained
entry must name an ancestor of it, and build numbers must be unique positive
integers at most 2147483647, in descending order below the current build number. Dates and every
note are validated too. A missing or malformed supplied file fails generation.
Omit `--previous` when no archive exists, such as the first numbered rollout:
the result contains only the current build, with no inferred historical build
numbers. A preview may include a supplied published archive.

The default catch-up manifest is
`port/assets/changelog/catchup.json`; `--catchup <path>` selects another.
The tool reads Git history and the manifest, then writes only the requested
output directory. Each output file is replaced atomically after successful
validation. JSON encoding, entry dates from Git and note ordering are
deterministic for the same selected history and manifest.

The output directory contains:

- `changelog.json`: the selected history for the in-game menu.
- `notes.md`: all newly introduced highlights in `base..head`, or
  `baseline..head` without a base. With no highlights it contains
  `No player-facing notes were recorded for this build.` This describes missing
  notes without claiming that the code has no player-facing changes.

The menu JSON has this schema:

```json
{
  "schema_version": 2,
  "head": "0123456789012345678901234567890123456789",
  "entries": [
    {
      "build": 433,
      "date": "2026-10-18",
      "commit": "0123456789012345678901234567890123456789",
      "notes": ["Added controller navigation to the changelog."],
      "categories": ["INTERFACE"]
    }
  ]
}
```

The current entry comes first; previous entries retain their published build
numbers, commit IDs, dates and notes. Preview entries have `"build": null` and
are labeled `UNRELEASED`; published archives contain numbered entries only.
An optional `categories` list contains exactly one label per note; an empty
label means `GENERAL`. The collector omits this field for a new entry with no
categorized notes. Previous archives without it remain valid and unchanged;
category metadata on published entries is carried forward and validated.
Git history determines which notes enter the current release, including every
newly reachable commit's body on merged upstream and testing branches. It does
not determine release identities or create pages for commits. Notes follow
their chronological introduction to the first-parent history, preserving
topological and authored order within that introduction. Only commits in
`base..head`, limited to coverage after the catch-up baseline, contribute.

Duplicate notes ignore case, repeated whitespace and an optional trailing
period. The first introduction owns a duplicate, even if it is outside the
visible history window. Repeating an old annotation therefore does not publish
it again in a later release's `notes.md`. A duplicate also keeps its first
category; adding a new label does not create a new highlight.

The menu retains the current build and the seven newest previous builds, with an explicit
stderr notice if older entries are omitted. It preserves every note within
each retained entry; the renderer scrolls long entries. A retained history
with more than 4096 notes fails for manual consolidation. Release Markdown is
only the current release's highlights. Preview mode collects notes since the
base, or since the baseline when no base is supplied, and applies the same
4096-note limit. An empty range still produces the current build or preview
entry with an empty notes list.

## Catch-up history

The checked-in catch-up manifest records reviewed highlights for older
commits that predate `Player-Note:` annotations:

```json
{
  "schema_version": 1,
  "baseline": "0123456789012345678901234567890123456789",
  "notes": [
    {
      "commit": "1123456789012345678901234567890123456789",
      "text": "Added controller navigation to the changelog.",
      "category": "INTERFACE"
    }
  ]
}
```

The baseline is excluded and must be an ancestor of the selected head. Every
referenced commit must exist and use a full SHA; every text follows the same
note validation rules. Only references actually included in the selected
ancestry contribute. For several highlights on one commit, preserve their
intended order in the manifest. Catch-up highlights precede message annotations
on that commit and participate in the same deduplication. To omit a catch-up
highlight, remove its entry instead of using the text `none`.
The optional `category` uses the same label rules as annotations. Existing
catch-up headings are manually assigned from the reviewed change descriptions;
they do not change the notes' text, commit provenance or release boundaries.

## CI and published history

The workflow's `changelog` job checks out full history and generates the notes
once. On `main`, `tools/changelog_release_base.py` lists published `build-*`
releases, excludes drafts, prereleases and the current run's tag, then chooses
the most recently published release whose tag resolves to an ancestor of the
selected commit. The workflow publishes builds only from `main`; ancestry
checking prevents a newer unrelated release from becoming the range base.
The helper reads the documented `tagName`, `publishedAt`, `isDraft` and
`isPrerelease` fields from
[`gh release list`](https://cli.github.com/manual/gh_release_list).

With no prior published ancestor, generation uses the catch-up baseline. A
prior release older than that baseline is valid: the baseline still defines
the start of authored changelog coverage. Failed or cancelled builds create
no published base, so the next successful release includes their highlights.
Other branches receive one `UNRELEASED` preview entry.

On `main`, the job passes `GITHUB_RUN_NUMBER` as `--build-number`, together with
`--require-notes`. If the release range contains new commits but no new highlights,
generation fails before any platform builds or publication. An empty range or a
range containing only explicit `Player-Note: none` commits is allowed. For an
internal-only integration, review the entire release range and put
`Player-Note: none` on its tip commit to explicitly confirm that decision.
An omission on an unrelated maintenance commit does not excuse an unannotated
merge. Repeating an already released note does not satisfy this check.

Upstream authors need not use our annotation convention. When merging upstream,
review its changes and add short `Player-Note:` summaries to the integration
commit, or add reviewed notes for that commit to the catch-up manifest. This
guard catches empty releases; it cannot judge whether nonempty notes describe
every player-facing change. That remains part of integration review.

If the previous
release has a `changelog.json` asset, the helper downloads it and the collector
uses it as `--previous`. A release from before this feature has no archive and
starts the numbered history at the current release. Existing but invalid
archives fail rather than silently dropping known build history. Unpublished
preview snapshots never become previous published entries. The helper accepts
only the named JSON asset, limits it to 2 MiB, and checks its head and newest
build number against the selected release tag before the collector validates
the complete archive.

The job renders the text-only menu from `changelog.json` and uploads one
`player-changelog` artifact. Linux, Windows, Android and web all download this
same artifact before building, installing its menu XML and JSON. Consumers
therefore need no Git history or note-generation dependencies of their own.
The collection job uses read-only repository permission. Release publication
keeps the existing `contents: write` permission in the release job.

After all four platforms succeed, publication uses the generated `notes.md`
and attaches `changelog.json` alongside the build archives. The JSON records
the selected head and retained history, providing an archive of exactly the
notes used for that build. Later builds carry the previous release's retained
entries forward in their new archive. They therefore preserve exact release
boundaries even after the workflow deletes older releases and tags. Full Git
history and authored notes determine the current range; the latest published
archive supplies the previous release pages. No unpublished commit grouping
is presented as a release.

Repeating generation for the same source and base produces the same files.
The artifact upload replaces the same named artifact on reruns, and base
selection excludes the current run's release tag. Release publication retains
the workflow's existing `gh release create` behavior: a fully published run's
tag already exists, so rerunning publication is not an automatic replacement
of the existing release.

## Verification

```sh
python -m pytest -q tools/test_player_notes.py
```

These tests create real, temporary Git repositories. They cover merged
ancestry, chronological collection, release ranges, deduplication, explicit
omissions, previews, catch-up filtering, missing commits, shallow history,
unsafe text and revision-option injection. They also check deterministic
output, numbered release snapshots, failed-build recovery, empty releases,
archive validation and preservation of existing outputs when validation fails.
Category checks cover explicit labels, old uncategorized archives, duplicate
ownership, unsafe labels, parallel metadata lengths and collection through to
the renderer's grouped blue-heading tables. These tests do not establish
visual menu correctness or game compatibility; those
require the renderer's checks and the project's platform validation.
