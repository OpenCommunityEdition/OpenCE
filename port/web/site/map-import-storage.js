/* Browser-owned import/download transactions. OPFS directories cannot be
 * atomically renamed across the supported browsers. An owned journal hides a
 * fresh MCC directory until commit; CE imports require the exclusive game
 * guard and are recovered before any later game starts. Existing data is never
 * overwritten. Native MCC staging does not use this implementation. */
'use strict';
(function (root) {
  const GUARD = 'opence-game-data', MUTATION = 'opence-mcc-mutation';
  const MCC_JOURNALS = 'mcc_transactions', CE_JOURNALS = 'ce_import_transactions';
  const JOURNAL_LIMIT = 1024 * 1024, CHUNK = 1024 * 1024;
  let transaction = null, gameRelease = null;

  function plainPath(value) {
    if (typeof value !== 'string' || !value.length || value.length >= 128) throw new Error('Invalid transaction path.');
    const parts = value.split('/');
    if (parts.length > 8 || parts.some(p => !p || /^[. ]|[. ]$/.test(p) ||
        /[^\x20-\x7e]|[\\:*?"<>|]/.test(p) || /^(con|prn|aux|nul|conin\$|conout\$|com[0-9]|lpt[0-9])(?:[ .]|$)/i.test(p))) {
      throw new Error('Unsafe transaction path.');
    }
    return parts;
  }

  async function lock(name, mode = 'exclusive') {
    if (!navigator.locks?.request) throw new Error('This browser cannot safely coordinate map storage between tabs.');
    let acquired, failed;
    const ready = new Promise((resolve, reject) => { acquired = resolve; failed = reject; });
    const request = navigator.locks.request(name, { mode, ifAvailable: true }, async held => {
      if (!held) { failed(new Error('Map storage is busy in another tab. Quit that game or finish its download first.')); return; }
      let release;
      const done = new Promise(resolve => { release = resolve; });
      acquired(() => { release(); return request; });
      await done;
    }).catch(failed);
    return ready;
  }

  async function directory(parent, parts, create = false) {
    for (const part of parts) parent = await parent.getDirectoryHandle(part, { create });
    return parent;
  }

  async function absent(parent, name) {
    try { await parent.getFileHandle(name); return false; }
    catch (e) { if (e.name === 'TypeMismatchError') return false; if (e.name !== 'NotFoundError') throw e; }
    try { await parent.getDirectoryHandle(name); return false; }
    catch (e) { if (e.name === 'TypeMismatchError') return false; if (e.name !== 'NotFoundError') throw e; }
    return true;
  }

  async function removeEntry(parent, name) {
    try { await parent.removeEntry(name); }
    catch (e) { if (e.name !== 'NotFoundError') throw e; }
  }

  async function readText(parent, name, limit) {
    const file = await (await parent.getFileHandle(name)).getFile();
    if (file.size > limit) throw new Error('Oversized transaction record.');
    return new TextDecoder('utf-8', { fatal: true }).decode(await file.arrayBuffer());
  }

  // This runs only in a Worker: both Firefox and Chromium support sync handles.
  async function write(parent, name, file, cancelled = () => false, progress = () => {}) {
    if (!await absent(parent, name)) throw new Error('A destination already exists: ' + name);
    const handle = await parent.getFileHandle(name, { create: true });
    const access = await handle.createSyncAccessHandle();
    try {
      let position = 0;
      while (position < file.size) {
        if (cancelled()) throw new Error('Import cancelled.');
        const count = Math.min(CHUNK, file.size - position);
        const data = new Uint8Array(await file.slice(position, position + count).arrayBuffer());
        if (data.length !== count) throw new Error('The selected file could not be read.');
        let done = 0;
        while (done < count) {
          const n = access.write(data.subarray(done), { at: position + done });
          if (!Number.isInteger(n) || n <= 0 || n > count - done) throw new Error('The browser could not finish writing the file.');
          done += n;
        }
        position += count; progress(count);
      }
      access.flush();
    } finally { access.close(); }
  }

  async function writeText(parent, name, text) { await write(parent, name, new Blob([text])); }

  function validate(journal, download = false, ce = false) {
    if (!journal || journal.version !== 1 || !Array.isArray(journal.files) || !Array.isArray(journal.companions) ||
        !journal.files.length || journal.files.length > (download ? 168 : 4096) || journal.companions.length > 2) throw new Error('Invalid transaction journal.');
    const folder = journal.folder;
    if (ce ? folder !== '' : typeof folder !== 'string' ||
        !(download ? /^dl_[0-9a-f]{20}$/ : /^[a-z0-9][a-z0-9 _-]{0,39}$/).test(folder)) throw new Error('Invalid transaction folder.');
    if (!ce) plainPath(folder);
    const paths = new Set(), folded = new Set(); let total = 0;
    for (const entry of journal.files) {
      const parts = plainPath(entry.path);
      if (ce && (parts.length !== 1 || !entry.path.endsWith('.map'))) throw new Error('Invalid CE import path.');
      if (!Number.isSafeInteger(entry.size) || entry.size < 0 || entry.size > 0x7fffffff) throw new Error('Invalid transaction size.');
      if (folded.has(entry.path.toLowerCase()) || ['download.receipt', '.complete'].includes(entry.path.toLowerCase())) throw new Error('Conflicting transaction file.');
      paths.add(entry.path); folded.add(entry.path.toLowerCase()); total += entry.size;
      if (total > (download ? 1024 ** 3 : 16 * 1024 ** 3)) throw new Error('Transaction exceeds the size limit.');
    }
    for (const path of journal.companions) {
      if (!['workshop_id.txt', 'download.receipt'].includes(path) || folded.has(path.toLowerCase())) throw new Error('Conflicting generated companion.');
      paths.add(path); folded.add(path.toLowerCase());
    }
    const directories = new Map();
    for (const path of paths) {
      const parts = path.split('/');
      while (parts.length > 1) {
        parts.pop();
        const prefix = parts.join('/'), key = prefix.toLowerCase();
        if (folded.has(key)) throw new Error('File/directory collision.');
        if (directories.has(key) && directories.get(key) !== prefix) throw new Error('Conflicting directory case.');
        directories.set(key, prefix);
      }
    }
    return { paths, total };
  }

  async function space() {
    if (!navigator.storage?.estimate) return { known: false, available: 0 };
    const { quota, usage } = await navigator.storage.estimate();
    if (!Number.isFinite(quota) || !Number.isFinite(usage) || quota < 0 || usage < 0) return { known: false, available: 0 };
    return { known: true, available: Math.max(0, Math.floor(quota - usage)) };
  }

  function commitText(folder) { return 'MCC-COMMIT-1\n' + folder + '\n'; }

  async function begin(payload, manual = false, ce = false) {
    if (transaction) throw new Error('Another transaction is active.');
    const journal = { version: 1, folder: payload.folder, files: payload.files, companions: payload.companions || [] };
    const { total } = validate(journal, !manual, ce);
    const release = await lock(MUTATION);
    let pending = null;
    try {
      const rootDir = await navigator.storage.getDirectory();
      const journalDir = await rootDir.getDirectoryHandle(ce ? CE_JOURNALS : MCC_JOURNALS, { create: true });
      const parent = await rootDir.getDirectoryHandle(ce ? 'custom_maps' : 'mcc_maps', { create: true });
      const key = ce ? crypto.randomUUID().replace(/-/g, '') : journal.folder;
      if (!await absent(journalDir, key + '.json')) throw new Error('This package has an unfinished storage transaction.');
      if (ce) {
        for (const entry of journal.files) if (!await absent(parent, entry.path)) throw new Error('Already installed; existing files are kept: ' + entry.path);
      } else if (!await absent(parent, journal.folder)) throw new Error('That package folder already exists. Choose a different name.');
      // A receipt with no package folder (checked above) and no journal was
      // left by a removal cut off before removeEnd. Download folders are named
      // after the package's contents, so it would block that package for good.
      if (!ce && !await absent(journalDir, key + '.commit')) await removeEntry(journalDir, key + '.commit');
      const estimate = await space();
      if (estimate.known && estimate.available < total + JOURNAL_LIMIT) throw new Error('Not enough browser storage for this import.');
      const text = JSON.stringify(journal);
      if (text.length > JOURNAL_LIMIT) throw new Error('Transaction record exceeds the limit.');
      pending = { journal, journalDir, parent, key, ce, release, folder: null };
      await writeText(journalDir, key + '.json', text);
      pending.folder = ce ? parent : await parent.getDirectoryHandle(journal.folder, { create: true });
      transaction = pending;
      return { ok: true };
    } catch (error) {
      // If creating the journal failed, no package file has been touched.
      if (pending && !pending.folder) await removeEntry(pending.journalDir, pending.key + '.json').catch(() => {});
      await release(); throw error;
    }
  }

  async function cleanupOwned(item) {
    const { paths } = validate(item.journal, false, item.ce);
    let folder;
    try { folder = item.ce ? item.parent : await item.parent.getDirectoryHandle(item.journal.folder); }
    catch (e) { if (e.name === 'NotFoundError') return; throw e; }
    const dirs = new Set();
    for (const path of paths) {
      const parts = path.split('/');
      while (parts.length > 1) { parts.pop(); dirs.add(parts.join('/')); }
    }
    if (!item.ce) {
      let count = 0;
      async function inspect(dir, prefix = '', depth = 0) {
        if (depth > 8) throw new Error('Unexpected directory in unfinished package.');
        for await (const [name, handle] of dir.entries()) {
          const path = prefix + name;
          if (++count > 8192 || (handle.kind === 'directory' ? !dirs.has(path) : !paths.has(path))) throw new Error('Unrecognized content retained in unfinished package.');
          if (handle.kind === 'directory') await inspect(handle, path + '/', depth + 1);
        }
      }
      await inspect(folder);
    }
    for (const path of paths) {
      const parts = path.split('/'), leaf = parts.pop();
      let dir;
      try { dir = await directory(folder, parts); }
      catch (e) { if (e.name === 'NotFoundError') continue; throw e; }
      await removeEntry(dir, leaf);
    }
    for (const path of [...dirs].sort((a, b) => b.split('/').length - a.split('/').length)) {
      const parts = path.split('/'), leaf = parts.pop();
      try { await removeEntry(await directory(folder, parts), leaf); }
      catch (e) { if (e.name !== 'NotFoundError') throw e; }
    }
    if (!item.ce) await removeEntry(item.parent, item.journal.folder);
  }

  async function finish(payload, commit) {
    const item = transaction;
    if (!item || payload.folder !== item.journal.folder) throw new Error('No matching storage transaction.');
    let complete = false;
    try {
      if (commit && !item.ce) await writeText(item.journalDir, item.key + '.commit', commitText(item.journal.folder));
      // Invalidate publication before cancellation can remove package files.
      if (!commit && !item.ce) await removeEntry(item.journalDir, item.key + '.commit');
      if (!commit && !payload.clean) await cleanupOwned(item);
      await removeEntry(item.journalDir, item.key + '.json');
      complete = true;
      return { ok: true };
    } catch (error) {
      if (commit && !item.ce) {
        try { await removeEntry(item.journalDir, item.key + '.commit'); }
        catch {
          // The engine must not delete files while a commit may still name
          // them. Fence this worker; reopening will inspect the durable state.
          fenced = true; complete = true;
          return { uncertain: true };
        }
      }
      throw error;
    } finally {
      // A failed commit still belongs to the caller, which must roll back.
      // Failed rollback retains its journal for recovery but releases the tab.
      if (complete || !commit) { transaction = null; await item.release(); }
    }
  }

  // Caller holds the mutation lease. No arbitrary recursive deletion: recovery
  // validates a complete bounded record, then removes only its listed paths.
  async function recover() {
    const rootDir = await navigator.storage.getDirectory();
    for (const ce of [false, true]) {
      let journalDir;
      try { journalDir = await rootDir.getDirectoryHandle(ce ? CE_JOURNALS : MCC_JOURNALS); }
      catch (e) { if (e.name === 'NotFoundError') continue; throw e; }
      let count = 0, pendingCount = 0;
      for await (const [name, handle] of journalDir.entries()) {
        if (++count > 4096) throw new Error('Too many storage transaction records.');
        if (handle.kind !== 'file' || !name.endsWith('.json')) continue;
        if (++pendingCount > 64) throw new Error('Too many unfinished storage transactions.');
        const key = name.slice(0, -5);
        if (ce ? !/^[0-9a-f]{32}$/.test(key) : !/^[a-z0-9][a-z0-9 _-]{0,39}$/.test(key)) throw new Error('Unrecognized storage journal retained.');
        const journal = JSON.parse(await readText(journalDir, name, JOURNAL_LIMIT));
        validate(journal, false, ce);
        if (!ce && journal.folder !== key) throw new Error('Storage journal names disagree.');
        const parent = await rootDir.getDirectoryHandle(ce ? 'custom_maps' : 'mcc_maps', { create: true });
        let committed = false;
        if (!ce && !await absent(journalDir, key + '.commit')) {
          committed = await readText(journalDir, key + '.commit', 96) === commitText(key);
          if (!committed) throw new Error('Malformed commit record retained.');
        }
        if (!committed) await cleanupOwned({ journal, parent, ce });
        await removeEntry(journalDir, name);
      }
    }
  }

  async function startGame() {
    const release = await lock(MUTATION);
    try {
      await recover();
      if (gameRelease) throw new Error('This tab already owns a game.');
      gameRelease = await lock(GUARD, 'shared');
    } finally { await release(); }
  }

  async function visible(folder) {
    if (!/^dl_[0-9a-f]{20}$/.test(folder)) return { visible: true };
    try {
      const journals = await (await navigator.storage.getDirectory()).getDirectoryHandle(MCC_JOURNALS);
      return { visible: await absent(journals, folder + '.json') };
    } catch (e) { if (e.name === 'NotFoundError') return { visible: true }; throw e; }
  }

  let removal = null;
  async function dispatchLocal(action, payload = {}, ownerGame = false) {
    if (fenced) throw new Error('Close this tab and reopen it to recover storage.');
    if (action === 'space') return space();
    if (action === 'begin') return begin(payload);
    if (action === 'commit' || action === 'rollback') return finish(payload, action === 'commit');
    if (action === 'visible') return visible(payload.folder);
    if (action === 'removeBegin') {
      if (removal || transaction || !/^dl_[0-9a-f]{20}$/.test(payload.folder)) throw new Error('Invalid removal lease.');
      const release = await lock(MUTATION);
      try {
        const held = (await navigator.locks.query()).held.filter(l => l.name === GUARD);
        if (held.length > (gameRelease || ownerGame ? 1 : 0)) throw new Error('Quit other games before removing a package.');
        removal = { folder: payload.folder, release }; return { ok: true };
      } catch (e) { await release(); throw e; }
    }
    if (action === 'removeEnd') {
      if (!removal || removal.folder !== payload.folder) throw new Error('No matching removal lease.');
      try {
        const storage = await navigator.storage.getDirectory();
        const maps = await storage.getDirectoryHandle('mcc_maps');
        if (await absent(maps, payload.folder)) {
          try {
            const journals = await storage.getDirectoryHandle(MCC_JOURNALS);
            await removeEntry(journals, payload.folder + '.commit');
          } catch (e) { if (e.name !== 'NotFoundError') throw e; }
        }
        return { ok: true };
      } finally { await removal.release(); removal = null; }
    }
    throw new Error('Unknown map storage operation.');
  }

  let service = null, sequence = 0, fenced = false;
  const requests = new Map();
  function fence() {
    fenced = true;
    if (service) service.terminate();
    service = null;
    // Publication may have finished before the worker stopped replying. Never
    // turn an unknown commit into ordinary failure and delete its package.
    for (const request of requests.values()) request.resolve({ uncertain: true });
    requests.clear();
    return { ok: true };
  }
  function dispatch(action, payload = {}) {
    if (action === 'fence') return Promise.resolve(fence());
    if (fenced) return Promise.reject(new Error('Map storage stopped responding. Close this tab and reopen it to recover storage.'));
    if (typeof document === 'undefined') return dispatchLocal(action, payload);
    if (!service) {
      service = new Worker('map-storage-worker.js');
      service.onmessage = event => {
        const request = requests.get(event.data.id);
        if (!request) return;
        requests.delete(event.data.id);
        if (event.data.error) request.reject(new Error(event.data.error));
        else {
          request.resolve(event.data.result);
          if (event.data.result?.uncertain) fence();
        }
      };
      service.onerror = fence;
      service.onmessageerror = fence;
    }
    return new Promise((resolve, reject) => {
      const id = ++sequence;
      requests.set(id, { resolve, reject });
      service.postMessage({ id, action, payload, ownerGame: !!gameRelease });
    });
  }

  async function importPlan(plan, cancelled, progress) {
    const release = await lock(GUARD);
    try {
      const mutation = await lock(MUTATION);
      try { await recover(); } finally { await mutation(); }
      await begin({ folder: plan.folder, files: plan.entries.map(e => ({ path: e.path, size: e.size })), companions: [] }, true, plan.family === 'ce');
      try {
        for (const entry of plan.entries) {
          if (cancelled()) throw new Error('Import cancelled.');
          const parts = plainPath(entry.path), leaf = parts.pop();
          const dir = await directory(transaction.folder, parts, true);
          await write(dir, leaf, entry.file, cancelled, n => progress(entry.path, n));
        }
        if (cancelled()) throw new Error('Import cancelled.');
        const result = await finish({ folder: plan.folder }, true);
        if (result.uncertain) throw new Error('The import outcome is uncertain. Close this tab and reopen it to recover storage.');
      } catch (error) {
        if (transaction) try { await finish({ folder: plan.folder }, false); }
        catch (cleanup) { throw new Error(error.message + ' Recovery is required: ' + cleanup.message); }
        throw error;
      }
    } finally { await release(); }
  }

  function worker(planner) {
    let busy = false, cancelled = false;
    root.onmessage = async event => {
      if (event.data.type === 'cancel') { cancelled = true; return; }
      if (busy || event.data.type !== 'import') return;
      busy = true; cancelled = false;
      try {
        const plan = await planner(event.data.files, event.data.options);
        const total = plan.entries.reduce((sum, e) => sum + e.size, 0);
        let done = 0, last = 0;
        await importPlan(plan, () => cancelled, (file, n) => {
          done += n;
          if (Date.now() - last > 100 || done === total) { root.postMessage({ type: 'progress', file, done, total }); last = Date.now(); }
        });
        root.postMessage({ type: 'done', title: plan.title, folder: plan.folder, files: plan.entries.length, bytes: total });
      } catch (error) { root.postMessage({ type: 'error', message: String(error.message || error) }); }
      finally { busy = false; }
    };
  }

  const api = { dispatch, dispatchLocal, worker, startGame, lock, recover, importPlan, validate, plainPath, write, GUARD, MUTATION };
  root.HaloMapStorage = api;
  if (typeof module !== 'undefined') module.exports = api;
})(globalThis);
