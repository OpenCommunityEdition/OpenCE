#!/usr/bin/env python3
"""The 32-bit x86 system root of the portable Linux build (configure.py
--portable), and the SDL3 source it builds libSDL3.so.0 from.

An executable built against the build machine's glibc asks for the symbols
of that version (GLIBC_2.43 on an up-to-date Arch Linux) and starts on no
older system. The portable build is therefore compiled and linked against
Debian 11's i386 glibc 2.31, the baseline of the Steam Runtime 3 ("sniper"),
which SteamOS 3, the Steam Deck and every distribution since 2020 have, and
against the headers of the libraries SDL loads when it starts (X11, Wayland
and libdecor, PipeWire, PulseAudio, ALSA, D-Bus, udev), which it opens itself
(dlopen) and does not link to.

Each package comes from the first of these that has it: archive.debian.org,
which keeps every Debian release, deb.debian.org, and last
snapshot.debian.org, whose URLs (the archive as it was at a moment when the
package was in it) do not change. SDL's source comes from its GitHub release,
or libsdl.org's. Each download is tried a few times, and checked against the
SHA-256 digests here.

    python tools/linux_sysroot.py <folder>

makes <folder>/sysroot and <folder>/SDL3 once, and writes <folder>/stamp
(the list of what it holds) last; a folder whose stamp matches is left as it
is. Python alone: no dpkg, no root.

    python tools/linux_sysroot.py --check-sdl <SDL_build_config.h>

stops the build if SDL was built without one of the drivers it needs.
"""

import hashlib
import http.client
import io
import os
import shutil
import sys
import tarfile
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import List, Tuple

DEBIAN_MIRRORS = [
    "https://archive.debian.org/debian/",
    "https://deb.debian.org/debian/",
]

# (and the last resort: the archive as snapshot.debian.org has it at the time
# given with each package, when the package was in it)
SNAPSHOT = "https://snapshot.debian.org/archive/debian/{}/"

