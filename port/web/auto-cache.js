"use strict";

// Optional static-site importer. Completed maps survive retries and reloads.
window.haloAutoCache = async function (api) {
    const panel = document.getElementById("download-panel");
    const title = document.getElementById("download-title");
    const percent = document.getElementById("download-percent");
    const progress = document.getElementById("download-progress");
    const detail = document.getElementById("download-detail");
    let fraction = 0;
    const show = (state, heading, description, value = null) => {
        panel.hidden = false;
        panel.dataset.state = state;
        title.textContent = heading;
        detail.textContent = description;
        if (value === null) {
            progress.removeAttribute("value");
            percent.textContent = "…";
        } else {
            fraction = Math.max(0, Math.min(1, value));
            progress.value = fraction;
            percent.textContent = `${Math.floor(fraction * 100)}%`;
        }
    };
    const ready = maps => show("ready", "Already downloaded",
        `${api.formatBytes(maps.reduce((sum, map) => sum + map.size, 0))} saved in this browser. Ready to play.`, 1);
    window.haloAutoCache.refresh = maps => {
        if (!api.mapProblems(maps).length) ready(maps);
        else if (panel.dataset.state === "ready") show("paused", "Download needed", "Reload this page to download the game again.", 0);
    };
    show("checking", "Checking game download", "Looking for game data saved in this browser…");
    const cached = await api.storedMaps();
    if (!api.mapProblems(cached).length) { ready(cached); return true; }
    const controller = new AbortController();
    const signal = controller.signal;
    const cancel = document.createElement("button");
    cancel.textContent = "Cancel download";
    cancel.type = "button";
    cancel.onclick = () => controller.abort();
    const retry = async (task) => {
        for (let attempt = 0; ; attempt++) {
            signal.throwIfAborted();
            try { return await task(); }
            catch (error) {
                if (signal.aborted || attempt >= 2 || error.name === "QuotaExceededError") throw error;
                api.setStatus(`Download interrupted; retrying in ${(attempt + 1) * 3} seconds...`);
                show("downloading", "Retrying download", `Retrying in ${(attempt + 1) * 3} seconds. Completed maps are safe.`, fraction);
                await new Promise(resolve => setTimeout(resolve, (attempt + 1) * 3000));
            }
        }
    };
    const fetchFile = async (url) => {
        const response = await fetch(url, {
            cache: "no-store", credentials: "omit",
            signal: AbortSignal.any([signal, AbortSignal.timeout(600000)]),
        });
        if (!response.ok) throw new Error(`Download returned HTTP ${response.status}`);
        return response;
    };
    // Pull one network chunk at a time so large maps do not accumulate in RAM.
    const mapStream = (file, base) => {
        let next = 0, reader = null, received = 0;
        return new ReadableStream({
            async pull(output) {
                try {
                    signal.throwIfAborted();
                    while (true) {
                        if (!reader) {
                            if (next === file.chunks.length) { output.close(); return; }
                            const response = await fetchFile(new URL(file.chunks[next].path, base));
                            reader = response.body.getReader();
                            received = 0;
                        }
                        const {done, value} = await reader.read();
                        if (done) {
                            reader.releaseLock();
                            reader = null;
                            if (received !== file.chunks[next].size) throw new Error(`Incomplete download: ${file.name}`);
                            next++;
                            continue;
                        }
                        received += value.byteLength;
                        if (received > file.chunks[next].size) throw new Error(`Oversized download: ${file.name}`);
                        output.enqueue(value);
                        return;
                    }
                } catch (error) {
                    await reader?.cancel().catch(() => {});
                    output.error(error);
                }
            },
            cancel() { return reader?.cancel(); },
        });
    };
    const controls = [api.elements.discInput, api.elements.folderInput, api.elements.clearButton];
    try {
        await api.withGameLock(async () => {
            api.setBusy(true);
            controls.forEach(control => control.disabled = true);
            api.elements.startButton.disabled = true;
            api.elements.status.after(cancel);
            api.setStatus("Checking game data downloads...");
            const base = new URL(api.base.endsWith("/") ? api.base : api.base + "/", location.href);
            const remote = await retry(async () => (await fetchFile(new URL("manifest.json", base))).json());
            const names = new Set(api.expectedMaps.map(name => `maps/${name}.map`));
            if (remote.version !== 2 || !Array.isArray(remote.files) || remote.files.length !== names.size) throw new Error("Incomplete download manifest");
            for (const file of remote.files) {
                if (!names.delete(file.name) || !Number.isSafeInteger(file.size) || file.size < 2048 || file.size > 512 * 1024 * 1024
                    || !/^[a-f0-9]{64}$/.test(file.sha256) || !Array.isArray(file.chunks) || !file.chunks.length
                    || file.chunks.length > 32 || file.chunks.some(chunk =>
                        !/^chunks\/[a-z0-9_]+\.map\.part[0-9]{3}$/.test(chunk.path)
                        || !Number.isSafeInteger(chunk.size) || chunk.size < 1 || chunk.size > 48 * 1024 * 1024)
                    || file.chunks.reduce((sum, chunk) => sum + chunk.size, 0) !== file.size) throw new Error("Invalid download manifest");
            }
            const existing = new Map((await api.storedMaps()).filter(map => !map.problem).map(map => [map.name, map.size]));
            const wanted = remote.files.filter(file => existing.get(file.name.slice(5)) !== file.size);
            const total = wanted.reduce((sum, file) => sum + file.size, 0);
            const allBytes = remote.files.reduce((sum, file) => sum + file.size, 0);
            const cachedBytes = allBytes - total;
            const required = allBytes + api.cacheBytes;
            if (!await api.canGrowBy(required - await api.folderBytes(["halo"]))) throw new Error(api.storageMessage(required));
            navigator.storage.persist?.().catch(() => {});
            const manifest = await api.currentManifest();
            let done = 0;
            for (const file of wanted) {
                const name = file.name.slice(5);
                await retry(async () => {
                    api.setStatus(`Downloading ${name}...`);
                    let lastUpdate = 0;
                    const updateProgress = written => {
                        const downloaded = cachedBytes + done + written;
                        show("downloading", "Downloading game data",
                            `${api.formatBytes(downloaded)} of ${api.formatBytes(allBytes)} · Keep this tab open.`,
                            Math.min(0.99, downloaded / allBytes));
                    };
                    updateProgress(0);
                    await api.storeMap(manifest, name, mapStream(file, base), file.size, (written) => {
                        api.elements.progress.value = (done + written) / total;
                        const now = Date.now();
                        if (now - lastUpdate >= 100 || written === file.size) {
                            updateProgress(written);
                            lastUpdate = now;
                        }
                    }, async () => {
                        api.setStatus(`Verifying ${name}...`);
                        show("checking", "Checking downloaded data", "Verifying the download before saving it. Keep this tab open.", fraction);
                        const folder = await api.directory(["halo", "data", "maps"], false);
                        const cached = await (await folder.getFileHandle(name)).getFile();
                        const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", await cached.arrayBuffer()));
                        const actual = Array.from(digest, byte => byte.toString(16).padStart(2, "0")).join("");
                        if (actual !== file.sha256) throw new Error(`${name} failed its integrity check`);
                        signal.throwIfAborted();
                    });
                });
                done += file.size;
            }
        });
        return true;
    } catch (error) {
        show(signal.aborted ? "paused" : "error", signal.aborted ? "Download paused" : "Download interrupted",
            signal.aborted ? "Completed maps are saved. Reload to resume downloading." : error.message + ". Reload to retry.", fraction);
        api.setStatus(signal.aborted ?
            "Download paused. Completed maps are cached. Reload to resume, or choose your disc image." :
            `Automatic download failed: ${error.message}. Reload to retry, or choose your disc image.`, "error");
        return false;
    } finally {
        api.setBusy(false);
        controls.forEach(control => control.disabled = false);
        api.elements.progress.hidden = true;
        cancel.remove();
        await api.refreshMaps(true);
    }
};
