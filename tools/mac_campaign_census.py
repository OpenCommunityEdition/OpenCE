#!/usr/bin/env python3
"""Read-only census of a Halo PC retail cache file's scenario (little-endian).

Usage: python3 -I tools/mac_campaign_census.py [--json] map [map ...]
Field offsets follow struct scenario in source/scenario/scenario_definitions.h
(tag_reference 16 bytes, tag_block 12, tag_data 20). A failed sanity check
prints a warning and the value is reported as None, never guessed.
"""
import collections, json, struct, sys

BASE = 0x40440000
R, B, D = ('ref', 16), ('block', 12), ('data', 20)
LAYOUT = [
    ('ugly_structure_bsp', R), ('unloved_globals', R), ('bad_sky', R),
    ('sky_references', B), ('type', ('raw', 2)), ('flags', ('raw', 2)),
    ('scenario_references', B), ('local_north', ('raw', 4)),
    ('header_unused', ('raw', 20)), ('reference_unused', ('raw', 136)),
    ('predicted_ui_resources', B), ('functions', B),
    ('editor_scenario_data', D), ('comments', B), ('user_edit_unused', ('raw', 224)),
    ('object_names', B), ('scenery', B), ('scenery_palette', B),
    ('bipeds', B), ('biped_palette', B), ('vehicles', B), ('vehicle_palette', B),
    ('equipment', B), ('equipment_palette', B), ('weapons', B), ('weapon_palette', B),
    ('device_groups', B), ('machines', B), ('machine_palette', B),
    ('controls', B), ('control_palette', B), ('light_fixtures', B),
    ('light_fixtures_palette', B), ('sound_scenery', B), ('sound_scenery_palette', B),
    ('unused_blocks', ('raw', 84)),
    ('starting_profiles', B), ('players', B), ('trigger_volumes', B),
    ('recorded_animations', B), ('netgame_flags', B), ('netgame_equipment', B),
    ('scenario_starting_equipment', B), ('bsp_switch_trigger_volumes', B),
    ('decals', B), ('decal_palette', B), ('detail_object_collection_palette', B),
    ('render_unused', ('raw', 84)),
    ('ai_actor_palette', B), ('ai_encounters', B), ('ai_command_lists', B),
    ('ai_animation_references', B), ('ai_script_references', B),
    ('ai_recording_references', B), ('ai_conversations', B),
    ('hs_syntax_data', D), ('hs_string_constants', D), ('hs_scripts', B),
    ('hs_globals', B), ('hs_references', B), ('hs_source_files', B),
    ('scripting_unused', ('raw', 24)),
    ('cutscene_flags', B), ('cutscene_camera_points', B), ('cutscene_chapter_titles', B),
    ('rapidly_dwindling_unused_space', ('raw', 108)),
    ('custom_object_names', R), ('ingame_help_text', R), ('hud_messages', R),
    ('structure_bsp_references', B),
]
OFF, BLOCKS, DATAS = {}, [], []
_o = 0
for _n, (_k, _s) in LAYOUT:
    OFF[_n] = _o
    if _k == 'block': BLOCKS.append(_n)
    if _k == 'data': DATAS.append(_n)
    _o += _s
SCENARIO_SIZE = _o
SCRIPT_TYPES = ['startup', 'dormant', 'continuous', 'static', 'stub', 'command_script']
MAXN = 65536


