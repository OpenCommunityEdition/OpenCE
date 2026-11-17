/* The page/game bridge delegates OPFS writes here: synchronous access handles
 * are available in a dedicated Worker, not the document's main thread. */
'use strict';
importScripts('map-import-storage.js');
self.onmessage = async event => {
  const { id, action, payload, ownerGame } = event.data;
  try {
    const result = await self.HaloMapStorage.dispatchLocal(action, payload, ownerGame === true);
    self.postMessage({ id, result });
  } catch (error) {
    self.postMessage({ id, error: String(error.message || error).slice(0, 320) });
  }
};
