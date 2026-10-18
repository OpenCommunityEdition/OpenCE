#!/usr/bin/env python3
"""Runs PowerSpark's updater end to end on an old Mac, against a local test server (never the real site).

  python3 tools/mac_update_checks.py --mac c2d --remote /Volumes/Extra/OpenCE/agents/updater \\
      --binaries DIR [--local-only]

What it does, from the Mac this runs on:
  1. makes the fixture from the launcher (PowerSpark) and downloader (PowerSpark-update) that the old Mac built
     in REMOTE/out (build with the commands in tools/mac_app_bundle.sh, or REMOTE/build.sh): an old app (0.1), the
     new one (0.2) zipped, the ones that must be refused, and signed appcasts, with a test key made here;
  2. serves them over HTTPS on localhost:18443 (tools/mac_update_test_server.py);
  3. copies the fixture to REMOTE/fixture and runs tools/mac_update_scenarios.sh there over ssh, with -R
     18443:localhost:18443 so the old Mac reaches the server. The scenarios run on scratch copies in REMOTE/scratch.
With --local-only the fixture and server are made and left running (for trying by hand).
Exit status 0 when every scenario passed.
"""
import argparse
import hashlib
import os
import plistlib
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import powerspark_sign_update as signer  # noqa: E402

PORT = 18443
OLD = ("0.1", 20261018)
NEW = ("0.2", 20261101)


def make_app(path, version, build, binaries, key_b64, bundle_id="org.opence.PowerSpark"):
    contents = os.path.join(path, "Contents")
    for d in ("MacOS", "Resources"):
        os.makedirs(os.path.join(contents, d))
    for name in ("PowerSpark", "PowerSpark-update"):
        shutil.copy(os.path.join(binaries, name), os.path.join(contents, "MacOS", name))
    # (a stand-in for the game program: any program with the right processors)
    shutil.copy(os.path.join(binaries, "PowerSpark"), os.path.join(contents, "MacOS", "PowerSpark-game"))
    for name in os.listdir(os.path.join(contents, "MacOS")):
        os.chmod(os.path.join(contents, "MacOS", name), 0o755)
    with open(os.path.join(contents, "Info.plist"), "wb") as f:
        plistlib.dump({
            "CFBundleIdentifier": bundle_id, "CFBundleExecutable": "PowerSpark", "CFBundleName": "PowerSpark",
            "CFBundlePackageType": "APPL", "CFBundleShortVersionString": version, "CFBundleVersion": str(build),
            "CFBundleInfoDictionaryVersion": "6.0",
        }, f)
    with open(os.path.join(contents, "Resources", "marker.txt"), "w") as f:
        f.write("version %s\n" % version)


def zip_app(app, archive):
    subprocess.run(["ditto", "-c", "-k", "--keepParent", "--norsrc", app, archive], check=True)


def item_for(archive, url, version, build, seed, notes="", min_os="10.4"):
    data = open(archive, "rb").read()
    item = {"version": version, "build": build, "url": url, "sha256": hashlib.sha256(data).hexdigest(),
            "length": len(data), "minimumSystemVersion": min_os}
    if notes:
        item["notes"] = notes
    item["signature"] = __import__("base64").b64encode(signer.sign(seed, signer.statement(item))).decode()
    return item


def write_feed(path, items):
    with open(path, "wb") as f:
        plistlib.dump({"feed": "powerspark", "versions": items}, f)


def branding(path, feed, ca, key):
    values = {"CAFile": ca}
    if feed:
        values.update({"UpdateFeedURL": feed, "UpdatePublicKey": key})
    with open(path, "wb") as f:
        plistlib.dump(values, f)


