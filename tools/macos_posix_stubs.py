#!/usr/bin/env python3

import re
import sys


def main():
    header, output, imports = sys.argv[1:4]
    text = open(header, encoding="utf-8").read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    prototypes = re.findall(r"^([A-Za-z_][\w \*]*?)\b(posix_\w+)\s*\(([^;]*?)\);", text, re.M | re.S)
    out = [
        "#include <errno.h>",
        '#include "posix.h"',
        "",
        "int host_errno(void);",
        "",
    ]
    names = []
    for ret, name, params in prototypes:
        ret = " ".join(ret.split())
        params = " ".join(params.split())
        args = []
        if params != "void":
            for param in params.split(","):
                m = re.search(r"(\w+)\s*$", param.strip())
                args.append(m.group(1))
        host_name = "hostposix_" + name[len("posix_"):]
        names.append(host_name)
        out.append(f"{ret} {host_name}({params});")
        out.append(f"{ret} {name}({params})")
        out.append("{")
        call = f"{host_name}({', '.join(args)})"
        if ret == "void":
            out.append(f"\t{call};")
            out.append("\terrno = host_errno();")
        else:
            out.append(f"\t{ret} host_result = {call};")
            out.append("")
            out.append("\terrno = host_errno();")
            out.append("\treturn host_result;")
        out.append("}")
        out.append("")
    with open(output, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    with open(imports, "w", encoding="utf-8") as f:
        f.write("\n".join(names) + "\n")


if __name__ == "__main__":
    main()
