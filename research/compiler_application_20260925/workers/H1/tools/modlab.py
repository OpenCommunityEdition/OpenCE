"""H1 lab (never lands): compile a generated TU with the INSTALLED modern MSVC (14.51, x86) at /Od and report
the .bss/.data layout. Used only to characterise the later /Od build's placement law, as data."""
import os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bsslab

MCL = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x86\cl.exe"
OUT = os.path.join(bsslab.OUT, "modern")
os.makedirs(OUT, exist_ok=True)


def compile_tu(entries, tag, lang="/TC", extra_flags=(), extra=""):
    ext = ".c" if lang == "/TC" else ".cpp"
    src = os.path.join(OUT, tag + ext)
    obj = os.path.join(OUT, tag + ".obj")
    open(src, "w", newline="\n").write(bsslab.make_source(entries, extra=extra))
    if os.path.exists(obj):
        os.remove(obj)
    r = subprocess.run([MCL, "/nologo", "/c", "/Od", lang, "/GS-"] + list(extra_flags) + ["/Fo" + obj, src],
                       capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0:
        raise RuntimeError("compile failed %s\n%s" % (tag, r.stdout[-2000:]))
    names = [e[0] for e in entries]
    res = bsslab.read_obj(obj, names)
    # C++ names are decorated (?name@@3...); map them back
    if lang == "/TP":
        import coffio
        o = coffio.load(obj)
        syms = {}
        for sy in o.symbols:
            nm = sy["name"]
            if nm.startswith("?"):
                base = nm[1:].split("@", 1)[0]
                if base in names and sy["sec"] > 0:
                    s = o.sections[sy["sec"] - 1]
                    syms[base] = (s["name"], sy["sec"], sy["value"], sy["storage"])
        for k, v in syms.items():
            res["syms"][k] = v
        res["order"] = sorted(res["syms"], key=lambda n: (res["syms"][n][1], res["syms"][n][2]))
    return res