def build_fixture(root, binaries):
    import base64
    files, fix = os.path.join(root, "files"), os.path.join(root, "fixture")
    os.makedirs(files)
    os.makedirs(fix)
    seed, other = os.urandom(32), os.urandom(32)
    key = base64.b64encode(signer.public_key(seed)).decode()
    base = "https://localhost:%d" % PORT
    stage = os.path.join(root, "stage")
    os.makedirs(stage)

    old = os.path.join(fix, "old", "PowerSpark.app")
    os.makedirs(os.path.dirname(old))
    make_app(old, OLD[0], OLD[1], binaries, key)
    new = os.path.join(stage, "PowerSpark.app")
    make_app(new, NEW[0], NEW[1], binaries, key)
    good = os.path.join(files, "PowerSpark-0.2.zip")
    zip_app(new, good)
    notes = "A first line of notes.\n\nSecond paragraph, with a few more words.\n- an item\n- another item"

    # one that is a different app, signed properly
    stage2 = os.path.join(root, "stage2")
    os.makedirs(stage2)
    wrong = os.path.join(stage2, "PowerSpark.app")
    make_app(wrong, "0.3", 20261201, binaries, key, bundle_id="org.example.Other")
    wrong_zip = os.path.join(files, "PowerSpark-0.3.zip")
    zip_app(wrong, wrong_zip)
    # one whose bytes changed after it was signed
    tampered = os.path.join(files, "PowerSpark-0.2t.zip")
    data = bytearray(open(good, "rb").read())
    data[len(data) // 2] ^= 0xFF
    open(tampered, "wb").write(bytes(data))

    current = item_for(good, base + "/PowerSpark-0.1.zip", OLD[0], OLD[1], seed)
    feeds = {
        "ok": [current, item_for(good, base + "/PowerSpark-0.2.zip", NEW[0], NEW[1], seed, notes)],
        "current": [current],
        "badsig": [item_for(good, base + "/PowerSpark-0.2.zip", NEW[0], NEW[1], other, notes)],
        "wrongid": [item_for(wrong_zip, base + "/PowerSpark-0.3.zip", "0.3", 20261201, seed)],
        "chunked": [item_for(good, base + "/chunked/PowerSpark-0.2.zip", NEW[0], NEW[1], seed, notes)],
        "redirect": [item_for(good, base + "/redirect/PowerSpark-0.2.zip", NEW[0], NEW[1], seed, notes)],
        "truncated": [item_for(good, base + "/truncated/PowerSpark-0.2.zip", NEW[0], NEW[1], seed, notes)],
    }
    tampered_item = item_for(good, base + "/PowerSpark-0.2t.zip", NEW[0], NEW[1], seed, notes)
    feeds["tampered"] = [tampered_item]
    for name, items in feeds.items():
        write_feed(os.path.join(files, "appcast-%s.plist" % name), items)
    shutil.copy(os.path.join(files, "appcast-ok.plist"), os.path.join(files, "appcast.plist"))

    # the ones with the same name but a different path in front: feeds served chunked, redirected, or 503
    ca = "ca.pem"
    for name in feeds:
        feed = base + ("/chunked/appcast-%s.plist" % name if name == "chunked" else
                       "/redirect/appcast-%s.plist" % name if name == "redirect" else "/appcast-%s.plist" % name)
        branding(os.path.join(fix, "branding-%s.plist" % name), feed, ca, key)
    branding(os.path.join(fix, "branding-down.plist"), "https://localhost:1/appcast.plist", ca, key)
    branding(os.path.join(fix, "branding-status.plist"), base + "/status/503", ca, key)
    branding(os.path.join(fix, "branding-wrongca.plist"), base + "/appcast-ok.plist", "other-ca.pem", key)
    branding(os.path.join(fix, "branding-nofeed.plist"), None, ca, key)
    return fix, files, seed, key


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mac", default="c2d")
    ap.add_argument("--remote", default="/Volumes/Extra/OpenCE/agents/updater")
    ap.add_argument("--binaries", help="a folder holding the launcher PowerSpark and PowerSpark-update (universal)")
    ap.add_argument("--archs", default="i386 ppc")
    ap.add_argument("--local-only", action="store_true")
    ap.add_argument("--remote-command", help="run this on the old Mac (with the fixture and the tunnel) instead of the scenarios")
    args = ap.parse_args()

    root = tempfile.mkdtemp(prefix="powerspark-update-test-")
    binaries = args.binaries
    if not binaries:
        binaries = os.path.join(root, "bin")
        os.makedirs(binaries)
        for name in ("PowerSpark", "PowerSpark-update"):
            subprocess.run(["scp", "-q", "%s:%s/out/%s" % (args.mac, args.remote, name), binaries], check=True)
    fix, files, seed, key = build_fixture(root, binaries)
    certs = os.path.join(root, "certs")
    server = subprocess.Popen([sys.executable, os.path.join(HERE, "mac_update_test_server.py"), "--dir", files,
                               "--certs", certs, "--port", str(PORT)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        for _ in range(50):
            if os.path.exists(os.path.join(certs, "ca.pem")):
                time.sleep(0.5)
                break
            time.sleep(0.1)
        shutil.copy(os.path.join(certs, "ca.pem"), os.path.join(fix, "ca.pem"))
        # a CA list that does not vouch for the server (another certificate made the same way)
        import mac_update_test_server
        _, other_ca = mac_update_test_server.make_certificate(os.path.join(root, "other-certs"))
        shutil.copy(other_ca, os.path.join(fix, "other-ca.pem"))
        if args.local_only:
            print("fixture: %s\nserver: https://localhost:%d/ (pid %d)" % (root, PORT, server.pid))
            server.wait()
            return 0
        subprocess.run(["ssh", args.mac, "rm -rf '%s/fixture' && mkdir -p '%s'" % (args.remote, args.remote)], check=True)
        subprocess.run(["rsync", "-a", "--delete", fix + "/", "%s:%s/fixture/" % (args.mac, args.remote)], check=True)
        subprocess.run(["scp", "-q", os.path.join(HERE, "mac_update_scenarios.sh"),
                        "%s:%s/scenarios.sh" % (args.mac, args.remote)], check=True)
        result = subprocess.run(["ssh", "-o", "ExitOnForwardFailure=yes", "-R", "%d:localhost:%d" % (PORT, PORT), args.mac,
                                 args.remote_command or "sh '%s/scenarios.sh' '%s/fixture' '%s/scratch' %s" %
                                 (args.remote, args.remote, args.remote, args.archs)])
        print("server saw:")
        log = os.path.join(root, "server.log")
        if os.path.exists(log):
            print("".join("  " + l for l in open(log).readlines()[-12:]))
        return result.returncode
    finally:
        server.terminate()
        shutil.rmtree(root, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
