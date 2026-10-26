"""The dedicated server's protocol simulator (tools/server_sim.py) and the
admin page's files (port/assets/server_ui): the HTTP API's headers, sessions,
CSRF, Origin, roles and lockout; the RCON over WebSocket with its framing and
faults; the RCON client's exit codes; and the page's files against its
Content-Security-Policy (the check the build's embedder uses).

    python3 -m pytest -q tools/test_server_sim.py
"""

import json
import os
import re
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

import pytest

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

import opence_rcon  # noqa: E402
import server_sim  # noqa: E402

UI = TOOLS.parent / "port" / "assets" / "server_ui"


class Sim:
    def __init__(self, *args):
        self.process = subprocess.Popen([sys.executable, str(TOOLS / "server_sim.py"), "--port", "0", *args],
                                        stdout=subprocess.PIPE, text=True)
        line = self.process.stdout.readline()
        match = re.match(r"listening on (http://(\S+):(\d+))", line)
        assert match, line
        self.url, self.host, self.port = match.group(1), match.group(2), int(match.group(3))
        deadline = time.monotonic() + 10
        while self.request("GET", "/healthz")[0] != 200:
            assert time.monotonic() < deadline, "the simulator did not start hosting"
            time.sleep(0.02)

    def stop(self):
        self.process.kill()
        self.process.wait()

    def request(self, method, path, body=None, headers=None, raw=None):
        """(status, headers, the answer's JSON or bytes)"""
        data = raw if raw is not None else (json.dumps(body).encode() if body is not None else None)
        request = urllib.request.Request(self.url + path, data=data, method=method, headers=headers or {})
        if data is not None and "Content-Type" not in (headers or {}):
            request.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(request, timeout=10) as response:
                status, answer_headers, content = response.status, response.headers, response.read()
        except urllib.error.HTTPError as failure:
            status, answer_headers, content = failure.code, failure.headers, failure.read()
        try:
            return status, answer_headers, json.loads(content)
        except ValueError:
            return status, answer_headers, content

    def login(self, password="opence-admin", **headers):
        status, answer_headers, body = self.request("POST", "/api/login", {"password": password}, headers=headers)
        assert status == 200, body
        cookie = answer_headers["Set-Cookie"]
        return body, cookie

    def browser(self, password="opence-admin"):
        """the headers a browser on the page sends with a change"""
        body, cookie = self.login(password)
        return {"Cookie": cookie.split(";")[0], "Origin": self.url, "X-CSRF-Token": body["csrf"]}

    def raw(self, data, read=True):
        with socket.create_connection((self.host, self.port), timeout=5) as connection:
            connection.sendall(data)
            if not read:
                return b""
            answer = b""
            while True:
                try:
                    chunk = connection.recv(65536)
                except socket.timeout:
                    break
                if not chunk:
                    break
                answer += chunk
            return answer


@pytest.fixture(scope="module")
def sim():
    running = Sim("--still", "--tick", "0.05")
    yield running
    running.stop()


@pytest.fixture
def fresh():
    started = []

    def start(*args):
        running = Sim(*args)
        started.append(running)
        return running

    yield start
    for running in started:
        running.stop()


# ---------- the page's files and the Content-Security-Policy


@pytest.mark.parametrize("name", ["index.html", "app.js", "app.css", "icon.svg"])
def test_the_page_files_keep_to_the_policy(name):
    assert server_sim.csp_problems(name, (UI / name).read_text()) == []


def test_the_policy_check_finds_what_the_policy_blocks():
    assert server_sim.csp_problems("index.html", "<script>alert(1)</script>")
    assert server_sim.csp_problems("index.html", '<script src="app.js">x()</script>')
    assert not server_sim.csp_problems("index.html", '<script src="app.js" defer></script>')
    assert server_sim.csp_problems("index.html", "<style>p{}</style>")
    assert server_sim.csp_problems("index.html", '<p style="color:red">')
    assert server_sim.csp_problems("index.html", '<button onclick="go()">')
    assert server_sim.csp_problems("index.html", '<a href="javascript:go()">')
    assert server_sim.csp_problems("index.html", '<img src="https://example.org/x.png">')
    assert server_sim.csp_problems("index.html", '<link rel="stylesheet" href="//cdn.example/x.css">')
    assert not server_sim.csp_problems("index.html", '<a href="https://github.com/OpenCommunityEdition">source</a>')
    assert server_sim.csp_problems("app.js", "node.innerHTML = text;")
    assert server_sim.csp_problems("app.js", "eval(text)")
    assert server_sim.csp_problems("app.js", "new Function('return 1')")
    assert server_sim.csp_problems("app.js", "setTimeout('go()', 5)")
    assert server_sim.csp_problems("app.js", "node.setAttribute('onclick', 'x')")
    assert server_sim.csp_problems("app.js", "node.style.cssText = 'x'")
    assert server_sim.csp_problems("app.js", "fetch('https://example.org')")
    assert not server_sim.csp_problems("app.js", "node.textContent = text; retrieval(text);")
    assert server_sim.csp_problems("app.css", "@import url(x.css);")
    assert server_sim.csp_problems("app.css", "a { background: url(data:image/png;base64,AA); }")
    assert server_sim.csp_problems("app.css", "a { background: url('https://example.org/x.png'); }")
    assert not server_sim.csp_problems("app.css", "a { background: url(icon.svg); }")
    assert server_sim.csp_problems("icon.svg", "<svg><script>x()</script></svg>")
    assert server_sim.csp_problems("icon.svg", '<svg><rect style="fill:red"/></svg>')


def test_the_page_is_small():
    total = sum((UI / name).stat().st_size for name in ("index.html", "app.js", "app.css", "icon.svg"))
    assert total < 96 * 1024, total


# ---------- HTTP


def test_every_answer_carries_the_headers(sim):
    for path in ("/healthz", "/api/status", "/nothing", "/", "/app.js", "/app.css", "/icon.svg"):
        status, headers, _ = sim.request("GET", path)
        assert headers["Content-Security-Policy"] == "default-src 'self'; frame-ancestors 'none'", path
        assert headers["X-Content-Type-Options"] == "nosniff"
        assert headers["Referrer-Policy"] == "same-origin"
    status, headers, body = sim.request("GET", "/api/status")
    assert (status, headers["Content-Type"], headers["Cache-Control"]) == (401, "application/json", "no-store")
    assert body == {"ok": False, "error": {"code": "denied", "message": "sign in first"}}


