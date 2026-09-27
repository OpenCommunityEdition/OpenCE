#!/usr/bin/env python3
"""Reject a Linux link in which a weak reference has no definition.

The Linux build marks every MSVC inline function `#pragma weak` (see
tools/linux_msvc_semantics.py). A reference to such a name that no object
defines stays a weak undefined symbol, which the linker silently resolves to
address 0 - and then drops from a non-PIE executable's symbol table, so the
check has to look at the input objects rather than the output.

It also rejects an executable that imports glibc's wide character
functions: the game uses a 16-bit wchar_t, so they would misread its strings.

Usage: linux_link_check.py <response file listing the objects> [executable]
"""

import re
import shlex
import shutil
import subprocess
import sys
from typing import Dict, List, Set

# Weak references the C runtime and libgcc make themselves.
RUNTIME_WEAK_REFERENCES = {
    "__gmon_start__",
    "_ITM_deregisterTMCloneTable",
    "_ITM_registerTMCloneTable",
    "__cxa_finalize",
    "_Jv_RegisterClasses",
    "__pthread_key_create",
    "pthread_cancel",
}


# llvm-nm also reads the LLVM bitcode objects of link-time optimised builds
NM = shutil.which("llvm-nm") or "nm"


def symbol_table(objects: List[str]) -> Dict[str, Set[str]]:
    """Map each object symbol name to the set of nm type letters seen."""
    output = subprocess.run(
        [NM, "--format=posix", *objects],
        check=True, capture_output=True, text=True,
    ).stdout
    symbols: Dict[str, Set[str]] = {}
    for line in output.splitlines():
        fields = line.split()
        # "name type [value size]"; object headers end with ':'
        if len(fields) >= 2 and not fields[0].endswith(":") and len(fields[1]) == 1:
            symbols.setdefault(fields[0], set()).add(fields[1])
    return symbols


def main() -> None:
    with open(sys.argv[1], "r", encoding="utf-8") as response:
        objects = [path for line in response for path in shlex.split(line)]
    symbols = symbol_table(objects)
    # global definitions only: a file-local static copy (t, d, ...) cannot
    # satisfy a reference from another object
    defined_types = set("TDBRCVW")
    missing = sorted(
        name
        for name, types in symbols.items()
        if types & {"w", "v"}
        and not types & defined_types
        and name not in RUNTIME_WEAK_REFERENCES
    )
    if len(sys.argv) > 2:
        imports = subprocess.run(
            [NM, "--undefined-only", sys.argv[2]],
            check=True, capture_output=True, text=True,
        ).stdout.split()
        wide = sorted(
            name for name in imports
            if re.match(r"(wcs|wmem|[a-z]*wprintf|towlower|towupper|isw)", name.split("@")[0])
        )
        if wide:
            print("the executable imports glibc wide character functions, which assume "
                  "a 32-bit wchar_t: " + ", ".join(wide))
            sys.exit(1)
    if missing:
        print(f"{len(missing)} weak reference(s) that no object defines "
              "(they would silently resolve to address 0):")
        for name in missing:
            print(f"  {name}")
        sys.exit(1)


if __name__ == "__main__":
    main()
