#!/usr/bin/env python3
"""A local HTTPS server for testing PowerSpark's updater (tools/mac_update_checks.sh), never the real site.

  python3 tools/mac_update_test_server.py --dir FILES --certs CERTS [--port 18443]

Serves FILES (an appcast.plist and release zips) at https://localhost:PORT/ with a certificate of its own made
in CERTS (key.pem, and ca.pem, which is the CA list to give the updater). A Mac that cannot reach this one
reaches it through an ssh tunnel: ssh -R PORT:localhost:PORT that-mac. Paths with a prefix make the answer
misbehave, to test the updater's failures:
  /chunked/NAME    the file with Transfer-Encoding: chunked
  /redirect/NAME   a 302 to /NAME
  /truncated/NAME  the headers of the whole file and half of it
  /slow/NAME       the file at about 200 KB a second
  /status/CODE     an empty answer with that status
Every request is logged to server.log in the folder that holds FILES.
"""
import argparse
import http.server
import os
import ssl
import subprocess
import sys
import time

CONFIG = """[req]
distinguished_name = dn
x509_extensions = ext
prompt = no
[dn]
CN = localhost
[ext]
subjectAltName = DNS:localhost,IP:127.0.0.1
basicConstraints = CA:TRUE
"""


def make_certificate(certs):
    os.makedirs(certs, exist_ok=True)
    key, ca = os.path.join(certs, "key.pem"), os.path.join(certs, "ca.pem")
    if os.path.exists(key) and os.path.exists(ca):
        return key, ca
    config = os.path.join(certs, "openssl.cnf")
    with open(config, "w") as f:
        f.write(CONFIG)
    subprocess.run(["openssl", "ecparam", "-genkey", "-name", "prime256v1", "-noout", "-out", key], check=True)
    subprocess.run(["openssl", "req", "-new", "-x509", "-key", key, "-out", ca, "-days", "30", "-config", config],
                   check=True)
    return key, ca


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    root = "."
    log_path = None

    def log_message(self, format, *args):
        line = "%s %s" % (time.strftime("%H:%M:%S"), format % args)
        print(line, flush=True)
        if self.log_path:
            with open(self.log_path, "a") as f:
                f.write(line + "\n")

    def finish_headers(self, status, length=None, extra=()):
        self.send_response(status)
        if length is not None:
            self.send_header("Content-Length", str(length))
        for name, value in extra:
            self.send_header(name, value)
        self.send_header("Connection", "close")
        self.end_headers()

    def do_GET(self):
        path = self.path.split("?", 1)[0]
        mode = None
        for prefix in ("chunked", "redirect", "truncated", "slow", "status"):
            if path.startswith("/%s/" % prefix):
                mode, path = prefix, path[len(prefix) + 1:]
                break
        if mode == "status":
            self.finish_headers(int(path.strip("/") or 500), 0)
            return
        name = os.path.normpath(path.lstrip("/"))
        full = os.path.join(self.root, name)
        if name.startswith("..") or not os.path.isfile(full):
            self.finish_headers(404, 0)
            return
        data = open(full, "rb").read()
        if mode == "redirect":
            self.finish_headers(302, 0, [("Location", "/" + name)])
        elif mode == "chunked":
            self.finish_headers(200, None, [("Transfer-Encoding", "chunked")])
            for i in range(0, len(data), 1000):
                piece = data[i:i + 1000]
                self.wfile.write(b"%x\r\n" % len(piece) + piece + b"\r\n")
            self.wfile.write(b"0\r\n\r\n")
        elif mode == "truncated":
            self.finish_headers(200, len(data))
            self.wfile.write(data[:len(data) // 2])
            self.wfile.flush()
        elif mode == "slow":
            self.finish_headers(200, len(data))
            for i in range(0, len(data), 20000):
                self.wfile.write(data[i:i + 20000])
                self.wfile.flush()
                time.sleep(0.1)
        else:
            self.finish_headers(200, len(data))
            self.wfile.write(data)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", required=True)
    ap.add_argument("--certs", required=True)
    ap.add_argument("--port", type=int, default=18443)
    args = ap.parse_args()
    key, ca = make_certificate(args.certs)
    Handler.root = os.path.abspath(args.dir)
    Handler.log_path = os.path.join(os.path.dirname(Handler.root), "server.log")
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(ca, key)
    server = http.server.ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    server.socket = context.wrap_socket(server.socket, server_side=True)
    print("serving %s at https://localhost:%d/ (CA list: %s)" % (Handler.root, args.port, ca), flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    sys.exit(main())
