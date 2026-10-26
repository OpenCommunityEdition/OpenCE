#!/usr/bin/env python3
"""A command-line client of the dedicated server's remote console (RCON,
docs/dedicated_server.md): a WebSocket to /ws with the subprotocol
opence-rcon-1. Standard library only.

    python3 tools/opence_rcon.py --url http://127.0.0.1:8080 --password … sv_status
    python3 tools/opence_rcon.py --url http://127.0.0.1:8080 --password … sv_say "back in five"
    python3 tools/opence_rcon.py --url http://127.0.0.1:8080              (an interactive session)
    python3 tools/opence_rcon.py --events player,chat,game                (prints events as they come)

The password comes from --password, else OPENCE_RCON_PASSWORD, else it is
asked for; --token takes a session's token instead. Exit codes: 0 the
command ran, 1 the server refused or failed it, 2 no connection or the
sign-in was refused.

A command given on the command line goes as its command and its words
("command" and "args"), each word as the shell gave it: the server quotes
one with a space as it makes the line. An interactive session's lines go as
they are typed ("line").
"""

import argparse
import base64
import getpass
import hashlib
import json
import os
import socket
import ssl
import struct
import sys
import threading
import time
from urllib.parse import urlsplit

SUBPROTOCOL = "opence-rcon-1"
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


class RconError(Exception):
    """no connection, a broken one, or a refused sign-in"""


class Closed(RconError):
    def __init__(self, code, reason):
        super().__init__(f"the server closed the connection ({code}{': ' + reason if reason else ''})")
        self.code = code
        self.reason = reason


class WebSocket:
    """a client's WebSocket (RFC 6455): text messages, masked frames,
    fragments joined, pings answered"""

    def __init__(self, url, timeout=10, headers=None, protocols=(SUBPROTOCOL,)):
        parts = urlsplit(url)
        secure = parts.scheme in ("https", "wss")
        if parts.scheme not in ("http", "https", "ws", "wss") or not parts.hostname:
            raise RconError(f"{url} is not an http(s):// or ws(s):// address")
        port = parts.port or (443 if secure else 80)
        try:
            sock = socket.create_connection((parts.hostname, port), timeout=timeout)
            if secure:
                sock = ssl.create_default_context().wrap_socket(sock, server_hostname=parts.hostname)
        except OSError as failure:
            raise RconError(f"cannot reach {parts.hostname}:{port}: {failure}") from None
        self.sock = sock
        self.buffer = b""
        self.send_lock = threading.Lock()
        key = base64.b64encode(os.urandom(16)).decode()
        host = parts.hostname if parts.port is None else f"{parts.hostname}:{parts.port}"
        lines = [f"GET {parts.path.rstrip('/') + '/ws' if not parts.path.endswith('/ws') else parts.path} HTTP/1.1",
                 f"Host: {host}", "Upgrade: websocket", "Connection: Upgrade", f"Sec-WebSocket-Key: {key}",
                 "Sec-WebSocket-Version: 13"]
        if protocols:
            lines.append("Sec-WebSocket-Protocol: " + ", ".join(protocols))
        for name, value in (headers or {}).items():
            lines.append(f"{name}: {value}")
        try:
            sock.sendall(("\r\n".join(lines) + "\r\n\r\n").encode())
            while b"\r\n\r\n" not in self.buffer:
                chunk = sock.recv(65536)
                if not chunk:
                    raise RconError("the server closed the connection during the handshake")
                self.buffer += chunk
                if len(self.buffer) > 65536:
                    raise RconError("a handshake answer too long")
        except OSError as failure:
            raise RconError(f"the handshake failed: {failure}") from None
        head, self.buffer = self.buffer.split(b"\r\n\r\n", 1)
        status_line, *header_lines = head.decode("latin-1").split("\r\n")
        self.status = int(status_line.split(" ")[1]) if len(status_line.split(" ")) > 1 else 0
        self.headers = {}
        for line in header_lines:
            name, _, value = line.partition(":")
            self.headers[name.strip().lower()] = value.strip()
        if self.status != 101:
            body = self.buffer
            length = int(self.headers.get("content-length", "0") or 0)
            try:
                while len(body) < length:
                    chunk = sock.recv(65536)
                    if not chunk:
                        break
                    body += chunk
                message = json.loads(body.decode("utf-8")).get("error", {}).get("message", "")
            except (OSError, ValueError, AttributeError):
                message = ""
            sock.close()
            raise RconError(f"the server refused the WebSocket ({self.status}{': ' + message if message else ''})")
        expected = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
        if self.headers.get("sec-websocket-accept") != expected:
            raise RconError("the server's Sec-WebSocket-Accept is wrong")
        if protocols and self.headers.get("sec-websocket-protocol") not in protocols:
            raise RconError("the server did not take the subprotocol " + SUBPROTOCOL)
        sock.settimeout(None)

    def send_frame(self, opcode, payload, fin=True, mask=True):
        first = (0x80 if fin else 0) | opcode
        size = len(payload)
        bit = 0x80 if mask else 0
        if size < 126:
            head = struct.pack("!BB", first, bit | size)
        elif size < 65536:
            head = struct.pack("!BBH", first, bit | 126, size)
        else:
            head = struct.pack("!BBQ", first, bit | 127, size)
        if mask:
            key = os.urandom(4)
            payload = bytes(byte ^ key[index % 4] for index, byte in enumerate(payload))
            head += key
        with self.send_lock:
            self.sock.sendall(head + payload)

    def send(self, message):
        self.send_frame(0x1, json.dumps(message, separators=(",", ":")).encode("utf-8"))

    def read(self, count):
        while len(self.buffer) < count:
            try:
                chunk = self.sock.recv(1 << 20)
            except socket.timeout:
                raise
            except OSError as failure:
                raise RconError(f"the connection broke: {failure}") from None
            if not chunk:
                raise RconError("the connection was lost")
            self.buffer += chunk
        data, self.buffer = self.buffer[:count], self.buffer[count:]
        return data

    def frame(self):
        first, second = self.read(2)
        size = second & 0x7F
        if size == 126:
            size = struct.unpack("!H", self.read(2))[0]
        elif size == 127:
            size = struct.unpack("!Q", self.read(8))[0]
        if second & 0x80:
            raise RconError("the server masked a frame")
        if size > 64 << 20:
            raise RconError("a frame of more than 64 MiB")
        return bool(first & 0x80), first & 0x0F, self.read(size)

    def receive(self, timeout=None):
        """the next message (a dict); Closed when the server closes"""
        self.sock.settimeout(timeout)
        pieces = []
        try:
            while True:
                fin, opcode, payload = self.frame()
                if opcode == 0x9:
                    self.send_frame(0xA, payload)
                    continue
                if opcode == 0xA:
                    continue
                if opcode == 0x8:
                    code = struct.unpack("!H", payload[:2])[0] if len(payload) >= 2 else 1005
                    try:
                        self.send_frame(0x8, payload[:2])
                    except OSError:
                        pass
                    raise Closed(code, payload[2:].decode("utf-8", "replace"))
                pieces.append(payload)
                if fin:
                    return json.loads(b"".join(pieces).decode("utf-8"))
        except socket.timeout:
            raise TimeoutError("no message in time") from None
        finally:
            self.sock.settimeout(None)

    def close(self, code=1000):
        try:
            self.send_frame(0x8, struct.pack("!H", code))
            self.sock.close()
        except OSError:
            pass


