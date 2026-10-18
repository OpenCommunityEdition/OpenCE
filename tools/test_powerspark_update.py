"""Host tests of PowerSpark's updater: the signing tool, and port/mac/app/update_core.c against what the tool writes.

  python3 -m pytest -q tools/test_powerspark_update.py

The tool's Ed25519 is checked against RFC 8032's test vectors, then its output is read by update_core.c (Monocypher's
Ed25519, as the launcher has) through test_update_core, so the two implementations agree on the signed statement. The
appcast's reading, the version order, the bundle checks and the swap in a temporary folder are test_update_core.c's own
tests, which this runs too. Needs a Mac (Core Foundation) and a C compiler.
"""
import base64
import hashlib
import os
import plistlib
import shutil
import subprocess
import sys
import zipfile

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import powerspark_sign_update as signer  # noqa: E402

APP = os.path.join(ROOT, "port", "mac", "app")
MONO = os.path.join(ROOT, "port", "third_party", "monocypher")

# RFC 8032, section 7.1
VECTORS = [
    ("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
     "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", "",
     "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"),
    ("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
     "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c", "72",
     "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"),
]


def compile_test(tmp_path):
    exe = tmp_path / "test_update_core"
    cc = shutil.which("clang") or shutil.which("cc")
    if sys.platform != "darwin" or not cc:
        pytest.skip("needs a Mac and a C compiler")
    result = subprocess.run(
        [cc, "-std=gnu99", "-Wall", "-Wno-deprecated-declarations", "-I" + MONO, "-I" + APP, "-o", str(exe),
         os.path.join(APP, "test_update_core.c"), os.path.join(APP, "update_core.c"), os.path.join(MONO, "monocypher.c"),
         os.path.join(MONO, "monocypher-ed25519.c"), "-framework", "CoreFoundation"], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return str(exe)


@pytest.mark.parametrize("seed,public,message,signature", VECTORS)
def test_rfc8032_vectors(seed, public, message, signature):
    seed_bytes = bytes.fromhex(seed)
    assert signer.public_key(seed_bytes).hex() == public
    assert signer.sign(seed_bytes, bytes.fromhex(message)).hex() == signature
    assert signer.verify(bytes.fromhex(public), bytes.fromhex(message), bytes.fromhex(signature))
    assert not signer.verify(bytes.fromhex(public), bytes.fromhex(message) + b"x", bytes.fromhex(signature))


def test_core_checks(tmp_path):
    exe = compile_test(tmp_path)
    result = subprocess.run([exe, str(tmp_path / "scratch")], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "update core: ok" in result.stdout


def make_zip(path, version="0.2", build="20261101", bundle_id="org.opence.PowerSpark", programs=True):
    with zipfile.ZipFile(path, "w") as z:
        z.writestr("PowerSpark.app/Contents/Info.plist", plistlib.dumps({
            "CFBundleIdentifier": bundle_id, "CFBundleShortVersionString": version, "CFBundleVersion": build}))
        if programs:
            z.writestr("PowerSpark.app/Contents/MacOS/PowerSpark", b"x")
            z.writestr("PowerSpark.app/Contents/MacOS/PowerSpark-game", b"x")


def test_sign_and_read_back(tmp_path):
    exe = compile_test(tmp_path)
    keys = tmp_path / "keys"
    assert signer.main(["keygen", "--out", str(keys)]) == 0
    private = keys / signer.PRIVATE_NAME
    assert oct(os.stat(private).st_mode & 0o777) == "0o600"
    public = (keys / signer.PUBLIC_NAME).read_text().strip()
    assert len(base64.b64decode(public)) == 32
    # a key is never replaced
    with pytest.raises(SystemExit):
        signer.main(["keygen", "--out", str(keys)])

    archive = tmp_path / "PowerSpark-0.2.zip"
    make_zip(archive)
    notes = tmp_path / "notes.txt"
    notes.write_text("What is new.\n\nA second paragraph.\n")
    feed = tmp_path / "appcast.plist"
    url = "https://www.example.test/powerspark/PowerSpark-0.2.zip"
    assert signer.main(["sign", str(archive), "--key", str(private), "--url", url, "--notes", str(notes),
                        "--appcast", str(feed)]) == 0
    item = plistlib.loads(feed.read_bytes())["versions"][0]
    assert (item["version"], item["build"], item["url"], item["length"]) == ("0.2", 20261101, url, archive.stat().st_size)
    assert item["sha256"] == hashlib.sha256(archive.read_bytes()).hexdigest()
    assert item["minimumSystemVersion"] == "10.4" and item["notes"].startswith("What is new.")
    assert signer.main(["verify", str(feed), "--public", public]) == 0

    # the launcher's C code reads it, and chooses it for an older app, not for this one or a newer one
    def ask(version, build, key=public):
        out = subprocess.run([exe, "--appcast", str(feed), key, version, str(build)], capture_output=True, text=True)
        return out.returncode, out.stdout.strip()

    code, text = ask("0.1", 20261018)
    assert code == 0 and text.startswith("available 0.2 20261101 %d %s" % (archive.stat().st_size, item["sha256"])), text
    assert ask("0.2", 20261101)[1].startswith("current")
    assert ask("0.2", 20260101)[1].startswith("available 0.2")
    assert ask("0.3", 1)[1].startswith("current")
    # another key's signature is refused
    other = base64.b64encode(signer.public_key(os.urandom(32))).decode()
    code, text = ask("0.1", 20261018, other)
    assert code == 1 and "not signed" in text, text

    # a second release goes in beside the first; the same release again replaces it
    archive3 = tmp_path / "PowerSpark-0.10.zip"
    make_zip(archive3, version="0.10", build="20270101")
    signer.main(["sign", str(archive3), "--key", str(private), "--url", url.replace("0.2", "0.10"), "--appcast", str(feed)])
    signer.main(["sign", str(archive), "--key", str(private), "--url", url, "--appcast", str(feed)])
    versions = [v["version"] for v in plistlib.loads(feed.read_bytes())["versions"]]
    assert versions == ["0.2", "0.10"]
    assert ask("0.2", 20261101)[1].startswith("available 0.10")

    # a changed item is refused by the tool's own check and by the launcher's code
    data = plistlib.loads(feed.read_bytes())
    data["versions"][1]["length"] += 1
    feed.write_bytes(plistlib.dumps(data))
    assert signer.main(["verify", str(feed), "--public", public]) == 1
    assert ask("0.2", 20261101)[0] == 1


@pytest.mark.parametrize("kwargs,why", [
    ({"bundle_id": "org.example.Other"}, "bundle identifier"),
    ({"version": "two"}, "version"),
    ({"programs": False}, "lacks"),
])
def test_signing_refuses_other_zips(tmp_path, kwargs, why):
    signer.main(["keygen", "--out", str(tmp_path / "k")])
    archive = tmp_path / "x.zip"
    make_zip(archive, **kwargs)
    with pytest.raises(SystemExit) as caught:
        signer.main(["sign", str(archive), "--key", str(tmp_path / "k" / signer.PRIVATE_NAME),
                     "--url", "https://example.test/x.zip", "--appcast", str(tmp_path / "a.plist")])
    assert why in str(caught.value)


def test_signing_refuses_http_and_odd_zips(tmp_path):
    signer.main(["keygen", "--out", str(tmp_path / "k")])
    key = str(tmp_path / "k" / signer.PRIVATE_NAME)
    archive = tmp_path / "x.zip"
    make_zip(archive)
    with pytest.raises(SystemExit):
        signer.main(["sign", str(archive), "--key", key, "--url", "http://example.test/x.zip",
                     "--appcast", str(tmp_path / "a.plist")])
    with zipfile.ZipFile(archive, "a") as z:
        z.writestr("extra.txt", "x")
    with pytest.raises(SystemExit) as caught:
        signer.main(["sign", str(archive), "--key", key, "--url", "https://example.test/x.zip",
                     "--appcast", str(tmp_path / "a.plist")])
    assert "top level" in str(caught.value)
