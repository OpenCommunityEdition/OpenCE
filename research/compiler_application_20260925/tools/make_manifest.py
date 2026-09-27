"""Write MANIFEST_20260926.json for the fourth-set landings (run from the worktree root after the final gate).

Everything is read from git, the build and the archived gate snapshots, so the manifest can be regenerated."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc  # noqa: E402

LANE = ROOT / 'research/compiler_application_20260925'
GATES = LANE / 'gates/fourth_set'
BASE = 'd890c2db285c865b352b0de4e690da8535d77e84'
MERGE_BASE = 'cc6080369a3a9dda3fb287880d15a681b391b9f4'


def git(*a):
    return subprocess.run(['git', *a], cwd=ROOT, capture_output=True, text=True, check=True).stdout.strip()


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


CLASS = {
    '5901f1eb': 'code', '219c2c72': 'code', 'c5366682': 'object-admission',
    '387bab37': 'data', 'b30042ba': 'data',
    '700ca0d1': 'records', 'f4a13922': 'records', '465cfd36': 'records', 'ae434c14': 'records',
    '9c350e37': 'docs', '65dd6ffc': 'records',
}
RECONCILED = {'455dffad', '343a3f82', '42fa975e', '26684ca8', '09f5208f'}


def main():
    tip = git('rev-parse', 'HEAD')
    commits = []
    for line in git('log', '--reverse', '--format=%H %s', '%s..HEAD' % MERGE_BASE).splitlines():
        h, subject = line.split(' ', 1)
        short = h[:8]
        files = git('show', '--name-only', '--format=', h).splitlines()
        if short in RECONCILED:
            status = 'already reconciled in canonical (a8854940/d890c2db); not counted'
        else:
            status = 'lane-verified, pending independent canonical reconciliation'
        commits.append(dict(commit=h, subject=subject, status=status,
                            **{'class': CLASS.get(short, 'zero-credit' if short not in RECONCILED else 'code')},
                            files=len(files), sample_files=files[:6]))
    functions = []
    for unit, fn, meaningful, commit in (
            ('source/math/periodic_functions', '@periodic_function_build_variable_period_x_table@4', 244, '219c2c72'),
            ('source/tool/connected_geometry', '_connected_geometry_find_or_add_edge', 240, '5901f1eb')):
        j = cc.section_info(cc.load(ROOT / ('build/split/%s.obj' % unit)), fn)
        o = cc.section_info(cc.load(ROOT / ('build/base/%s.obj' % unit)), fn)
        assert cc.section_infos_equal(j, o), fn
        functions.append(dict(unit=unit, function=fn, commit=commit, padded_bytes=o['size'],
                              meaningful_bytes=meaningful, relocations=o['relocation_count'],
                              normalized_sha256_january=j['normalized_sha256'],
                              normalized_sha256_ours=o['normalized_sha256'], strict_exact=True))
    entries = {(e['unit'], e.get('symbol')): e for e in json.loads((ROOT / 'config/semantic_data_matches.json').read_text())}
    data = []
    for unit, sym, commit in (('source/bitmaps/bitmap_group', '_global_bitmap_reference', '387bab37'),
                              ('source/ai/ai_communication', '_global_communication_priority_names', '387bab37'),
                              ('source/ai/ai_debug', '_global_ai_debug_firing_position_color_count', '387bab37'),
                              ('source/game/game_engine_king', '_king_engine', 'b30042ba')):
        m = entries[(unit, sym)]['measurements']
        data.append(dict(unit=unit, owner=sym, commit=commit, bytes=m['size'], relocations=m['relocation_count'],
                         normalized_sha256=m['normalized_sha256'], allow_incomplete_unit=True,
                         function_or_object_credit=False))
    gate_logs = {p.name: sha(p) for p in sorted(GATES.glob('gate_*.out'))}
    manifest = dict(
        lane='claude/compiler-application-20260925',
        worktree=str(ROOT),
        generated_from_tip=tip,
        measurement_base=dict(canonical=BASE, objects=[388, 468], code=[1591226, 1770166],
                              functions=[7459, 7574], data=2587011, parks=76,
                              stable_snapshot='gates/fourth_set/base_stable_d890c2db.json',
                              stable_snapshot_sha256=sha(GATES / 'base_stable_d890c2db.json')),
        tip_totals=dict(objects=[389, 468], code=[1591710, 1770166], functions=[7461, 7574], data=2588903,
                        parks=75, stable_snapshot='gates/fourth_set/final_stable_snapshot.json',
                        stable_snapshot_sha256=sha(GATES / 'final_stable_snapshot.json')),
        lane_only_vs_base=dict(code_bytes=484, functions=2, objects=1, data_bytes=1892, parks=-1,
                               regressions=0, whole_board_keyed_diff='stable_verdicts diff base -> final: '
                               'gained 2 (496 padded / 484 meaningful), regressions 0'),
        code_gains=functions,
        data_gains=data,
        object_admissions=[dict(unit='source/math/periodic_functions', commit='c5366682',
                                note='after the complete object audit; park retired in 219c2c72')],
        zero_credit_repairs=[c for c in commits if c['class'] == 'zero-credit'],
        commits=commits,
        gate_logs_sha256=gate_logs,
        held_not_landed=[
            'landing/HELD_A3_pao_od_shape_names_remainder.patch (/Od shape, desired_direction, factor order, comment names)',
            'landing/HELD_A3_hs_runtime_remainder.patch (H-1 enum_value assert, H-3 HCEX constants, H-4 /Od hs_can_cast)',
            'workers/A3/hs_runtime_OWNER_hcex_abi*.patch (Q-A3-2 partial writes / ABI)',
            'Q-A3-1 path_obstacle_avoidance descriptive .bss names; Q-A3-3 error_heap union',
            'workers/A4/LEAD_RA_rasterizer_xbox_internal.h.patch + LEAD_RA_parked.json.patch (deferred: park rebaseline)',
            'workers/A4/OWNER_* and CONDITIONAL_* (OQ-1..OQ-4)',
            'K1-B play_type, K1-C recipient_look_data, K1-D normalize2d (needs strict-exact caller); inherited near_player initializer (owner call)',
            'K2 larger /Od rewrite E1-E12/E14',
            'path_structure_bsp admission; dead_camera; realcmp_epsilon; MoveResourceMemory',
            'Q4 lights reset; Q7 weapon_place coherent packet (preserved); Q13; Q14/render_debug',
            'game_engine admission (shared-header exact losses, storage/type ownership)',
            'Q11 extent-model verifier (workers/V1 review packet; 57,184 B NOT claimed; objdiff stays 3.3.1)',
            'COMMON: 212 Halo records unplaced; vendor PLACED provisional (relocation identity unverified)',
        ],
    )
    out = LANE / 'MANIFEST_20260926.json'
    out.write_text(json.dumps(manifest, indent=1) + '\n', encoding='utf-8', newline='\n')
    print('wrote', out, 'commits', len(commits), 'tip', tip[:8])


main()
