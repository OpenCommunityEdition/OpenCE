"""Summarize population evidence without treating protocol stand-ins as game clients."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path

if __package__:
    from .test_dual_combat import samples
else:
    from test_dual_combat import samples


def analyze(root):
    manifest = json.loads((root / 'manifest.json').read_text())
    resource_path = root / 'resources.json'
    resources = json.loads(resource_path.read_text()) if resource_path.exists() else []
    result = {'map': manifest['map'], 'variants': manifest['variants'],
              'binary_sha256': manifest['binary_sha256'], 'peers': {}}
    snapshots = {}
    for instance in manifest['instances']:
        name = instance['name']
        work = Path(instance['work'])
        rows = samples(work / 'session.log')
        snapshots[name] = {(r['game'], r['tick'], r['player']): r for r in rows}
        local = [r for r in rows if r['local']]
        lives, waiting, max_wait, previous, death = [], 0, 0, None, None
        for row in local:
            if previous and row['game'] == previous['game']:
                waiting = waiting + row['tick'] - previous['tick'] if not row['alive'] and not previous['alive'] else 0
                max_wait = max(max_wait, waiting)
                if previous['alive'] and not row['alive'] and row['deaths'] > previous['deaths']:
                    death = {'game': row['game'], 'dead_tick': row['tick'],
                             'old_unit': previous['unit'], 'deaths': row['deaths']}
                if death and row['alive'] and row['attached'] and row['unit'] != death['old_unit']:
                    lives.append(dict(death, respawn_tick=row['tick'], new_unit=row['unit']))
                    death = None
            else:
                death = None
                waiting = 0
            previous = row
        latest = max(((r['game'], r['tick']) for r in rows), default=None)
        final = [r for r in rows if (r['game'], r['tick']) == latest]
        live = [r for r in local if r['alive'] and r['attached']]
        text = (work / 'session.log').read_text(errors='replace')
        debug = (work / 'debug.txt').read_text(errors='replace') if (work / 'debug.txt').exists() else ''
        memory = [r for r in resources if r['peer'] == name and r['memory'] and r['elapsed_seconds'] >= 60]
        summary = {
            'samples': len(local), 'last_game_tick': latest,
            'unique_player_slots_observed': len({r['player'] for r in rows}),
            'connected_players_final': sum(not r['quit'] for r in final) if final and 'quit' in final[0] else None,
            'active_units_final': sum(bool(r['alive']) for r in final),
            'matching_map_starts': sorted({r['matching_starts'] for r in rows if 'matching_starts' in r}),
            'local_deaths_max': max((r['deaths'] for r in local), default=0),
            'local_kills_max': max((r['kills'] for r in local), default=0),
            'local_death_respawns': lives, 'unfinished_death': death,
            'local_no_unit_longest_seconds': round(max_wait / 30, 2),
            'local_live_bad_camera_samples': sum(r['camera_cluster'] < 0 or r['camera_distance'] >= 3 for r in live),
            'rtt_max_ms': max((r['ping'] for r in rows if r['machine'] == 1), default=None),
            'local_rtt_max_ms': max((r['ping'] for r in local), default=None),
            'exception': 'ASSERTION FAILED' in text or 'EXCEPTION' in debug or 'crash: exception' in text,
            'artificial_combat_fixture': 'combat stage:' in text or 'infection test: damage' in text,
            'environmental_death_injections': text.count('population test: environmental death'),
            'memory_after_60s': {},
        }
        for field in ('rss_bytes', 'private_bytes'):
            values = [r['memory'][field] / 1048576 for r in memory]
            if values:
                tail = [r['memory'][field] / 1048576 for r in memory
                        if r['elapsed_seconds'] >= memory[-1]['elapsed_seconds'] - 300]
                summary['memory_after_60s'][field] = {
                    'first_mib': round(values[0], 2), 'last_mib': round(values[-1], 2),
                    'peak_mib': round(max(values), 2), 'delta_mib': round(values[-1] - values[0], 2),
                    'last_300s_delta_mib': round(tail[-1] - tail[0], 2), 'samples': len(values)}
        result['peers'][name] = summary
    host = snapshots['host']
    result['synchronization'] = {}
    for name, remote in snapshots.items():
        if name == 'host':
            continue
        paired = [(host[key], remote[key]) for key in host.keys() & remote.keys()]
        differences = Counter()
        position_max = 0
        for a, b in paired:
            for field in ('unit', 'alive', 'team', 'kills', 'deaths', 'score', 'quit'):
                if a.get(field) != b.get(field):
                    differences[field] += 1
            if a['alive'] and b['alive'] and a['unit'] == b['unit']:
                position_max = max(position_max, math.dist(a['position'], b['position']))
        result['synchronization'][name] = {'paired_snapshots': len(paired),
            'field_disagreements': dict(differences), 'position_difference_max': round(position_max, 3)}
    # Late clients can sample on different ticks. Compare only an unchanged
    # final ten-second window; this is not evidence of tick-by-tick agreement.
    events_path = root / 'events.json'
    events = json.loads(events_path.read_text()) if events_path.exists() else []
    release = next((e for e in events if e['event'] == 'disconnect_protocol_standins'), None)
    if release and 'late' in snapshots:
        fields = ('player', 'machine', 'unit', 'alive', 'attached', 'team',
                  'kills', 'deaths', 'score', 'weapons', 'skull')
        states, checks = {}, {}
        for name, peer in snapshots.items():
            ticks = {}
            for (game, tick, _), row in peer.items():
                ticks.setdefault((game, tick), []).append(row)
            last = max(ticks, default=None)
            window = [rows for (game, tick), rows in sorted(ticks.items())
                      if last and game == last[0] and tick >= last[1] - 300]
            normalized = [sorted(tuple(r.get(f) for f in fields) for r in rows if not r.get('quit'))
                          for rows in window]
            states[name] = normalized[-1] if normalized else []
            checks[name] = {'samples': len(window),
                'unchanged': len(window) >= 10 and all(s == states[name] for s in normalized),
                'connected_players': len(states[name]),
                'active_units': sum(s[3] and s[4] for s in states[name])}
        full_before = {}
        for (_, tick, _), row in host.items():
            if tick <= release['host_tick']:
                full_before[tick] = full_before.get(tick, 0) + (not row.get('quit'))
        late_local = [r for r in snapshots['late'].values() if r['local']]
        conditions = {
            'full_before_release': max(full_before.values(), default=0) == manifest['participants'],
            'late_entered_after_release': bool(late_local) and min(r['tick'] for r in late_local) > release['host_tick'],
            'three_stable_active_players_each': len(checks) == 3 and all(
                c['unchanged'] and c['connected_players'] == 3 and c['active_units'] == 3 for c in checks.values()),
            'same_final_connected_state': bool(states['host']) and all(s == states['host'] for s in states.values()),
            'no_exception_or_bad_live_camera': all(not p['exception'] and not p['local_live_bad_camera_samples']
                                                  for p in result['peers'].values()),
            'no_unexpected_exit': not any(e['event'] == 'unexpected_engine_exit' for e in events),
        }
        result['disconnect_and_late_entry'] = {
            'result': 'PASS' if all(conditions.values()) else 'FAIL', 'checks': conditions,
            'stable_final_window': checks, 'state_fields': fields, 'final_states': states,
            'note': 'Stable final state only; exact paired-tick coverage is reported separately.'}
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    args = parser.parse_args()
    report = analyze(args.root)
    (args.root / 'analysis.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report))
