"""Opt-in two-engine vehicle seat and remote melee regression."""
import argparse
import json
from pathlib import Path
import test_dual_combat as dual
from test_infection_inventory_session import audits

VARIANTS = ','.join(f'infection-seat-{vehicle}-{seat}-{alpha}'
                    for vehicle, seat in ((0, 0), (0, 1), (0, 2), (1, 0), (2, 0)) for alpha in (0, 1))


def verify(root):
    manifest = json.loads((root / 'manifest.json').read_text())
    variants = manifest['variants'].split(',')
    rows = {peer: dual.samples(root / peer / 'session.log') for peer in ('host', 'client')}
    events = audits(root / 'host/session.log')
    result = {'result': 'PASS', 'full_engines': 2, 'protocol_bots': 0, 'seats': []}
    for game, name in enumerate(variants):
        _, _, vehicle, seat, alpha = name.split('-')
        alpha = int(alpha)
        hits = [r for r in events if r['game'] == game and r['stage'] == 'before']
        samples = [r for r in rows['host'] if r['game'] == game]
        assert samples, f'{name}: missing round'
        if hits:
            assert all(r['skull'] and r['zombie'] and r['multiplier'] == 2 for r in hits)
            assert all(bool(r['remote']) == bool(alpha) for r in hits), 'wrong damage authority'
            for peer in rows:
                life = [r for r in rows[peer] if r['game'] == game and r['machine'] != alpha]
                before, dead, after = dual.lifecycle(life, name + ' ' + peer)
                assert before['team'] == 0 and after['team'] == 1
                # Unit creation and reliable inventory can arrive in separate
                # updates. Require the skull within two sampled seconds, and
                # never accept human weapons during that interval.
                settled = [r for r in life if r['unit']==after['unit'] and after['tick']<=r['tick']<=after['tick']+60]
                assert all(r['weapons']==0 or (r['weapons']==1 and r['skull']) for r in settled)
                assert any(r['weapons']==1 and r['skull'] for r in settled)
                assert any(r['winner'] == 1 and r['over'] for r in life)
            state = 'PASS exposed rider hit, infected, respawned, victory'
        else:
            log = (root / 'host/session.log').read_text(errors='replace')
            assert f'vehicle probe: protected complete type={vehicle} seat={seat}' in log, name + ': no valid damage/protection evidence'
            assert all(r['deaths'] == 0 for r in samples), 'protected occupant damaged'
            state = 'PASS protection retained; no exposed target'
        result['seats'].append({'variant': name, 'result': state})
    for peer in rows:
        log = (root / peer / 'session.log').read_text(errors='replace')
        assert 'ASSERTION FAILED' not in log and 'EXCEPTION' not in log
        assert 'vehicle probe: FAIL' not in log, peer + ': exposed seat was not damaged'
        assert 'range control health=1.000/1.000' in log, 'missing out-of-range control'
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', required=True, type=Path)
    parser.add_argument('--variants', default=VARIANTS)
    parser.add_argument('--seconds', default=580, type=int)
    parser.add_argument('--verify-only', action='store_true')
    args = parser.parse_args()
    if not args.verify_only:
        manifest = dual.prepare(Path('build/windows/halo.exe').resolve(),
            Path('assets/maps'), args.root, 'bloodgulch', args.variants)
        for index, instance in enumerate(manifest['instances']):
            instance['overrides'].update(HALO_TEST_INPUT='look:vehicle', HALO_NETWORK_TEST_PICKUP_WEAPON=args.variants,
                HALO_NET_ADDRESS=f'127.0.0.{236+index}', HALO_NET_BROADCAST=f'127.0.0.{237-index}')
        dual.run(manifest, args.root, args.seconds)
    try:
        result = verify(args.root)
    except AssertionError as e:
        result = {'result': 'FAIL', 'error': str(e)}
    (args.root / ('vehicle-recheck.json' if args.verify_only else 'vehicle-result.json')).write_text(json.dumps(result, indent=2))
    print(json.dumps(result))
    return int(result['result'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
