"""Opt-in two full engines plus protocol stand-ins; no claim of AI clients."""
import argparse
import json
import os
import re
import shutil
from pathlib import Path
import subprocess
import sys
import time
import ctypes
from ctypes import wintypes

if __package__:
    from .test_dual_combat import prepare, samples, sockets
else:
    from test_dual_combat import prepare, samples, sockets


def memory_usage(pid):
    """Read the tested process only; no game memory or state is modified."""
    if os.name != 'nt':
        return None
    class Counters(ctypes.Structure):
        _fields_ = [('cb', wintypes.DWORD), ('faults', wintypes.DWORD)] + [
            (name, ctypes.c_size_t) for name in ('peak_rss', 'rss', 'peak_paged',
                'paged', 'peak_nonpaged', 'nonpaged', 'pagefile', 'peak_pagefile', 'private')]
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    psapi = ctypes.WinDLL('psapi', use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(Counters), wintypes.DWORD]
    handle = kernel.OpenProcess(0x410, False, pid)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        counters = Counters(); counters.cb = ctypes.sizeof(counters)
        if not psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            raise ctypes.WinError(ctypes.get_last_error())
        return {'rss_bytes': counters.rss, 'private_bytes': counters.private}
    finally:
        kernel.CloseHandle(handle)


def run(args):
    manifest = prepare(args.exe, args.maps, args.root, args.map, args.variant)
    manifest.update(participants=args.count, full_engines=2, protocol_standins=args.count-2)
    processes, engines, logs = [], [], []
    bot=None
    resource_samples=[]; events=[]
    host_address=f'127.0.0.{args.address_base}'
    client_address=f'127.0.0.{args.address_base+1}'
    def start_bots():
        log=(args.root/'bots.log').open('wb'); logs.append(log)
        process=subprocess.Popen([sys.executable, str(Path(__file__).with_name('system_link_bots.py')),
            '--host',host_address,'--first-address',f'127.0.0.{args.bot_address_base}','--machines',str(args.count-2),
            '--seconds',str(args.seconds+45),'--status-every','10'], stdout=log, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
        processes.append(process); manifest['bot_pid']=process.pid
        return process
    try:
        for instance in manifest['instances']:
            if args.client_last and instance['name']=='client':
                bot=start_bots()
                deadline=time.monotonic()+30
                while time.monotonic()<deadline:
                    if f"'pregame': {args.count-2}" in (args.root/'bots.log').read_text(errors='replace'): break
                    time.sleep(.25)
                else: raise AssertionError('stand-ins did not join before the last real client')
            work = Path(instance['work'])
            override = instance['overrides']
            override.update(HALO_NET_ADDRESS=host_address if instance['name']=='host' else client_address,
                HALO_NET_BROADCAST=client_address if instance['name']=='host' else host_address)
            if instance['name']=='host' and args.extra_client_after:
                override['HALO_NET_BROADCAST']+=f',127.0.0.{args.address_base+2}'
            override.update(HALO_NETWORK_TEST_START=str(20+args.count//10), HALO_TEST_INPUT=args.input)
            if args.client_last: override['HALO_NETWORK_TEST_START']='40'
            if args.shared_address:
                override.update(HALO_NET_ADDRESS=host_address, HALO_NET_BROADCAST=host_address)
            env = {k:v for k,v in os.environ.items() if not k.upper().startswith('HALO_')}
            env.update(override)
            log = (work/'session.log').open('wb'); logs.append(log)
            p = subprocess.Popen([str(work/'halo.exe')], cwd=work, env=env,
                stdout=log, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
            processes.append(p); engines.append(p); instance['pid']=p.pid
            # Do not race the host's own loopback connection: machine numbers
            # are assigned in join order, not by whether a process hosts.
            deadline=time.monotonic()+30
            joined=None
            while time.monotonic()<deadline and p.poll() is None:
                debug=work/'debug.txt'
                if debug.exists():
                    joined=re.search(r'our machine is #(\d+)',debug.read_text(errors='replace'))
                    if joined: break
                time.sleep(.25)
            assert joined, f'{instance["name"]}: full engine did not join'
            instance['machine']=int(joined.group(1))
            expected=0 if instance['name']=='host' else (args.count-1 if args.client_last else 1)
            assert instance['machine']==expected, 'unexpected machine assignment'
            # A machine connection precedes its local player registration.
            # Wait for both, so stand-ins cannot take the real player's slot.
            deadline=time.monotonic()+20
            marker=f'server added player from machine #{expected} at controller index #0 to the game'
            registered=False
            while time.monotonic()<deadline:
                if marker in (args.root/'host'/'debug.txt').read_text(errors='replace'):
                    registered=True; break
                if p.poll() is not None: break
                time.sleep(.25)
            assert registered, f'{instance["name"]}: local player was not registered'
        if bot is None: bot=start_bots()
        (args.root/'manifest.json').write_text(json.dumps(manifest,indent=2))
        start=time.monotonic(); captured=False; next_memory=0; released=False; extra_started=False
        while time.monotonic()-start < args.seconds:
            elapsed=time.monotonic()-start
            if args.extra_client_after and not extra_started and elapsed>=args.extra_client_after:
                source=manifest['instances'][1]
                work=args.root/'late'; work.mkdir()
                shutil.copytree(Path(source['work'])/'maps',work/'maps')
                for filename in ('halo.exe','SDL3.dll','config.toml'):
                    shutil.copy2(Path(source['work'])/filename,work/filename)
                override=dict(source['overrides'])
                override.update(HALO_DATA_ROOT=str(work),HALO_SAVE_ROOT=str(work/'saves'),
                    HALO_NET_ADDRESS=f'127.0.0.{args.address_base+2}',HALO_NET_BROADCAST=host_address)
                env={k:v for k,v in os.environ.items() if not k.upper().startswith('HALO_')}
                env.update(override)
                log=(work/'session.log').open('wb'); logs.append(log)
                p=subprocess.Popen([str(work/'halo.exe')],cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,
                    creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
                processes.append(p); engines.append(p)
                manifest['instances'].append({'name':'late','work':str(work),'overrides':override,'pid':p.pid})
                events.append({'event':'extra_full_client_started','elapsed_seconds':round(elapsed,2)})
                (args.root/'manifest.json').write_text(json.dumps(manifest,indent=2))
                extra_started=True
            if elapsed >= next_memory:
                for instance, process in zip(manifest['instances'], engines):
                    if process.poll() is None:
                        resource_samples.append({'elapsed_seconds':round(elapsed,2),
                            'peer':instance['name'], 'pid':process.pid, 'memory':memory_usage(process.pid)})
                next_memory=elapsed+5
            if args.release_bots_after and not released and elapsed>=args.release_bots_after:
                before=samples(args.root/'host'/'session.log')
                events.append({'event':'disconnect_protocol_standins','elapsed_seconds':round(elapsed,2),
                    'host_tick':max((r['tick'] for r in before),default=0)})
                bot.terminate(); bot.wait(timeout=15); released=True
            if not captured and elapsed > 25:
                sockets(engines,args.root); captured=True
            ended=[{'peer':i['name'],'pid':p.pid,'exit_code':p.poll()} for i,p in zip(manifest['instances'],engines)
                   if p.poll() is not None]
            if ended:
                events.append({'event':'unexpected_engine_exit','elapsed_seconds':round(elapsed,2),'processes':ended})
                break
            time.sleep(.5)
    finally:
        for p in processes:
            if p.poll() is None: p.terminate()
            p.wait(timeout=15)
        for log in logs: log.close()
        (args.root/'resources.json').write_text(json.dumps(resource_samples,indent=2))
        (args.root/'events.json').write_text(json.dumps(events,indent=2))
    result = {'participants':args.count,'full_engines':len(engines),'protocol_standins':args.count-2,'peers':{}}
    peer_rows={}
    client_machine=manifest['instances'][1]['machine']
    for name in ('host','client'):
        work=args.root/name
        text=(work/'session.log').read_text(errors='replace')
        debug=(work/'debug.txt').read_text(errors='replace')
        rows=samples(work/'session.log')
        peer_rows[name]=rows
        final_tick=max((r['tick'] for r in rows),default=0)
        final=[r for r in rows if r['tick']==final_tick]
        local=[r for r in rows if r['local'] and r['tick']>=300]
        live=[r for r in local if r['alive'] and r['attached']]
        result['peers'][name]={
            'expected_machine':next(i['machine'] for i in manifest['instances'] if i['name']==name),
            'assertion': 'ASSERTION FAILED' in text or 'EXCEPTION' in debug or 'crash: exception' in text,
            'last_tick':final_tick,
            'players_seen':len({r['player'] for r in rows}),
            'connected_players_final':sum(not r['quit'] for r in final) if final and 'quit' in final[0] else None,
            'matching_map_starts':final[0].get('matching_starts') if final else None,
            'live_players_final':sum(r['alive'] for r in final),
            'blocked_spawns_final':sum(r.get('spawn_candidates')==0 and not r['alive'] for r in final),
            'local_samples':len(local),'local_missing_unit':sum(not r['alive'] for r in local),
            'local_camera_without_unit':sum(r['unit']==-1 and r['camera_cluster']<0 for r in local),
            'local_wait_without_unit_and_timer':sum(r['unit']==-1 and r['respawn']<=0 for r in local),
            'local_machines':sorted({r['machine'] for r in local}),
            'camera_bad':sum(r['camera_cluster']<0 or r['camera_distance']>=3 for r in live),
            'rtt_max':max((r['ping'] for r in rows if r['machine']==client_machine),default=-1)}
    bots=(args.root/'bots.log').read_text(errors='replace')
    result['bots_tail']=bots.splitlines()[-3:]
    common={r['tick'] for r in peer_rows['host']} & {r['tick'] for r in peer_rows['client']}
    tick=max(common,default=0)
    snapshots={n:{r['player']:r for r in rows if r['tick']==tick} for n,rows in peer_rows.items()}
    disagreements=[]
    for player in snapshots['host'].keys() | snapshots['client'].keys():
        a=snapshots['host'].get(player); b=snapshots['client'].get(player)
        fields=('unit','alive','attached','team','kills','deaths','score','skull','weapons')
        if not a or not b or any(a[f]!=b[f] for f in fields): disagreements.append(player)
    result['sync']={'common_tick':tick,'players':len(snapshots['host']),'disagreements':disagreements}
    result['memory']={}
    for name in ('host','client'):
        measured=[r for r in resource_samples if r['peer']==name and r['memory'] and r['elapsed_seconds']>=40]
        result['memory'][name]={'samples_after_warmup':len(measured)}
        if measured:
            for field in ('rss_bytes','private_bytes'):
                values=[r['memory'][field] for r in measured]
                result['memory'][name][field]={'first':values[0],'last':values[-1],'peak':max(values),
                    'delta':values[-1]-values[0]}
    release_event=next((e for e in events if e['event']=='disconnect_protocol_standins'),None)
    if release_event:
        release_tick=release_event['host_tick']
        result['release']={'event':release_event,'peers':{}}
        for name, rows in peer_rows.items():
            local_before=[r for r in rows if r['local'] and 300<=r['tick']<=release_tick]
            local_after=[r for r in rows if r['local'] and r['tick']>release_tick+300]
            result['release']['peers'][name]={
                'missing_before':sum(not r['alive'] for r in local_before),
                'alive_after':sum(bool(r['alive']) for r in local_after),
                'missing_after':sum(not r['alive'] for r in local_after),
                'first_live_tick_after':next((r['tick'] for r in local_after if r['alive']),None)}
    if extra_started:
        rows=samples(args.root/'late'/'session.log')
        local=[r for r in rows if r['local']]
        result['late_client']={'samples':len(local),'alive_samples':sum(bool(r['alive']) for r in local),
            'machines':sorted({r['machine'] for r in local}),
            'teams':sorted({r['team'] for r in local}),
            'first_tick':min((r['tick'] for r in local),default=None)}
    result['network_result']='PASS' if all(not r['assertion'] and r['last_tick']>=900 and
        r['players_seen']==args.count and r['local_samples'] and not r['local_missing_unit'] and
        not r['camera_bad'] and r['local_machines']==[r['expected_machine']]
        for r in result['peers'].values()) and tick>=900 and not disagreements else 'FAIL'
    result['result']='PASS' if all(not r['assertion'] and r['last_tick']>=900 and
        r['players_seen']==args.count and r['live_players_final']==args.count and
        r['local_samples'] and not r['local_missing_unit'] and not r['camera_bad']
        and r['local_machines']==[r['expected_machine']]
        for r in result['peers'].values()) and tick>=900 and not disagreements else 'FAIL'
    result['result_criterion']='static population: all units spawned and neither local player ever dead/waiting'
    (args.root/'result.json').write_text(json.dumps(result,indent=2)); print(json.dumps(result),flush=True)
    return result['result']=='PASS'


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True); p.add_argument('--maps',type=Path,required=True)
    p.add_argument('--root',type=Path,required=True); p.add_argument('--count',type=int,default=10)
    p.add_argument('--variant',default='team_slayer'); p.add_argument('--map',default='beavercreek')
    p.add_argument('--seconds',type=int,default=65); p.add_argument('--input',default='look:population')
    p.add_argument('--shared-address',action='store_true',help='Negative control: deliberately give both engines one address')
    p.add_argument('--client-last',action='store_true',help='Join the second real engine after the stand-ins to diagnose spawn starvation')
    p.add_argument('--release-bots-after',type=float,default=0,
        help='Disconnect the stand-in process after this many observation seconds; never move its units')
    p.add_argument('--extra-client-after',type=float,default=0,
        help='Start a third full engine after this many observation seconds; uses the normal LAN join path')
    p.add_argument('--address-base',type=int,default=200,help='Last octet for host; clients use the following two addresses')
    p.add_argument('--bot-address-base',type=int,default=10,help='First stand-in address last octet')
    a=p.parse_args()
    if not 3<=a.count<=128: p.error('count must be 3..128, including two full engines')
    if not 1<=a.address_base<=252 or not 1<=a.bot_address_base<=257-a.count:
        p.error('addresses must remain in 127.0.0.1..254')
    if set(range(a.address_base,a.address_base+3)) & set(range(a.bot_address_base,a.bot_address_base+a.count-2)):
        p.error('full engines and stand-ins must use different addresses')
    sys.exit(0 if run(a) else 1)
