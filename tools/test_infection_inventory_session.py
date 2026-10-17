"""Two full engines: native/custom equipment, map changes and audited melee.

Only the existing combat fixture positions players; all hits use controller
buttons, real weapons and host validation. Never runs unless invoked directly.
"""
import argparse
import json
import re
from pathlib import Path
import shutil
import test_dual_combat as dual

VARIANTS = ('slayer,infection-stock-generic,infection-stock-map,slayer,'
            'infection-stock-custom-remote,infection-stock-unarmed-on,'
            'infection-stock-unarmed-off-remote,slayer')


def audits(path):
    return [json.loads(line.split('damage audit: ', 1)[1])
            for line in path.read_text(errors='replace').splitlines() if 'damage audit: {' in line]


def verify(root):
    manifest = json.loads((root / 'manifest.json').read_text())
    variants = manifest['variants'].split(',')
    peers = {peer: dual.samples(root / peer / 'session.log') for peer in ('host', 'client')}
    result = {'result': 'PASS', 'full_clients': 2, 'protocol_bots': 0, 'rounds': [], 'damage': []}
    for peer, rows in peers.items():
        log = (root / peer / 'session.log').read_text(errors='replace')
        assert 'ASSERTION FAILED' not in log and 'EXCEPTION' not in log, peer + ': engine exception'
        names = {(int(g), int(d)): name for g, d, name in re.findall(
            r'inventory tag: game=(\d+) machine=\d+ slot=\d+ definition=(-?\d+) name=([^\r\n]+)', log)}
        for game, variant in enumerate(variants):
            samples = [r for r in rows if r['game'] == game]
            assert {r['machine'] for r in samples} == {0, 1}, f'{peer} game {game}: missing player'
            infection = variant.startswith('infection')
            unarmed = 'unarmed' in variant
            for machine in (0, 1):
                life = [r for r in samples if r['machine'] == machine]
                before, dead, after = dual.lifecycle(life, f'{peer} game {game} machine {machine}')
                humans = [r for r in life if r['alive'] and r['deaths'] == 0 and
                          (not infection or r['team'] == 0)]
                if humans:
                    expected = 0 if unarmed else (2 if infection else before['weapons'])
                    assert unarmed or expected > 0, 'native Slayer spawned without a weapon'
                    assert all(r['weapons'] == expected and not r['skull'] for r in humans), \
                        f'{peer} game {game}: human loadout lost or substituted'
                    if infection:
                        assert all(r['speed'] == 1 and r['max_shield'] > 0 for r in humans)
                    if 'custom' in variant and names:
                        tags = {names[(game, w)].rsplit('\\', 1)[-1]
                                for w in humans[0]['inventory'] if w != -1}
                        assert tags == {'sniper rifle', 'shotgun'}, f'wrong custom weapons: {tags}'
                if infection:
                    active = [r for r in life if r['alive'] and r['phase'] >= 2 and r['team'] == 1]
                    assert active, f'{peer} game {game}: no zombie'
                    assert all(r['weapons'] == 1 and r['skull'] and r['grenades'] == [0, 0] for r in active)
                    assert all(abs(r['speed'] - 1.8) < .001 for r in active)
                    assert all((r['max_shield'] == 0) == ('off' in variant) for r in active)
                    assert any(r['winner'] == 1 and r['over'] for r in samples), f'{peer} game {game}: no victory'
                    assert 4 <= (after['tick'] - dead['tick']) / 30 <= 7, 'respawn outside native retry interval'
                else:
                    assert after['weapons'] == before['weapons'] and not after['skull'] and after['max_shield'] > 0
                    assert sorted(w & 65535 for w in after['inventory'] if w != -1) == \
                           sorted(w & 65535 for w in before['inventory'] if w != -1), 'native Slayer respawn loadout changed'
                    assert any(r['over'] for r in samples), f'{peer} game {game}: Slayer incomplete'
            result['rounds'].append({'peer': peer, 'game': game, 'variant': variant, 'result': 'PASS'})
    events = audits(root / 'host' / 'session.log')
    for game, variant in enumerate(variants):
        before = [e for e in events if e['game'] == game and e['stage'] == 'before']
        assert before, f'game {game}: no host damage evidence'
        assert all(e['multiplier'] == (2 if e['zombie'] and e['skull'] else 1) for e in before)
        if 'unarmed' in variant:
            hits = [e for e in before if not e['zombie'] and e['weapon'] == -1]
            assert hits, f'game {game}: missing real unarmed melee'
            assert all(e['multiplier'] == 1 for e in hits)
            after = next(e for e in events if e['stage'] == 'after' and e['game'] == game and
                         e['tick'] == hits[0]['tick'] and e['target'] == hits[0]['target'])
            assert after['body'] < hits[0]['body'] or after['shield'] < hits[0]['shield'], 'no actual unarmed damage'
            result['damage'].append({'game': game, 'unarmed_hits': len(hits), 'first': hits[0], 'after': after})
        if variant.startswith('infection'):
            hits = [e for e in before if e['zombie'] and e['skull']]
            assert hits and all(e['multiplier'] == 2 for e in hits)
            if 'remote' in variant:
                assert any(e['remote'] for e in hits), 'remote zombie damage not validated by host'
    if manifest['instances'][0]['overrides']['HALO_TEST_INPUT'] == 'look:inventory-profile':
        log = (root / 'host' / 'session.log').read_text(errors='replace')
        assert 'inventory profile: FAIL' not in log, 'native profile creation or reload failed; see host log'
        for game, variant in enumerate(variants):
            assert f'inventory profile: PASS game={game} base_off=1 selected=1 infection={int(variant.startswith("infection"))} options_equal=1' in log
        result['native_disk_profiles'] = 'PASS'
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=Path('build/windows/halo.exe'))
    parser.add_argument('--maps', type=Path, default=Path('assets/maps'))
    parser.add_argument('--seconds', type=int, default=750)
    parser.add_argument('--variants', default=VARIANTS)
    parser.add_argument('--map', default='ratrace')
    parser.add_argument('--profile', action='store_true')
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if not args.verify_only:
        manifest = dual.prepare(args.exe.resolve(), args.maps, args.root, args.map, args.variants)
        for index, instance in enumerate(manifest['instances']):
            instance['overrides'].update(HALO_TEST_INPUT='look:inventory-profile' if args.profile else 'look:inventory',
                HALO_NET_ADDRESS=f'127.0.0.{232 + index}', HALO_NET_BROADCAST=f'127.0.0.{233 - index}')
            shutil.copy2(args.maps / 'bloodgulch.map', Path(instance['work']) / 'maps' / 'bloodgulch.map')
        dual.run(manifest, args.root, args.seconds)
    try:
        result = verify(args.root)
    except AssertionError as error:
        result = {'result': 'FAIL', 'error': str(error)}
    path = args.root / ('inventory-recheck.json' if args.verify_only else 'inventory-result.json')
    path.write_text(json.dumps(result, indent=2))
    print(json.dumps(result))
    return 0 if result['result'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
