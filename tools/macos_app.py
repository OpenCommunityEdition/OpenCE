"""Assemble OpenCE.app from the Swift executable and the guest image.

  macos_app.py <executable> <guest image> <entitlements> <codesign> <version> <app>

SwiftPM (port/macos/app) links the executable; this lays out the bundle
(Contents/MacOS/OpenCE, Contents/Info.plist, Contents/Resources/
halo_guest.elf) and signs it ad hoc with the hypervisor entitlement, which
the app needs to create its VM. The bundle is made beside its final place
and moved there, so that no unsigned app is ever left.
"""

import plistlib
import shutil
import subprocess
import sys
from pathlib import Path

# (as tools/macos_build.py's MACOS_MINIMUM)
MINIMUM_SYSTEM = "26.0"


def info_plist(version: str) -> bytes:
    return plistlib.dumps({
        "CFBundleDevelopmentRegion": "en",
        "CFBundleExecutable": "OpenCE",
        "CFBundleIdentifier": "org.opence.OpenCE",
        "CFBundleInfoDictionaryVersion": "6.0",
        "CFBundleName": "OpenCE",
        "CFBundleDisplayName": "OpenCE",
        "CFBundlePackageType": "APPL",
        "CFBundleShortVersionString": version,
        "CFBundleVersion": version,
        "LSApplicationCategoryType": "public.app-category.action-games",
        "LSMinimumSystemVersion": MINIMUM_SYSTEM,
        "NSHighResolutionCapable": True,
        "NSPrincipalClass": "NSApplication",
    })


def assemble(executable: Path, image: Path, entitlements: Path, codesign: str, version: str, app: Path) -> None:
    staging = app.with_name(app.name + ".partial")
    shutil.rmtree(staging, ignore_errors=True)
    (staging / "Contents/MacOS").mkdir(parents=True)
    (staging / "Contents/Resources").mkdir()
    shutil.copy2(executable, staging / "Contents/MacOS/OpenCE")
    # (data, not a program the system runs)
    shutil.copyfile(image, staging / "Contents/Resources/halo_guest.elf")
    (staging / "Contents/Resources/halo_guest.elf").chmod(0o644)
    (staging / "Contents/Info.plist").write_bytes(info_plist(version))
    subprocess.run([codesign, "-s", "-", "-f", "--entitlements", str(entitlements), str(staging)], check=True)
    shutil.rmtree(app, ignore_errors=True)
    staging.rename(app)


def main() -> None:
    executable, image, entitlements, codesign, version, app = sys.argv[1:]
    assemble(Path(executable), Path(image), Path(entitlements), codesign, version, Path(app))


if __name__ == "__main__":
    main()
