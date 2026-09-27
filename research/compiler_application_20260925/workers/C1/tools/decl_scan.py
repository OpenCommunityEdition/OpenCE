"""READ-ONLY. Every declaration of every pooled symbol in our source tree (snapshot).

For each pooled record's C identifier, scans snapshot source/**, libs/** (*.c, *.h,
*.cpp, *.inc) with comments/strings blanked, and classifies each occurrence that is a
DECLARATOR (not a use):

  extern        'extern' declaration (file scope or block scope)
  tentative     file-scope definition with no initialiser  (-> COMMON in VC7 C)
  initialised   file-scope definition with '=' initialiser
  static        'static' storage (a different, file-private symbol)
Other occurrences are counted as uses; #define mentions are reported separately.
Also records the enclosing #if/#ifdef stack (include guards removed).

Writes data/decls.json.  Heuristic lexer: every row carries the statement text so a
reviewer can check it; nothing downstream treats a row as ownership proof.
"""
import json
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402

TOK = re.compile(r"[A-Za-z_][A-Za-z0-9_]*|0[xX][0-9A-Fa-f]+|\d+\.?\d*|\S")
EXTS = {".c", ".h", ".cpp", ".inc"}


def blank(text):
    """Blank comments and string/char literals, keep newlines and offsets."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)):
                out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def scan_file(path, names):
    raw = open(path, encoding="latin-1").read()
    txt = blank(raw)
    lines = txt.split("\n")
    rows = []
    # preprocessor stack + mask directive lines
    stack = []
    cond_at_line = []
    pp_lines = set()
    guard_candidates = {}
    i = 0
    logical = []
    while i < len(lines):
        ln = lines[i]
        s = ln.strip()
        start = i
        if s.startswith("#"):
            full = s
            while full.endswith("\\") and i + 1 < len(lines):
                i += 1
                full = full[:-1] + " " + lines[i].strip()
            for k in range(start, i + 1):
                pp_lines.add(k)
            d = re.match(r"#\s*(\w+)\s*(.*)", full)
            if d:
                kw, rest = d.group(1), d.group(2).strip()
                if kw in ("if", "ifdef", "ifndef"):
                    stack.append([kw + " " + rest, start])
                elif kw in ("elif", "else"):
                    if stack:
                        stack[-1][0] = stack[-1][0] + " | " + kw + " " + rest
                elif kw == "endif":
                    if stack:
                        stack.pop()
                elif kw == "define" and stack:
                    top = stack[-1][0]
                    m1 = re.match(r"ifndef (\w+)$", top)
                    m2 = re.match(r"(\w+)", rest)
                    if m1 and m2 and m1.group(1) == m2.group(1):
                        stack[-1][0] = "<include guard>"
                # macro mentions
                if kw == "define":
                    for nm in names:
                        if re.search(r"\b%s\b" % re.escape(nm), full):
                            rows.append({"kind": "macro", "line": start + 1, "text": full[:200]})
        for k in range(start, i + 1):
            cond_at_line.append([c for c, _ in stack if c != "<include guard>"])
        i += 1
    # token stream (skip directive lines)
    toks = []
    for ln_no, ln in enumerate(lines):
        if ln_no in pp_lines:
            continue
        for m in TOK.finditer(ln):
            toks.append((m.group(0), ln_no))
    depths = []
    dd = 0
    for u, _ in toks:
        depths.append(dd)
        if u == "{":
            dd += 1
        elif u == "}":
            dd -= 1
    # locate statements: split at ';' '{' '}' boundaries, but keep brace-initialisers
    # of file-scope declarations inside the statement.
    i = 0
    n = len(toks)
    while i < n:
        t, ln = toks[i]
        if t in names:
            # statement bounds
            a = i - 1
            d = 0
            while a >= 0:
                u = toks[a][0]
                if u in (")", "]"):
                    d += 1
                elif u in ("(", "["):
                    d -= 1
                    if d < 0:
                        d = 0
                elif d == 0 and u in (";",):
                    break
                elif d == 0 and u == "}":
                    # could be end of an initialiser brace inside this statement: stop anyway
                    break
                elif d == 0 and u == "{":
                    # brace-initialiser start '= {' belongs to the statement
                    if a > 0 and toks[a - 1][0] in ("=", ",", "{"):
                        a -= 1
                        continue
                    break
                a -= 1
            b = i + 1
            d = 0
            while b < n:
                u = toks[b][0]
                if u in ("(", "[", "{"):
                    d += 1
                elif u in (")", "]", "}"):
                    d -= 1
                    if d < 0:
                        break
                elif u == ";" and d == 0:
                    break
                b += 1
            stmt = [x[0] for x in toks[a + 1:b + 1]]
            pos = i - (a + 1)
            row = classify(stmt, pos, t, ln, cond_at_line[ln] if ln < len(cond_at_line) else [],
                           depths[i])
            row["name"] = t
            rows.append(row)
        i += 1
    return rows


def file_depth(toks, i):
    """Brace depth at token i (crude, cached per call site is fine for our sizes)."""
    d = 0
    for k in range(i):
        u = toks[k][0]
        if u == "{":
            d += 1
        elif u == "}":
            d -= 1
    return d


KEYW = {"extern", "static", "const", "volatile", "struct", "union", "enum", "unsigned", "signed",
        "long", "short", "int", "char", "float", "double", "void", "register", "typedef",
        "__declspec", "__cdecl", "__stdcall", "__fastcall", "_inline", "__inline", "inline"}


def classify(stmt, pos, name, ln, conds, depth):
    text = " ".join(stmt)
    row = {"line": ln + 1, "text": text[:240], "depth": depth, "conds": conds}
    # paren depth at pos, initialiser state, preceding/following tokens
    pd = bd = kd = 0
    in_init = False
    for k in range(pos):
        u = stmt[k]
        if u == "(":
            pd += 1
        elif u == ")":
            pd -= 1
        elif u == "[":
            kd += 1
        elif u == "]":
            kd -= 1
        elif u == "{":
            bd += 1
        elif u == "}":
            bd -= 1
        elif u == "=" and pd == 0 and bd == 0 and kd == 0:
            in_init = True
        elif u == "," and pd == 0 and bd == 0 and kd == 0:
            in_init = False
    prev = stmt[pos - 1] if pos > 0 else ""
    nxt = stmt[pos + 1] if pos + 1 < len(stmt) else ""
    first = stmt[0] if stmt else ""
    is_fnptr = prev == "*" and pos >= 2 and stmt[pos - 2] == "(" and nxt == ")"
    if "typedef" in stmt[:pos]:
        row["kind"] = "typedef"
        return row
    if in_init or bd > 0 or kd > 0:
        row["kind"] = "use"
        return row
    if pd > 0 and not is_fnptr:
        row["kind"] = "use"
        return row
    if nxt == "(" and not is_fnptr:
        row["kind"] = "function"
        return row
    # a declarator needs a type-ish token before it
    head = stmt[:pos]
    typeish = bool(head) and (head[-1] in ("*", ",", "(") or re.match(r"[A-Za-z_]\w*$", head[-1]) is not None)
    if not typeish or head[-1] in ("return", "sizeof", "case", "goto"):
        row["kind"] = "use"
        return row
    # plain expression statements like 'x = foo;' have no type before the name
    if head and head[-1] not in KEYW and not re.match(r"[A-Za-z_]\w*$", head[-1]) and head[-1] not in ("*", ",", "("):
        row["kind"] = "use"
        return row
    # after the declarator: [..] then '=' ?
    k = pos + 1
    if is_fnptr:
        # skip ') (params)'
        pdp = 0
        k = pos + 2
        if k < len(stmt) and stmt[k] == "(":
            while k < len(stmt):
                if stmt[k] == "(":
                    pdp += 1
                elif stmt[k] == ")":
                    pdp -= 1
                    if pdp == 0:
                        k += 1
                        break
                k += 1
    while k < len(stmt) and stmt[k] == "[":
        while k < len(stmt) and stmt[k] != "]":
            k += 1
        k += 1
    after = stmt[k] if k < len(stmt) else ""
    if after not in (";", ",", "=", ""):
        row["kind"] = "use"
        return row
    if depth > 0 and "extern" not in head and "static" not in head:
        # block-scope local of the same name, or an expression: not a global declaration
        row["kind"] = "local_or_use"
        return row
    # a head that is just an identifier like 'foo bar' at depth 0 is a declaration;
    if "extern" in head:
        row["kind"] = "extern"
    elif "static" in head:
        row["kind"] = "static"
    elif after == "=":
        row["kind"] = "initialised"
    else:
        row["kind"] = "tentative"
    row["type_head"] = " ".join(t for t in head if t not in ("extern", "static"))[:160]
    row["declarator_suffix"] = " ".join(stmt[pos + 1:k])[:80]
    return row


def main():
    recs, _ = C.pool_records()
    cnames = {C.c_name(r["name"]): r["name"] for r in recs if r["section"] == ".bss"}
    names = set(cnames)
    out = defaultdict(list)
    roots = [C.SNAP / "source", C.SNAP / "libs"]
    nfiles = 0
    for root in roots:
        for dp, dn, fn in os.walk(root):
            for f in fn:
                if os.path.splitext(f)[1].lower() not in EXTS:
                    continue
                p = os.path.join(dp, f)
                raw = open(p, encoding="latin-1").read()
                if not any(nm in raw for nm in names):
                    continue
                nfiles += 1
                rel = os.path.relpath(p, C.SNAP).replace("\\", "/")
                present = [nm for nm in names if nm in raw]
                for row in scan_file(p, set(present)):
                    # recover the name for macro rows
                    if row["kind"] == "macro":
                        for nm in present:
                            if re.search(r"\b%s\b" % re.escape(nm), row["text"]):
                                out[cnames[nm]].append(dict(row, file=rel, name=nm))
                        continue
                    out[cnames[row["name"]]].append(dict(row, file=rel))
    json.dump(out, open(C.DATA / "decls.json", "w"), indent=1)
    from collections import Counter
    kinds = Counter()
    for nm, rows in out.items():
        for r in rows:
            kinds[r["kind"]] += 1
    print("files scanned with a hit:", nfiles, "names with any hit:", len(out))
    print(dict(kinds))


if __name__ == "__main__":
    main()