class Rcon:
    """a signed-in RCON connection: commands and their replies, events"""

    def __init__(self, url, password=None, token=None, by=None, timeout=10):
        self.timeout = timeout
        self.socket = WebSocket(url, timeout=timeout)
        self.next_id = 1
        self.pending = []
        hello = self.socket.receive(timeout)
        if hello.get("type") != "hello":
            raise RconError(f"the server did not say hello: {hello}")
        self.hello = hello
        if token:
            self.socket.send({"type": "auth", "token": token})
        else:
            message = {"type": "auth", "password": password or ""}
            if by:
                message["by"] = by
            self.socket.send(message)
        while True:
            try:
                answer = self.socket.receive(timeout)
            except Closed as closed:
                raise RconError(f"signed out: {closed}") from None
            if answer.get("type") == "auth":
                break
            self.pending.append(answer)
        if not answer.get("ok"):
            refusal = answer.get("error") or {}
            raise RconError(f"sign-in refused: {refusal.get('code', '')} {refusal.get('message', '')}".strip())
        self.role = answer.get("role")

    def command(self, line, timeout=None, args=None):
        """the reply to one command: a whole line, or (with args) a command and its words, which the server
        quotes as it joins them (a word with a space, or an empty one)"""
        identifier = self.next_id
        self.next_id += 1
        if args is None:
            self.socket.send({"type": "command", "id": identifier, "line": line})
        else:
            self.socket.send({"type": "command", "id": identifier, "command": line, "args": list(args)})
        deadline = time.monotonic() + (timeout or max(self.timeout, 30))
        while True:
            message = self.socket.receive(max(0.1, deadline - time.monotonic()))
            if message.get("type") == "reply" and message.get("id") == identifier:
                return message
            if message.get("type") == "error" and message.get("id") == identifier:
                return {"type": "reply", "id": identifier, "ok": False, "lines": [], "error": message.get("error")}
            if message.get("type") == "overflow":
                raise RconError("the server dropped the connection: it was read too slowly")
            self.pending.append(message)

    def subscribe(self, groups, after=None):
        message = {"type": "subscribe", "events": list(groups)}
        if after is not None:
            message["after"] = after
        self.socket.send(message)

    def receive(self, timeout=None):
        if self.pending:
            return self.pending.pop(0)
        return self.socket.receive(timeout)

    def close(self):
        self.socket.close()


