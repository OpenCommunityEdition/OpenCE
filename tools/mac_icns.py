#!/usr/bin/env python3
"""Writes a .icns file from one square PNG, for the Mac app (port/mac/README.md).

Mac OS X 10.4 reads only the classic icon types (RLE colour channels with a
separate 8-bit mask: is32/s8mk, il32/l8mk, ih32/h8mk, it32/t8mk), so those are
written for 16, 32, 48 and 128 pixels. 10.5 and 10.6 add 256 and 512 pixels
(ic08, ic09), which they read as JPEG 2000 (with an alpha channel); 10.4
skips them. PNG in those two types is a 10.7 feature, so JPEG 2000 is the
default here; --png writes PNG instead.

usage: mac_icns.py [--png] [--small16 PNG] [--small32 PNG] picture.png out.icns
       mac_icns.py --check file.icns

--small16 and --small32 give a hand-simplified picture for those two sizes
(the picture is scaled to 16 or 32 pixels when they are not given).
--check parses a file, decodes every classic element back and prints a
summary; it exits with 1 when the file is not valid.
"""
import io
import struct
import sys

from PIL import Image, ImageFilter

CLASSIC = (
    (16, b"is32", b"s8mk"),
    (32, b"il32", b"l8mk"),
    (48, b"ih32", b"h8mk"),
    (128, b"it32", b"t8mk"),
)


def rle(channel):
    """The icns run-length coding of one channel (a bytes object).

    A byte 0..127 is followed by that many plus one literal bytes; a byte
    128..255 is followed by one byte repeated that minus 125 times (3 to 130).
    """
    out = bytearray()
    i = 0
    n = len(channel)
    while i < n:
        run = 1
        while i + run < n and run < 130 and channel[i + run] == channel[i]:
            run += 1
        if run >= 3:
            out.append(run + 125)
            out.append(channel[i])
            i += run
            continue
        start = i
        i += 1
        while i < n and i - start < 128:
            if i + 2 < n and channel[i] == channel[i + 1] == channel[i + 2]:
                break
            i += 1
        out.append(i - start - 1)
        out += channel[start:i]
    return bytes(out)


def unrle(data, count):
    out = bytearray()
    i = 0
    while len(out) < count:
        b = data[i]
        i += 1
        if b < 128:
            out += data[i:i + b + 1]
            i += b + 1
        else:
            out += bytes([data[i]]) * (b - 125)
            i += 1
    if len(out) != count or i != len(data):
        raise ValueError("run-length data does not fill its channel exactly")
    return bytes(out)


def element(tag, data):
    return tag + struct.pack(">I", len(data) + 8) + data


def scaled(image, size):
    """Resized with Lanczos on premultiplied alpha (no dark or light fringe)."""
    if image.size == (size, size):
        return image.convert("RGBA")
    premultiplied = image.convert("RGBa").resize((size, size), Image.LANCZOS)
    return premultiplied.convert("RGBA")


def sharpened(pic):
    """A light sharpening of the colour (not the mask) for the 16 and 32 pixel
    sizes, where a downscaled photograph-like picture is soft."""
    r, g, b, a = pic.split()
    rgb = Image.merge("RGB", (r, g, b)).filter(ImageFilter.UnsharpMask(0.8, 90, 2))
    r, g, b = rgb.split()
    return Image.merge("RGBA", (r, g, b, a))


def classic(pic, colour_tag, mask_tag):
    if pic.width <= 32:
        pic = sharpened(pic)
    r, g, b, a = pic.split()
    body = b"".join(rle(bytes(c.tobytes())) for c in (r, g, b))
    if pic.width == 128:
        body = b"\0\0\0\0" + body
    return element(colour_tag, body) + element(mask_tag, a.tobytes())


def large(pic, tag, use_png):
    buffer = io.BytesIO()
    if use_png:
        pic.save(buffer, "PNG")
    else:
        # a JP2 file (not a bare codestream), four components, RGB and alpha
        pic.save(buffer, "JPEG2000", irreversible=False, quality_mode="rates",
                 quality_layers=[1], no_jp2=False)
    return element(tag, buffer.getvalue())


def check(path):
    data = open(path, "rb").read()
    if data[:4] != b"icns" or struct.unpack(">I", data[4:8])[0] != len(data):
        print("bad header or total length")
        return 1
    seen = {}
    offset = 8
    while offset < len(data):
        tag = data[offset:offset + 4]
        length = struct.unpack(">I", data[offset + 4:offset + 8])[0]
        if length < 8 or offset + length > len(data):
            print("element %r overruns the file" % tag)
            return 1
        seen[tag] = data[offset + 8:offset + length]
        offset += length
    for size, colour, mask in CLASSIC:
        if colour not in seen or mask not in seen:
            print("missing %r or %r" % (colour, mask))
            return 1
        count = size * size
        body = seen[colour]
        if size == 128:
            if body[:4] != b"\0\0\0\0":
                print("it32 lacks its four zero bytes")
                return 1
            body = body[4:]
        # three channels, each coded on its own: decode them one after another
        position = 0
        for plane in range(3):
            out = bytearray()
            while len(out) < count:
                b = body[position]
                position += 1
                if b < 128:
                    out += body[position:position + b + 1]
                    position += b + 1
                else:
                    out += bytes([body[position]]) * (b - 125)
                    position += 1
            if len(out) != count:
                print("%r plane %d is %d bytes, not %d" % (colour, plane, len(out), count))
                return 1
        if position != len(body):
            print("%r has %d bytes left over" % (colour, len(body) - position))
            return 1
        if len(seen[mask]) != count:
            print("%r is %d bytes, not %d" % (mask, len(seen[mask]), count))
            return 1
        print("%s/%s %3d px ok" % (colour.decode(), mask.decode(), size))
    for tag, size in ((b"ic08", 256), (b"ic09", 512)):
        if tag not in seen:
            print("%s absent (10.5 and 10.6 show the 128 pixel icon)" % tag.decode())
            continue
        body = seen[tag]
        kind = "PNG" if body[:8] == b"\x89PNG\r\n\x1a\n" else \
            "JPEG 2000" if body[:12] == b"\0\0\0\x0cjP  \r\n\x87\n" else "unknown"
        im = Image.open(io.BytesIO(body))
        print("%s %3d px %s %s %s" % (tag.decode(), size, kind, im.mode, im.size))
        if kind == "unknown" or im.size != (size, size):
            return 1
    return 0


def main():
    args = sys.argv[1:]
    if len(args) == 2 and args[0] == "--check":
        sys.exit(check(args[1]))
    use_png = False
    small = {}
    while args and args[0].startswith("--"):
        if args[0] == "--png":
            use_png = True
            args = args[1:]
        elif args[0] in ("--small16", "--small32") and len(args) > 1:
            small[int(args[0][7:])] = Image.open(args[1])
            args = args[2:]
        else:
            sys.exit(__doc__)
    if len(args) != 2:
        sys.exit(__doc__)
    image = Image.open(args[0])
    if image.width != image.height or image.width < 256:
        sys.exit("the picture must be square, 256 pixels or more")
    body = b""
    for size, colour, mask in CLASSIC:
        body += classic(scaled(small.get(size, image), size), colour, mask)
    body += large(scaled(image, 256), b"ic08", use_png)
    if image.width >= 512:
        body += large(scaled(image, 512), b"ic09", use_png)
    with open(args[1], "wb") as out:
        out.write(element(b"icns", body))


main()
