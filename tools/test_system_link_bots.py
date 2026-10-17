"""Settings diagnostics for the lightweight clients; no sockets or game assets."""
import struct
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

import pytest

from tools.system_link_bots import network_game_counts, network_game_layout


@pytest.mark.parametrize('machines,players', [(128, 128), (4, 16)])
@pytest.mark.parametrize('options_size', [0, 0x1C, 0x24, 0x28, 0x30])
def test_settings_counts(machines, players, options_size):
    layout = network_game_layout(machines, players, options_size)
    settings = bytearray(layout['size'])
    struct.pack_into('<h', settings, layout['machine_count'], machines)
    struct.pack_into('<h', settings, layout['player_count'], players)
    assert network_game_counts(settings) == (machines, players)
    assert network_game_counts(settings[:-1]) == (None, None)
    struct.pack_into('<h', settings, layout['player_count'], players + 1)
    assert network_game_counts(settings) == (None, None)


def test_current_settings_size():
    assert network_game_layout(128, 128)['size'] == 13140
    assert network_game_counts(b'') == (None, None)


@pytest.mark.skipif(not all(os.environ.get(k) for k in
                   ('OPENCE_TEST_STANDINS', 'OPENCE_TEST_EXE', 'OPENCE_TEST_MAPS', 'OPENCE_TEST_RUN_ROOT')),
                   reason='opt-in local host with eight protocol stand-ins for 180+ seconds ingame')
def test_eight_standins_stay_connected():
    """These clients acknowledge packets; they are not AI or game simulations."""
    root = Path(os.environ['OPENCE_TEST_RUN_ROOT'])
    exe = Path(os.environ['OPENCE_TEST_EXE'])
    maps = Path(os.environ['OPENCE_TEST_MAPS'])
    (root / 'maps').mkdir(parents=True, exist_ok=False)
    for name in ('ui.map', 'ratrace.map', 'carousel.map'):
        shutil.copy2(maps / name, root / 'maps' / name)
    shutil.copy2(exe, root / exe.name)
    shutil.copy2(exe.with_name('SDL3.dll'), root / 'SDL3.dll')
    (root / 'config.toml').write_text('[crash_reports]\nupload="no"\n[discord]\napplication_id=""\n'
                                    '[display]\nhigh_res_hud=false\nhigh_res_text=false\n')
    env = os.environ.copy()
    env.update(HALO_DATA_ROOT=str(root), HALO_SAVE_ROOT=str(root / 'saves'),
               HALO_NETWORK_TEST='host:ratrace:infection', HALO_NETWORK_TEST_START='25',
               HALO_NET_ADDRESS='127.0.0.31', HALO_NET_BROADCAST='127.0.0.31', HALO_NET_ONLINE='false',
               HALO_NETWORK_TEST_KILL='0', HALO_NETWORK_TEST_SHOOT='0', HALO_TEST_INPUT='look:0',
               HALO_HIDDEN_WINDOW='1', HALO_NULL_RENDERER='1', HALO_EXIT_AFTER='240')
    flags = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
    processes = []
    with (root / 'host.log').open('wb') as host_log, (root / 'bots.log').open('wb') as bots_log:
        try:
            processes.append(subprocess.Popen([str(root / exe.name)], cwd=root, env=env,
                             stdout=host_log, stderr=subprocess.STDOUT, creationflags=flags))
            time.sleep(5)
            processes.append(subprocess.Popen([sys.executable, str(Path(__file__).with_name('system_link_bots.py')),
                             '--host', '127.0.0.31', '--first-address', '127.0.0.32', '--machines', '8',
                             '--start', '--seconds', '235', '--status-every', '10'],
                             stdout=bots_log, stderr=subprocess.STDOUT, creationflags=flags))
            assert processes[-1].wait(timeout=255) == 0
        finally:
            for process in processes:
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=15)
    log = (root / 'bots.log').read_text(errors='replace')
    samples = re.findall(r'\[\s*([\d.]+)\] states \{\x27ingame\x27: 8\}; host game: 9 machines, 9 players; latest update (\d+)', log)
    assert len(samples) >= 2, 'never observed eight clients and valid host counts'
    assert float(samples[-1][0]) - float(samples[0][0]) >= 180, 'did not stay ingame beyond 180 seconds'
    assert int(samples[-1][1]) > int(samples[0][1]), 'host stopped sending updates'
    host = (root / 'host.log').read_text(errors='replace')
    assert 'infection: active players=9 zombies=1' in host
    assert 'ASSERTION FAILED' not in host and 'crash: exception' not in host
