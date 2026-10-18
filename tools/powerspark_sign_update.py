#!/usr/bin/env python3
"""Signs a PowerSpark release zip and adds it to the update feed (the appcast).

Run this on your own Mac, never in CI: the private key must not leave it.

  python3 tools/powerspark_sign_update.py keygen --out DIR
      Makes the release key pair in DIR: powerspark-update-key.private (keep
      it secret and back it up) and powerspark-update-key.public (the line
      for branding.plist's UpdatePublicKey).

  python3 tools/powerspark_sign_update.py sign PowerSpark-0.2.zip \\
      --key DIR/powerspark-update-key.private \\
      --url https://www.cytrusretro.com/powerspark/PowerSpark-0.2.zip \\
      [--notes notes.txt] [--appcast appcast.plist] [--min-os 10.4]
      Reads the version and build from the app inside the zip, signs the
      release, and puts its item into the appcast (a new file if there is
      none; an item of the same version and build is replaced). Upload the
      zip and the appcast to your site.

  python3 tools/powerspark_sign_update.py verify appcast.plist --public KEY
      Checks every item's signature against the public key.

The zip is made with: ditto -c -k --keepParent --norsrc PowerSpark.app PowerSpark-0.2.zip

The signed statement is the one port/mac/app/update_core.c checks:
  PowerSpark-update-1\\nversion=V\\nbuild=B\\nsha256=H\\nlength=L\\nurl=U\\nminimumSystemVersion=M\\n
Ed25519 is written out here (RFC 8032) so that nothing but Python 3 is
needed; signing takes a few milliseconds.
"""
import argparse
import base64
import hashlib
import os
import plistlib
import re
import stat
import sys
import zipfile

BUNDLE_ID = "org.opence.PowerSpark"
MAX_LENGTH = 1024 * 1024 * 1024
PRIVATE_NAME = "powerspark-update-key.private"
PUBLIC_NAME = "powerspark-update-key.public"

# ---------- Ed25519 (RFC 8032, section 6)

