#!/usr/bin/env python
"""Texture packs for OpenCE: list, verify, install and remove them.

    opence_texture_pack.py [--data-root DIR] list
    opence_texture_pack.py [--data-root DIR] verify PACK
    opence_texture_pack.py [--data-root DIR] install ZIP-OR-URL --sha256 HASH [--name NAME] [--force]
    opence_texture_pack.py [--data-root DIR] remove PACK

A pack is the folder DIR/texture_packs/PACK (docs/texture_packs.md). This
tool is separate from the game: the game only reads a folder, it never opens
an archive or the network. `install` downloads only from the URL you give it
(there is no default), checks the archive against the SHA-256 you give,
unpacks it into a scratch folder, checks every name and file, and only then
moves the pack into texture_packs. Nothing is run from the archive.

It runs on Python 2.5 or later and on Python 3 (a Mac OS X 10.4 or 10.5
machine can use it as it is; Python 2.5 has no https certificate checking,
so download there only from a URL you trust, or copy the pack over from a
modern machine instead).
"""
from __future__ import with_statement
import hashlib
import os
import re
import shutil
import stat
import struct
import sys
import tempfile
import zipfile

try:
    import urllib2 as urlrequest
except ImportError:
    import urllib.request as urlrequest

MAX_ARCHIVE = 2048 * 1024 * 1024        # the archive, in bytes
MAX_MEMBER = 96 * 1024 * 1024           # one file (the game's limit)
MAX_TOTAL = 4096 * 1024 * 1024          # all the files unpacked
MAX_MEMBERS = 20000
MAX_EDGE = 4096
SAFE_COMPONENT = re.compile(r'^[A-Za-z0-9_.\- ]+$')
PACK_NAME = re.compile(r'^[A-Za-z0-9_\-][A-Za-z0-9_.\- ]{0,98}$')


class PackError(Exception):
    pass


def default_root():
    for candidate in (os.environ.get('HALO_DATA_ROOT'), os.environ.get('HALO_MAC_DATA_ROOT'),
                      '/Applications/Halo.app/Contents/Resources/GameData'):
        if candidate and os.path.isdir(candidate):
            return candidate
    return os.getcwd()


def packs_dir(root):
    return os.path.join(root, 'texture_packs')


def check_pack_name(name):
    if not PACK_NAME.match(name) or name.startswith('.') or '..' in name:
        raise PackError('bad pack name %r (letters, digits, space, - _ . only)' % name)


# ---- pack.toml: top-level key = "string" lines, then [files] "path" = "sha256" ----

def parse_toml(text):
    info = {}
    files = {}
    section = None
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith('#'):
            continue
        if line.startswith('['):
            section = line.strip('[] \t')
            continue
        match = re.match(r'^("(?:[^"\\]|\\.)*"|[A-Za-z0-9_\-]+)\s*=\s*"((?:[^"\\]|\\.)*)"\s*(#.*)?$', line)
        if not match:
            continue
        key = match.group(1)
        if key.startswith('"'):
            key = key[1:-1]
        value = match.group(2).replace('\\"', '"').replace('\\\\', '\\')
        if section is None:
            info[key] = value
        elif section == 'files':
            files[key.replace('\\', '/')] = value.lower()
    if 'license' in info and 'licence' not in info:
        info['licence'] = info['license']
    return info, files


def read_info(pack_path):
    path = os.path.join(pack_path, 'pack.toml')
    if not os.path.isfile(path) or os.path.islink(path):
        raise PackError('no pack.toml')
    if os.path.getsize(path) > 65536:
        raise PackError('pack.toml is larger than 64 KB')
    with open(path, 'rb') as handle:
        text = handle.read().decode('utf-8', 'replace')
    info, files = parse_toml(text)
    for key in ('name', 'licence'):
        if not info.get(key):
            raise PackError('pack.toml has no %s' % key)
    return info, files


# ---- DDS ----

