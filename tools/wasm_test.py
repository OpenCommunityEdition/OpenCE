"""Runs the freestanding WebAssembly modules the engine tests build.

The tests compile the game's actual C functions to wasm32 with the NDK's clang
and call the module's run_tests(), which returns 0 or the number of the check
that failed. Node runs it when it is installed; otherwise a Chromium browser
(Edge, which Windows has, or Chrome) does, without a window.
"""
import base64
import os
import re
import shutil
import subprocess
from pathlib import Path

BROWSERS = (
    "msedge", "chrome", "chromium", "google-chrome",
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
)


def _browser():
    for name in BROWSERS:
        path = shutil.which(name) or (name if os.path.isfile(name) else None)
        if path:
            return path
    return None


def run_tests(wasm: Path) -> int:
    """the module's run_tests() result"""
    wasm = Path(wasm)
    node = shutil.which("node")
    if node:
        script = wasm.with_suffix(".cjs")
        script.write_text(
            'const fs=require("fs");const i=new WebAssembly.Instance(new WebAssembly.Module('
            'fs.readFileSync(process.argv[2])));console.log("RESULT:"+i.exports.run_tests());', encoding="utf-8")
        output = subprocess.run([node, str(script), str(wasm)], check=True, capture_output=True, text=True).stdout
    else:
        browser = _browser()
        if not browser:
            raise RuntimeError("These tests need Node, or Edge or Chrome, to run WebAssembly")
        page = wasm.with_suffix(".html")
        page.write_text(
            "<!doctype html><body><script>const b=Uint8Array.from(atob('"
            + base64.b64encode(wasm.read_bytes()).decode("ascii")
            + "'),c=>c.charCodeAt(0));WebAssembly.instantiate(b).then(m=>{document.body.textContent="
            "'RESULT:'+m.instance.exports.run_tests();}).catch(e=>{document.body.textContent='RESULT:error '+e;});"
            "</script></body>", encoding="utf-8")
        profile = wasm.parent / "browser-profile"
        output = subprocess.run(
            [browser, "--headless=new", "--disable-gpu", "--no-first-run", f"--user-data-dir={profile}",
             "--virtual-time-budget=10000", "--dump-dom", page.resolve().as_uri()],
            check=True, capture_output=True, text=True, timeout=120).stdout
    match = re.search(r"RESULT:(-?\d+)", output)
    if not match:
        raise RuntimeError("The WebAssembly test did not finish: " + output.strip()[-400:])
    return int(match.group(1))


def check(wasm: Path, name: str, passed: str) -> None:
    """runs the module; raises with the failed check's number, or prints `passed`"""
    result = run_tests(wasm)
    if result:
        raise AssertionError(f"{name} regression {result}")
    print(passed)
