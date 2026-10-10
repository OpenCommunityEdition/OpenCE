/*
WEB_LIBRARY.JS

The JavaScript half of the web runtime (tools/web_build.py --js-library).
The game runs on a Web Worker (PROXY_TO_PTHREAD); these functions run on
the page's thread, where Emscripten proxies them, and hand what they are
given to the page (port/web/site/app.js, Module.haloMessage).
*/

addToLibrary({
  // kind: 0 a status, 1 a notice, 3 a fatal error (the game stops)
  web_js_post__proxy: 'sync',
  web_js_post: (kind, text) => {
    var message = UTF8ToString(text);
    if (Module.haloMessage) Module.haloMessage(kind, message);
    else if (kind == 3) console.error(message);
    else console.log(message);
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
});
