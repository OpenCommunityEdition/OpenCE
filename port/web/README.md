# The web build

The web build runs the game in a web browser, as WebAssembly. It is the
same game and platform layer as the Linux build, with the OpenGL ES renderer
of the Android build drawing through WebGL 2. Play it at
<https://halocombatevolved.com/>: every build of `main` that passes on all
platforms is published there (and as `halo-web-release.zip` in the release,
for your own server).

The site serves no game data. The page copies the `maps/` folder out of your
own Xbox disc image of Halo: Combat Evolved into the browser's storage for
the site. Nothing is uploaded.

## What it needs

- A desktop browser: Chrome or Edge, or Firefox. Safari 17 or later may
  work. The page checks what it needs and says what is missing.
- A 64-bit browser that gives the page 2.1 GB of memory (the Xbox's memory
  window is at its top: "How it operates"). Desktop browsers do; a phone's
  may not (the page says so), and iPhones are untried.
- About 3 GB of the browser's storage: the maps (about 2 GB), and the
  game's copies of the maps it played (up to about 800 MB, as on the Xbox's
  hard disk), and the disc image (`.iso` or `.xiso`) on the computer.
- The site over https (or `localhost`): the game's threads need
  `SharedArrayBuffer`, and so a cross-origin isolated page. GitHub Pages
  cannot send the headers for that; the site's service worker adds them, so
  the page loads once more on the first visit.

## Game data

1. Open the site. The **This browser** panel lists the checks.
2. Under **Game data**, choose **Choose a disc image…** and select the disc
   image. The page reads it in place and copies `maps/` into the browser's
   storage (about a minute for 2 GB). **Choose a maps folder…** takes a
   `maps/` folder that an earlier extraction made (a desktop build's).
3. Choose **Play**.

Every map is checked as the game checks it before it is copied (its `head`
and `foot` signatures, the Xbox cache version and the build of a retail disc,
`01.10.12.2276`, or of the decompilation, `01.01.14.2342`). A disc image is
untrusted input: the reader checks every offset and size in it.

The page asks the browser to keep the site's data. A browser that is short
of space may delete it otherwise; **Settings and data** says which. The
storage belongs to the site's address: `halocombatevolved.com` and the
repository's `github.io` address do not share it.

## Playing

The controls are the desktop builds' (refer to "Controls" in
[port/linux/README.md](../linux/README.md)), with these differences:

- Crouch is **C** only: a page cannot keep Ctrl+W (close the tab), Ctrl+S
  or Ctrl+D from the browser.
- A click on the game takes the mouse for aiming; **Escape** gives it back
  (the browser's rule). In the menus the mouse moves the menus' pointer.
- **Tab** stays with the game (the scoreboard, switching weapons).
- The **Fullscreen** button (top right) takes the page fullscreen. F11 is
  the browser's own.
- A controller works once one of its buttons is pressed: browsers show a
  page a controller only then.
- The sound starts at the first click or key press (browsers allow sound only
  then).
- On a touchscreen, the Android app's on-screen controls (a stick, the
  controller's buttons named by the profile's mapping, a swipe to turn) show
  in a game while no controller is connected (`input.touch_controls`:
  `auto`, `on`, `off`); the menus take taps. The phone vibrates with the
  controller's rumble where the browser lets it. There is no layout editor
  nor gyroscope aiming yet.

