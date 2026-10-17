"""999 reserves across Infection/FFA/Team Slayer on the same manual-report map."""
import argparse
import json
from pathlib import Path
import re
import test_dual_combat as dual


def verify(root):
    manifest = json.loads((root/'manifest.json').read_text())
    checks = []
    for peer in ('host', 'client'):
        log = (root/peer/'session.log').read_text(errors='replace')
        assert not re.search(r'EXCEPTION|ASSERTION FAILED|!!! ASSERT', log), peer
        rows = [{k: int(v) for k, v in re.findall(r'(\w+)=(-?\d+)', line)}
                for line in log.splitlines() if 'ammo off:' in line]
        for game, mode in enumerate(manifest['variants'].split(',')):
            expected = game % 2 == 0 if manifest.get('menu_toggle') else mode.startswith('infection')
            if manifest.get('menu_toggle'):
                assert f'ammo map: game={game} name=levels\\test\\prisoner\\prisoner' in log, (peer, game, 'wrong map')
                if peer == 'host':
                    assert f'menu toggle: game={game} applied=1 same_variant=1' in log, (game, 'menu callback failed')
            weapons = [r for r in rows if r['game'] == game]
            assert weapons and any(r['parent'] != -1 for r in weapons), (peer, mode, 'no player weapons')
            assert all(r['infection'] == expected and r['option'] == 999 for r in weapons), (peer, mode, 'activation')
            customized = [r for r in weapons if r['override'] != -1]
            assert bool(customized) == expected, (peer, mode, 'scope')
            for weapon in weapons:
                # The existing wire format sends ammo in unit inventories,
                # not loose-weapon creation records. Host map contents and
                # held inventories are the authority checked here; a client's
                # unowned copy may retain tag ammo until a host pickup.
                if expected and peer != 'host' and weapon['parent'] == -1:
                    continue
                if weapon['override'] != -1:
                    assert weapon['reserve'] == weapon['cap'] == weapon['override'] == 999, (peer, mode, weapon)
                else:
                    assert weapon['reserve']+weapon['loaded'] == weapon['initial'], (peer, mode, weapon)
                    assert weapon['cap'] == weapon['native_cap'], (peer, mode, weapon)
            checks.append(dict(peer=peer, game=game, mode=mode, infection=expected, result='PASS', weapons=len(weapons),
                               customized=len(customized), skipped_loose_client_ammo=sum(
                                   expected and peer != 'host' and w['parent']==-1 for w in weapons)))
    return dict(result='PASS', full_engines=2, protocol_bots=0, map=manifest['map'], checks=checks,
                scope='Host map and held inventories on both peers; OFF checks all copies. ON loose-client ammo is not a wire invariant.')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--exe', type=Path, default=Path('build/windows/halo.exe'))
    p.add_argument('--maps', type=Path, default=Path('assets/maps'))
    p.add_argument('--verify-only', action='store_true')
    p.add_argument('--menu-toggle', action='store_true')
    p.add_argument('--render', action='store_true')
    a = p.parse_args()
    if not a.verify_only:
        manifest = dual.prepare(a.exe.resolve(), a.maps,
                                a.root, 'prisoner', 'infection,infection,infection' if a.menu_toggle else 'infection,slayer,team_slayer,infection',
                                render=a.render, capture_every=300 if a.render else 0)
        manifest['menu_toggle'] = a.menu_toggle
        for n, peer in enumerate(manifest['instances']):
            peer['overrides'].update(HALO_TEST_INPUT='look:inventory-menu-toggle' if a.menu_toggle else 'look:inventory-scope',
                                     HALO_NET_ADDRESS=f'127.0.0.{230+n}', HALO_NET_BROADCAST=f'127.0.0.{231-n}')
        dual.run(manifest, a.root, 230)
    try:
        result = verify(a.root)
    except AssertionError as exc:
        result = dict(result='FAIL', error=str(exc))
    (a.root/('scope-scoped-result.json' if a.verify_only else 'scope-result.json')).write_text(json.dumps(result, indent=2))
    print(json.dumps(result))
    return int(result['result'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
