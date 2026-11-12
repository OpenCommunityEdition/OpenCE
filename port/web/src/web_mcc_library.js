/* MCC-only bridge. The page owns Web Locks, quota and transaction journals. */
addToLibrary({
  web_js_mcc_storage__proxy: 'async',
  web_js_mcc_storage: (actionPtr, payloadPtr, mailbox, generation) => {
    const index = mailbox >>> 2;
    if (Atomics.load(HEAP32, index) !== generation) return;
    const action = UTF8ToString(actionPtr);
    const payload = UTF8ToString(payloadPtr);
    const finish = (result, available = 0) => {
      if (Atomics.load(HEAP32, index) !== generation) return;
      Atomics.store(HEAP32, index + 2, available >>> 0);
      Atomics.store(HEAP32, index + 3, Math.floor(available / 4294967296));
      Atomics.store(HEAP32, index + 1, result);
      Atomics.notify(HEAP32, index + 1);
    };
    Promise.resolve().then(() => {
      if (Atomics.load(HEAP32, index) !== generation) return false;
      if (typeof Module.haloMccStorage !== 'function') return false;
      return Module.haloMccStorage(action, JSON.parse(payload));
    }).then(result => {
      if (result && result.uncertain === true) finish(-2);
      else if (action === 'space') {
        const available = result && result.available;
        finish(result && result.known && Number.isSafeInteger(available) && available >= 0 ? 1 : 0,
          Number.isSafeInteger(available) && available >= 0 ? available : 0);
      } else finish(result && (action === 'visible' ? result.visible === true : result.ok === true) ? 1 : 0);
    }).catch(() => finish(0));
  },
  web_js_mcc_storage_fence__proxy: 'async',
  web_js_mcc_storage_fence: () => {
    if (typeof Module.haloMccStorage === 'function')
      Promise.resolve(Module.haloMccStorage('fence', {})).catch(() => {});
  },
});
