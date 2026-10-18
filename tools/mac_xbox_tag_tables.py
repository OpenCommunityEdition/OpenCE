#!/usr/bin/env python3
"""Generate port/mac/xbox_tag_tables.c: the byte-swap layout of every tag group of an Xbox map.

Sources, all the engine's own:
  - the compiler's layouts of the tag structures (clang, 32-bit, MSVC-like: tools/mac_xbox_tag_layouts.py)
  - the validator's schemas after their macros (tools/mac_xbox_schema.txt, written by tools/mac_xbox_schema_dump.c
    from port/linux/game/tag_schema_*.c): which structure each group's root and each block's element is, and
    which fields are blocks, data and references
  - comments in the headers that name a block's element where no schema does
usage: python3 tools/mac_xbox_tag_tables.py [--report]   (from the repository root; clang and the sources)
"""
import re, sys, os, glob, collections

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mac_xbox_tag_layouts as layouts

SCHEMA_TXT = "tools/mac_xbox_schema.txt"
SCHEMA_SOURCES = ["port/linux/game/tag_schema_%s.c" % n for n in ("objects", "models", "collision", "render", "effects", "scenario")]
OUT = "port/mac/xbox_tag_tables.c"

# ---------------------------------------------------------------- op kinds (xbox_tag_tables.h)
OP = dict(BYTES=0, SWAP16=1, SWAP32=2, FLOAT32=3, SWAP64=4, BLOCK=5, DATA=6, REF=7, ARRAY=8, HOOK=9)
# data kinds
DATA = dict(BYTES=0, HS_SYNTAX=1, EVENTS=2, ANIM_INFO=3, ANIM_DEFAULT=4, ANIM_DATA=5, WORDS32=6, WORDS16=7)

# ---------------------------------------------------------------- the compiler's layouts
class Rec(object):
    def __init__(self, kind, name):
        self.kind, self.name, self.members, self.size = kind, name, [], 0

LINE = re.compile(r"^\s*(\d+)(?::(\d+)-(\d+))?\s+\|\s( *)(.*)$")

def parse_layouts(text):
    recs = {}
    cur = None
    stack = []
    for line in text.split("\n"):
        if line.startswith("*** Dumping AST Record Layout"):
            cur = None; stack = []; continue
        m = LINE.match(line)
        if not m:
            m2 = re.match(r"^\s*\|\s+\[sizeof=(\d+)", line)
            if m2 and cur is not None:
                cur.size = int(m2.group(1))
            continue
        off, hi, lo, indent, rest = int(m.group(1)), m.group(2), m.group(3), m.group(4), m.group(5)
        level = len(indent) // 2
        if cur is None:
            mm = re.match(r"^(struct|union) (.*?)\s*$", rest)
            if not mm: continue
            name = mm.group(2)
            if name in recs:        # the same record from another unit
                cur = Rec(mm.group(1), name)
                cur.dup = True
            else:
                cur = Rec(mm.group(1), name); recs[name] = cur
            stack = [cur]
            continue
        # a member: type and name (the name is the last word)
        mm = re.match(r"^(.*?)\s+(\S+)\s*$", rest)
        if rest.endswith(" "):          # an anonymous member: its type only
            ty, name = rest.strip(), ""
        elif mm: ty, name = mm.group(1), mm.group(2)
        else: ty, name = rest, ""
        member = dict(offset=off, type=ty, name=name, bit=(hi is not None), children=[])
        # the parent is the nearest shallower member
        while len(stack) > level: stack.pop()
        parent = stack[-1]
        if isinstance(parent, Rec) and not getattr(parent, "dup", False): parent.members.append(member)
        elif isinstance(parent, dict): parent["children"].append(member)
        if isinstance(parent, Rec) and getattr(parent, "dup", False): pass
        stack.append(member)
    return {k: v for k, v in recs.items()}

SCALARS = {"char": 1, "signed char": 1, "unsigned char": 1, "short": 2, "unsigned short": 2, "int": 4, "unsigned int": 4,
           "long": 4, "unsigned long": 4, "float": 4, "long long": 8, "unsigned long long": 8, "double": 8, "_Bool": 1}

def split_array(ty):
    m = re.match(r"^(.*?)((?:\[\d+\])+)$", ty)
    if not m: return ty, []
    return m.group(1), [int(x) for x in re.findall(r"\[(\d+)\]", m.group(2))]