def test_the_page_is_served_with_an_etag(sim):
    status, headers, page = sim.request("GET", "/")
    assert status == 200 and headers["Content-Type"] == "text/html; charset=utf-8"
    assert page == (UI / "index.html").read_bytes()
    assert sim.request("GET", "/app.js")[1]["Content-Type"] == "text/javascript; charset=utf-8"
    assert sim.request("GET", "/icon.svg")[1]["Content-Type"] == "image/svg+xml"
    status, _, _ = sim.request("GET", "/", headers={"If-None-Match": headers["ETag"]})
    assert status == 304


def test_health_and_metrics_need_no_session(sim):
    status, _, body = sim.request("GET", "/healthz")
    assert status == 200 and body["state"] == "playing"
    status, headers, text = sim.request("GET", "/metrics")
    assert status == 200 and headers["Content-Type"].startswith("text/plain")
    assert b"opence_players 5" in text


def test_signing_in_sets_the_cookie_and_the_session_answers(sim):
    body, cookie = sim.login()
    assert body["ok"] and body["role"] == "admin"
    assert re.fullmatch(r"[0-9a-f]{64}", body["token"]) and re.fullmatch(r"[0-9a-f]{32}", body["csrf"])
    assert body["expires"] > time.time() + 1700
    assert cookie == f"opence_session={body['token']}; HttpOnly; SameSite=Strict; Path=/"
    status, _, session = sim.request("GET", "/api/session", headers={"Cookie": cookie.split(";")[0]})
    assert session == {"ok": True, "role": "admin", "csrf": body["csrf"], "expires": session["expires"]}
    status, _, session = sim.request("GET", "/api/session", headers={"Authorization": "Bearer " + body["token"]})
    assert status == 200
    # (X-Forwarded-Proto is believed only from the trusted proxy)
    _, direct = sim.login(**{"X-Forwarded-Proto": "https"})
    assert not direct.endswith("; Secure")
    moderator, _ = sim.login("opence-mod")
    assert moderator["role"] == "moderator"
    status, headers, _ = sim.request("POST", "/api/logout", {}, headers={
        "Cookie": cookie.split(";")[0], "Origin": sim.url, "X-CSRF-Token": body["csrf"]})
    assert status == 200 and "Max-Age=0" in headers["Set-Cookie"]
    assert sim.request("GET", "/api/session", headers={"Cookie": cookie.split(";")[0]})[0] == 401


def test_changes_from_the_page_need_the_csrf_token_and_its_origin(sim):
    headers = sim.browser()
    assert sim.request("POST", "/api/say", {"text": "hi"}, headers=headers)[0] == 200
    status, _, body = sim.request("POST", "/api/say", {"text": "hi"}, headers={**headers, "X-CSRF-Token": "0" * 32})
    assert (status, body["error"]["code"]) == (403, "csrf")
    status, _, body = sim.request("POST", "/api/say", {"text": "hi"}, headers={**headers, "Origin": "http://evil.example"})
    assert (status, body["error"]["code"]) == (403, "origin")
    no_origin = {key: value for key, value in headers.items() if key != "Origin"}
    status, _, body = sim.request("POST", "/api/say", {"text": "hi"}, headers=no_origin)
    assert (status, body["error"]["code"]) == (403, "origin")
    status, _, body = sim.request("POST", "/api/login", {"password": "opence-admin"}, headers={"Origin": "http://evil.example"})
    assert (status, body["error"]["code"]) == (403, "origin")
    # a script's bearer token needs neither
    token = sim.login()[0]["token"]
    assert sim.request("POST", "/api/say", {"text": "hi"}, headers={"Authorization": "Bearer " + token})[0] == 200


def test_roles(fresh):
    sim = fresh("--still", "--tick", "0.05", "--viewer-password", "opence-view")
    viewer = sim.browser("opence-view")
    moderator = sim.browser("opence-mod")
    status, _, body = sim.request("POST", "/api/say", {"text": "hi"}, headers=viewer)
    assert (status, body["error"]) == (403, {"code": "forbidden", "message": "needs the moderator role"})
    assert sim.request("GET", "/api/players", headers=viewer)[0] == 200
    assert sim.request("GET", "/api/status", headers=viewer)[2]["listing"]["invite"] == ""
    assert sim.request("GET", "/api/status", headers=sim.browser())[2]["listing"]["invite"].startswith("halo://join/")
    status, _, body = sim.request("PUT", "/api/rotation", {"entries": ["bloodgulch:ctf"]}, headers=moderator)
    assert (status, body["error"]["code"]) == (403, "forbidden")
    assert sim.request("GET", "/api/bans", headers=moderator)[0] == 403
    assert sim.request("POST", "/api/rotation/next", {}, headers=moderator)[0] == 200
    status, _, body = sim.request("POST", "/api/command", {"line": "sv_set server.name X"}, headers=moderator)
    assert (status, body["error"]["code"]) == (403, "forbidden")
    status, _, body = sim.request("POST", "/api/command", {"line": "quit"}, headers=sim.browser())
    assert (status, body["error"]["code"]) == (403, "forbidden")
    players = sim.request("GET", "/api/players", headers=moderator)[2]["players"]
    assert "address" not in players[0]
    target = players[0]
    status, _, body = sim.request("POST", f"/api/players/{target['slot']}/ban",
                                  {"name": target["name"], "duration": "forever"}, headers=moderator)
    assert (status, body["error"]["code"]) == (400, "failed")
    assert body["error"]["message"].startswith("ban: a moderator's ban is for a day at most")
    status, _, body = sim.request("POST", f"/api/players/{target['slot']}/ban",
                                  {"name": target["name"], "duration": "1d", "reason": "griefing"}, headers=moderator)
    assert status == 200, body
    bans = sim.request("GET", "/api/bans", headers=sim.browser())[2]["bans"]
    assert bans[-1]["players"] == target["name"] and bans[-1]["reason"] == "griefing (by the moderator)"
    assert 86000 < bans[-1]["until"] - time.time() <= 86400


def test_wrong_passwords_lock_the_address_out(fresh):
    sim = fresh("--still", "--tick", "0.05")
    for _ in range(5):
        status, _, body = sim.request("POST", "/api/login", {"password": "wrong"})
        assert (status, body["error"]["code"]) == (401, "denied")
    status, headers, body = sim.request("POST", "/api/login", {"password": "opence-admin"})
    assert (status, body["error"]["code"]) == (429, "locked")
    assert 590 <= int(headers["Retry-After"]) <= 601
    assert body["error"]["retry_after"] == int(headers["Retry-After"])


