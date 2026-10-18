#!/usr/bin/env python
"""Make a checked native view of the real shared XML menu parser."""
from __future__ import print_function
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read_text(path):
    with open(path, "r") as stream:
        return stream.read()


def write(path, content):
    directory = os.path.dirname(path)
    if directory and not os.path.isdir(directory):
        os.makedirs(directory)
    if not os.path.isfile(path) or read_text(path) != content:
        with open(path, "w") as stream:
            stream.write(content)


def parser_view():
    source = read_text(os.path.join(ROOT, "port/linux/src/menu_files.c"))
    art = "/* ---------- art */"
    if source.count(art) != 1:
        raise SystemExit("menu art source boundary changed")
    source = source[:source.index(art)]
    header_end = "#define MAXIMUM_ART 1024"
    if source.count(header_end) != 1:
        raise SystemExit("menu parser header boundary changed")
    body = source[source.index("#define MAXIMUM_DEPTH 32"):]

    file_read_start = "static unsigned char *file_read(const char *path, unsigned long *size)"
    file_read_end = "/* the embedded files, then those of the menus folder:"
    if body.count(file_read_start) != 1 or body.count(file_read_end) != 1:
        raise SystemExit("menu file-read source boundary changed")
    start = body.index(file_read_start)
    end = body.index(file_read_end, start)
    body = body[:start] + body[end:]

    gather_start = "static void files_gather(void)"
    gather_end = "/* whether a bitmap's path stays inside the menus folder:"
    if body.count(gather_start) != 1 or body.count(gather_end) != 1:
        raise SystemExit("menu gathering source boundary changed")
    start = body.index(gather_start)
    end = body.index(gather_end, start)
    gather = '''static void files_gather(void)
{
\tunsigned int index;
\tfor (index = 0; index < menu_files_embedded_count; index++)
\t{
\t\tconst struct menu_file_embedded *embedded = &menu_files_embedded[index];
\t\tsize_t length = strlen(embedded->path);
\t\tif (length > 4 && !strcmp(embedded->path + length - 4, ".xml"))
\t\t\tfile_add(embedded->path, (const unsigned char *)embedded->data, embedded->size, 0);
\t}
}

'''
    body = body[:start] + gather + body[end:]

    file_data_start = "static const unsigned char *file_data(const char *path, unsigned long *size)"
    file_data_end = "/* ---------- reading */"
    if body.count(file_data_start) != 1 or body.count(file_data_end) != 1:
        raise SystemExit("menu bitmap file source boundary changed")
    start = body.index(file_data_start)
    end = body.index(file_data_end, start)
    body = body[:start] + body[end:]

    prefix = '''/* Checked native view of menu_files.c's Expat XML reader. */
/* port: this is the Mac's parser: its elements are those of the platform mac */
#define MENU_PLATFORM_MAC 1
#include "port/linux/include/halo_menus.h"
#include "menu_files.h"
#define __deprecated__(message)
#include "expat.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void platform_log(const char *format, ...);

'''
    return prefix + body


def embedded_xml():
    listing_path = os.path.join(ROOT, "port/assets/menus/menus.json")
    with open(listing_path, "r") as stream:
        listing = json.load(stream)["files"]
    names = [name for name in listing if name.endswith(".xml")]
    paths = [os.path.join(ROOT, "port/assets/menus", name) for name in names]
    if MAC_EXTRA:
        # port: the Mac's own screens (the pause menu), after the generated ones
        for extra in sorted(os.listdir(os.path.join(ROOT, "port/mac/menus"))):
            if extra.endswith(".xml"):
                names.append("mac/" + extra)
                paths.append(os.path.join(ROOT, "port/mac/menus", extra))
    lines = ['#include "menu_files.h"', ""]
    for index, name in enumerate(names):
        path = paths[index]
        with open(path, "rb") as stream:
            data = stream.read()
        lines.append("static const unsigned char menu_%d[] __attribute__((aligned(4))) = {" % index)
        for start in range(0, len(data), 16):
            values = []
            for byte in data[start:start + 16]:
                if not isinstance(byte, int):
                    byte = ord(byte)
                values.append("0x%02x" % byte)
            lines.append("\t" + ", ".join(values) + ",")
        lines.append("};")
    lines += ["", "const struct menu_file_embedded menu_files_embedded[] = {"]
    for index, name in enumerate(names):
        lines.append('\t{ "%s", (const unsigned int *)menu_%d, sizeof(menu_%d) },' % (name, index, index))
    lines += ["\t{ 0, 0, 0 },", "};", "const unsigned int menu_files_embedded_count = %d;" % len(names), ""]
    return "\n".join(lines)


MAC_EXTRA = "--mac" in sys.argv
if MAC_EXTRA:
    sys.argv.remove("--mac")
if len(sys.argv) != 3:
    raise SystemExit("usage: mac_menu_files_view.py [--mac] PARSER.c EMBEDDED_XML.c")
write(sys.argv[1], parser_view())
xml = embedded_xml()
write(sys.argv[2], xml)
with open(os.path.join(ROOT, "port/assets/menus/menus.json"), "r") as stream:
    count = sum(name.endswith(".xml") for name in json.load(stream)["files"])
print("generated checked menu XML parser and %d embedded XML assets" % count)