# (path in the Debian archive, SHA-256, a snapshot.debian.org time that has
# it), all i386 from Debian 11 "bullseye" but PipeWire's (bullseye-backports':
# SDL needs 0.3.44 or later, bullseye has 0.3.19) and Wayland's and
# libdecor's (below). Only the C library's and libgcc's are linked to; the
# rest are SDL's headers, and the libraries it reads the names of.
PACKAGES: List[Tuple[str, str, str]] = [
    # the C library and the compiler's runtime (crtbegin.o, libgcc)
    ("pool/main/g/glibc/libc6_2.31-13+deb11u11_i386.deb",
     "8198abd817cbcd615ce3207fb924317b53cc0b522bfe57d689757de13fe00d66", "20240817T023924Z"),
    ("pool/main/g/glibc/libc6-dev_2.31-13+deb11u11_i386.deb",
     "7d10b944894d2b6ccab08ccf51aae6cef6c5f5773f36696e6d9f34c21e0365b4", "20240817T023924Z"),
    ("pool/main/l/linux/linux-libc-dev_5.10.223-1_i386.deb",
     "e57d6065a43e763e54a0a80afd66fa2371bcdc3b035f9f4463816e9c6fc26049", "20240815T030336Z"),
    ("pool/main/g/gcc-10/libgcc-s1_10.2.1-6_i386.deb",
     "0a52edec5d626f8d9acf1998248d276760aff3950f44a308d27d4a2a488fe18d", "20210111T024320Z"),
    ("pool/main/g/gcc-10/libgcc-10-dev_10.2.1-6_i386.deb",
     "64a544dcf5463225603cb025a05c7b79dea9876004c09d81af2bc31ffe0ccc1e", "20210111T024320Z"),
    # SDL's X11 video driver
    ("pool/main/libx/libx11/libx11-6_1.7.2-1+deb11u2_i386.deb",
     "a0307da50139370781beac6fe6d3b173493af11d0a35c43c053c539d5d67969c", "20231012T085604Z"),
    ("pool/main/libx/libx11/libx11-dev_1.7.2-1+deb11u2_i386.deb",
     "6674c5517140f65addb055b2fcbab42e83c299555ecf516daf40758f938cf2d0", "20231012T085604Z"),
    ("pool/main/x/xorgproto/x11proto-dev_2020.1-1_all.deb",
     "d5568d587d9ad2664c34c14b0ac538ccb3c567e126ee5291085a8de704a565f5", "20200415T024424Z"),
    # (xcb/xcb.h, for SDL's Vulkan headers)
    ("pool/main/libx/libxcb/libxcb1-dev_1.14-3_i386.deb",
     "b1d522ad25e5d4f208c73c2f20e6c918a869d11171835b7f792f487ff367d788", "20210201T212616Z"),
    ("pool/main/libx/libxext/libxext6_1.3.3-1.1_i386.deb",
     "b93e2d194c4d0aa5ca0c056883787b1fac35941f5ce9ec4fa6baedfba4cdb5a9", "20201218T204051Z"),
    ("pool/main/libx/libxext/libxext-dev_1.3.3-1.1_i386.deb",
     "0f2ca4c3c5e74e6ce6c99da60cbf1a66de1665a5703ddfb725a5855cc9ef7281", "20201218T204051Z"),
    ("pool/main/libx/libxcursor/libxcursor1_1.2.0-2_i386.deb",
     "593d4ac35460cc397d1fa96d251b62c26eb3f807b2e0bfe21dc33abef68811eb", "20190714T093329Z"),
    ("pool/main/libx/libxcursor/libxcursor-dev_1.2.0-2_i386.deb",
     "c3db4042a513952624d6f06485599f1c84d4095ebfaa753ddc243fa07f4bf81c", "20190714T093329Z"),
    ("pool/main/libx/libxi/libxi6_1.7.10-1_i386.deb",
     "004182ad1167319b5153fa736a41b646806586739db93e6a06db857ff8553505", "20200603T030403Z"),
    ("pool/main/libx/libxi/libxi-dev_1.7.10-1_i386.deb",
     "7a1b570d9bdf70d7e79e8b03a87c48cbb40a68ed672920f6170c67ec270edfc2", "20200603T030403Z"),
    ("pool/main/libx/libxrandr/libxrandr2_1.5.1-1_i386.deb",
     "b9a2d67526ee1a6cd4eda834534a21bd4a5a3c795a4945214553933560d21ce2", "20161207T032837Z"),
    ("pool/main/libx/libxrandr/libxrandr-dev_1.5.1-1_i386.deb",
     "bffbcd2b6eb5459aebada2adba44b32415cfdb8e27e2ee736b908e4c29f03360", "20161207T032837Z"),
    ("pool/main/libx/libxfixes/libxfixes3_5.0.3-2_i386.deb",
     "2250bbc75dcf0d73fc84b69150dcaefefc071ad2a28985a5a69abfeac2290f11", "20200414T204951Z"),
    ("pool/main/libx/libxfixes/libxfixes-dev_5.0.3-2_i386.deb",
     "3c81c546f61ed7af6a6a30ad3709b75dc74a983dff8f00dcce30eaff5a8c18e1", "20200414T204951Z"),
    ("pool/main/libx/libxrender/libxrender1_0.9.10-1_i386.deb",
     "0b71b3f530a1b039b64f74c0223b433bd73fe0164916b60b25f9f3f11a6b9bfb", "20161205T212728Z"),
    ("pool/main/libx/libxrender/libxrender-dev_0.9.10-1_i386.deb",
     "7a4a1cbc6e2004c520dc5882e0367c8ce5c7094e07922e792992d1d27a5829a4", "20161205T212728Z"),
    ("pool/main/libx/libxss/libxss1_1.2.3-1_i386.deb",
     "eb50d23f8e849a7f86a6313f3e48c5c1fbf6cf82ca34ea5ef32bc99ae980c64a", "20180906T152804Z"),
    ("pool/main/libx/libxss/libxss-dev_1.2.3-1_i386.deb",
     "e3f4268c8c5ca5a1e7a2b549470f9eca58cf9974b0176a929dd2185026895e7e", "20180906T152804Z"),
    # SDL's OpenGL (GL/gl.h, GL/glx.h)
    ("pool/main/libg/libglvnd/libgl-dev_1.3.2-1_i386.deb",
     "1603066c9e658adbd71296b9a5db08d771c92900ac90035b9e210b9d7e2b7a3d", "20200729T210030Z"),
    ("pool/main/libg/libglvnd/libglx-dev_1.3.2-1_i386.deb",
     "97c1367f7ecf6c7e103a0eb85e119b3e22d9d882d52587a53733ff93daa94c1d", "20200729T210030Z"),
    # SDL's Wayland video driver: Debian 12's Wayland (1.21), whose headers
    # match the protocol code that a recent wayland-scanner writes
    # (wl_proxy_marshal_flags, Wayland 1.20)
    ("pool/main/w/wayland/libwayland-client0_1.21.0-1_i386.deb",
     "1454b74db0b175bd4ef2b14ca6e66269e5d05e1bafa4fdaa6efa15424872061a", "20220709T031205Z"),
    ("pool/main/w/wayland/libwayland-cursor0_1.21.0-1_i386.deb",
     "5ea0a94c004d546fc41e426a47269b4251af8a0e691db57a011d72f43c62d3b7", "20220709T031205Z"),
    ("pool/main/w/wayland/libwayland-egl1_1.21.0-1_i386.deb",
     "0b73ffd5ddd6b3c6fbbd74dc0b5d8259614515e89e791fafe3472168f6b0149c", "20220709T031205Z"),
    ("pool/main/w/wayland/libwayland-dev_1.21.0-1_i386.deb",
     "c5051589b50623bc71a0a54e006571740b066ec61476b38808ef5f93fc0c95d3", "20220709T031205Z"),
    # (libdecor, the window decorations GNOME's Wayland asks clients to draw:
    # Debian 12's, as it needs Wayland 1.20's headers)
    ("pool/main/libd/libdecor-0/libdecor-0-0_0.1.1-2_i386.deb",
     "74b39bf96b3870899883c8e3b6a16bca75e8c5828614a7e1b9d061439ee6bb35", "20230211T151657Z"),
    ("pool/main/libd/libdecor-0/libdecor-0-dev_0.1.1-2_i386.deb",
     "31a307669c759629853ae06d7898a4e6355a9c155278b2a1a482d583ca3eac66", "20230211T151657Z"),
    ("pool/main/libf/libffi/libffi-dev_3.3-6_i386.deb",
     "a699fc6b1dbeff5f3633be8b3f8d31526b69d11bf435c5865e50ac249b98f94c", "20210221T031914Z"),
    ("pool/main/libx/libxkbcommon/libxkbcommon0_1.0.3-2_i386.deb",
     "b6d8929647c0c182f63d1c886b7fe8b23674383b2585401b2377ff61d2fe670e", "20201126T153155Z"),
    ("pool/main/libx/libxkbcommon/libxkbcommon-dev_1.0.3-2_i386.deb",
     "1a88ce18baafa1404452b451b0b3ce1a8eebf6204eedd498e38b34c5d6f42c44", "20201126T153155Z"),
    ("pool/main/libg/libglvnd/libegl1_1.3.2-1_i386.deb",
     "aba3025fe2becdcf9ed7d3822a52cd98ba6921fd8ccdd7825ec5a1f99b437fc8", "20200729T210030Z"),
    ("pool/main/libg/libglvnd/libegl-dev_1.3.2-1_i386.deb",
     "5073d41e5be4bffad4b19a5ebf93c2ad43b420d00417a778e76d84f831a47a66", "20200729T210030Z"),
    # SDL's audio drivers
    ("pool/main/a/alsa-lib/libasound2_1.2.4-1.1_i386.deb",
     "1e7da3fa4ba22b7079bceb18e8ce3494d9dced9b1cffe5c6bb9aae8e64ecc9f7", "20210104T204018Z"),
    ("pool/main/a/alsa-lib/libasound2-dev_1.2.4-1.1_i386.deb",
     "7ea775aaa87bc4d391b800c5cbdcacdc298bb52d9a82dff31073860dc9737309", "20210104T204018Z"),
    ("pool/main/p/pulseaudio/libpulse0_14.2-2_i386.deb",
     "c84c1a3dbd17355cdf6eaf23c2313db18faf3581ca7ec95cf3a2177d1c70a577", "20210227T024018Z"),
    ("pool/main/p/pulseaudio/libpulse-dev_14.2-2_i386.deb",
     "c530edf490a67c2ece2bf54563872693d9f02ce6069e73b2b9b5372d965ef8e8", "20210227T024018Z"),
    ("pool/main/p/pipewire/libpipewire-0.3-0_0.3.65-2~bpo11+1_i386.deb",
     "1d032b638081704393bc847e2e1f7a00c6894fa262ed0272fb7282962e8915a9", "20230217T033139Z"),
    ("pool/main/p/pipewire/libpipewire-0.3-dev_0.3.65-2~bpo11+1_i386.deb",
     "5f67d3e5e435251a34e55a10b4834a794e6b48b83d6da39aab267b35ce9aba33", "20230217T033139Z"),
    ("pool/main/p/pipewire/libspa-0.2-dev_0.3.65-2~bpo11+1_i386.deb",
     "5128545520aa6444af8c3f61c72e5f8d39ced7556ce07dfdedcbec291c4a5048", "20230217T033139Z"),
    # the desktop's D-Bus (the screen saver, the system's settings) and
    # udev (gamepads plugged in and out)
    ("pool/main/d/dbus/libdbus-1-3_1.12.28-0+deb11u1_i386.deb",
     "7d8d30ca9cfa726f61e5b71dc6294ca3266147208d13cdec1fbefb217b0a9d25", "20230625T033524Z"),
    ("pool/main/d/dbus/libdbus-1-dev_1.12.28-0+deb11u1_i386.deb",
     "8ca893c7e096f70fd87745744a042507bd335bd6c13d98ba57bb6ecfa1170210", "20230625T033524Z"),
    ("pool/main/s/systemd/libudev1_247.3-7+deb11u5_i386.deb",
     "07891a18db3527472eaafb26e07ba26d3bfb926dbe2c01d3282e6b5fe61454cc", "20240616T023746Z"),
    ("pool/main/s/systemd/libudev-dev_247.3-7+deb11u5_i386.deb",
     "634a5cc9d7bc226d75ced0414750e32b07e82531e8b2b091e41708d42978db3d", "20240616T023746Z"),
]

