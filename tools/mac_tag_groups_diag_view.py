#!/usr/bin/env python
"""Generate a diagnostic-only tag-block accessor view for native PPC runs.

The view preserves the original bounds assertion and adds the block descriptor
and call-site values immediately before it, so a bad map/runtime index can be
traced without weakening the check.
"""
import sys

source_path = "source/tag_files/tag_groups.c"
source = open(source_path, "r").read()
old = '''\t\tmatch_vassert("c:\\\\halo\\\\SOURCE\\\\tag_files\\\\tag_groups.c", 3089, index>=0 && index<block->count,
\t\t\tcsprintf(temporary,
\t\t\t\t"#%d is not a valid %s index in [#0,#%d)",
\t\t\t\tindex,
\t\t\t\tblock->definition ? block->definition->name : "<unknown>", block->count));'''
new = '''\t\tif (index < 0 || index >= block->count)
\t\t{
\t\t\tfprintf(stderr,
\t\t\t"native tag-block bounds: block=%p index=%ld count=%ld element=%ld "
\t\t\t"base=%p definition=%p caller=%08lx\\n",
\t\t\t(void *)block, index, block->count, element_size, block->address,
\t\t\t(void *)block->definition,
\t\t\t(unsigned long)__builtin_return_address(0));
\t\t}
\t\tmatch_vassert("c:\\\\halo\\\\SOURCE\\\\tag_files\\\\tag_groups.c", 3089, index>=0 && index<block->count,
\t\t\tcsprintf(temporary,
\t\t\t\t"#%d is not a valid %s index in [#0,#%d)",
\t\t\t\tindex,
\t\t\t\tblock->definition ? block->definition->name : "<unknown>", block->count));'''
if len(sys.argv) != 2:
    raise SystemExit("usage: mac_tag_groups_diag_view.py output.c")
if source.count(old) != 1:
    raise SystemExit("tag-block bounds assertion source boundary changed")
source = source.replace('#include "tag_files/tag_groups.h"',
                        '#include "tag_files/tag_groups.h"\n#include <stdio.h>', 1)
with open(sys.argv[1], "w") as output:
    output.write(source.replace(old, new))
