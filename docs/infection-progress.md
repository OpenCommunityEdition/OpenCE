# Infection contribution validation

This is the public validation summary. Private logs, device identifiers,
network addresses, commercial assets and build artifacts are not committed.
The detailed local development ledger and all original evidence are retained
outside the contribution checkout. PASS below is scoped to the stated check.

## Historical P33 campaign (before upstream integration)

Source base: `b1ef7f7e7aff00f0efdca4d575f7d9196d0b1590` plus the Infection
development changes. These are historical results, not a claim that every
case was repeated on the rebased contribution branch.

| Platform / check | Result | Evidence and limits |
| --- | --- | --- |
| Windows Debug and Release | PASS | Both built. Release session 137: 12 checks, three full engines, no protocol bots; FF ON/OFF, remote damage, penalties, conversion, respawn, wins and Team Slayer score limit. Session 131: human win at 9000 active ticks. Session 132: 16 variant/ammo checks. |
| Windows capacity | PASS, scoped | Earlier runs accepted 128 protocol connections and demonstrated spawn recovery after disconnects. These are not 128 simulated full clients; stationary participants can occupy native spawn locations. Session 135 used six full engines for 241 seconds with late join, disconnect and rejoin. |
| Linux build and runtime | PASS, scoped | Clang 18.1.3, portable ILP32 build. `linux-verdict-02`: 12 checks / three full engines; `linux-survival`: three peer checks at 9000 ticks; original CTF/Oddball/King/Race in the eight completed rounds of `linux-native-completed-prefix`, then `linux-slayer-return`: four Infection-to-Slayer checks. A longer nine-mode runner timed out after eight rounds; it is not counted as a complete pass. |
| Linux local test suite | Initial FAIL; targeted repairs passed | One full run: 565 passed, 24 failed, 31 skipped. Failures were fixture diagnostic stubs, x87-vs-SSE arithmetic, and missing compiler-rt. Targeted recheck: 53 passed, two skipped, one TSan environment failure. That same TSan binary passed in an isolated network-disabled container with the necessary seccomp setting. The full suite was not repeated. |
| Linux graphics | PASS, scoped | OpenGL 4.5 Mesa/llvmpipe rendered a menu under Xvfb and exited normally after 20 seconds. This is not physical-GPU multiplayer coverage. |
| Web build and transport | PASS | Emscripten 6.0.10; 13 JavaScript unit checks; Chromium WebGL2/worker/OPFS smoke; Chromium and Firefox native WebRTC/DTLS, including loss; browser/browser and both native/browser host directions connected. |
| Web Infection | PASS, scoped | `web-infection-rendered`: eight peer-round checks, Linux/browser in both hosting directions and both initial zombie roles, real combat, deaths/new-unit respawns, inventories, conversions and matching zombie wins. Dedicated Web FF and human-survival scenarios were not separately run. |
| Android build and rendered startup | PASS | NDK 27.2.12479018, Clang 18 guest compiler and JDK17; ARM64 APK built. Rendered on Motorola Edge 50 Fusion, Android 16. |
| Android/Windows Infection | PASS, scoped | Twelve completed peer-round checks, two full engines and no bots in passing runs. Both hosting directions, both initial zombie roles, conversion, skull-only inventory, damage, death/respawn, return to Slayer and Android-host survival at exactly 9000 ticks. LAN, one device; not exhaustive device or Internet coverage. |
| Android complete Friendly Fire | INCOMPLETE | The third full participant did not discover the LAN host; a protocol helper behind container NAT later timed out. Partial FF ON damage and a human betrayal victim remaining human were observed. No complete Android FF ON/OFF plus penalty/respawn verdict exists. |

Historical binary SHA-256 identifiers:

- Windows Debug: `39ecb0c335e15d57ba2cda9bc0b6f0247184dfc0b5fa6d42f4fb16176b4e5c0b`.
- Windows Release: `93b5fc14f47b2d97f8c86f267ed4e562c97ea64b4ebc69a90f6869a235fa62fd`.
- Linux Debug: `67317c7c53e95aa4f4c3ab30e074541b6e24757de4db83f066768863904c47f6`.
- Android Debug APK: `6db488556abe2d718cd46eabbe16f356fe04d3c62d07cdc18e1e72ddcbd4a346`.

## Integration with current upstream

Target: `main`, `7524ba61` (build 220). The original dirty workspace remains
intact; integration is in a separate checkout. Upstream history and menu
additions are retained. Conflicts were limited to protocol numbering and
the menu generator. Personal test-path defaults were changed to `assets/maps`;
explicit path arguments continue to select the caller's local data.

Protocol 34 combines upstream 29's Custom Edition map-name/hash agreement
with P33's 48-byte options, reliable role state and damage-rule agreement.
Retaining 33 would incorrectly admit private P33 clients with older map
semantics. GPVO v4 and the Infection gameplay rules are unchanged.