# SDL3's source release (the version of the other ports' SDL), from GitHub
# or libsdl.org
# (as tools/windows_build.py's SDL_VERSION and tools/android_build.py's SDL_TAG)
SDL_VERSION = "3.4.16"
SDL_URLS = [
    f"https://github.com/libsdl-org/SDL/releases/download/release-{SDL_VERSION}/SDL3-{SDL_VERSION}.tar.gz",
    f"https://www.libsdl.org/release/SDL3-{SDL_VERSION}.tar.gz",
]
SDL_SHA256 = "7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68"


# what SDL must have been built with (its SDL_build_config.h): the drivers a
# desktop needs, each loaded when SDL starts rather than linked to (libdecor
# too: without its library's name, SDL would link to it, and the Steam Deck
# has no 32-bit libdecor)
SDL_REQUIRED = [
    "SDL_VIDEO_DRIVER_X11_DYNAMIC", "SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC", "HAVE_LIBDECOR_H",
    "SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_LIBDECOR", "SDL_VIDEO_OPENGL_GLX",
    "SDL_VIDEO_OPENGL_EGL", "SDL_AUDIO_DRIVER_PIPEWIRE_DYNAMIC", "SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC",
    "SDL_AUDIO_DRIVER_ALSA_DYNAMIC", "SDL_UDEV_DYNAMIC", "HAVE_DBUS_DBUS_H",
]