class Census:
    def __init__(self, path):
        self.path = path
        self.warn = []
        self.d = open(path, 'rb').read()

    def w(self, msg):
        self.warn.append(msg)

    def at(self, addr, what, span=1):
        o = self.idx_off + (addr - BASE)
        if addr < BASE or o < 0 or o + span > len(self.d):
            self.w('%s: address 0x%x outside file' % (what, addr))
            return None
        return o

    def run(self):
        d = self.d
        r = {'file': self.path, 'size': len(d)}
        if d[0:4] != b'daeh':
            self.w('header magic is %r, not b"daeh" (little-endian head)' % d[0:4])
        self.idx_off, = struct.unpack_from('<I', d, 0x10)
        if self.idx_off + 36 > len(d):
            self.w('tag index offset 0x%x outside file' % self.idx_off)
            return self.finish(r)
        tags_addr, scen_handle, _, count = struct.unpack_from('<IIII', d, self.idx_off)
        r['tag_count'] = count
        if count >= MAXN:
            self.w('tag count %d >= %d' % (count, MAXN))
            return self.finish(r)
        t0 = self.at(tags_addr, 'tag array', 32 * count)
        if t0 is None:
            return self.finish(r)
        tags = []
        by_handle = {}
        for i in range(count):
            g = d[t0 + 32 * i:t0 + 32 * i + 4]
            h, na, da = struct.unpack_from('<III', d, t0 + 32 * i + 12)
            name = ''
            no = self.at(na, 'tag name %d' % i)
            if no is not None:
                e = d.find(b'\0', no, no + 512)
                name = d[no:e if e >= 0 else no + 512].decode('latin1')
            grp = g[::-1].decode('latin1')
            tags.append((grp, h, name, da))
            by_handle[h] = (grp, name)
        gc = collections.Counter(t[0] for t in tags)
        r['groups'] = dict(gc)
        sc = [t for t in tags if t[1] == scen_handle and t[0] == 'scnr']
        if not sc:
            sc = [t for t in tags if t[0] == 'scnr']
        if len(sc) != 1:
            self.w('scenario tag not found uniquely (%d candidates)' % len(sc))
            return self.finish(r)
        r['scenario'] = sc[0][2]
        s = self.at(sc[0][3], 'scenario data', SCENARIO_SIZE)
        if s is None:
            return self.finish(r)
        blocks = {}
        for n in BLOCKS:
            c, a, _ = struct.unpack_from('<III', d, s + OFF[n])
            if c >= MAXN:
                self.w('block %s count %d >= %d' % (n, c, MAXN))
                blocks[n] = None
                continue
            if c and self.at(a, 'block ' + n, c) is None:
                blocks[n] = None
                continue
            blocks[n] = (c, a)
        r['blocks'] = {n: (v[0] if v else None) for n, v in blocks.items()}

        def ref_name(o):
            grp, = struct.unpack_from('4s', d, o)
            h, = struct.unpack_from('<I', d, o + 12)
            return grp[::-1].decode('latin1'), by_handle.get(h, (None, '?'))[1], h
        # BSPs
        bsps = []
        v = blocks.get('structure_bsp_references')
        if v:
            for i in range(v[0]):
                o = self.at(v[1], 'bsp ref') + 32 * i
                if o + 32 > len(d):
                    self.w('bsp ref %d outside file' % i); break
                bsps.append(ref_name(o + 16)[1])
        r['bsps'] = bsps
        # scripts
        st = collections.Counter()
        v = blocks.get('hs_scripts')
        if v:
            for i in range(v[0]):
                o = self.at(v[1], 'script') + 92 * i
                if o + 92 > len(d):
                    self.w('script %d outside file' % i); break
                t, = struct.unpack_from('<h', d, o + 32)
                if 0 <= t < len(SCRIPT_TYPES): st[SCRIPT_TYPES[t]] += 1
                else:
                    st['bad(%d)' % t] += 1
                    self.w('script %d has type %d' % (i, t))
        r['scripts_by_type'] = dict(st)
        # references
        rg = collections.Counter()
        refs = []
        v = blocks.get('hs_references')
        if v:
            for i in range(v[0]):
                o = self.at(v[1], 'hs ref') + 40 * i
                if o + 40 > len(d):
                    self.w('hs ref %d outside file' % i); break
                g, n, h = ref_name(o + 24)
                rg[g] += 1
                refs.append((g, n))
        r['hs_ref_groups'] = dict(rg)
        r['hs_refs'] = refs
        # data
        for n in DATAS:
            size, fl, off, addr, _ = struct.unpack_from('<IIIII', d, s + OFF[n])
            if size > (1 << 28):
                self.w('data %s size %d implausible' % (n, size)); size = None
            r[n + '_size'] = size
            if n == 'hs_syntax_data' and size:
                o = self.at(addr, 'syntax data', 56)
                if o is not None:
                    maxn, esz, cnt = (struct.unpack_from('<h', d, o + 32)[0],
                                      struct.unpack_from('<h', d, o + 34)[0],
                                      struct.unpack_from('<h', d, o + 46)[0])
                    if esz != 20 or not (0 <= cnt <= maxn < MAXN):
                        self.w('syntax header suspicious (max %d size %d count %d)' % (maxn, esz, cnt))
                    r['syntax_max'] = maxn
                    r['syntax_nodes'] = cnt
        return self.finish(r)

    def finish(self, r):
        r['warnings'] = self.warn
        return r


def main(argv):
    as_json = '--json' in argv
    paths = [a for a in argv if not a.startswith('--')]
    out = []
    for p in paths:
        c = Census(p)
        r = c.run()
        out.append(r)
        if as_json:
            continue
        print('== %s (%d bytes)' % (p, r['size']))
        print('scenario %s, tags %s' % (r.get('scenario'), r.get('tag_count')))
        for n, v in r.get('blocks', {}).items():
            print('  %-34s %s' % (n, v))
        print('  bsps %d: %s' % (len(r.get('bsps', [])), ', '.join(repr(b) for b in r.get('bsps', []))))
        print('  scripts by type', r.get('scripts_by_type'))
        print('  hs ref groups', r.get('hs_ref_groups'))
        print('  syntax nodes', r.get('syntax_nodes'), 'of', r.get('syntax_max'),
              'string data', r.get('hs_string_constants_size'))
        print('  top groups', collections.Counter(r.get('groups', {})).most_common(25))
        for w in r['warnings']:
            print('  WARNING', w)
    if as_json:
        json.dump(out, sys.stdout, indent=1)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
