"""Stage the CI signing key; published main builds must never use debug keys."""

import base64
import binascii
import os
import sys
from pathlib import Path


def main():
    encoded = os.environ.get("KEYSTORE", "").strip()
    password = os.environ.get("HALO_ANDROID_KEYSTORE_PASSWORD", "")
    required = os.environ.get("GITHUB_REF") == "refs/heads/main"
    if not encoded or not password:
        if required or encoded or password:
            print("::error::Android signing requires both ANDROID_KEYSTORE_BASE64 "
                  "and ANDROID_KEYSTORE_PASSWORD; refusing a debug-signed release.")
            return 1
        print("::warning::No Android signing secrets: this branch uses a debug key "
              "and cannot update installed releases.")
        return 0

    try:
        key = base64.b64decode("".join(encoded.split()), validate=True)
        if not key:
            raise ValueError("empty keystore")
    except (binascii.Error, ValueError):
        print("::error::ANDROID_KEYSTORE_BASE64 is not a nonempty base64 keystore.")
        return 1

    path = Path(os.environ["RUNNER_TEMP"]) / "halo.keystore"
    # A fresh runner owns this file. Refuse to overwrite an existing key.
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as output:
        output.write(key)
    with open(os.environ["GITHUB_ENV"], "a", encoding="utf-8") as output:
        output.write(f"HALO_ANDROID_KEYSTORE={path}\n")
    # Gradle validates the keystore, password and halo alias when signing.
    return 0


if __name__ == "__main__":
    sys.exit(main())