def check_sdl(build_config: Path) -> None:
    """stops (exit status 1) if SDL was built without one of SDL_REQUIRED,
    as when the system root lacks a header SDL looks for"""
    defined = {line.split()[1] for line in build_config.read_text().splitlines()
               if line.startswith("#define ") and len(line.split()) > 2}
    missing = [name for name in SDL_REQUIRED if name not in defined]
    if missing:
        sys.exit(f"SDL3 was built without {', '.join(missing)}: refer to {build_config.parent.parent.parent}"
                 "/CMakeFiles/CMakeConfigureLog.yaml")


def stamp_text() -> str:
    """what a complete folder's stamp holds"""
    return "".join(f"{path} {digest}\n" for path, digest, _ in PACKAGES) + f"{SDL_URLS[0]} {SDL_SHA256}\n"


# (each URL's tries, a moment apart, before the next URL)
TRIES = 3
RETRY_SECONDS = 2


def download(urls: List[str], digest: str) -> bytes:
    """the file at the first of urls that answers, checked against digest"""
    errors = []
    for url in urls:
        for attempt in range(1, TRIES + 1):
            if attempt > 1:
                time.sleep(RETRY_SECONDS * (attempt - 1))
            try:
                with urllib.request.urlopen(url, timeout=60) as response:
                    data = response.read()
            # (a connection cut short is http.client.IncompleteRead, not an
            # OSError)
            except (OSError, http.client.HTTPException) as error:
                errors.append(f"{url}: {error!r}")
                # (the mirror does not have it: no use asking again)
                if isinstance(error, urllib.error.HTTPError) and error.code == 404:
                    break
                continue
            actual = hashlib.sha256(data).hexdigest()
            if actual == digest:
                return data
            errors.append(f"{url}: SHA-256 {actual}, expected {digest}")
    raise RuntimeError("cannot download " + "; ".join(errors))


