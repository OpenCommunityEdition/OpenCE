import os, crossmap, exeindex, janfn
ROOT = r"C:\halo-worktrees\claude-compiler-application-20260925"
sidx = exeindex.load(os.path.join(ROOT, "scratch", "campaign", "workers", "H1", "sapien_index.json"))
for obj, owners in [("objects/objects", {"_object_name_list", "_object_memory_pool", "_object_globals"}),
                    ("math/periodic_functions", {"_transition_function_tables", "_periodic_function_tables", "_function_tables_initialized"})]:
    path = os.path.join(ROOT, "build", "split", "source", *obj.split("/")) + ".obj"
    fmap, res, dsyms, secname = crossmap.run(path, owners, verbose=False, idx=sidx)
    o, jf, _, _ = janfn.facts(path)
    for own in sorted(owners):
        users = [f for f, d in jf.items() if any(x[2] and x[2][0] == own and x[2][1] == 0 for x in d['data'])]
        mapped = [(f, hex(fmap[f]), [s[:28] for s in jf[f]['strs']][:2]) for f in users if f in fmap]
        print(obj, own, '->', [hex(a) for a in res.get(own, [])], 'via', mapped[:2])
