# Browser invitations for native multiplayer

The source-built browser preview accepts the desktop game's `halo://join/<44 hexadecimal characters>` invitations. The equivalent shareable link uses HTTPS:

```
https://fqlx.github.io/halo-ce-universal/#join=<44 hexadecimal characters>
```

The launcher can paste native invites and copy browser links. The fragment is not included in HTTP requests or referrer headers. An invite grants access to its host's current session; it stops working after that host exits. A Discord channel URL or an expired Discord rich invitation is not itself a game token.

**Deployment status:** the source-built runtime replaces Apollo to address Safari startup. A public native relay must still be configured and actual gameplay verified before this URL can join desktop games. The default package leaves native joining unavailable. A generated link or a relay connection is not proof of joining a match.

## Why the relay is required

Browsers cannot use the desktop build's raw UDP sockets. `port/web/site/gateway.js` carries the browser's virtual game sockets over a secure WebSocket. A session-specific worker in `port/relay` runs the existing native MQTT, encrypted P2P and KCP networking. It joins only the invited host and forwards game traffic; it does not run game simulation or need the ISO/maps.

The browser receives the worker's native identity and peer address mapping before starting its network stack. Game discovery and the actual match continue through Halo's **Multiplayer → System Link** menu. The launcher enables Play once the invited host connects. A host being offline or unreachable is shown separately from a relay connection.

Browser-only WebRTC rooms are also available in the source port. They have a different protocol and room code; putting a native invite into a browser room does not join the native host.

## Build the browser

Use Emscripten 6.0.10, Python 3 and Ninja:

```sh
source build/emsdk/emsdk_env.sh
python3 configure.py --release
ninja web
python3 tools/build_multiplayer_pages.py --relay wss://YOUR-RELAY-HOST/join
```

The output is `dist/browser-multiplayer`. All paths are relative, so it can be served under a GitHub Pages project or a preview subdirectory. An empty `--relay` produces an explicitly unavailable desktop-join state. Configure only an operator-controlled relay URL; links cannot override it. Relay access keys are entered in the launcher and sent in the first WebSocket message, never embedded in the public site or invitation.

The source-built engine includes current native network version 4. The existing `tools/build_pages.py` still packages the previous hash-pinned Apollo engine. Its WebAssembly has no multiplayer sockets, so adding a URL argument to that runtime cannot enable multiplayer.

## Deploy the relay

See [tools/browser-relay/README.md](tools/browser-relay/README.md) for build/run configuration and required limits. The relay needs a Linux host with outbound MQTT/STUN and UDP connectivity, and WSS termination. GitHub Pages serves static files and cannot run this process. Public deployments require an access capability and exact allowed browser origins. Private/reserved peer destinations are rejected by the production worker.

Before enabling a production native relay, verify browser-to-native gameplay. Runtime-only updates can ship with the relay unset. Update both the game's static branch and the game subdirectory in the personal-site mirror, preserving the personal site's root files and CNAME.

## Validation

```sh
node --test tools/tests/native-invite.test.cjs
npm ci --prefix tools/browser-relay
npm test --prefix tools/browser-relay
make -C port/relay test
```

The native fixture exercises the real encrypted P2P path with a local broker and echo host. The frontend tests exercise framing, session permissions, worker lifetime and backpressure. These are transport tests, not a Halo match: launch a compatible native host, open a fresh invitation in the browser, discover it under System Link, join the lobby, and play together before marking crossplay verified.