def deb_data(deb: bytes) -> tarfile.TarFile:
    """the data.tar.* member of a .deb (an ar archive)"""
    if not deb.startswith(b"!<arch>\n"):
        raise RuntimeError("not a Debian package")
    offset = 8
    while offset + 60 <= len(deb):
        header = deb[offset:offset + 60]
        name = header[:16].decode().strip().rstrip("/")
        size = int(header[48:58].decode())
        body = deb[offset + 60:offset + 60 + size]
        if name.startswith("data.tar"):
            return tarfile.open(fileobj=io.BytesIO(body), mode="r:*")
        offset += 60 + size + (size & 1)
    raise RuntimeError("a Debian package without data.tar")


# what the packages hold that the build does not need
UNUSED = (("usr", "share", "doc"), ("usr", "share", "man"), ("usr", "share", "locale"), ("usr", "share", "lintian"),
          ("usr", "share", "bug"), ("usr", "bin"), ("sbin",), ("usr", "sbin"))


def extract(archive: tarfile.TarFile, destination: Path, strip: int = 0) -> None:
    """an archive's files, folders and links into destination, nothing
    outside it (no device files, no absolute or upward names, nothing
    through a link of its own that leads out of it)"""
    root = destination.resolve()
    for member in archive.getmembers():
        parts = [part for part in Path(member.name).parts if part not in (".", "/")][strip:]
        if not parts or ".." in parts or any(tuple(parts[:len(unused)]) == unused for unused in UNUSED):
            continue
        target = destination.joinpath(*parts)
        if not target.parent.resolve().is_relative_to(root):
            continue
        if member.isdir():
            target.mkdir(parents=True, exist_ok=True)
        elif member.isfile() or member.islnk():
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.is_symlink():
                target.unlink()
            with archive.extractfile(member) as source, open(target, "wb") as output:
                shutil.copyfileobj(source, output)
            os.chmod(target, member.mode & 0o755 | 0o600)
        elif member.issym():
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.is_symlink() or target.exists():
                target.unlink()
            link = member.linkname
            # (an absolute link, /lib/i386-linux-gnu/libudev.so.1, points
            # into the system root instead of the build machine's)
            if link.startswith("/"):
                link = os.path.relpath(destination / link.lstrip("/"), target.parent)
            os.symlink(link, target)


def make(folder: Path) -> None:
    stamp = folder / "stamp"
    if stamp.is_file() and stamp.read_text() == stamp_text():
        return
    for name in ("stamp", "sysroot", "SDL3"):
        path = folder / name
        if path.is_dir():
            shutil.rmtree(path)
        elif path.exists():
            path.unlink()
    sysroot = folder / "sysroot"
    sysroot.mkdir(parents=True)
    for path, digest, snapshot in PACKAGES:
        print(f"Downloading {path.rsplit('/', 1)[-1]}", flush=True)
        mirrors = DEBIAN_MIRRORS + [SNAPSHOT.format(snapshot)]
        with deb_data(download([mirror + path for mirror in mirrors], digest)) as archive:
            extract(archive, sysroot)
    print(f"Downloading SDL3-{SDL_VERSION}.tar.gz", flush=True)
    with tarfile.open(fileobj=io.BytesIO(download(SDL_URLS, SDL_SHA256)), mode="r:gz") as archive:
        extract(archive, folder / "SDL3", strip=1)
    stamp.write_text(stamp_text())


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--check-sdl":
        check_sdl(Path(sys.argv[2]))
    elif len(sys.argv) == 2:
        make(Path(sys.argv[1]))
    else:
        sys.exit(__doc__)