Checks on the integrated source (2026-10-17):

| Check | Result | Evidence / command |
| --- | --- | --- |
| Windows portable Debug build | PASS | `python configure.py --portable --pgo=off --lto=off`; `ninja -j8 windows`. |
| Windows targeted rules/menus/format/Web graph | 103 PASS, 9 SKIP, 1 environment FAIL | Selected Infection, ammo, verifier, protocol and menu tests. The Web socket harness requires POSIX `pthread.h`; it failed under Windows clang, then passed on Linux. No failed gameplay assertion was hidden. |
| Source-hook regression | 24 PASS | `python -m pytest -q tools/harness/tests/test_stale_player_damage.py tools/test_infection_presentation.py`. |
| Final wire and captured-party verifier | 8 PASS | Recorded protocol check plus the party verifier against retained full-client evidence. Its seven evidence-dependent cases SKIP when `OPENCE_PARTY_EVIDENCE` is absent. |
| Windows full-engine sequence | PASS | `test_dual_combat.py --map bloodgulch --seconds 240`: two full engines, no bots, four rounds (Slayer, Infection host zombie, Infection remote zombie, Slayer). Real damage, deaths/new-unit respawns, conversion and matching zombie victories. 57 paired live snapshots, zero team disagreements, maximum position difference 0.025, matching postgame statistics, maximum RTT 13ms and local camera distance 0.62. |
| Linux portable Debug build | PASS | `ninja -j8 linux` with Clang 18.1.3. |
| Linux targeted rules and Web socket harness | 46 PASS | `python3 -m pytest -q tools/test_web_build.py::test_the_sockets_inside_the_page tools/test_infection.py tools/test_infection_manual_options.py tools/harness/tests/test_stale_player_damage.py`. |
| Web build and JavaScript tests | PASS | `ninja -j8 web`, Emscripten 6.0.10; `node --test port/web/tests/*.test.js`: 18 passed. |
| Web startup and native transport | PASS | `smoke.mjs` in Chromium: worker, WebGL2, OPFS and engine startup; `webrtc_native.mjs chromium firefox`: native DTLS exchanges, also with 10% datagram loss. |
| Integrated Web/native sessions | PASS, connection/tick scope | `internet.mjs build/web/site <maps> build/linux/halo`: browser/browser, Linux host/browser client, browser host/Linux client all completed through the local broker and WebRTC. This is not a fresh Infection combat campaign or a remote-NAT test. |
| Android ARM64 guest, host and APK | PASS, build only | `ninja -j8 android android_apk`, NDK 27.2.12479018, Clang 18 and JDK17. The integrated APK has not been reinstalled or functionally retested on the phone. Historical P33 device results above remain separately scoped. |

Linux/Web/Android configure command (supply your installed toolchain paths):

```sh
python3 configure.py --portable --pgo=off --lto=off --web-emcc <emsdk>/upstream/emscripten/emcc --android-ndk <ndk> --android-guest-cc <clang>
```

The Windows-to-Linux validation copy initially failed on CRLF patch/template
inputs, a relocated SDL CMake cache and the Gradle wrapper's executable bit.
Canonical LF text, a fresh SDL build directory and the Git-recorded executable
permission resolved these setup failures without an engine or build-script
change. Node 24 from Emscripten was used for current Playwright; system Node18
was too old. The main campaign was not repeated wholesale.

Integrated artifact SHA-256 identifiers (artifacts are not committed):

- Windows Debug: `beaac2cdf0602b5d388579c832378fcb67cd19d1eb62f2859173f81244023c31`.
- Linux Debug: `92f9ab88df0948cbc9634763f131c7816bd0a7893dc520b95cfa5af87fbea136`.
- Web WASM: `99040c49315be1a585835f3fc4e536cbfcca806aeffc80f8a1d5f4519cd20153`.
- Android Debug APK: `108b45770b32aa0c552b8a0048cdfa994b7d336dc16813fd425ea0c37854341b`.

## Remaining limits

- Complete Android FF ON/OFF with a reachable third participant remains pending.
- The pre-integration base has an independent null-renderer/high-resolution-HUD
  crash: postgame icons call `hud_hires_png_texture` and `glGenTextures` without
  a GL context. The fixture disables HUD/text rendering; no engine workaround
  is included here. Its reproduction on current upstream remains separate.
- Android menu-driven persistence, Android Internet traversal, original-mode
  Android scenarios and broader device/GPU coverage were not executed.
- Web FF and human survival, Linux full-engine 10/16-player soaks and
  physical-GPU multiplayer were not repeated during this contribution phase.
- The historical Ammo999 report is closed by the project owner. It is not
  reopened or presented as a newly fixed issue in this contribution.
- Maintainer review and canonical CI remain necessary. A Draft PR is not a
  release-readiness claim.