The game keeps `config.toml`, the saved games and profiles (`save/`) and
`debug.txt` in the browser's storage beside `maps/`. **Settings and data**
exports the saved games, the profiles and `config.toml` as a zip (not the
game's copies of the maps), downloads the log, and deletes the game
data. The game's own Settings screens have the settings that apply in a
browser.

### Playing together

**Over the internet**, as the Linux, Windows and Android builds play, and
with them: Multiplayer, Create Game > Internet hosts a game (PUBLIC lists it
in everyone's server browser), and Join Game > Server Browser lists the
public games, the native builds' and the browsers' alike. A game hosted here
shows its invite link at the top of the page, with a button that copies it:
the site's address with `?join=<code>`, which opens the site and joins the
game, and which a native build takes too (pasted, or on its command line);
the native builds' own `halo://join/` links work pasted into the address
after `?join=`. Network Setup's INTERNET PLAY turns it off.

The browser reaches each other machine of the game directly, over WebRTC
(its router permitting, as for the native builds: "Internet play" in
[port/linux/README.md](../linux/README.md)); a native build takes the
browser's WebRTC on its tunnel's port (port/linux/src/p2p_webrtc.c). They
find each other through the same public MQTT brokers, here over secure
WebSockets (EMQX's, HiveMQ's and Mosquitto's: the native builds' first broker
has none), with everything sealed as between native builds.

**In a room**: under **Play together**, **Create a room** makes a room and
its link (`https://halocombatevolved.com/?room=<secret>`): send it to your
friends.
Everyone who opens it is in the room, and the browsers of a room are as the
machines of one local network to the game: host a game under Multiplayer and
the others find it there, as system link games are found (up to 16
browsers). The link's secret is the room: anyone with it can come in.

The browsers connect to each other directly (WebRTC, through each one's
router with the help of the STUN servers internet play uses). To find each
other they post to a topic of the room on public MQTT brokers (EMQX's,
HiveMQ's and Mosquitto's, over secure WebSockets): the topic is named by a
hash of the secret and every message is sealed with a key derived from it,
so the brokers see nothing of the room. A network that lets no WebRTC
through (some company and school networks) keeps a browser out of rooms.

### Address parameters

For testing, the page's address takes:

| Parameter | Effect |
| --- | --- |
| `?room=<secret>` | Joins that room (32 hex digits); `?brokers=ws://host:port[,...]` uses other MQTT brokers (WebSocket listeners), for rooms and internet play, as the tests do. |
| `?join=<code>` | Joins that internet game once the game starts (an invite's 64 hex digits, as a `halo://join/` link ends). |
| `?set=NAME=value` | Sets a setting's environment variable (the settings table in [port/linux/README.md](../linux/README.md)); repeat it for more. `?set=HALO_GL_DEBUG=1` reports WebGL errors in the log. |
| `?menu=<screen>` | Opens a menu screen (`debug.menu_open`). |
| `?map=<level>` | Starts a level: `a10`, or `levels\test\bloodgulch\bloodgulch`. The page writes `init.txt` for it, and removes it at the next visit without `?map=`. |
| `?play` | Starts the game at once (when the game data is there). |

## Updates

`version.json` names each build (a hash of the site's files, the build
number, the commit and the network version). The service worker keeps the
files of one build together, so that the site starts without the network.
When a new build is out, the page says so; **Update** downloads it whole and
reloads.

## Limits

- No system link with the native builds on a local network (a browser has
  no UDP): they play over the internet, as invites and the server browser
  find them ("Playing together"). Split screen works.
- Internet play has no relay, as on the native builds: two machines whose
  networks let no direct connection through (some company and school
  networks block WebRTC) cannot play together.
- No Custom Edition maps (their tag data's address is in the C heap there).
- No Bink videos (as on the other ports).
- The browser's WebGL 2 is OpenGL ES 3.0 without its 3.1 and 3.2 additions:
  the visibility tests (lens flares) report whether anything was drawn, a
  frame late; supersampling and SMAA are not offered (FXAA and MSAA are).
- Performance depends on the browser's graphics: Chrome and Edge are the
  fastest. A software renderer (no graphics acceleration) is very slow.

## Building

Install Emscripten 6.0.10 with [emsdk](https://emscripten.org/docs/getting_started/downloads.html):

```sh
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
~/emsdk/emsdk install 6.0.10
~/emsdk/emsdk activate 6.0.10
source ~/emsdk/emsdk_env.sh
```

Then, in the repository:

```sh
python configure.py            # finds emcc on the PATH, else ~/emsdk's (--web-emcc PATH)
ninja web                      # build/web/site/
python tools/web_serve.py      # http://localhost:8000/
```

The first build fetches SDL 3.4.16's source and builds it as an Emscripten
port (`port/web/halo_sdl3.py`, its hash checked), once. `configure.py
--release` makes the release build (no assertions checked); the debug build
stops at the first failed assertion, which the page shows with the log.

`tools/web_serve.py` serves the site with the isolation headers, byte ranges
and no caching; `--as-github-pages` leaves the headers out, so that the
service worker isolates the page as on GitHub Pages; a folder argument
serves another site (a `halo-web-*.zip` from a release, say).
`python tools/ci_build.py web release` makes the site in
`dist/halo-web-release/`, as CI does.

### Tests

```sh
node --test "port/web/tests/*.test.js"   # the disc image reader, the service worker
python -m pytest -q tools/test_web_build.py  # the build's graph, version.json, the sockets, the menus
node port/web/tests/smoke.mjs dist/halo-web-release  # the site in a headless Chromium (Playwright)
node port/web/tests/rooms.mjs dist/halo-web-release  # two browsers in a room (and mosquitto)
node port/web/tests/webrtc_native.mjs chromium firefox  # the native builds' WebRTC with browsers
node port/web/tests/internet.mjs build/web/site <maps folder>  # internet play with the Linux build
```

The smoke test serves the site as GitHub Pages does, checks that the page
isolates itself and passes its checks, and starts the game with an empty
maps folder, which gets as far as looking for the menus' map. The rooms test
starts a local broker and two browsers that join one room and connect. A
game between two browsers needs the maps: two pages of the same build with
the multiplayer maps imported, one opened with
`?room=<secret>&set=HALO_NETWORK_TEST=host:bloodgulch` and the other with
`?room=<secret>&set=HALO_NETWORK_TEST=join` (`debug.network_test`,
port/linux/NETCODE.md), play one and log every player's state each second.

The WebRTC test builds `port/linux/src/p2p_webrtc.c` (with `posix_dtls.c`
and Mbed TLS) for this computer around one connection, which a browser's
data channel opens as signalling would have it; the browser sends it
messages of every size, which come back, then again with a tenth of the
datagrams lost each way. The internet play test needs the maps (`ui.map`
and the multiplayer maps, which each browser imports) and the Linux build
(`build/linux/halo`), and plays three games through a local broker: two
browsers by invite, the native build hosting with a browser joining from the
server browser, and the reverse (`debug.network_test` `browse`,
`debug.network_test_public`).

Playwright's screenshots of the page in headless Firefox show the game's
canvas black (it is drawn by a worker); the game draws. Check Firefox headed,
on a virtual display.

## How it operates

The game is compiled for `wasm32`, an ILP32 target as the Xbox was (32-bit
`int`, `long` and pointers), with the ABI the other builds reproduce
(16-bit `wchar_t`, MSVC's extensions, common symbols, no fused
multiply-adds). `tools/web_build.py` writes the graph.

- **Code paths.** `HALO_WEB` and `HALO_GLES`: the OpenGL ES renderer, and
  what a browser does differently. Not `HALO_ANDROID`, which is the Android
  app (its touch overlay, its window, its files): in a browser, the menus,
  the mouse and the keyboard are the desktop's.
- **Memory.** The game expects the Xbox's memory window at `0x80000000`
  (`port/linux/src/platform.h`), 128 MB as on Android. The WebAssembly
  memory is made `0x88000000` bytes, so that the window is its top 128 MB;
  `port/web/src/web_main.c` keeps the C heap below it. `xbox_memory.c`
  clears a block where it would map fresh pages, and only remembers
  protection.
- **Threads.** Emscripten's pthreads, with `PROXY_TO_PTHREAD`: the game's
  `main` runs on a Web Worker, where it may block; the page's own thread only
  passes events. The game's canvas is moved to that worker
  (`OFFSCREENCANVASES_TO_PTHREAD`).
- **Frames.** A worker's canvas shows what was drawn when the worker returns
  to its event loop, so `main.c` runs one iteration of the game's loop for
  each of the browser's frames (`emscripten_set_main_loop`) instead of its
  own `while`. A halt stops the game (`abort`); the page shows the log.
- **Entry.** `web_main.c` is the program's `main`: it turns the page's
  `--NAME=value` arguments into environment variables, mounts the site's
  Origin Private File System at `/data` (WasmFS), and starts the game
  (`shell_xbox.c`'s `main`, renamed `halo_game_main`).
- **Calls.** WebAssembly calls a function only as its own type, where x86
  shrugged. The link fails on a direct call that does not match
  (`-Wl,--fatal-warnings`); the declarations were corrected. There is no
  link-time optimisation: LLVM's would turn calls whose parameter types
  differ only in width, which WebAssembly passes alike, into traps.
- **WebGL 2.** What the Android renderer needed of ES 3.0 the browser has;
  the rest:
  - No texture swizzle: textures are put in RGBA order as they are
    converted (`xbox_textures.c`).
  - S3TC only for 2D textures whose sides are multiples of 4: the others
    are decoded.
  - A vertex stride of at most 255 bytes: the 256-byte immediate-mode
    vertex is drawn from an array for each attribute (`d3d8_gl.c`).
  - No feedback loops: a draw that samples the target it draws into samples
    a copy of it.
  - No `glCopyImageSubData`, `glDrawElementsBaseVertex` or
    `glMemoryBarrier`: the ES renderer's blits and rebased indices.
  - Buffer writes are copies the browser orders with the draws, so the
    streaming buffers never wait for the GPU (`port/web/src/web_gl_host.c`).
- **Memory watch.** WebAssembly cannot protect pages, so the textures'
  memory is watched by hashing it once a frame, as the Android host does
  under the x86 emulator (`port/web/src/web_memory_watch.c`,
  `port/android/host/host_watch_hash.c`). A page that has stayed the same
  for 30 checks is hashed every 4 frames, until the game locks it or a
  file is read into it.
- **Sockets.** `port/web/src/web_net.c` gives `posix.h`'s sockets inside
  the page: one machine with a loopback address and an address of its own
  network (`10.0.0.1`, or the room's); datagrams to either, or broadcast,
  reach the sockets bound to their port, and streams connect to the socket
  listening on theirs. Split screen (a network game over loopback) and the
  game's own start-up use them.
- **Internet play.** The native builds' (port/linux/src/p2p*.c), all of it:
  the invites, the signalling and its sealing, the server browser, the
  tunnel's sealed packets and KCP streams, the stand-ins the game's sockets
  reach peers through. Two things differ. A broker is a WebSocket URL, which
  `posix_resolve_ipv4` gives a stand-in address (198.19.0.x) and a stream
  socket connected to it is the page's WebSocket (`web_library.js`), the
  same MQTT bytes. And the tunnel to each peer is a WebRTC data channel
  (`port/web/src/web_p2p.c`, `site/p2p.js`): the page keeps a connection
  ready, whose ICE credentials, addresses (IPv4, and its own as mDNS names)
  and certificate's hash a request or an answer carries, and makes up the
  other end's description from what signalling told: a native build's is an
  ICE-lite agent and a DTLS server, whose credentials come from the
  session's secret; of two browsers, the host is the DTLS server. What the
  channel receives goes to the tunnel's socket from the connection's stand-in
  address (198.18.x.y).
- **Rooms.** The other browsers of a room have addresses of their own in
  `10.0.0.0/8`, which `net.js` chooses (and the game is started with:
  `HALO_WEB_ADDRESS`). What the game sends to one, or broadcasts, is a frame
  `net.js` carries over WebRTC: a datagram on a data channel that may lose it
  (unordered, not resent), a stream's opening, bytes and end on one that
  does not. What arrives, `web_net_receive` checks and hands to the sockets.
  System link and the netcode run unchanged on top, so the network version
  needs no change for it.
- **Left out.** The self-updater (the site updates itself), the disc image
  reader (the page's `xiso-worker.js` is), STUN, UPnP and Discord (a page
  reaches none: its WebRTC finds its own addresses), and the page-fault
  memory watch.
- **The page and the game.** The memory is shared, so the page reads and
  writes a few words of it itself (`port/web/src/web_shared.h`): the frames
  shown, whose count standing still while a map loads makes the page say
  so, and the touch controls' state, which `web_touch.c` hands the game as
  the Android host does (`touch_input.c`).
- **SDL.** SDL 3.4.16, the release the other builds use, built as an
  Emscripten port: windows, input, controllers and audio (an AudioWorklet).
  Its buffer is SDL's own size, not the desktop's 512 frames, which
  underran in browsers.

### Game source changes

Each is marked `port:` and guarded by `HALO_WEB` but where it corrects a
declaration:

- `main/main.c`: one iteration of the main loop per browser frame; a halt
  stops the game.
- `cache/cache_files_windows.c`: the cache thread starts at a function of
  `CreateThread`'s type.
- `cseries/errors.c`: `debug.txt`'s lines go to the page's log too.
- `rasterizer/xbox/rasterizer_xbox_text.c`, `rasterizer_xbox_motion_sensor.c`,
  `rasterizer_xbox_plasma_energy.c`, `networking/network_game_manager.c`,
  `objects/object_types.c`: declarations given their definitions' types.

## Hosting

`.github/workflows/build.yml` builds the site in the `web` job, puts both
builds in the release (`halo-web-release.zip`, `halo-web-debug.zip`), and
the `pages` job deploys the release build to GitHub Pages: only from `main`,
only once every platform built and the release is out, and never cancelled
halfway. The Discord post has a Web field and the site's address.

Once, an owner of the organization sets it up:

1. Organization settings → Pages → **Add a verified domain**:
   `halocombatevolved.com`. Add the TXT record GitHub shows
   (`_github-pages-challenge-OpenCommunityEdition.halocombatevolved.com`)
   at the DNS host (Cloudflare) and verify. This also frees the domain from
   another account's site that still claims it.
2. Repository settings → Pages → Build and deployment → Source: **GitHub
   Actions**.
3. Repository settings → Pages → Custom domain: `halocombatevolved.com`.
   The DNS records are there already (the four apex `A` records of GitHub
   Pages, `www` a `CNAME` to the apex), **DNS only** at Cloudflare, so that
   GitHub can issue its certificate; then tick **Enforce HTTPS**.
4. Nothing else: the deployment needs no secret, and the `github-pages`
   environment appears with the first one (give it no required reviewer).

Later, Cloudflare can serve the site (proxied, SSL *Full (strict)*) with a
response header rule adding `Cross-Origin-Opener-Policy: same-origin`,
`Cross-Origin-Embedder-Policy: require-corp` and
`Cross-Origin-Resource-Policy: same-origin`: the first visit then needs no
reload, and Cloudflare's cache takes GitHub Pages' bandwidth (100 GB a month;
a first visit downloads about 10 MB, compressed).

## Files

| File | What it is |
| --- | --- |
| `tools/web_build.py` | The `ninja web` graph |
| `port/web/halo_sdl3.py` | SDL 3.4.16 as an Emscripten port |
| `port/web/src/web_main.c` | The entry point: the page's arguments, the storage, the heap's limit |
| `port/web/src/web_net.c` | The sockets inside the page, the room's other browsers, and the brokers' WebSockets |
| `port/web/src/web_p2p.c` | Internet play's WebRTC connections |
| `port/web/src/web_memory_watch.c` | The memory watch by hashing |
| `port/web/src/web_gl_host.c` | The renderer's services for WebGL 2 |
| `port/web/src/web_stubs.c` | UPnP and an MSVC intrinsic |
| `port/web/src/web_shared.h`, `web_touch.c` | The words of memory the page reads and writes: frames shown, the touch controls' state |
| `port/web/src/web_library.js`, `web_pre.js` | The JavaScript halves: messages to the page, the brokers' WebSockets, the workers' errors |
| `port/web/site/` | The page (`index.html`, `app.js`, `style.css`), the touch controls (`touch.js`), rooms (`net.js`), internet play's WebRTC (`p2p.js`), its service worker (`sw.js`), the importer (`xiso-worker.js`), the web app's manifest and icons |
| `port/web/stamp_version.py`, `licenses.py` | `version.json` and `licenses.txt` |
| `port/web/tests/` | The tests above |
| `tools/web_serve.py` | A local server for the site |