def level_bytes(kind, width, height):
    if kind == 'rgba':
        return width * height * 4
    return ((width + 3) // 4) * ((height + 3) // 4) * (8 if kind == 'dxt1' else 16)


def check_dds(data):
    """Return a short description of a DDS file, or raise PackError."""
    if len(data) < 128:
        raise PackError('shorter than a DDS header')
    if data[0:4] != 'DDS '.encode('ascii'):
        raise PackError('bad magic')
    fields = struct.unpack('<32I', data[0:128])
    (size, flags, height, width, _pitch, _depth, mips) = fields[1:8]
    pf_size, pf_flags, fourcc, bits, rmask, gmask, bmask, amask = fields[19:27]
    caps2 = fields[28]
    if size != 124 or pf_size != 32:
        raise PackError('bad header sizes')
    if not (flags & 0x20000) or mips == 0:
        mips = 1
    if width < 1 or height < 1 or width > MAX_EDGE or height > MAX_EDGE or (width & (width - 1)) or (height & (height - 1)):
        raise PackError('width and height must be powers of two up to %d' % MAX_EDGE)
    longest = max(width, height)
    chain = 1
    while (1 << (chain - 1)) < longest:
        chain += 1
    if mips > 13 or mips > chain:
        raise PackError('bad mip count %d' % mips)
    if caps2 & 0x200000:
        raise PackError('volume textures are not supported')
    faces = 1
    if caps2 & 0x200:
        if (caps2 & 0xFC00) != 0xFC00:
            raise PackError('a cube map needs all six faces')
        if width != height:
            raise PackError('cube map faces must be square')
        faces = 6
    if pf_flags & 0x4:
        kinds = {0x31545844: 'dxt1', 0x33545844: 'dxt3', 0x35545844: 'dxt5'}
        if fourcc not in kinds:
            raise PackError('only DXT1, DXT3, DXT5 and 32-bit BGRA are supported')
        kind = kinds[fourcc]
    elif (pf_flags & 0x40) and bits == 32 and rmask == 0xFF0000 and gmask == 0xFF00 and bmask == 0xFF and amask in (0xFF000000, 0):
        kind = 'rgba'
    else:
        raise PackError('only DXT1, DXT3, DXT5 and 32-bit BGRA are supported')
    total = 0
    for level in range(mips):
        total += level_bytes(kind, max(1, width >> level), max(1, height >> level))
    if total * faces != len(data) - 128:
        raise PackError('size %d does not match the header (%d expected)' % (len(data), total * faces + 128))
    return '%s %dx%d, %d levels%s' % (kind.upper(), width, height, mips, ', cube' if faces == 6 else '')


def check_relative(name):
    """A pack's file name is safe: relative, no .., plain characters, no symlink tricks."""
    if not name or name.startswith('/') or name.startswith('\\') or '\\' in name or ':' in name or '\0' in name:
        raise PackError('unsafe path %r' % name)
    for part in name.split('/'):
        if part in ('', '.', '..') or not SAFE_COMPONENT.match(part):
            raise PackError('unsafe path %r' % name)
    if len(name) > 400:
        raise PackError('path too long %r' % name)


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        while True:
            block = handle.read(1 << 20)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def verify_pack(pack_path, report):
    """Check a pack; returns the number of files. Raises PackError at the first problem."""
    info, hashes = read_info(pack_path)
    count = 0
    seen = {}
    for dirpath, dirnames, filenames in os.walk(pack_path):
        for entry in list(dirnames) + list(filenames):
            full = os.path.join(dirpath, entry)
            if os.path.islink(full):
                raise PackError('symbolic link %s' % full)
        for filename in filenames:
            full = os.path.join(dirpath, filename)
            relative = full[len(pack_path):].lstrip(os.sep).replace(os.sep, '/')
            if relative == 'pack.toml':
                continue
            check_relative(relative)
            if relative.lower().endswith(('.txt', '.md')):
                continue  # a readme or licence text: the game ignores it
            if not relative.lower().endswith('.dds'):
                raise PackError('%s: a pack holds .dds pictures, pack.toml and .txt or .md notes only' % relative)
            if not re.match(r'^.+/[0-9]+\.dds$', relative):
                raise PackError('%s: a picture is named <tag path>/<bitmap index>.dds' % relative)
            size = os.path.getsize(full)
            if size > MAX_MEMBER:
                raise PackError('%s: larger than %d MB' % (relative, MAX_MEMBER // (1024 * 1024)))
            with open(full, 'rb') as handle:
                data = handle.read()
            try:
                description = check_dds(data)
            except PackError:
                raise PackError('%s: %s' % (relative, sys.exc_info()[1]))
            expected = hashes.get(relative)
            if expected is not None and hashlib.sha256(data).hexdigest() != expected:
                raise PackError('%s: SHA-256 differs from pack.toml' % relative)
            seen[relative] = 1
            count += 1
            report('  ok %s (%s)' % (relative, description))
    for relative in hashes:
        if relative not in seen:
            raise PackError('pack.toml lists %s, which is not in the pack' % relative)
    if count == 0:
        raise PackError('no pictures')
    report('%s %s by %s, licence %s: %d pictures%s' % (
        info.get('name'), info.get('version', ''), info.get('author', '?'), info.get('licence'), count,
        '' if hashes else ' (pack.toml lists no SHA-256 checksums)'))
    return count


# ---- commands ----

def command_list(root, args):
    base = packs_dir(root)
    if not os.path.isdir(base):
        print('no texture_packs folder in %s' % root)
        return 0
    found = 0
    for name in sorted(os.listdir(base)):
        path = os.path.join(base, name)
        if name.startswith('.') or not os.path.isdir(path) or os.path.islink(path):
            continue
        found += 1
        try:
            info, _hashes = read_info(path)
            print('%s: %s %s by %s, licence %s' % (name, info.get('name'), info.get('version', ''),
                                                    info.get('author', '?'), info.get('licence')))
        except PackError:
            print('%s: not a valid pack (%s)' % (name, sys.exc_info()[1]))
    if not found:
        print('no packs in %s' % base)
    return 0


def command_verify(root, args):
    if len(args) != 1:
        raise PackError('verify takes a pack name')
    check_pack_name(args[0])
    path = os.path.join(packs_dir(root), args[0])
    if not os.path.isdir(path) or os.path.islink(path):
        raise PackError('no pack %s' % args[0])
    verify_pack(path, lambda line: sys.stdout.write(line + '\n'))
    print('verified')
    return 0


def download(url, destination):
    if not (url.startswith('https://') or url.startswith('http://')):
        raise PackError('only http and https URLs')
    sys.stdout.write('downloading %s\n' % url)
    source = urlrequest.urlopen(url)
    total = 0
    try:
        with open(destination, 'wb') as out:
            while True:
                block = source.read(1 << 16)
                if not block:
                    break
                total += len(block)
                if total > MAX_ARCHIVE:
                    raise PackError('the download is larger than %d MB' % (MAX_ARCHIVE // (1024 * 1024)))
                out.write(block)
    finally:
        source.close()


def safe_extract(archive, scratch):
    """Unpack a zip into the empty folder scratch, checking every member first."""
    members = archive.infolist()
    if len(members) > MAX_MEMBERS:
        raise PackError('more than %d files in the archive' % MAX_MEMBERS)
    names = []
    total = 0
    for member in members:
        name = member.filename
        mode = (member.external_attr >> 16) & 0xFFFF
        if stat.S_ISLNK(mode):
            raise PackError('symbolic link in the archive: %r' % name)
        if (mode & 0xF000) and not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
            raise PackError('not a plain file or folder: %r' % name)
        if member.flag_bits & 0x1:
            raise PackError('the archive is encrypted')
        isdir = name.endswith('/')
        check_relative(name.rstrip('/'))
        if not isdir:
            if member.file_size > MAX_MEMBER:
                raise PackError('%r is larger than %d MB' % (name, MAX_MEMBER // (1024 * 1024)))
            total += member.file_size
            if total > MAX_TOTAL:
                raise PackError('the archive unpacks to more than %d MB' % (MAX_TOTAL // (1024 * 1024)))
        names.append((member, name, isdir))
    # one top folder around pack.toml is stripped
    prefix = ''
    if not any(n == 'pack.toml' for (_m, n, _d) in names):
        tops = set([n.split('/')[0] for (_m, n, _d) in names])
        if len(tops) == 1:
            top = list(tops)[0]
            if any(n == top + '/pack.toml' for (_m, n, _d) in names):
                prefix = top + '/'
    seen = set()
    for (member, name, isdir) in names:
        if prefix:
            if not name.startswith(prefix):
                raise PackError('%r is outside the pack folder' % name)
            name = name[len(prefix):]
            if not name:
                continue
        target = os.path.join(scratch, *name.rstrip('/').split('/'))
        if os.path.commonprefix([os.path.realpath(target), os.path.realpath(scratch)]) != os.path.realpath(scratch):
            raise PackError('%r leaves the folder' % name)
        key = name.lower()
        if key in seen and not isdir:
            raise PackError('%r appears twice' % name)
        seen.add(key)
        if isdir:
            if not os.path.isdir(target):
                os.makedirs(target)
            continue
        folder = os.path.dirname(target)
        if not os.path.isdir(folder):
            os.makedirs(folder)
        data = archive.read(member.filename)
        if len(data) != member.file_size:
            raise PackError('%r has the wrong size' % name)
        with open(target, 'wb') as out:
            out.write(data)


def command_install(root, args):
    source = None
    expected = None
    name = None
    force = False
    index = 0
    while index < len(args):
        arg = args[index]
        if arg == '--sha256' and index + 1 < len(args):
            expected = args[index + 1].strip().lower()
            index += 2
        elif arg == '--name' and index + 1 < len(args):
            name = args[index + 1]
            index += 2
        elif arg == '--force':
            force = True
            index += 1
        elif source is None and not arg.startswith('--'):
            source = arg
            index += 1
        else:
            raise PackError('unknown argument %r' % arg)
    if not source or not expected:
        raise PackError('install needs a zip file or URL and --sha256 HASH')
    if not re.match(r'^[0-9a-f]{64}$', expected):
        raise PackError('--sha256 is 64 hexadecimal digits')
    if name is not None:
        check_pack_name(name)
    base = packs_dir(root)
    if not os.path.isdir(base):
        os.makedirs(base)
    # a scratch folder inside texture_packs, so the final move stays on one volume
    scratch_root = tempfile.mkdtemp('.install', '.scratch-', base)
    try:
        archive_path = os.path.join(scratch_root, 'download.zip')
        unpacked = os.path.join(scratch_root, 'unpacked')
        os.mkdir(unpacked)
        if source.startswith('http://') or source.startswith('https://'):
            download(source, archive_path)
        else:
            if not os.path.isfile(source):
                raise PackError('no such file %s' % source)
            shutil.copyfile(source, archive_path)
        actual = sha256_of(archive_path)
        if actual != expected:
            raise PackError('SHA-256 of the archive is %s, not %s; nothing was installed' % (actual, expected))
        if not zipfile.is_zipfile(archive_path):
            raise PackError('not a zip file')
        archive = zipfile.ZipFile(archive_path, 'r')
        try:
            safe_extract(archive, unpacked)
        finally:
            archive.close()
        verify_pack(unpacked, lambda line: None)
        info, _hashes = read_info(unpacked)
        if name is None:
            name = re.sub(r'[^A-Za-z0-9_.\- ]', '_', info['name']).strip('. ')[:60] or 'pack'
            check_pack_name(name)
        destination = os.path.join(base, name)
        if os.path.lexists(destination):
            if not force:
                raise PackError('%s is installed already (--force replaces it)' % name)
            if os.path.islink(destination):
                raise PackError('%s is a symbolic link; not replacing it' % destination)
            shutil.rmtree(destination)
        shutil.move(unpacked, destination)
        print('installed %s (%s %s, licence %s) into %s' % (name, info.get('name'), info.get('version', ''),
                                                              info.get('licence'), destination))
        print('use it with display.texture_pack = "%s" (or HALO_TEXTURE_PACK=%s)' % (name, name))
    finally:
        shutil.rmtree(scratch_root, True)
    return 0


def command_remove(root, args):
    if len(args) != 1:
        raise PackError('remove takes a pack name')
    check_pack_name(args[0])
    path = os.path.join(packs_dir(root), args[0])
    if os.path.islink(path):
        raise PackError('%s is a symbolic link; remove it by hand' % path)
    if not os.path.isdir(path):
        raise PackError('no pack %s' % args[0])
    read_info(path)  # only a folder that is a pack is removed
    shutil.rmtree(path)
    print('removed %s' % args[0])
    return 0


def main(argv):
    args = list(argv[1:])
    root = None
    if len(args) >= 2 and args[0] == '--data-root':
        root = args[1]
        args = args[2:]
    if not args or args[0] in ('-h', '--help', 'help'):
        sys.stdout.write(__doc__)
        return 0
    commands = {'list': command_list, 'verify': command_verify, 'install': command_install, 'remove': command_remove}
    if args[0] not in commands:
        sys.stderr.write('unknown command %s\n' % args[0])
        return 2
    try:
        return commands[args[0]](root or default_root(), args[1:])
    except PackError:
        sys.stderr.write('error: %s\n' % sys.exc_info()[1])
        return 1
    except (IOError, OSError, zipfile.BadZipfile):
        sys.stderr.write('error: %s\n' % sys.exc_info()[1])
        return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
