#!/usr/bin/env python3
"""Package the verified browser runtime for a static host, without game data."""
import argparse
import html as html_utils
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from urllib.parse import urlsplit

import setup_browser

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "dist/github-pages"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-repository", help="Optional public source repository URL for the deployment receipt")
    parser.add_argument("--data-source", help="Optional HTTPS directory with a version-2 map manifest and chunks")
    args = parser.parse_args()
    if args.data_source:
        parsed = urlsplit(args.data_source)
        if (parsed.scheme != "https" and not (parsed.scheme == "http" and parsed.hostname in ("localhost", "127.0.0.1"))
                or not parsed.hostname or parsed.username or parsed.password or parsed.query or parsed.fragment):
            parser.error("data-source must be an HTTPS URL (or HTTP loopback for local testing)")
    # Never copy build/web wholesale: it can contain local experiments.
    expected = set(setup_browser.FILES) | set(setup_browser.LOCAL_SCRIPTS) | {
        "download-provenance.json", ".nojekyll", "deployment.json", "auto-cache.js",
    }
    OUTPUT.mkdir(parents=True, exist_ok=True)
    unexpected = {path.name for path in OUTPUT.iterdir()} - expected
    if unexpected:
        raise ValueError(f"Unexpected files in {OUTPUT}: {sorted(unexpected)}")
    with tempfile.TemporaryDirectory(prefix="halo-pages-") as temporary:
        staging = Path(temporary)
        setup_browser.main(staging)
        index = staging / "index.html"
        html = index.read_text().replace("<title>Halo</title>", "<title>Play Halo CE</title>")
        html = html.replace("Local copy of", "Self-hosted copy of")
        shutil.copyfile(ROOT / "port/web/auto-cache.js", staging / "auto-cache.js")
        if args.data_source:
            html = html.replace('<script src="launcher.js"></script>',
                f'<meta name="halo-data-source" content="{html_utils.escape(args.data_source, quote=True)}">\n'
                '<script src="auto-cache.js"></script>\n<script src="launcher.js"></script>')
            start = html.index('\t\t\t<p class="help">')
            end = html.index('</p>', start) + len('</p>')
            html = html[:start] + ('<p class="help">Game data downloads automatically on your first visit '
                '(about 1.9 GB) and stays in this browser for next time. Keep this tab open until '
                'the download finishes, then press Play. You can also choose your own Xbox disc image below.</p>') + html[end:]
            launcher = staging / "launcher.js"
            script = launcher.read_text()
            patches = {
                'async function storeMap(manifest, name, source, size, onProgress) {':
                    'async function storeMap(manifest, name, source, size, onProgress, verify) {',
                '\t\tmanifest[name] = size;': '\t\tif (verify) await verify();\n\t\tmanifest[name] = size;',
                '\t\tif (parameters.get("data")) {': '''\t\tif (!parameters.get("data") && !parameters.has("manual")) {
            const ready = await window.haloAutoCache({
                base: document.querySelector('meta[name="halo-data-source"]').content,
                elements, storedMaps, mapProblems, directory, currentManifest, storeMap,
                canGrowBy, folderBytes, storageMessage, withGameLock, formatBytes,
                setStatus, refreshMaps, expectedMaps: EXPECTED_MAPS, cacheBytes: GAME_CACHE_BYTES,
                setBusy: value => { busy = value; },
            });
            if (!ready) return;
        }
\t\tif (parameters.get("data")) {''',
            }
            for before, after in patches.items():
                if script.count(before) != 1:
                    raise ValueError("Pinned launcher changed; automatic-cache hook no longer matches")
                script = script.replace(before, after)
            launcher.write_text(script)
        # All runtime URLs stay relative so /halo-ce-universal/ works.
        index.write_text(html)
        receipt_path = staging / "download-provenance.json"
        receipt = json.loads(receipt_path.read_text())
        receipt["local_index_sha256"] = setup_browser.digest(index.read_bytes())
        receipt["local_changes"].append("Static hosting title and attribution")
        receipt["local_files"]["auto-cache.js"] = {
            "bytes": (staging / "auto-cache.js").stat().st_size,
            "sha256": setup_browser.digest((staging / "auto-cache.js").read_bytes()),
        }
        if args.data_source:
            receipt["local_changes"].append("Automatic map caching with integrity checks and retry")
            receipt["local_launcher_sha256"] = setup_browser.digest((staging / "launcher.js").read_bytes())
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")
        (staging / ".nojekyll").touch()
        deployment = {
            "source_commit": subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "game_data_included": False,
            "files": {path.name: {"bytes": path.stat().st_size,
                                  "sha256": setup_browser.digest(path.read_bytes())}
                      for path in sorted(staging.iterdir())},
        }
        if args.source_repository:
            deployment["source_repository"] = args.source_repository
        if args.data_source:
            deployment["data_source"] = args.data_source
        (staging / "deployment.json").write_text(json.dumps(deployment, indent=2) + "\n")
        assert {path.name for path in staging.iterdir()} == expected
        for path in staging.iterdir():
            shutil.copyfile(path, OUTPUT / path.name)
    total = sum(path.stat().st_size for path in OUTPUT.iterdir())
    print(f"Static site: {OUTPUT} ({total:,} bytes; no maps or disc images)")


if __name__ == "__main__":
    main()