# ---------------------------------------------------------------- the schemas
class Schema(object):
    pass

def parse_schema(path):
    groups, defs, order = [], {}, []
    for line in open(path):
        p = line.split()
        if not p or p[0].startswith("#"): continue
        if p[0] == "group":
            groups.append(dict(tag=p[1].replace("_", " ") if p[1].endswith("_") and False else p[1], parents=p[2:4], definition=p[4]))
        elif p[0] == "def":
            defs[p[1]] = dict(name=p[1], size=int(p[2]), fields=[]); order.append(p[1])
        elif p[0] == "field":
            defs[p[1]]["fields"].append(dict(type=int(p[2]), size=int(p[3]), count=int(p[4]), offset=int(p[5]), maximum=int(p[6]),
                                             target=p[7] if p[7] != "-" else None, name=p[8] if len(p) > 8 else ""))
    return groups, defs, order

def definition_types(defs):
    """definition name -> C type, from the TAG_SCHEMA_DEFINITION lines of the sources"""
    types = {}
    text = ""
    for f in SCHEMA_SOURCES: text += open(f, errors="replace").read() + "\n"
    for m in re.finditer(r"TAG_SCHEMA_DEFINITION\(\s*([A-Za-z_0-9#]+)\s*,\s*((?:struct|union)\s+[A-Za-z_0-9]+|[A-Za-z_0-9]+)\s*,", text):
        types[m.group(1)] = m.group(2)
    # the names made by a macro (name##_palette): the macro's uses
    return types

# a block of plain numbers: its definition's type
SCALAR_DEFS = {"long": ("SWAP32", 4), "unsigned long": ("SWAP32", 4), "dword": ("SWAP32", 4), "real": ("FLOAT32", 4),
               "word": ("SWAP16", 2), "short": ("SWAP16", 2), "byte": ("BYTES", 1), "char": ("BYTES", 1), "unsigned char": ("BYTES", 1)}

# block elements the schemas and the headers' comments do not name (struct, field) -> element struct
OVERRIDES = {("weapon_hud_interface_definition", "warning_sounds"): "hud_sound_definition",
             ("grenade_hud_interface_definition", "warning_sounds"): "hud_sound_definition",
             ("unit_hud_interface_definition", "warning_sounds"): "hud_sound_definition"}
# blocks the engine never reads (the validator leaves them out; counts are reported by the checker)
UNREAD = {("_weapon_definition", "unused_block"), ("ai_command_list_definition", "unused_blocks"), ("ai_conversation", "unused"),
          ("damage_resistance", "modifiers"), ("game_globals", "playlist"), ("scenario", "comments"), ("scenario", "decals"),
          ("scenario", "functions"), ("scenario", "scenario_references"), ("scenario", "unused_blocks"), ("squad_definition", "unused_block"),
          ("structure_bsp", "markers")}

# ---------------------------------------------------------------- the engine's own definitions of the same structures
ENGINE_CACHE = "/tmp/opence-engine-layouts.json"
def engine_variants():
    """name -> [(file, Rec)] for every structure the engine's units (source/**/*.c) define, one per distinct layout"""
    import json
    def pack(rec): return dict(kind=rec.kind, name=rec.name, size=rec.size, members=rec.members)
    def unpack(d):
        r = Rec(d["kind"], d["name"]); r.size = d["size"]; r.members = d["members"]; return r
    if os.path.exists(ENGINE_CACHE):
        try: return {k: [(f, unpack(d)) for (f, d) in v] for k, v in json.load(open(ENGINE_CACHE)).items()}
        except Exception: pass
    from concurrent.futures import ThreadPoolExecutor
    files = sorted(glob.glob("source/**/*.c", recursive=True))
    def one(f):
        try:
            o, e, rc = layouts.dump(f)
            return f, parse_layouts(o + e + "\n")
        except Exception:
            return f, {}
    out = {}
    with ThreadPoolExecutor(8) as ex:
        for f, recs in ex.map(one, files):
            for name, rec in recs.items():
                sig = (rec.kind, rec.size, tuple((m["offset"], m["type"], m["name"]) for m in rec.members))
                lst = out.setdefault(name, [])
                if not any(x[2] == sig for x in lst): lst.append((f, rec, sig))
    out = {k: [(f, r) for (f, r, sig) in v] for k, v in out.items()}
    json.dump({k: [(f, pack(r)) for (f, r) in v] for k, v in out.items()}, open(ENGINE_CACHE, "w"))
    return out

