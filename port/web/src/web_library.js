/*
WEB_LIBRARY.JS

The JavaScript half of the web runtime (tools/web_build.py --js-library).
The game runs on a Web Worker (PROXY_TO_PTHREAD); these functions run on
the page's thread, where Emscripten proxies them, and hand what they are
given to the page (port/web/site/app.js, Module.haloMessage), or keep the
WebSockets of internet play's brokers.
*/

addToLibrary({
  // Emscripten's clock (clock_gettime, the game's GetTickCount and
  // QueryPerformanceCounter: port/linux/src/xbox_kernel.c) with this
  // thread's time origin read once: reading performance.timeOrigin each time
  // made a read of the clock take 1.7 times as long (in a worker in Chrome,
  // where performance.now() alone takes about 300 ns). (Runs on the calling
  // thread.)
  $webTimeOrigin: 0,
  emscripten_get_now__deps: ['$webTimeOrigin'],
  emscripten_get_now: () => (webTimeOrigin ||= performance.timeOrigin) + performance.now(),

  // kind: 0 a status, 1 a notice, 2 an invite link of a game hosted for the
  // internet (to offer), 3 a fatal error (the game stops), 4 the game quits
  // as the player asked (the page starts again: port_mods.c)
  web_js_post__proxy: 'sync',
  web_js_post: (kind, text) => {
    var message = UTF8ToString(text);
    if (Module.haloMessage) Module.haloMessage(kind, message);
    else if (kind == 3) console.error(message);
    else console.log(message);
  },

  // a question for the player (port/linux/src/sdl_platform.c's
  // platform_ask): the page's dialog, its buttons' labels one to a line; the
  // button chosen goes in the word the game waits on (web_shared.h's
  // _web_answer). (async: the game's thread waits on the word, its strings
  // kept until then)
  web_js_ask__proxy: 'async',
  web_js_ask__deps: ['web_state'],
  web_js_ask: (title, text, labels) => {
    var answer = (index) => Atomics.store(HEAP32, (_web_state() >> 2) + 49, index);
    var buttons = UTF8ToString(labels).split('\n');
    if (Module.haloAsk) Module.haloAsk(UTF8ToString(title), UTF8ToString(text), buttons, answer);
    else answer(0);
  },

  // a frame for another browser of the room (port/web/src/web_net.c; the
  // page's net.js carries it): the C side's copy, freed here once taken.
  // (async: the game's thread does not wait for the page)
  web_js_net_send__proxy: 'async',
  web_js_net_send__deps: ['free'],
  web_js_net_send: (address, reliable, frame, size) => {
    var bytes = HEAPU8.slice(frame, frame + size);
    _free(frame);
    if (Module.haloNetSend) Module.haloNetSend(address >>> 0, reliable, bytes);
  },

  // internet play's brokers (port/linux/src/p2p_signal.c), MQTT over a
  // WebSocket for each socket web_net.c connects to one's URL; their events
  // go back by the socket's index and its connection's number
  $webSockets: {},
  web_js_websocket_open__proxy: 'async',
  web_js_websocket_open__deps: ['$webSockets', 'malloc', 'free'],
  web_js_websocket_open: (index, connection, url) => {
    var socket;
    var closed = () => {
      if (webSockets[index] && webSockets[index].socket === socket) delete webSockets[index];
      _web_net_websocket_closed(index, connection);
    };
    try {
      socket = new WebSocket(UTF8ToString(url), ['mqtt']);
    } catch (error) {
      _web_net_websocket_closed(index, connection);
      return;
    }
    socket.binaryType = 'arraybuffer';
    webSockets[index] = { socket, connection };
    socket.onopen = () => _web_net_websocket_opened(index, connection);
    socket.onmessage = (event) => {
      if (!(event.data instanceof ArrayBuffer) || !event.data.byteLength) return;
      var bytes = new Uint8Array(event.data);
      var copy = _malloc(bytes.length);
      if (!copy) return;
      HEAPU8.set(bytes, copy);
      _web_net_websocket_data(index, connection, copy, bytes.length);
      _free(copy);
    };
    socket.onclose = closed;
    socket.onerror = closed;
  },
  web_js_websocket_send__proxy: 'async',
  web_js_websocket_send__deps: ['$webSockets', 'free'],
  web_js_websocket_send: (index, connection, bytes, size) => {
    var copy = HEAPU8.slice(bytes, bytes + size);
    var entry = webSockets[index];
    _free(bytes);
    if (entry && entry.connection === connection && entry.socket.readyState === 1) entry.socket.send(copy);
  },
  web_js_websocket_close__proxy: 'async',
  web_js_websocket_close__deps: ['$webSockets'],
  web_js_websocket_close: (index, connection) => {
    var entry = webSockets[index];
    if (!entry || entry.connection !== connection) return;
    delete webSockets[index];
    entry.socket.onclose = entry.socket.onerror = entry.socket.onmessage = null;
    entry.socket.close();
  },

  // the server browser's master servers (port/linux/src/master_list.c): a
  // GET of the url (the C side's copy, freed here once read), its answer
  // written into the C side's slot (struct web_fetch_slot: id, done, status,
  // body, size, Signature, Signature-Key), which its thread waits on. The
  // slot is claimed for the answer first, its id changed to -id
  // (compareExchange): an answer that comes after the thread gave up on it
  // (its id 0, or another request's) is dropped, and the thread, which gives
  // up the same way, never takes an answer meant for an earlier request. 1 MB
  // at most, 10 seconds at most. (async: the game's thread does not wait for
  // the page)
  web_js_fetch__proxy: 'async',
  web_js_fetch__deps: ['malloc', 'free'],
  web_js_fetch: (url, slot, id) => {
    var address = UTF8ToString(url);
    _free(url);
    var finish = (status, bytes, signature, key) => {
      if (Atomics.compareExchange(HEAP32, slot >> 2, id, -id) !== id) return;
      var body = 0;
      if (bytes && bytes.length <= 1048576) {
        body = _malloc(bytes.length + 1);
        if (body) {
          HEAPU8.set(bytes, body);
          HEAPU8[body + bytes.length] = 0;
        }
      }
      HEAP32[(slot >> 2) + 2] = body ? status : 0;
      HEAP32[(slot >> 2) + 3] = body;
      HEAP32[(slot >> 2) + 4] = body ? bytes.length : 0;
      stringToUTF8(signature || '', slot + 20, 128);
      stringToUTF8(key || '', slot + 148, 32);
      Atomics.store(HEAP32, (slot >> 2) + 1, 1);
    };
    var controller = new AbortController();
    var timer = setTimeout(() => controller.abort(), 10000);
    fetch(address, { signal: controller.signal, cache: 'no-store' })
      .then((response) => response.arrayBuffer().then((buffer) => {
        clearTimeout(timer);
        finish(response.status, new Uint8Array(buffer), response.headers.get('Signature'),
          response.headers.get('Signature-Key'));
      }))
      .catch(() => finish(0, null, '', ''));
  },

  // internet play's WebRTC connections (port/web/src/web_p2p.c), which the
  // page makes (port/web/site/p2p.js, Module.haloP2P): a command, the
  // connection, and a copy of its text or packet, freed here
  web_js_p2p__proxy: 'async',
  web_js_p2p__deps: ['free'],
  web_js_p2p: (command, connection, data, size) => {
    var bytes = data ? HEAPU8.slice(data, data + size) : null;
    if (data) _free(data);
    if (Module.haloP2P) Module.haloP2P(command, connection, bytes);
  },
});
