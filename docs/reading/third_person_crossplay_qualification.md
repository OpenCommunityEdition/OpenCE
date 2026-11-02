# Session compatibility qualification, 1 November 2026

This records the `testing` merge and selectable-session work on
`online-third-person`. The merged testing revision is `b8d365fc3`; the final
implementation is `d0d96b2176d84894153b6f6b8d8a88f5be1bef29`.
Read the [camera walkthrough](third_person.md#session-compatibility) for
ownership, admission and the two session modes.

## Review and menu findings

The combined review covered the upstream merge, optional relay byte,
advertisements, bounded camera admission, host policy, client teardown,
checkpoint guards, settings and generated menus. The ordinary OpenCE 41
contract and independent MCC 63 capability remain separate. No stencil
shadows, cosmetic head tracking or camera gamepad bindings were added.

The real host flow exposed a premature lock: the engine opens its socket
before showing setup. Compatibility now locks at lobby entry, remote
admission or match start. The host's initial loopback admission remains
editable; clients cannot change the mode. Production-function regressions
exercise this distinction and retain the normal admission checks.

Host setup retains its existing controls. Invite Link moved into Session
Options, leaving room for two help lines above the main footer. Compatibility
and policy use standard row dimensions and fonts. Spinner bounds align the
text with its label and include both clickable arrows. The invite row copies
the complete address while displaying a short action prompt.

Rendered browser captures at 1280x960 and 1280x720 were inspected. Mouse
clicks changed compatibility in both directions and changed the camera
policy; keyboard navigation also worked. The final widescreen host capture
shows both help lines aligned with the labels and clear of the footer.

## Runtime evidence

The final gameplay engine was tested at `8744b631f`; the following
`d0d96b217` change only adjusts generated selector bounds and their test.
Windows used a portable release build; browsers used the matching CI debug
build in headless Edge. Signalling was private and local.

| Check | Observed result |
| --- | --- |
| Official OpenCE crossplay | Unmodified OpenCE build-337 (`899219cc0a14b03bc12f0517a1e9ee4c37158af8`) hosted GulchCE and joined GulchCE on Xbox Blood Gulch. Both directions passed default simulation tolerances with 45 two-player samples. GulchCE kept its saved camera preference on while enforcing first person. |
| Camera authority and admission | GulchCE Forced On and Forced Off passed with 44-45 samples. An official OpenCE client was refused entry to a camera session even with Forced Off. |
| Windows/browser gameplay | Four Blood Gulch sessions: both host directions in Crossplay and GulchCE Third Person. Each peer logged 45 two-player samples. All four passed the unmodified comparator tolerances and established WebRTC connections. Forced On overrode disabled personal preferences; Crossplay disabled the camera and its muzzle override. |
| Offline split screen | One rendered engine with two local players. Only player 2 scoped; only player 1 used free look and returned on release. Logged aim stayed fixed during free look/release. Turning player 2 afterward changed only that player's aim. Captures were inspected. |

Earlier focused runs in this assignment additionally covered official
OpenCE crossplay in both directions on CE Death Island, GulchCE Player
Choice, CE camera sessions, and MCC Mercury sessions with different package
folder names. MCC baseline and camera sessions continued after checkpoint
restore. Two Windows a30 camera sessions, Forced On and Forced Off, exercised
three reconnects and repeated checkpoint restores with 80 ms receive
latency and 2% simulated loss. These are opening-session checks, not full
campaign completions. Their evidence remains applicable to the unchanged
gameplay paths; the later setup-lock correction was retested on Xbox and
with the MCC lifecycle/admission fixtures.

The combat runners combine bot firing with synthetic reported-hit probes,
pickups, deaths and vehicle events. Accepted-hit counts are therefore not
exclusively physical gunfire. Passing the comparator means staying within
its existing tolerances, not zero transient position drift.

## Automated checks and limits

The final local regression run passed 131 checks with six platform-dependent
skips. Menu regeneration left the checked-in generated output unchanged.
The [build workflow](https://github.com/chestahh/GulchCE/actions/runs/41359912174)
passed for Linux, Windows, Android and web; the
[MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/41359186563)
passed for the final engine revision. Chromium and Firefox native WebRTC
transport checks passed, including simulated datagram loss. The final CI
Linux harness passed 2,147 tests, with 27 skips and seven subtests.

Local two-peer tests do not establish public WAN reachability, compatibility
with future OpenCE protocol revisions, full campaign correctness or runtime
behavior on Android hardware. No such claims are made here.

Disposable maps, binaries, browser profiles and duplicate captures were
removed after retaining useful logs and screenshots. Test saves were
archived with original relative paths and every exported file's SHA-256
verified before loose copies were removed. Original game data and user saves
were not modified. Local evidence lives under the worktree's
`build/session-compatibility`; the delivered bundle includes its concise
validation record.