# ---------------------------------------------------------------- the main flatten
class Gen(object):
    def __init__(self):
        self.groups, self.defs, self.order = parse_schema(SCHEMA_TXT)
        text = ""
        for f in SCHEMA_SOURCES:
            o, e, rc = layouts.dump(f)
            text += o + e + "\n"
        self.recs = parse_layouts(text)
        self.variants = engine_variants()
        self.overlaid = []
        self.quiet = 0
        self.clashes = set()
        self.deftype = definition_types(self.defs)
        self.warnings = []
        self.struct_of_def = {}
        self.layouts = collections.OrderedDict()   # struct name -> dict(ops, size)
        self.ids = {}
        self.unions = collections.Counter()
        self.opaque_blocks = collections.Counter()
        self.used_types = set()
        self.comment_types = self.scan_comments()
        self.usage_types = self.scan_usage()

    def warn(self, text):
        if text not in self.warnings: self.warnings.append(text)

    def scan_usage(self):
        """field name -> the element structs the engine's code takes from a block of that name"""
        found = collections.defaultdict(set)
        for f in glob.glob("source/**/*.c", recursive=True) + glob.glob("port/linux/game/*.c"):
            if "tag_schema" in f: continue
            try: text = open(f, errors="replace").read()
            except Exception: continue
            for m in re.finditer(r"TAG_BLOCK_GET_ELEMENT\(\s*&?\(?[A-Za-z_0-9\.\->\[\]\(\)\*]*?([A-Za-z_0-9]+)\s*,\s*[^,]+,\s*struct\s+([A-Za-z_0-9]+)\s*\)", text):
                found[m.group(1)].add(m.group(2))
        return found

    def scan_comments(self):
        """(struct, field) -> element struct named in a comment after a tag_block field"""
        found = {}
        files = glob.glob("source/**/*.h", recursive=True) + glob.glob("source/**/*.c", recursive=True) + glob.glob("port/linux/game/*.h")
        for f in files:
            try: text = open(f, errors="replace").read()
            except Exception: continue
            for sm in re.finditer(r"\bstruct\s+([A-Za-z_0-9]+)\s*\n?\{(.*?)\n\};", text, re.S):
                for fm in re.finditer(r"struct\s+tag_block\s+([A-Za-z_0-9]+)\s*;\s*//\s*([A-Za-z_0-9]+)", sm.group(2)):
                    found.setdefault((sm.group(1), fm.group(1)), fm.group(2))
        return found

    # ---- which struct a definition is
    def struct_for(self, defname):
        if defname in self.struct_of_def: return self.struct_of_def[defname]
        d = self.defs[defname]
        cands = []
        t = self.deftype.get(defname)
        if t in SCALAR_DEFS:
            self.struct_of_def[defname] = "#" + t
            return "#" + t
        if t is None and defname in self.recs and self.recs[defname].kind == "struct":
            self.struct_of_def[defname] = defname
            return defname
        if t:
            nm = t.split()[-1]
            if nm in self.recs and self.recs[nm].size == d["size"]: cands.append(nm)
        if not cands:
            # by size and the offsets of its fields
            for nm, r in self.recs.items():
                if r.size != d["size"] or r.kind != "struct" or "::" in nm: continue
                names = {m["name"]: m["offset"] for m in r.members}
                ok = True
                for f in d["fields"]:
                    if f["name"] and "." not in f["name"] and f["name"] in names and names[f["name"]] != f["offset"]: ok = False
                if ok and all(("." in f["name"] or not f["name"] or f["name"] in names or f["type"] == 10) for f in d["fields"]):
                    cands.append(nm)
        if len(cands) != 1:
            self.warn("definition %s: %d struct candidates %s" % (defname, len(cands), cands[:4]))
            nm = cands[0] if cands else None
        else: nm = cands[0]
        self.struct_of_def[defname] = nm
        return nm

    def layout_id(self, struct):
        if struct not in self.ids:
            self.ids[struct] = len(self.ids)
            self.layouts[struct] = None   # filled by build()
        return self.ids[struct]

    # ---- the leaf walk of one struct
    def leaves(self, struct, base, out, depth=0, field_path=""):
        """append (offset, op, count, arg, detail) for every byte-bearing member of struct, absolute offsets;
        then the scalars that the engine's own definitions of the same structure (other units) type where
        the validator's definition leaves bytes (an 'unused' array the engine reads as reals)"""
        mine = []
        self.leaves_base(struct, base, mine, depth, field_path)
        if not self.quiet: self.overlay(struct, base, mine, depth)
        out.extend(mine)

    def lookup(self, name):
        """a structure by name: the validator's, else the engine's own definition (embedded structures of an engine variant)"""
        r = self.recs.get(name)
        if r is None:
            vs = self.variants.get(name)
            if vs: r = vs[0][1]
        return r

    def leaves_base(self, struct, base, out, depth=0, field_path="", rec=None):
        r = rec or self.lookup(struct)
        if r is None:
            self.warn("no layout for struct %s" % struct); return
        if depth > 12: self.warn("depth in %s" % struct); return
        if r.kind == "union":
            self.union(r, base, out, depth, field_path)
            return
        for m in r.members:
            self.member(m, base, out, depth, struct, field_path)

    def overlay(self, struct, base, ops, depth):
        primary = self.recs.get(struct)
        if primary is None or depth > 12: return
        vs = list(self.variants.get(struct, []))
        # the engine's structure for a tag is often named otherwise than the validator's (sky_definition / sky): the same size,
        # with the same blocks, data and references at the same places
        if not hasattr(self, "_by_size"):
            self._by_size = collections.defaultdict(list)
            for nm, lst in self.variants.items():
                for (f, rec) in lst: self._by_size[rec.size].append((nm, f, rec))
        def norm(n): return re.sub(r"(_definition|_full)$", "", n)
        for nm, lst in self.variants.items():
            if nm != struct and norm(nm) == norm(struct):
                for (f, rec) in lst: vs.append((f + " as " + nm, rec))
        mine_solid = None
        for (nm, f, rec) in self._by_size.get(primary.size, []):
            if nm == struct or rec.kind != "struct": continue
            if mine_solid is None:
                saved = list(self.warnings); self.quiet += 1
                tmp = []; self.leaves_base(struct, base, tmp, depth)
                self.quiet -= 1; self.warnings[:] = saved
                mine_solid = sorted((o - base, op) for (o, op, c, a, d) in tmp if op in ("REF", "BLOCK", "DATA"))
            if not mine_solid or len(mine_solid) < 2: break
            saved = list(self.warnings); self.quiet += 1
            tmp = []; self.leaves_base(nm, base, tmp, depth, "", rec)
            self.quiet -= 1; self.warnings[:] = saved
            if sorted((o - base, op) for (o, op, c, a, d) in tmp if op in ("REF", "BLOCK", "DATA")) == mine_solid:
                vs.append((f + " as " + nm, rec))
        have = set()
        for (f, rec) in vs:
            if rec.size > primary.size or rec is primary or rec.kind != primary.kind: continue
            extra = []
            saved = list(self.warnings); self.quiet += 1
            self.leaves_base(struct, base, extra, depth, "", rec)
            self.quiet -= 1; self.warnings[:] = saved
            wv = {"SWAP16": 2, "SWAP32": 4, "SWAP64": 8, "FLOAT32": 4}
            def cover(opx):
                res = set()
                for (o3, op3, c3, a3, d3) in opx:
                    if op3 in wv:
                        for i3 in range(c3): res.add((o3 + i3 * wv[op3], wv[op3]))
                return res
            def solid(opx):
                res = set()
                for (o3, op3, c3, a3, d3) in opx:
                    if op3 in ("REF", "BLOCK", "DATA"): res.add((o3, {"REF": 16, "BLOCK": 12, "DATA": 20}[op3]))
                    elif op3 == "ARRAY": res.add((o3, self.recs[a3].size * c3))
                return res
            mine_cover, theirs = cover(ops), cover(extra)
            blockers = mine_cover | solid(ops)
            # a layout of that name that disagrees with this one anywhere is another structure: not used
            conflict = [t for t in theirs if t not in mine_cover and any(m0 < t[0] + t[1] and t[0] < m0 + mw for (m0, mw) in blockers)]
            conflict += [t for t in solid(extra) if t not in solid(ops) and any(m0 < t[0] + t[1] and t[0] < m0 + mw for (m0, mw) in blockers)]
            if rec.size != primary.size and not (theirs & mine_cover) and not (solid(extra) and solid(extra) <= solid(ops)):
                continue
            if conflict:
                self.clashes.add((struct, rec.size, f, conflict[0][0] - base)); continue
            for (o, op, c, a, d) in extra:
                if op not in wv: continue
                lo, hi = o, o + wv[op] * c
                if hi > base + primary.size: continue
                if all(t in mine_cover for t in cover([(o, op, c, a, d)])): continue
                # the bytes must all be bytes or uncovered in what is there
                new = []
                for (o2, op2, c2, a2, d2) in ops:
                    if op2 == "BYTES" and o2 < hi and lo < o2 + c2:
                        if o2 < lo: new.append((o2, "BYTES", lo - o2, a2, d2))
                        if o2 + c2 > hi: new.append((hi, "BYTES", o2 + c2 - hi, a2, d2))
                    else: new.append((o2, op2, c2, a2, d2))
                new.append((o, op, c, a, d))
                ops[:] = new
                mine_cover = cover(ops)
                key = (struct, o - base, op, c, d)
                if key not in have:
                    have.add(key); self.overlaid.append("%s +%d %s x%d %s (%s)" % (struct, o - base, op, c, d, f))

    def member(self, m, base, out, depth, struct, path):
        ty, off = m["type"], base + m["offset"]
        if m["bit"]:
            self.warn("bit field in %s.%s" % (struct, m["name"]))
            out.append((off, "BYTES", 1, None, m["name"])); return
        elem, dims = split_array(ty)
        count = 1
        for d in dims: count *= d
        elem = elem.strip()
        name = m["name"]
        if elem in ("struct tag_block", "struct tag_data", "struct tag_reference"):
            kind = {"struct tag_block": "BLOCK", "struct tag_data": "DATA", "struct tag_reference": "REF"}[elem]
            for i in range(count):
                out.append((off + i * {"BLOCK": 12, "DATA": 20, "REF": 16}[kind], kind, 1, None, (struct, name)))
            return
        if elem in SCALARS or elem.endswith("*") or elem.startswith("enum ") or "(*)" in elem:
            size = SCALARS.get(elem, 4)
            op = "FLOAT32" if elem == "float" else {1: "BYTES", 2: "SWAP16", 4: "SWAP32", 8: "SWAP64"}[size]
            out.append((off, op, count, None, name))
            return
        mm = re.match(r"^(struct|union) (.*)$", elem)
        if mm:
            sname = mm.group(2).strip()
            # an anonymous struct or union: its children are the members
            if "anonymous" in sname or "unnamed" in sname:
                if dims and count > 1: self.warn("array of anonymous %s.%s" % (struct, name))
                self.children(m, base, out, depth, struct, mm.group(1))
                return
            r = self.lookup(sname)
            if r is None:
                self.warn("no layout for %s (member %s.%s)" % (elem, struct, name)); return
            if count == 1:
                self.leaves(sname, off, out, depth + 1, path + "." + name)
            else:
                out.append((off, "ARRAY", count, sname, name))
            return
        if elem.startswith("struct") or elem.startswith("union"):
            self.warn("odd member %s.%s : %s" % (struct, name, elem)); return
        self.warn("unknown type %r in %s.%s" % (elem, struct, name))

    def children(self, m, base, out, depth, struct, kind):
        if kind == "union":
            fake = Rec("union", "(anonymous)"); fake.members = m["children"]
            self.union(fake, base, out, depth, struct)
        else:
            for c in m["children"]: self.member(c, base, out, depth, struct, "")

    def union(self, r, base, out, depth, path):
        """members that start at the same place: if each reads as the same words, that; else the first and a warning"""
        views = []
        for m in r.members:
            v = []
            self.member(m, base, v, depth, r.name, path)
            views.append(v)
        key = lambda v: [(o, op if op != "FLOAT32" else "SWAP32", c) for (o, op, c, a, d) in self.expand(v)]
        keys = [key(v) for v in views]
        if all(k == keys[0] for k in keys):
            out.extend(views[0] if views else [])
            return
        # views that differ: prefer one with the same words as the largest view?
        self.unions[r.name] += 1
        self.warn("union %s in %s: its members are read differently (%s)" % (r.name, path, "; ".join("%s=%s" % (m["name"], key(v)[:4]) for m, v in zip(r.members, views))))
        out.extend(max(views, key=len) if views else [])

    def expand(self, ops):
        res = []
        for (o, op, c, a, d) in ops:
            if op in ("SWAP16", "SWAP32", "SWAP64", "FLOAT32", "BYTES"):
                w = {"SWAP16": 2, "SWAP32": 4, "FLOAT32": 4, "SWAP64": 8, "BYTES": 1}[op]
                for i in range(c): res.append((o + i * w, op, 1, None, d))
            else: res.append((o, op, c, a, d))
        return res

    # ---- build the layouts
    def build(self):
        # roots
        self.group_layouts = []
        for g in self.groups:
            s = self.struct_for(g["definition"]) if g["definition"] in self.defs else None
            g["struct"] = s
        self.root_name = {}
        for g in self.groups:
            st = g["struct"]
            if st and not st.startswith("#") and self.defs[g["definition"]]["size"] < self.recs[st].size:
                # a tag that ends before its structure does (control_definition: the editor's tail): its own layout
                g["struct"] = st + "@tag"; self.root_name[g["struct"]] = (st, self.defs[g["definition"]]["size"])
        done = set()
        self.element_of = {}     # (struct, field name) -> element struct
        queue = [g["struct"] for g in self.groups if g["struct"]]
        while queue:
            s = queue.pop(0)
            if s in done: continue
            done.add(s)
            if s in self.root_name:
                full, size = self.root_name[s]
                ops = []
                self.leaves(full, 0, ops)
                ops = [o for o in ops if o[0] < size]
                for o in ops:
                    if o[0] + WIDTH.get(o[1], 1) * (o[2] if o[1] not in ("BLOCK", "DATA", "REF", "ARRAY") else 1) > size and o[1] != "BYTES":
                        raise SystemExit("%s: an op past the tag's end" % s)
                self.layout_id(s)
                ops = self.resolve(full, ops, queue)
                self.layouts[s] = dict(size=size, ops=ops)
                continue
            self.layout_id(s)
            if s.startswith("#"):
                op, size = SCALAR_DEFS[s[1:]]
                self.layouts[s] = dict(size=size, ops=[(0, op, 1, None, s)])
                continue
            ops = []
            self.leaves(s, 0, ops)
            # blocks and arrays name more structs
            ops = self.resolve(s, ops, queue)
            self.layouts[s] = dict(size=self.recs[s].size, ops=ops)
        # 'ustr' shares the structures of 'str#' but its strings are UTF-16: its own copies, the strings as words
        if "string_list" in self.layouts and "string_list_entry" in self.layouts:
            sl, se = self.layouts["string_list"], self.layouts["string_list_entry"]
            self.layout_id("string_list@ustr"); self.layout_id("string_list_entry@ustr")
            self.layouts["string_list@ustr"] = dict(size=sl["size"], ops=[(o, op, c, "string_list_entry@ustr" if op == "BLOCK" else a, d) for (o, op, c, a, d) in sl["ops"]])
            self.layouts["string_list_entry@ustr"] = dict(size=se["size"], ops=[(o, op, c, "WORDS16" if op == "DATA" else a, d) for (o, op, c, a, d) in se["ops"]])
            for g in self.groups:
                if g["tag"] == "ustr": g["struct"] = "string_list@ustr"
        return

    def block_targets(self, struct):
        """offset -> element struct, from the schemas' block fields of every definition of this struct, and
        of the structures inline in it (at their offsets)"""
        if not hasattr(self, "_targets"):
            self._targets = {}
            self._by_struct = collections.defaultdict(list)
            for dn in self.defs: self._by_struct[self.struct_for(dn)].append(dn)
        if struct in self._targets: return self._targets[struct]
        result = {}
        self._targets[struct] = result
        for dn in self._by_struct.get(struct, []):
            self.collect(dn, 0, result, 0)
        return result

    def collect(self, dn, base, result, depth):
        if depth > 10: return
        for f in self.defs[dn]["fields"]:
            if f["type"] == 0 and f["target"]:
                result[base + f["offset"]] = self.struct_for(f["target"])
            elif f["type"] == 9 and f["target"]:
                for i in range(max(f["count"], 1)):
                    self.collect(f["target"], base + f["offset"] + i * f["size"], result, depth + 1)

    def load_struct(self, name):
        """a structure the six schema units do not include: the file that defines it, laid out too"""
        if name in self.recs: return True
        for f in glob.glob("source/**/*.[ch]", recursive=True):
            try: text = open(f, errors="replace").read()
            except Exception: continue
            if re.search(r"\bstruct\s+%s\s*\n?\{" % re.escape(name), text):
                src = f
                if f.endswith(".h"):
                    # a unit that includes the header and is a source in the tree
                    src = "/tmp/opence-extra-%s.c" % name
                    open(src, "w").write('#include "cseries.h"\n#include "%s"\n' % os.path.abspath(f))
                o, e, rc = layouts.dump(f if f.endswith(".c") else src)
                more = parse_layouts(o + e)
                for k, v in more.items(): self.recs.setdefault(k, v)
                return name in self.recs
        return False

    def block_element(self, root, offset, struct, field):
        t = self.block_targets(root).get(offset)
        if t: return t
        t = OVERRIDES.get((struct, field))
        if t and self.load_struct(t): return t
        t = self.comment_types.get((struct, field))
        if t and t in self.recs: return t
        u = [x for x in self.usage_types.get(field, ()) if x in self.recs]
        if len(u) == 1:
            self.used_types.add((struct, field, u[0]))
            return u[0]
        return None

    def resolve(self, struct, ops, queue):
        res = []
        for (o, op, c, a, d) in ops:
            if op == "BLOCK":
                el = self.block_element(struct, o, d[0], d[1])
                if el:
                    self.layout_id(el); queue.append(el)
                else: self.opaque_blocks[(d[0], d[1])] += 1
                res.append((o, op, 1, el, d))
            elif op == "ARRAY":
                self.layout_id(a); queue.append(a)
                res.append((o, op, c, a, d))
            elif op == "DATA":
                res.append((o, op, 1, self.data_kind(d[0], d[1]), d))
            else:
                res.append((o, op, c, a, d))
        return res

    def data_kind(self, struct, field):
        key = (struct, field)
        table = {("scenario", "hs_syntax_data"): "HS_SYNTAX", ("recorded_animation_definition", "event_stream"): "EVENTS",
                 ("animation", "frame_info"): "ANIM_INFO", ("animation", "default_data"): "ANIM_DEFAULT", ("animation", "data"): "ANIM_DATA",
                 ("structure_bsp", "cluster_data"): "WORDS32", ("hud_message_text_definition", "text_data"): "WORDS16", ("unicode_string_list_entry", "string"): "WORDS16"}
        return table.get(key, "BYTES")


