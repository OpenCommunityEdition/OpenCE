"""Inspect an addon's model bind poses (no proprietary map required)."""
import argparse
import struct
from pathlib import Path


def inspect(path):
    data = Path(path).read_bytes()
    u = lambda offset: struct.unpack_from('<I', data, offset)[0]
    tags, count = u(20), u(24)
    base = u(36)
    model_data = u(68)
    text = lambda offset: data[base + offset:].split(b'\0', 1)[0].decode('utf8')
    print(f'{path}: {count} tags, model data @{model_data:#x}')
    for index in range(count):
        group, _, _, name, root = struct.unpack_from('<5I', data, tags + index * 20)
        group = group.to_bytes(4, 'big').decode('ascii')
        if group not in ('mode', 'mod2') or text(name).endswith('\\weapon\\weapon'):
            continue
        m = base + root
        nodes, node_offset = u(m + 0xb8), u(m + 0xbc)
        print(f'{index}: {group} {text(name)} flags={u(m):#x} nodes={nodes}')
        for n in range(nodes):
            p = base + node_offset + n * 0x9c
            label = data[p:p+32].split(b'\0', 1)[0].decode('utf8')
            print(f'  node {n} {label} hierarchy={struct.unpack_from("<3h", data, p+32)} '
                  f'translation={struct.unpack_from("<3f", data, p+40)} '
                  f'inverse={struct.unpack_from("<13f", data, p+104)}')
        geometries, geometry_offset = u(m+0xd0), u(m+0xd4)
        for g in range(geometries):
            geo = base + geometry_offset + g*0x30
            parts, part_offset = u(geo+0x24), u(geo+0x28)
            for k in range(parts):
                p = base + part_offset + k*0x84
                vertices, offset = u(p+0x58), u(p+0x64)
                print(f'  geometry {g} part {k}: vertices={vertices} offset={offset:#x}')
                if vertices and model_data + offset + vertices*68 <= len(data):
                    positions = [struct.unpack_from('<3f', data, model_data+offset+i*68) for i in range(vertices)]
                    skins = {struct.unpack_from('<2h2f', data, model_data+offset+i*68+56) for i in range(vertices)}
                    print(f'    bounds={tuple((min(v[a] for v in positions), max(v[a] for v in positions)) for a in range(3))} skins={skins}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('addon')
    inspect(parser.parse_args().addon)