def test_thirty_password_checks_a_minute_in_all(fresh):
    sim = fresh("--still", "--tick", "0.05")
    for _ in range(29):
        assert sim.request("POST", "/api/login", {"password": "opence-admin"})[0] == 200
    assert sim.request("POST", "/api/login", {"password": "opence-admin"})[0] == 200
    status, headers, body = sim.request("POST", "/api/login", {"password": "opence-admin"})
    assert (status, body["error"]["code"]) == (429, "rate_limited") and int(headers["Retry-After"]) > 0


def test_requests_the_server_refuses(sim):
    assert b" 400 " in sim.raw(b"GET /api/status HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n")
    assert b" 400 " in sim.raw(b"POST /api/login HTTP/1.1\r\nHost: x\r\nExpect: 100-continue\r\nContent-Length: 2\r\n\r\n{}")
    assert b" 413 " in sim.raw(b"GET / HTTP/1.1\r\nHost: x\r\nX-Pad: " + b"a" * 9000 + b"\r\n\r\n")
    assert b" 413 " in sim.raw(b"POST /api/login HTTP/1.1\r\nHost: x\r\nContent-Length: 70000\r\n\r\n", read=True)
    assert b" 400 " in sim.raw(b"GET /../etc/passwd HTTP/1.1\r\nHost: x\r\n\r\n")
    assert b" 400 " in sim.raw(b"GET / HTTP/2.0\r\nHost: x\r\n\r\n")
    status, headers, body = sim.request("PATCH", "/api/status")
    assert (status, body["error"]["code"]) == (405, "method")
    status, headers, body = sim.request("DELETE", "/api/status", headers=sim.browser())
    assert (status, body["error"]["code"], headers["Allow"]) == (405, "method", "GET, HEAD")
    assert sim.request("GET", "/api/nothing")[0] == 401
    assert sim.request("GET", "/api/nothing", headers=sim.browser())[0] == 404
    status, _, body = sim.request("POST", "/api/say", raw=b"not json", headers=sim.browser())
    assert (status, body["error"]["code"]) == (400, "bad_request")
    status, _, body = sim.request("POST", "/api/say", {"text": ""}, headers=sim.browser())
    assert (status, body["error"]["code"], body["error"]["field"]) == (400, "schema", "text")