P = 2 ** 255 - 19
Q = 2 ** 252 + 27742317777372353535851937790883648493
D = -121665 * pow(121666, P - 2, P) % P
I = pow(2, (P - 1) // 4, P)


def _inv(x):
    return pow(x, P - 2, P)


def _recover_x(y, sign):
    xx = (y * y - 1) * _inv(D * y * y + 1) % P
    x = pow(xx, (P + 3) // 8, P)
    if (x * x - xx) % P != 0:
        x = x * I % P
    if (x * x - xx) % P != 0:
        return None
    if x & 1 != sign:
        x = P - x
    return x


_BY = 4 * _inv(5) % P
_BX = _recover_x(_BY, 0)
B = (_BX, _BY, 1, _BX * _BY % P)


def _add(p, q):
    a = (p[1] - p[0]) * (q[1] - q[0]) % P
    b = (p[1] + p[0]) * (q[1] + q[0]) % P
    c = 2 * p[3] * q[3] * D % P
    d = 2 * p[2] * q[2] % P
    e, f, g, h = b - a, d - c, d + c, b + a
    return (e * f % P, g * h % P, f * g % P, e * h % P)


def _mul(s, p):
    r = (0, 1, 1, 0)
    while s > 0:
        if s & 1:
            r = _add(r, p)
        p = _add(p, p)
        s >>= 1
    return r


def _compress(p):
    zi = _inv(p[2])
    x, y = p[0] * zi % P, p[1] * zi % P
    return (y | ((x & 1) << 255)).to_bytes(32, "little")


def _decompress(b):
    y = int.from_bytes(b, "little")
    sign = y >> 255
    y &= (1 << 255) - 1
    if y >= P:
        return None
    x = _recover_x(y, sign)
    return None if x is None else (x, y, 1, x * y % P)


def _expand(seed):
    h = hashlib.sha512(seed).digest()
    a = int.from_bytes(h[:32], "little")
    a &= (1 << 254) - 8
    a |= 1 << 254
    return a, h[32:]


def public_key(seed):
    a, _ = _expand(seed)
    return _compress(_mul(a, B))


def sign(seed, message):
    a, prefix = _expand(seed)
    pub = _compress(_mul(a, B))
    r = int.from_bytes(hashlib.sha512(prefix + message).digest(), "little") % Q
    big_r = _compress(_mul(r, B))
    k = int.from_bytes(hashlib.sha512(big_r + pub + message).digest(), "little") % Q
    s = (r + k * a) % Q
    return big_r + s.to_bytes(32, "little")


def verify(pub, message, signature):
    if len(pub) != 32 or len(signature) != 64:
        return False
    a = _decompress(pub)
    r = _decompress(signature[:32])
    s = int.from_bytes(signature[32:], "little")
    if a is None or r is None or s >= Q:
        return False
    k = int.from_bytes(hashlib.sha512(signature[:32] + pub + message).digest(), "little") % Q
    left = _mul(s, B)
    right = _add(r, _mul(k, a))
    return _compress(left) == _compress(right)


# ---------- the statement

def statement(item):
    return ("PowerSpark-update-1\nversion=%s\nbuild=%d\nsha256=%s\nlength=%d\nurl=%s\nminimumSystemVersion=%s\n" % (
        item["version"], item["build"], item["sha256"], item["length"], item["url"],
        item.get("minimumSystemVersion", ""))).encode("utf-8")


def read_seed(path):
    text = open(path).read()
    for line in text.splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            seed = base64.b64decode(line, validate=True)
            if len(seed) != 32:
                raise SystemExit("%s does not hold a 32-byte key" % path)
            return seed
    raise SystemExit("%s holds no key" % path)


def read_public(arg):
    text = open(arg).read().strip() if os.path.isfile(arg) else arg.strip()
    line = [l.strip() for l in text.splitlines() if l.strip() and not l.startswith("#")][0]
    key = base64.b64decode(line, validate=True)
    if len(key) != 32:
        raise SystemExit("the public key is not 32 bytes")
    return key


def app_facts(archive):
    """version, build and bundle id of the one app in the zip, with checks that it is PowerSpark"""
    with zipfile.ZipFile(archive) as z:
        names = [n for n in z.namelist() if not n.startswith("__MACOSX/")]
        tops = {n.split("/", 1)[0] for n in names if n.strip("/")}
        if len(tops) != 1 or not next(iter(tops)).endswith(".app"):
            raise SystemExit("the zip must hold one PowerSpark.app and nothing else at its top level (found %s)" %
                             ", ".join(sorted(tops)))
        app = next(iter(tops))
        for n in names:
            if n.startswith("/") or ".." in n.split("/"):
                raise SystemExit("the zip has a path that leaves the app: " + n)
        for need in ("Contents/Info.plist", "Contents/MacOS/PowerSpark", "Contents/MacOS/PowerSpark-game"):
            if app + "/" + need not in names:
                raise SystemExit("the zip's app lacks " + need)
        info = plistlib.loads(z.read(app + "/Contents/Info.plist"))
    if info.get("CFBundleIdentifier") != BUNDLE_ID:
        raise SystemExit("the app's bundle identifier is %r, not %s" % (info.get("CFBundleIdentifier"), BUNDLE_ID))
    version = str(info.get("CFBundleShortVersionString", ""))
    build = str(info.get("CFBundleVersion", ""))
    if not re.fullmatch(r"\d+(\.\d+)*", version) or not re.fullmatch(r"\d+", build):
        raise SystemExit("the app's version (%r) or build (%r) is not in the form the updater reads" % (version, build))
    return version, int(build)


# ---------- the commands

def cmd_keygen(args):
    os.makedirs(args.out, mode=0o700, exist_ok=True)
    private = os.path.join(args.out, PRIVATE_NAME)
    public = os.path.join(args.out, PUBLIC_NAME)
    if os.path.exists(private):
        raise SystemExit("%s exists; not replacing it. (If you lose it no copy of PowerSpark can be updated again.)" %
                         private)
    seed = os.urandom(32)
    fd = os.open(private, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "w") as f:
        f.write("# PowerSpark update signing key (Ed25519 seed, base64). SECRET: keep it out of every repository, "
                "and back it up.\n")
        f.write(base64.b64encode(seed).decode() + "\n")
    key = base64.b64encode(public_key(seed)).decode()
    with open(public, "w") as f:
        f.write(key + "\n")
    os.chmod(private, stat.S_IRUSR | stat.S_IWUSR)
    print("private key: " + private)
    print("public key:  " + key)


def cmd_sign(args):
    seed = read_seed(args.key)
    if not args.url.startswith("https://"):
        raise SystemExit("--url must be an https address")
    version, build = app_facts(args.archive)
    data = open(args.archive, "rb").read()
    if len(data) > MAX_LENGTH:
        raise SystemExit("the zip is larger than the updater accepts")
    item = {
        "version": version,
        "build": build,
        "url": args.url,
        "sha256": hashlib.sha256(data).hexdigest(),
        "length": len(data),
        "minimumSystemVersion": args.min_os,
    }
    if args.notes:
        item["notes"] = open(args.notes, encoding="utf-8").read().strip()
    item["signature"] = base64.b64encode(sign(seed, statement(item))).decode()
    assert verify(public_key(seed), statement(item), base64.b64decode(item["signature"]))
    feed = {"feed": "powerspark", "versions": []}
    if os.path.exists(args.appcast):
        with open(args.appcast, "rb") as f:
            feed = plistlib.load(f)
    feed["versions"] = [v for v in feed.get("versions", [])
                        if (v.get("version"), v.get("build")) != (version, build)]
    feed["versions"].append(item)
    feed["versions"].sort(key=lambda v: ([int(p) for p in str(v["version"]).split(".")], v.get("build", 0)))
    with open(args.appcast, "wb") as f:
        plistlib.dump(feed, f)
    print("%s: PowerSpark %s (build %d), %d bytes, sha256 %s..." % (args.appcast, version, build, len(data),
                                                                    item["sha256"][:16]))
    print("signed; upload the zip to " + args.url + " and the appcast to your site.")


def cmd_verify(args):
    pub = read_public(args.public)
    with open(args.appcast, "rb") as f:
        feed = plistlib.load(f)
    items = feed.get("versions", [feed])
    bad = 0
    for item in items:
        ok = verify(pub, statement(item), base64.b64decode(item.get("signature", "")))
        print("%s (build %s): %s" % (item.get("version"), item.get("build"), "signature good" if ok else "BAD SIGNATURE"))
        bad += not ok
    return 1 if bad else 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="command", required=True)
    k = sub.add_parser("keygen")
    k.add_argument("--out", required=True)
    s = sub.add_parser("sign")
    s.add_argument("archive")
    s.add_argument("--key", required=True)
    s.add_argument("--url", required=True)
    s.add_argument("--notes")
    s.add_argument("--appcast", default="appcast.plist")
    s.add_argument("--min-os", default="10.4")
    v = sub.add_parser("verify")
    v.add_argument("appcast")
    v.add_argument("--public", required=True)
    args = ap.parse_args(argv)
    return {"keygen": cmd_keygen, "sign": cmd_sign, "verify": cmd_verify}[args.command](args) or 0


if __name__ == "__main__":
    sys.exit(main())