WIDTH = {"BYTES": 1, "SWAP16": 2, "SWAP32": 4, "FLOAT32": 4, "SWAP64": 8, "BLOCK": 12, "DATA": 20, "REF": 16}

def finish(g):
    """each layout's ops sorted, runs merged, gaps made BYTES, overlaps refused; layout ids numbered"""
    ids = {name: i for i, name in enumerate(g.layouts)}
    out = []
    for name, L in g.layouts.items():
        size = L["size"]
        items = []
        for (o, op, c, a, d) in L["ops"]:
            if op == "ARRAY":
                items.append((o, op, c, ids[a], g.layouts[a]["size"] * c))
            elif op == "BLOCK":
                items.append((o, op, 1, ids[a] if a else -1, 12))
            elif op == "DATA":
                items.append((o, op, 1, DATA[a], 20))
            elif op == "REF":
                items.append((o, op, 1, 0, 16))
            else:
                items.append((o, op, c, 0, WIDTH[op] * c))
        split = []
        for it in items:
            (o, op, c, a, n) = it
            if op in ("BYTES", "SWAP16", "SWAP32", "FLOAT32", "SWAP64", "ARRAY") and c > 60000:
                w = n // c
                while c > 0:
                    k = min(c, 60000); split.append((o, op, k, a, k * w)); o += k * w; c -= k
            else: split.append(it)
        items = split
        items.sort(key=lambda t: (t[0], t[1]))
        merged = []
        at = 0
        for (o, op, c, a, n) in items:
            if o < at:
                raise SystemExit("%s: overlapping ops at %d (%s)" % (name, o, op))
            while o - at > 60000:
                merged.append([at, "BYTES", 60000, 0, 60000]); at += 60000
            if o > at: merged.append([at, "BYTES", o - at, 0, o - at])
            if merged and op in ("BYTES", "SWAP16", "SWAP32", "FLOAT32", "SWAP64") and merged[-1][1] == op and merged[-1][0] + merged[-1][4] == o and merged[-1][2] + c < 65535:
                merged[-1][2] += c; merged[-1][4] += n
            else:
                merged.append([o, op, c, a, n])
            at = o + n
        if at > size: raise SystemExit("%s: ops past the end (%d > %d)" % (name, at, size))
        while size - at > 60000:
            merged.append([at, "BYTES", 60000, 0, 60000]); at += 60000
        if at < size: merged.append([at, "BYTES", size - at, 0, size - at])
        out.append((name, size, merged))
    return ids, out