def test_keep_alive_answers_several_requests_on_one_connection(sim):
    answer = sim.raw(b"GET /healthz HTTP/1.1\r\nHost: x\r\n\r\nGET /healthz HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
    assert answer.count(b"HTTP/1.1 200 OK") == 2


def test_head_is_get_without_its_body(sim):
    answer = sim.raw(b"HEAD /healthz HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
    head, _, body = answer.partition(b"\r\n\r\n")
    assert head.startswith(b"HTTP/1.1 200 OK") and body == b""
    length = int(re.search(rb"Content-Length: (\d+)", head).group(1))
    assert length == len(sim.raw(b"GET /healthz HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
                         .partition(b"\r\n\r\n")[2])
    cookie = sim.browser()["Cookie"].encode()
    answer = sim.raw(b"HEAD /api/status HTTP/1.1\r\nHost: x\r\nCookie: " + cookie + b"\r\nConnection: close\r\n\r\n")
    assert answer.startswith(b"HTTP/1.1 200 OK") and answer.endswith(b"\r\n\r\n")


def test_behind_the_trusted_proxy(fresh):
    sim = fresh("--still", "--tick", "0.05", "--trusted-proxy", "127.0.0.1")
    proxied = {"X-Forwarded-For": "198.51.100.7, 203.0.113.9:51234", "X-Forwarded-Proto": "https",
               "X-Forwarded-Host": "halo.example.org"}
    _, cookie = sim.login(**proxied)
    assert cookie.endswith("; Secure")
    # (the forwarded host is the page's own origin, whatever the Host)
    headers = sim.browser()
    headers = {**headers, **proxied, "Origin": "https://halo.example.org"}
    assert sim.request("POST", "/api/say", {"text": "hi"}, headers=headers)[0] == 200
    # (the address locked out is the last entry's: another client goes on signing in)
    for _ in range(5):
        sim.request("POST", "/api/login", {"password": "wrong"}, headers={"X-Forwarded-For": "[2001:db8::5]:443"})
    status, _, body = sim.request("POST", "/api/login", {"password": "opence-admin"},
                                  headers={"X-Forwarded-For": "2001:db8::5"})
    assert (status, body["error"]["code"]) == (429, "locked")
    assert sim.request("POST", "/api/login", {"password": "opence-admin"}, headers=proxied)[0] == 200


def test_each_address_has_an_allowance(fresh):
    sim = fresh("--still", "--tick", "0.05", "--request-limit")
    for _ in range(59):
        assert sim.request("GET", "/api/status")[0] == 401
    # (a WebSocket upgrade counts as a request)
    connect(sim).close()
    status, headers, body = sim.request("GET", "/api/status")
    assert (status, body["error"]["code"], headers["Retry-After"]) == (429, "rate_limited", "60")
    with pytest.raises(opence_rcon.RconError, match=r"429"):
        connect(sim)
    assert sim.request("GET", "/healthz")[0] == 200


def test_player_actions_check_the_slot_and_name(sim):
    headers = sim.browser()
    players = sim.request("GET", "/api/players", headers=headers)[2]["players"]
    first, second = players[0], players[1]
    status, _, body = sim.request("POST", f"/api/players/{first['slot']}/tell", {"name": second["name"], "text": "x"},
                                  headers=headers)
    assert (status, body["error"]["code"], body["error"]["name"]) == (409, "changed", first["name"])
    status, _, body = sim.request("POST", f"/api/players/{first['slot']}/tell", {"name": first["name"], "text": "hello"},
                                  headers=headers)
    assert status == 200 and body == {"ok": True, "lines": []}
    status, _, body = sim.request("POST", "/api/players/99/kick", {"name": "Nobody"}, headers=headers)
    assert (status, body["error"]["code"]) == (404, "not_found")
    status, _, body = sim.request("POST", f"/api/players/{first['slot']}/mute", {"name": first["name"], "minutes": 5000},
                                  headers=headers)
    assert (status, body["error"]["code"], body["error"]["field"]) == (400, "schema", "minutes")


def test_the_rotation_is_validated_whole(sim):
    headers = sim.browser()
    status, _, rotation = sim.request("GET", "/api/rotation", headers=headers)
    assert set(rotation) == {"ok", "index", "entries", "next"}
    assert set(rotation["entries"][0]) == {"text", "map", "gametype", "rules", "rejected"}
    assert any(entry["rejected"] for entry in rotation["entries"])
    status, _, body = sim.request("PUT", "/api/rotation", {"entries": [
        "bloodgulch:ctf", "nowhere:slayer", "prisoner:slayer+lives=500", "sidewinder:ctf+moving_hill=true", "ratrace:nothing"]},
        headers=headers)
    assert (status, body["error"]["code"]) == (400, "rejected")
    assert [item["index"] for item in body["error"]["rejected"]] == [1, 2, 3, 4]
    assert sim.request("GET", "/api/rotation", headers=headers)[2] == rotation
    status, _, body = sim.request("PUT", "/api/rotation", {"entries": [
        "bloodgulch:ctf", "sidewinder:team_slayer+score=50+time=15", "custom_maps\\hugeass:swat"]}, headers=headers)
    # (a rule by its full name, its value a whole number: true 1, a choice its index)
    assert status == 200 and [entry["rules"] for entry in body["entries"]] == [
        {}, {"score_to_win": 50, "time_limit": 15}, {}]
    status, _, body = sim.request("PUT", "/api/rotation", {"entries": ["bloodgulch:ctf+ff=off+assault=true"]},
                                  headers=headers)
    assert status == 200 and body["entries"][0]["rules"] == {"friendly_fire": 1, "assault": 1}
    status, _, body = sim.request("PUT", "/api/rotation", {"entries": ["bloodgulch:ctf"] * 10 + ["x" * 256]},
                                  headers=headers)
    assert (status, body["error"]["code"]) == (400, "schema")
    status, _, body = sim.request("POST", "/api/rotation/play", {"map": "nowhere", "gametype": "ctf"}, headers=headers)
    assert (status, body["error"]["code"]) == (400, "failed")
    assert body["error"]["message"].startswith("sv_map: there is no map nowhere") and body["lines"]
    status, _, body = sim.request("POST", "/api/rotation/play", {"map": "bloodgulch ctf"}, headers=headers)
    assert (status, body["error"]["code"]) == (400, "schema")


def test_maps_have_titles_and_players_a_ping_once_playing(sim):
    headers = sim.browser()
    maps = {item["name"]: item for item in sim.request("GET", "/api/maps", headers=headers)[2]["maps"]}
    assert set(maps["bloodgulch"]) == {"name", "level", "kind", "title"} and maps["bloodgulch"]["title"] == "Blood Gulch"
    assert maps["bigassv2"]["title"] == "bigassv2" and maps["bigassv2"]["kind"] == "custom"
    status = sim.request("GET", "/api/status", headers=headers)[2]
    assert "frame_ms_avg" in status and (status["team_scores"] is None) == (not status["teams"])
    players = sim.request("GET", "/api/players", headers=headers)[2]["players"]
    assert "discord" not in players[0] and all(isinstance(player["ping"], int) for player in players)


def test_gametype_files_are_validated_with_a_line(sim):
    headers = sim.browser()
    status, _, gametypes = sim.request("GET", "/api/gametypes", headers=headers)
    assert {"ctf", "slayer_pro", "crazy_king", "team_rally", "swat", "hardcore_ctf"} <= set(gametypes["builtin"])
    assert gametypes["files"][0].keys() == {"name", "base", "description", "error"}
    assert any("-" in item["name"] for item in gametypes["files"])
    fields = gametypes["schema"]["fields"]
    assert {"key", "alias", "engine", "table", "type", "min", "max", "choices", "default", "description"} == \
        set(fields[0])
    assert {"team_vehicles", "goal_radar", "hardcore"} <= {field["key"] for field in fields}
    assert {field["type"] for field in fields} == {"integer", "boolean", "choice"}
    text = 'base = "oddball"\nname = "Ball"\n\n[oddball]\nball_spawn_count = 4\n'
    assert sim.request("PUT", "/api/gametypes/ball", {"text": text}, headers=headers)[0] == 200
    assert sim.request("GET", "/api/gametypes/ball", headers=headers)[2] == {"ok": True, "name": "ball", "text": text}
    for bad, line in ((text + "[ctf]\nassault = true\n", 6), (text.replace("4", "40"), 5), ("name = \"x\"\n", 1),
                      (text + "lives = 3\n", 6), ("base = ctf\n", 1)):
        status, _, body = sim.request("PUT", "/api/gametypes/ball", {"text": bad}, headers=headers)
        assert (status, body["error"]["code"], body["error"]["line"]) == (400, "invalid", line), (bad, body)
    status, _, body = sim.request("PUT", "/api/gametypes/ball", {"text": 'base = "slayer"\ndescription = "x"\n'},
                                  headers=headers)
    assert (status, body["error"]["code"], body["error"]["line"]) == (400, "invalid", 2)
    assert sim.request("DELETE", "/api/gametypes/ball", headers=headers)[0] == 200
    assert sim.request("GET", "/api/gametypes/ball", headers=headers)[0] == 404
    assert sim.request("PUT", "/api/gametypes/my-ball", {"text": text}, headers=headers)[0] == 200
    assert sim.request("DELETE", "/api/gametypes/my-ball", headers=headers)[0] == 200
    status, _, body = sim.request("GET", "/api/gametypes/Bad.Name", headers=headers)
    assert (status, body["error"]["code"]) == (404, "not_found")


def test_settings_secrets_and_pinned(sim):
    headers = sim.browser()
    settings = {item["name"]: item for item in sim.request("GET", "/api/settings", headers=headers)[2]["settings"]}
    assert settings["server.web_password"]["value"] == "" and settings["server.web_password"]["set"] is True
    assert settings["server.frame_rate"]["pinned"] and settings["server.frame_rate"]["env"] == "HALO_SERVER_FRAME_RATE"
    assert "server.web_port" not in settings and "server.web_viewer_password" not in settings
    assert set(settings["server.name"]) == {"name", "type", "value", "default", "comment", "env", "pinned", "secret",
                                            "choices", "min", "max"}
    # (typed values, and choices, min and max where they are known)
    assert settings["server.public"]["value"] is True and settings["server.max_players"]["value"] == 16
    assert settings["server.name"]["value"] == "OpenCE Server" and settings["server.name"]["choices"] is None
    assert settings["server.log_format"]["choices"] == ["text", "json"]
    assert (settings["server.max_players"]["min"], settings["server.max_players"]["max"]) == (2, 127)
    status, _, body = sim.request("PUT", "/api/settings", {
        "server.motd": "Welcome", "server.frame_rate": 30, "server.max_players": 500, "server.nothing": 1,
        "server.log_addresses": True, "server.region": "eu\nwest"}, headers=headers)
    assert status == 200 and body["applied"] == ["server.motd"]
    assert [(error["name"], error["code"]) for error in body["errors"]] == [
        ("server.frame_rate", "pinned"), ("server.max_players", "schema"), ("server.nothing", "not_found"),
        ("server.log_addresses", "forbidden"), ("server.region", "schema")]
    assert "needs the owner role" in body["errors"][3]["message"]
    # (a value as JSON or as its text)
    status, _, body = sim.request("PUT", "/api/settings", {"server.public": "false", "server.max_players": "12",
                                                           "server.postgame_seconds": 9}, headers=headers)
    assert body["errors"] == [] and len(body["applied"]) == 3
    settings = {item["name"]: item for item in sim.request("GET", "/api/settings", headers=headers)[2]["settings"]}
    assert (settings["server.public"]["value"], settings["server.max_players"]["value"]) == (False, 12)
    sim.request("PUT", "/api/settings", {"server.public": True, "server.max_players": 16}, headers=headers)
    status, _, body = sim.request("POST", "/api/command", {"line": "sv_set server.frame_rate 30"}, headers=headers)
    assert (status, body["error"]["code"]) == (409, "pinned")
    status, _, body = sim.request("POST", "/api/command", {"line": "sv_set server.auto_update install"},
                                  headers=headers)
    assert (status, body["error"]["code"]) == (403, "forbidden")


def test_the_log_and_events_cursors(sim):
    headers = sim.browser()
    status, _, first = sim.request("GET", "/api/log?after=0&limit=3", headers=headers)
    assert [line["n"] for line in first["lines"]] == [1, 2, 3] and first["next"] == 3 and first["missed"] == 0
    assert set(first["lines"][0]) == {"n", "time", "level", "cat", "text"}
    status, _, second = sim.request("GET", f"/api/log?after={first['next']}&limit=2", headers=headers)
    assert [line["n"] for line in second["lines"]] == [4, 5]
    status, _, beyond = sim.request("GET", "/api/log?after=99999999", headers=headers)
    assert beyond["lines"][0]["n"] == 1
    status, _, events = sim.request("GET", "/api/events?after=0", headers=headers)
    assert events["events"][0]["event"] == "server_started"
    assert all(event["event"] not in ("status", "log") for event in events["events"])
    # (log and status events take no number of the events')
    numbers = [event["seq"] for event in events["events"]]
    assert numbers == list(range(numbers[0], numbers[0] + len(numbers)))
    last = events["next"]
    sim.request("POST", "/api/command", {"line": "sv_reload"}, headers=headers)
    status, _, later = sim.request("GET", f"/api/events?after={last}", headers=headers)
    assert [event["event"] for event in later["events"]] == ["reloaded"]


# ---------- what fleets use: the owner's role from loopback, bans of addresses, drains, addresses in events,
# the server's identity


def test_the_owner_signs_in_from_loopback(fresh):
    sim = fresh("--still", "--tick", "0.05")
    owner, cookie = sim.login("opence-owner")
    assert owner["role"] == "owner"
    headers = {"Authorization": "Bearer " + owner["token"]}
    # (the addresses, the owner's settings, the game's own console, quit)
    assert "address" in sim.request("GET", "/api/players", headers=headers)[2]["players"][0]
    status, _, body = sim.request("PUT", "/api/settings", {"server.log_addresses": True, "server.auto_update": "check"},
                                  headers=headers)
    assert status == 200 and body["applied"] == ["server.log_addresses", "server.auto_update"] and not body["errors"]
    status, _, body = sim.request("POST", "/api/command", {"line": "game_speed 1"}, headers=headers)
    assert status == 200 and body["ok"], body
    assert sim.request("POST", "/api/command", {"line": "sv_set server.status_file /tmp/x"}, headers=headers)[0] == 200
    after = sim.request("GET", "/api/events?after=0", headers=headers)[2]["next"]
    status, _, body = sim.request("POST", "/api/command", {"line": "quit"}, headers=headers)
    assert status == 200 and body["ok"]
    events = sim.request("GET", f"/api/events?after={after}", headers=headers)[2]["events"]
    assert [(event["event"], event["reason"], event["code"]) for event in events] == [("shutdown", "quit", 0)]
    # (the RCON's sign-in with it)
    assert opence_rcon.Rcon(sim.url, password="opence-owner").role == "owner"


def test_the_owner_password_from_elsewhere_is_forbidden(fresh):
    # (through the trusted proxy on this machine: not the server's own machine's request)
    sim = fresh("--still", "--tick", "0.05", "--trusted-proxy", "127.0.0.1")
    for _ in range(5):
        status, _, body = sim.request("POST", "/api/login", {"password": "opence-owner"})
        assert (status, body["error"]["code"]) == (403, "forbidden")
    # (counted as wrong passwords are)
    status, _, body = sim.request("POST", "/api/login", {"password": "opence-admin"})
    assert (status, body["error"]["code"]) == (429, "locked")
    with pytest.raises(opence_rcon.RconError):
        opence_rcon.Rcon(sim.url, password="opence-owner")


def test_a_ban_of_an_address_needs_no_player(fresh):
    sim = fresh("--still", "--tick", "0.05")
    admin = sim.browser()
    moderator = sim.browser("opence-mod")
    owner = {"Authorization": "Bearer " + sim.login("opence-owner")[0]["token"]}
    present = sim.request("GET", "/api/players", headers=owner)[2]["players"][0]
    before = len(sim.request("GET", "/api/bans", headers=admin)[2]["bans"])
    status, _, body = sim.request("POST", "/api/bans", {"ip": "198.51.100.9"}, headers=moderator)
    assert (status, body["error"]["code"]) == (403, "forbidden")
    status, _, body = sim.request("POST", "/api/bans", {"ip": "198.51.100.9", "names": "Ghost", "duration": "1d",
                                                        "reason": "griefing"}, headers=admin)
    assert (status, body) == (200, {"ok": True, "n": before})
    bans = sim.request("GET", "/api/bans", headers=admin)[2]["bans"]
    assert (bans[before]["ip"], bans[before]["hwid"], bans[before]["players"]) == ("198.51.100.9", "none", "Ghost")
    assert bans[before]["reason"] == "griefing (by the admin)" and 86000 < bans[before]["until"] - time.time() <= 86400
    # (a present player's hardware id: dropped at once, banned)
    status, _, body = sim.request("POST", "/api/bans", {"hwid": present["hwid"].upper()}, headers=admin)
    assert (status, body["n"]) == (200, before + 1)
    assert present["slot"] not in [player["slot"] for player in sim.request("GET", "/api/players", headers=admin)[2]
                                   ["players"]]
    events = sim.request("GET", "/api/events?after=0", headers=admin)[2]["events"]
    assert any(event["event"] == "player_left" and event["name"] == present["name"] and event["reason"] == "banned"
               for event in events)
    assert any(event["event"] == "ban" and event["name"] == present["name"] and event["by"] == "admin"
               for event in events)
    for bad in ({}, {"ip": "300.1.1.1"}, {"hwid": "xyz"}, {"ip": "198.51.100.9", "duration": "soon"}, {"ip": 5}):
        status, _, body = sim.request("POST", "/api/bans", bad, headers=admin)
        assert (status, body["error"]["code"]) == (400, "schema"), bad
    status, _, body = sim.request("POST", "/api/command", {"line": "sv_ban_address 203.0.113.250 2h spam"},
                                  headers=admin)
    assert status == 200 and body["lines"] == [f"ban {before + 2} added, for 120 minutes"]
    assert sim.request("DELETE", f"/api/bans/{before + 2}", headers=admin)[0] == 200


def test_a_drain_until_empty_and_its_cancel(fresh):
    sim = fresh("--still", "--tick", "0.05")
    admin = sim.browser()
    owner = {"Authorization": "Bearer " + sim.login("opence-owner")[0]["token"]}
    assert sim.request("GET", "/api/status", headers=owner)[2]["drain"] is None
    status, _, body = sim.request("POST", "/api/stop", {"after": "empty"}, headers=admin)
    assert (status, body["error"]["code"]) == (403, "forbidden")
    for bad in ({"after": "later"}, {"after": "game", "restart": "yes"}, {}):
        status, _, body = sim.request("POST", "/api/stop", bad, headers=owner)
        assert (status, body["error"]["code"]) == (400, "schema"), bad
    status, _, body = sim.request("POST", "/api/stop", {"cancel": True}, headers=owner)
    assert (status, body["error"]["code"]) == (400, "failed")
    after = sim.request("GET", "/api/events?after=0", headers=owner)[2]["next"]
    status, _, body = sim.request("POST", "/api/stop", {"after": "game", "notice": "a restart soon", "restart": True},
                                  headers=owner)
    assert status == 200 and body["ok"] and body["lines"]
    drain = sim.request("GET", "/api/status", headers=owner)[2]["drain"]
    assert drain["after"] == "game" and drain["restart"] is True and abs(drain["since"] - time.time()) < 10
    status, _, body = sim.request("POST", "/api/stop", {"cancel": True}, headers=owner)
    assert status == 200 and sim.request("GET", "/api/status", headers=owner)[2]["drain"] is None
    status, _, body = sim.request("POST", "/api/stop", {"after": "empty"}, headers=owner)
    assert status == 200
    for player in sim.request("GET", "/api/players", headers=admin)[2]["players"]:
        assert sim.request("POST", f"/api/players/{player['slot']}/kick", {"name": player["name"]}, headers=admin)[0] == 200
    deadline = time.monotonic() + 10
    while True:
        events = sim.request("GET", f"/api/events?after={after}", headers=owner)[2]["events"]
        if any(event["event"] == "shutdown" for event in events) or time.monotonic() > deadline:
            break
        time.sleep(0.05)
    drains = [(event["after"], event["restart"], event["notice"]) for event in events if event["event"] == "drain"]
    assert drains == [("game", True, "a restart soon"), ("cancel", True, ""), ("empty", False, "")]
    assert [(event["reason"], event["code"]) for event in events if event["event"] == "shutdown"] == [("drained", 0)]
    assert sim.request("GET", "/api/status", headers=owner)[2]["drain"] is None


def test_a_stop_exits_with_its_code(fresh):
    sim = fresh("--still", "--tick", "0.05", "--exit-on-stop")
    owner = opence_rcon.Rcon(sim.url, password="opence-owner")
    reply = owner.command("sv_restart now")
    assert reply["ok"] and reply["lines"] == ["stopping (exit code 5)"]
    assert sim.process.wait(10) == 5


def test_addresses_in_the_player_events(fresh):
    sim = fresh("--still", "--tick", "0.05")
    owner = {"Authorization": "Bearer " + sim.login("opence-owner")[0]["token"]}
    admin = {"Authorization": "Bearer " + sim.login()[0]["token"]}
    joined = [event for event in sim.request("GET", "/api/events?after=0", headers=owner)[2]["events"]
              if event["event"] == "player_joined"]
    assert joined and all(re.fullmatch(r"\d+\.\d+\.\d+\.\d+", event["address"]) and event["hwid"] for event in joined)
    assert not any("address" in event for event in sim.request("GET", "/api/events?after=0", headers=admin)[2]["events"])
    rcon = opence_rcon.Rcon(sim.url, password="opence-owner")
    rcon.subscribe(["player"], after=0)
    message = rcon.receive(5)
    while message.get("event") != "player_joined":
        message = rcon.receive(5)
    assert message["replay"] and "address" in message and "hwid" in message
    players = sim.request("GET", "/api/players", headers=admin)[2]["players"]
    sim.request("POST", f"/api/players/{players[0]['slot']}/kick", {"name": players[0]["name"]}, headers=admin)
    message = rcon.receive(5)
    while message.get("event") != "player_left":
        message = rcon.receive(5)
    assert message["name"] == players[0]["name"] and "address" in message and "replay" not in message
    # (with server.log_addresses, every role's)
    logged = fresh("--still", "--tick", "0.05", "--log-addresses")
    viewer = {"Authorization": "Bearer " + logged.login("opence-mod")[0]["token"]}
    assert all("address" in event for event in logged.request("GET", "/api/events?after=0", headers=viewer)[2]["events"]
               if event["event"] == "player_joined")


def test_the_identity_in_the_status_and_server_started(fresh):
    sim = fresh("--still", "--tick", "0.05", "--instance", "eu-1")
    headers = {"Authorization": "Bearer " + sim.login("opence-mod")[0]["token"]}
    status = sim.request("GET", "/api/status", headers=headers)[2]
    identity = {key: status[key] for key in ("pid", "started", "instance", "region", "tunnel_port", "publisher_key_id",
                                             "web_port")}
    assert identity["pid"] == sim.process.pid and identity["instance"] == "eu-1" and identity["web_port"] == sim.port
    assert re.fullmatch(r"[0-9a-f]{16}", identity["publisher_key_id"]) and abs(identity["started"] - time.time()) < 60
    assert status["capabilities"] == opence_rcon.Rcon(sim.url, password="opence-admin").hello["capabilities"]
    started = sim.request("GET", "/api/events?after=0", headers=headers)[2]["events"][0]
    assert started["event"] == "server_started" and {key: started[key] for key in identity} == identity


# ---------- the RCON


def connect(sim, **headers):
    return opence_rcon.WebSocket(sim.url, headers=headers)


def test_rcon_hello_auth_commands_and_events(sim):
    rcon = opence_rcon.Rcon(sim.url, password="opence-admin", by="tests")
    assert rcon.hello["type"] == "hello" and rcon.hello["protocol"] == 1
    assert rcon.hello["capabilities"] == ["events", "status", "rotation", "settings", "bans", "owner_web", "ban_address",
                                          "stop", "event_addresses", "identity"]
    assert rcon.role == "admin"
    reply = rcon.command("sv_status")
    assert reply["ok"] and reply["id"] == 1 and reply["lines"][0].startswith("OpenCE Server: playing")
    # (a command that failed: its lines and ok false, as the real server answers; no error object)
    refused = rcon.command("sv_map nowhere")
    assert refused["ok"] is False and "error" not in refused
    assert refused["lines"][0].startswith("sv_map: there is no map nowhere")
    unknown = rcon.command("no_such_command")
    assert unknown["ok"] is False and "not a command of the server's" in unknown["lines"][0]
    rcon.subscribe(["player", "game", "status", "moderation"], after=0)
    seen = []
    while not any(message.get("event") == "status" for message in seen):
        seen.append(rcon.receive(5))
    assert seen[0]["event"] == "server_started" and seen[0]["replay"] is True
    assert all(message["replay"] for message in seen if message.get("event") not in ("status",))
    rcon.socket.send({"type": "ping"})
    rcon.socket.send({"type": "nonsense", "id": 9})
    answers = []
    while len([a for a in answers if a["type"] in ("pong", "error")]) < 2:
        answers.append(rcon.receive(5))
    assert {"type": "pong"} in answers
    assert next(a for a in answers if a["type"] == "error")["id"] == 9
    players = sim.request("GET", "/api/players", headers=sim.browser())[2]["players"]
    kicked = rcon.command(f"kick {players[-1]['slot']}")
    assert kicked["ok"] and kicked["lines"] == []
    reasons = {}
    while "kick" not in reasons or "player_left" not in reasons:
        message = rcon.receive(5)
        if message.get("event") in ("kick", "player_left"):
            reasons[message["event"]] = message
    assert reasons["kick"]["by"] == "admin" and reasons["kick"]["name"] == players[-1]["name"]
    assert reasons["player_left"]["reason"] == "kicked"
    rcon.close()


def test_rcon_command_shapes(sim):
    rcon = opence_rcon.Rcon(sim.url, password="opence-admin")
    # (a command and its words: a word with a space quoted)
    rcon.socket.send({"type": "command", "id": "a", "command": "sv_motd", "args": ["two words", "and"]})
    reply = rcon.receive(5)
    assert reply == {"type": "reply", "id": "a", "lines": ['server.motd = "two words" and'], "ok": True}
    rcon.socket.send({"type": "command", "id": 2, "line": "help " + "x" * 520})
    reply = rcon.receive(5)
    assert (reply["ok"], reply["error"]["code"], reply["lines"]) == (False, "bad_request", [])
    rcon.socket.send({"type": "command", "line": "sv_version"})
    assert rcon.receive(5)["id"] is None
    rcon.socket.send({"type": "subscribe", "events": ["nothing"], "id": 7})
    assert rcon.receive(5) == {"type": "error", "id": 7, "error": {"code": "bad_request",
                                                                   "message": "events is a list of event groups"}}
    rcon.close()


def test_rcon_refuses_a_wrong_password_and_closes(sim):
    socket_ = connect(sim)
    assert socket_.receive(5)["type"] == "hello"
    socket_.send({"type": "command", "id": 1, "line": "sv_status"})
    assert socket_.receive(5)["error"]["code"] == "denied"
    socket_.send({"type": "auth", "password": "wrong"})
    assert socket_.receive(5) == {"type": "auth", "ok": False, "error": {"code": "denied", "message": "wrong password"}}
    with pytest.raises(opence_rcon.Closed) as closed:
        socket_.receive(5)
    assert closed.value.code == 1008


def test_rcon_signs_in_by_the_cookie_from_the_page_only(sim):
    body, cookie = sim.login("opence-mod")
    socket_ = connect(sim, Cookie=cookie.split(";")[0], Origin=sim.url)
    assert socket_.receive(5)["type"] == "hello"
    assert socket_.receive(5) == {"type": "auth", "ok": True, "role": "moderator"}
    with pytest.raises(opence_rcon.RconError, match=r"403"):
        connect(sim, Cookie=cookie.split(";")[0], Origin="http://evil.example")
    with pytest.raises(opence_rcon.RconError, match=r"400.*subprotocol"):
        opence_rcon.WebSocket(sim.url, protocols=("chat",))
    token = opence_rcon.Rcon(sim.url, token=body["token"])
    assert token.role == "moderator"


def test_rcon_frames_from_the_client(sim):
    socket_ = connect(sim)
    socket_.receive(5)
    # a message in three fragments, a ping between them
    message = json.dumps({"type": "auth", "password": "opence-admin"}).encode()
    socket_.send_frame(0x1, message[:5], fin=False)
    socket_.send_frame(0x9, b"hi")
    socket_.send_frame(0x0, message[5:12], fin=False)
    socket_.send_frame(0x0, message[12:])
    assert socket_.receive(5) == {"type": "auth", "ok": True, "role": "admin"}
    socket_.send_frame(0x1, b'{"type":"ping"}', mask=False)
    with pytest.raises(opence_rcon.Closed) as closed:
        while True:
            socket_.receive(5)
    assert closed.value.code == 1002
    big = connect(sim)
    big.receive(5)
    big.send_frame(0x1, b" " * (64 * 1024 + 1))
    with pytest.raises(opence_rcon.Closed) as closed:
        big.receive(5)
    assert closed.value.code == 1009


def test_eight_connections_at_most(fresh):
    sim = fresh("--still", "--tick", "0.05")
    sockets = [connect(sim) for _ in range(8)]
    with pytest.raises(opence_rcon.RconError, match=r"503"):
        connect(sim)
    for socket_ in sockets:
        socket_.close()


def test_rcon_faults_fragments_and_big_replies(fresh):
    sim = fresh("--still", "--tick", "0.05", "--fragment", "--big-replies")
    rcon = opence_rcon.Rcon(sim.url, password="opence-admin")
    reply = rcon.command("sv_version")
    assert reply["ok"] and len(json.dumps(reply)) > 1 << 20


def test_rcon_slow_consumer_overflows(fresh):
    sim = fresh("--still", "--tick", "0.05", "--slow-consumer")
    rcon = opence_rcon.Rcon(sim.url, password="opence-admin")
    rcon.subscribe(["log"])
    messages = []
    with pytest.raises(opence_rcon.Closed) as closed:
        while True:
            messages.append(rcon.receive(10))
    assert closed.value.code == 1013
    assert messages[-1] == {"type": "overflow"}


def test_rcon_drop_cuts_the_socket(fresh):
    sim = fresh("--still", "--tick", "0.05", "--drop", "--drop-after", "0.5")
    rcon = opence_rcon.Rcon(sim.url, password="opence-admin")
    with pytest.raises(opence_rcon.RconError):
        while True:
            rcon.receive(5)


# ---------- the RCON client


def rcon_cli(sim, *args):
    return subprocess.run([sys.executable, str(TOOLS / "opence_rcon.py"), "--url", sim.url, *args],
                          capture_output=True, text=True, timeout=30, env={**os.environ, "OPENCE_RCON_PASSWORD": ""})


def test_the_rcon_client_exit_codes(sim):
    done = rcon_cli(sim, "--password", "opence-admin", "sv_version")
    assert done.returncode == 0 and done.stdout.startswith("OpenCE build-206")
    failed = rcon_cli(sim, "--password", "opence-mod", "sv_set", "server.name", "X")
    assert failed.returncode == 1 and "admin role" in failed.stdout + failed.stderr
    refused = rcon_cli(sim, "--password", "wrong", "sv_version")
    assert refused.returncode == 2 and "refused" in refused.stderr
    unreachable = subprocess.run([sys.executable, str(TOOLS / "opence_rcon.py"), "--url", "http://127.0.0.1:1",
                                  "--password", "x", "sv_version"], capture_output=True, text=True, timeout=30)
    assert unreachable.returncode == 2
    as_json = rcon_cli(sim, "--password", "opence-admin", "--json", "sv_players", "--json")
    reply = json.loads(as_json.stdout)
    assert reply["ok"] and isinstance(json.loads(reply["lines"][0])["players"], list)


def test_a_session_ends_after_it_is_idle():
    session = server_sim.Session("admin", "tests")
    assert session.valid(session.created + 60)
    assert not session.valid(session.used + 30 * 60 + 1)
    session.used = session.created + 11 * 3600
    assert not session.valid(session.created + 12 * 3600 + 1)


def test_rotation_grammar():
    class Options:
        seed = 1
        port = 0
        password = "x" * 8
        moderator_password = ""
        viewer_password = ""
        log_addresses = False
        verbose = False

    sim = server_sim.Sim(Options())
    assert sim.parse_entry("bloodgulch")["gametype"] == "slayer"
    assert sim.parse_entry("BloodGulch:CTF")["map"] == "bloodgulch"
    assert sim.parse_entry("levels\\test\\bloodgulch\\bloodgulch:ctf")["rejected"] is None
    assert sim.parse_entry("custom_maps\\hugeass:king+moving_hill=1")["rejected"] is None
    assert sim.parse_entry("custom_maps/hugeass:koth")["map"] == "hugeass"
    assert sim.parse_entry("hugeass:crazy_king")["rejected"] is None
    assert sim.parse_entry("bloodgulch:ctf+moving_hill=1")["rejected"] == \
        "moving_hill is a king gametype's rule, and this one is ctf"
    assert "takes 0 to 99, not 100" in sim.parse_entry("bloodgulch:slayer+lives=100")["rejected"]
    assert sim.parse_entry("bloodgulch:slayer+bogus=1")["rejected"] == 'no rule is named "bogus"'
    assert sim.parse_entry("bloodgulch:file=mine")["rejected"] == \
        "gametypes/mine.gametype is missing, or not a saved gametype"
    assert sim.parse_entry("bloodgulch:pistols-only")["variant"] == "Pistols"
    assert sim.parse_entry("# a comment") is None
    assert "longer than 255" in sim.parse_entry("bloodgulch:slayer+" + "x" * 300)["rejected"]
    entries, truncated = sim.parse_rotation("bloodgulch:ctf; # a comment, still\nprisoner\n  # another\n"
                                            "sidewinder:team_slayer")
    assert [entry["map"] for entry in entries] == ["bloodgulch", "prisoner", "sidewinder"] and not truncated
    assert sim.parse_rotation(", ".join(["bloodgulch:ctf"] * 400))[1]
    assert server_sim.split_words('tell "Mil Two" hello there') == ["tell", "Mil Two", "hello", "there"]
    assert server_sim.parse_duration("2h") == 7200 and server_sim.parse_duration("forever") == 0
    assert server_sim.parse_duration("2w") == 14 * 86400 and server_sim.parse_duration("90") == 5400
    assert server_sim.parse_duration("2x") == -1 and server_sim.parse_duration("11y") == -1
