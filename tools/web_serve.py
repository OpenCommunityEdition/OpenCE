#!/usr/bin/env python3
"""Serves the web build's site (port/web/README.md) on this computer:

    python tools/web_serve.py                       # build/web/site, what ninja web built
    python tools/web_serve.py dist/halo-web-debug   # a build from CI or a release zip
    python tools/web_serve.py --as-github-pages     # without the isolation headers

with the headers that make the page cross-origin isolated (which the game's
threads need), WebAssembly's type, single byte ranges and no caching, at
http://localhost:8000/ (a secure context, as https is). --as-github-pages
leaves the isolation headers out, as GitHub Pages does, so that the service
worker's (port/web/site/sw.js) is what isolates the page.
"""

import argparse
import os
import shutil
import sys
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
COPY_BUFFER_SIZE = 1 << 20


class SiteHandler(SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    isolate = True

    extensions_map = {
        **SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript; charset=utf-8",
        ".webmanifest": "application/manifest+json",
        ".json": "application/json",
    }

    def __init__(self, *args, **kwargs):
        self.range_remaining = None
        super().__init__(*args, **kwargs)

    def end_headers(self):
        if self.isolate:
            self.send_header("Cross-Origin-Opener-Policy", "same-origin")
            self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
            self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()

    def log_message(self, format, *args):
        if os.environ.get("WEB_SERVE_QUIET") != "1":
            super().log_message(format, *args)

    @staticmethod
    def parse_range(value, size):
        """(first, last) of a single "bytes=a-b" range; ValueError if none"""
        if not value.startswith("bytes=") or "," in value or "-" not in value:
            raise ValueError(value)
        first, last = value[6:].strip().split("-", 1)
        if not first:
            length = min(int(last), size)
            if length <= 0:
                raise ValueError(value)
            return size - length, size - 1
        start, end = int(first), int(last) if last else size - 1
        if start >= size or start > end:
            raise ValueError(value)
        return start, min(end, size - 1)

    def send_head(self):
        self.range_remaining = None
        range_header = self.headers.get("Range")
        path = self.translate_path(self.path)
        if not range_header or not os.path.isfile(path):
            return super().send_head()
        source = open(path, "rb")
        size = os.fstat(source.fileno()).st_size
        try:
            start, end = self.parse_range(range_header, size)
        except ValueError:
            source.close()
            self.send_response(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
            self.send_header("Content-Range", f"bytes */{size}")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return None
        source.seek(start)
        self.range_remaining = end - start + 1
        self.send_response(HTTPStatus.PARTIAL_CONTENT)
        self.send_header("Content-Type", self.guess_type(path))
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
        self.send_header("Content-Length", str(self.range_remaining))
        self.end_headers()
        return source

    def copyfile(self, source, outputfile):
        if self.range_remaining is None:
            shutil.copyfileobj(source, outputfile, COPY_BUFFER_SIZE)
            return
        while self.range_remaining:
            chunk = source.read(min(COPY_BUFFER_SIZE, self.range_remaining))
            if not chunk:
                break
            outputfile.write(chunk)
            self.range_remaining -= len(chunk)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("root", nargs="?", type=Path, default=ROOT / "build/web/site", help="the site's folder")
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--as-github-pages", action="store_true",
                        help="no isolation headers: the service worker isolates the page")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if not (root / "index.html").is_file():
        parser.error(f"{root} has no index.html: run `ninja web` first, or name a site's folder")

    class Handler(SiteHandler):
        isolate = not args.as_github_pages

        def __init__(self, *handler_args, **kwargs):
            super().__init__(*handler_args, directory=str(root), **kwargs)

    server = ThreadingHTTPServer((args.bind, args.port), Handler)
    print(f"Serving {root} at http://localhost:{server.server_address[1]}/", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