def print_reply(reply, out=sys.stdout):
    """a reply's lines on standard output; a failed command's (which say why) on standard error"""
    failed = not reply.get("ok")
    for line in reply.get("lines") or []:
        print(line, file=sys.stderr if failed else out)
    if failed:
        refusal = reply.get("error")
        # (a command that ran and failed: its lines, no error; one refused before it ran: an error)
        if refusal or not reply.get("lines"):
            refusal = refusal or {}
            print(f"error: {refusal.get('message') or refusal.get('code') or 'the command failed'}", file=sys.stderr)
        return 1
    return 0


def interactive(rcon):
    print(f"{rcon.hello.get('server', '')} ({rcon.hello.get('build', '')}, network version "
          f"{rcon.hello.get('network_version', '')}), signed in as {rcon.role}. quit or Ctrl-D leaves.")
    status = 0
    while True:
        try:
            line = input("> ").strip()
        except EOFError:
            print()
            return status
        if line in ("exit", "logout") or (line == "quit" and rcon.role != "owner"):
            return status
        if not line:
            continue
        try:
            status = print_reply(rcon.command(line))
        except (RconError, TimeoutError) as failure:
            print(f"opence_rcon: {failure}", file=sys.stderr)
            return 2


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                     epilog="Exit codes: 0 ok, 1 the command failed, 2 no connection or sign-in.")
    parser.add_argument("--url", default=os.environ.get("OPENCE_RCON_URL", "http://127.0.0.1:8080"),
                        help="the server's web address (OPENCE_RCON_URL)")
    parser.add_argument("--password", default=None, help="the admin or moderator password (OPENCE_RCON_PASSWORD)")
    parser.add_argument("--token", default=os.environ.get("OPENCE_RCON_TOKEN"),
                        help="a session token from POST /api/login instead (OPENCE_RCON_TOKEN)")
    parser.add_argument("--by", default=os.environ.get("USER", ""), help="who you are, for the moderation log")
    parser.add_argument("--timeout", type=float, default=10, help="seconds to wait for the server")
    parser.add_argument("--json", action="store_true", help="print the reply as JSON")
    parser.add_argument("--events", default="", help="groups to print events of (player,chat,game,moderation,"
                        "status,log,master), until Ctrl-C")
    parser.add_argument("--after", type=int, default=None, help="with --events: replay the events after this seq")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="a command line (none: an interactive session)")
    options = parser.parse_args(argv)
    password = options.password or os.environ.get("OPENCE_RCON_PASSWORD")
    if not password and not options.token:
        if not sys.stdin.isatty():
            print("opence_rcon: no password (--password or OPENCE_RCON_PASSWORD)", file=sys.stderr)
            return 2
        password = getpass.getpass("Password: ")
    try:
        rcon = Rcon(options.url, password=password, token=options.token, by=options.by, timeout=options.timeout)
    except (RconError, TimeoutError, ValueError) as failure:
        print(f"opence_rcon: {failure}", file=sys.stderr)
        return 2
    try:
        if options.events:
            rcon.subscribe([group.strip() for group in options.events.split(",") if group.strip()], options.after)
            while True:
                message = rcon.receive()
                if message.get("type") == "overflow":
                    print("opence_rcon: the server dropped the connection (read too slowly)", file=sys.stderr)
                    return 2
                print(json.dumps(message, ensure_ascii=False), flush=True)
        if options.command:
            # (the shell's words as they are: the server quotes one with a space)
            words = options.command[1:] if options.command[0] == "--" else options.command
            if not words:
                print("opence_rcon: no command", file=sys.stderr)
                return 2
            reply = rcon.command(words[0], timeout=max(options.timeout, 30), args=words[1:])
            if options.json:
                print(json.dumps(reply, ensure_ascii=False))
                return 0 if reply.get("ok") else 1
            return print_reply(reply)
        return interactive(rcon)
    except (KeyboardInterrupt, BrokenPipeError):
        return 0
    except (RconError, TimeoutError, ValueError) as failure:
        print(f"opence_rcon: {failure}", file=sys.stderr)
        return 2
    finally:
        rcon.close()


if __name__ == "__main__":
    sys.exit(main())