def emit(g, ids, layouts_out):
    lines = []
    w = lines.append
    w("/* GENERATED by tools/mac_xbox_tag_tables.py from the engine's tag structures and tools/mac_xbox_schema.txt: do not edit. */")
    w('#include "xbox_tag_tables.h"')
    w("")
    ops_all = []
    firsts = []
    for (name, size, ops) in layouts_out:
        firsts.append(len(ops_all)); ops_all.extend(ops)
    w("const struct mac_xbox_tag_op mac_xbox_tag_ops[%d] = {" % (len(ops_all) + 1))
    for (o, op, c, a, n) in ops_all:
        w("    { MAC_XBOX_OP_%s, %d, %d, %d }," % (op, min(c, 65535), o, a))
    w("    { 0, 0, 0, 0 } };")
    w("")
    w("const struct mac_xbox_tag_layout mac_xbox_tag_layouts[%d] = {" % len(layouts_out))
    for (name, size, ops), first in zip(layouts_out, firsts):
        w('    { %d, %d, %d, "%s" },' % (size, first, len(ops), name))
    w("};")
    w("const unsigned mac_xbox_tag_layout_count = %d;" % len(layouts_out))
    w("")
    groups = []
    for gr in g.groups:
        if gr["struct"]: groups.append((gr["tag"], ids[gr["struct"]]))
    w("const struct mac_xbox_tag_group mac_xbox_tag_groups[%d] = {" % (len(groups) + 1))
    for tag, lid in groups:
        t = tag.replace("_", " ") if len(tag) == 4 else tag
        w("    { 0x%08x, %d }, /* '%s' */" % (int.from_bytes(t.encode("latin1"), "big"), lid, t))
    w("    { 0, 0 } };")
    w("const unsigned mac_xbox_tag_group_count = %d;" % len(groups))
    w("")
    # offsets the blob hooks read (the compiler's)
    def off(struct, field):
        for m in g.recs[struct].members:
            if m["name"] == field: return m["offset"]
        raise SystemExit("no field %s.%s" % (struct, field))
    w("const struct mac_xbox_tag_hooks mac_xbox_tag_hook_offsets = {")
    w("    %d, /* animation layout */" % ids["animation"])
    for f in ("frame_count", "frame_size", "frame_info_type", "node_count", "flags", "nodes_with_translation_flags", "nodes_with_rotation_flags", "nodes_with_scale_flags", "frame_info", "default_data", "data"):
        w("    %d, /* animation.%s */" % (off("animation", f), f))
    w("    %d, /* recorded animation layout */" % ids["recorded_animation_definition"])
    w("    %d, /* recorded_animation_definition.unit_control_data_version */" % off("recorded_animation_definition", "unit_control_data_version"))
    w("    %d, /* recorded_animation_definition.event_stream */" % off("recorded_animation_definition", "event_stream"))
    w("};")
    return "\n".join(lines) + "\n"

def report(g, ids, layouts_out):
    print("groups %d layouts %d ops %d" % (len(g.groups), len(layouts_out), sum(len(o) for (_, _, o) in layouts_out)))

if __name__ == "__main__":
    g = Gen()
    g.build()
    ids, layouts_out = finish(g)
    text = emit(g, ids, layouts_out)
    if "--check" in sys.argv:
        print(len(g.warnings), "warnings"); sys.exit(0)
    open(OUT, "w").write(text)
    report(g, ids, layouts_out)
    for w in g.warnings: 
        if "hud_color_definition" not in w: print(" warn:", w[:200])
    for o in sorted(set(g.overlaid)): print(" engine definition types:", o)
    for o in sorted(g.clashes): print(" another structure of that name (not used):", o)
    print("hud_color_definition unions: the finer view is used (the engine reads objective.up_ticks/fade_ticks)")
    print("opaque blocks (counted by the checker): %s" % sorted(g.opaque_blocks))
