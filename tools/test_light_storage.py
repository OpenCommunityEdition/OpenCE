"""Link the real light-partition definitions and initialize their native caches.

No game assets, SDK, C runtime or profile needed. Windows uses COFF/i686;
Linux uses ELF/i386. Exercise both ordinary and full-LTO links. The Windows
negative control reinstates the retail-sized weak fallback and must corrupt
the light-data pointer, proving this checks storage selected by the linker.
"""

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def function(source, name):
    start = re.search(r"(?:static )?void " + re.escape(name) + r"\([^;{]*\)\s*\{", source).start()
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def owner_definition(source, declaration, export):
    # Use the owning source's tentative definition, not another typed fallback.
    assert declaration in source
    return export + declaration + "\n"


def run(cc):
    windows = sys.platform == "win32"
    common = (ROOT / "port/linux/src/halo_linker_common.c").read_text()
    lights = (ROOT / "source/objects/object_lights.c").read_text()
    partitions = (ROOT / "source/structures/cluster_partitions.c").read_text()
    macro = common[common.index("#define HALO_COMMON"):common.index("HALO_COMMON(ai_globals")]
    fallbacks = "\n".join(re.findall(
        r"^HALO_COMMON\((?:light_cluster_partition|light_data),[^\n]+", common, re.M))
    header = (ROOT / "source/structures/cluster_partitions.h").as_posix()
    prefix = f'''
/* Suppress header dependencies unrelated to the partition's layout. */
#define __REAL_MATH_H
#define __DATA_H
typedef union real_point3d real_point3d;
struct location;
struct data_array {{ unsigned long signature; }};
#include "{header}"
typedef char require_32_bit_abi[sizeof(void *) == 4 && sizeof(long) == 4 ? 1 : -1];
'''
    # Keep symbol layout observable under whole-program scalar replacement.
    export = '__declspec(dllexport) ' if windows else ''
    owner = prefix + owner_definition(lights, 'struct cluster_partition light_cluster_partition;', export)
    owner += owner_definition(lights, 'struct data_array *light_data;', export)
    owner += r'''
#define FALSE 0
#define MAXIMUM_CLUSTERS_PER_STRUCTURE 8
#define HALO_PORT_MAXIMUM_CLUSTER_REFERENCES 16
#define MAXIMUM_PORT_CLUSTER_PARTITIONS 8
static struct cluster_partition *cluster_partitions_port[MAXIMUM_PORT_CLUSTER_PARTITIONS];
static short cluster_partitions_port_count;
static unsigned long arena[1024], allocated;
static void *allocate(unsigned long bytes) {
    unsigned long words = (bytes + 3) / 4;
    void *result = arena + allocated;
    allocated += words;
    return result;
}
#define game_state_malloc(name, label, bytes) allocate(bytes)
#define reference_list_new(name, count) ((struct data_array *)allocate(sizeof(struct data_array)))
#define debug_malloc(bytes, clear, file, line) allocate(bytes)
#define sprintf(buffer, ...) ((void)0)
#define error(...) ((void)0)
static void csmemset(void *destination, int value, unsigned long bytes) {
    unsigned char *out = destination;
    while (bytes--) *out++ = (unsigned char)value;
}
'''
    forget = function(partitions, 'cluster_partition_port_forget')
    owner += forget
    owner += function(partitions, 'cluster_partition_new')
    owner += '''
void initialize_partition(void) {
    cluster_partition_new(&light_cluster_partition, "light");
    cluster_partition_port_forget(&light_cluster_partition);
}
'''
    caller = prefix + r'''
extern struct cluster_partition light_cluster_partition;
extern struct data_array *light_data;
void initialize_partition(void);
static struct data_array lights = { 0x64407440UL };
__attribute__((noinline,optnone)) static int check(void) {
    light_data = &lights;
    initialize_partition();
    if (light_data != &lights || light_data->signature != 0x64407440UL) return 71;
    if (!light_cluster_partition.cluster_first_data_references ||
        !light_cluster_partition.data_reference_data ||
        !light_cluster_partition.cluster_reference_data) return 72;
    if (!light_cluster_partition.port_previous_references ||
        !light_cluster_partition.port_cluster_data_references) return 73;
    if (light_cluster_partition.port_modification_count != 2) return 74;
    for (int i = 0; i < 16; i++) {
        if (light_cluster_partition.port_previous_references[i] != -1 ||
            light_cluster_partition.port_cluster_data_references[i] != -1) return 75;
    }
    return 0;
}
'''
    caller += 'int mainCRTStartup(void) { return check(); }\n' if windows else r'''
void _start(void) {
    int result = check();
    __asm__ volatile("int $0x80" : : "a"(1), "b"(result) : "memory");
    __builtin_unreachable();
}
'''
    with tempfile.TemporaryDirectory(prefix='halo-light-storage-') as directory:
        work = Path(directory)
        (work / 'owner.c').write_text(owner)
        (work / 'caller.c').write_text(caller)
        for lto in (False, True):
            for legacy in ((False, True) if windows and lto else (False,)):
                fallback = fallbacks
                if legacy:
                    fallback = re.sub(r'^HALO_COMMON\(light_cluster_partition,[^\n]+\n?', '', fallback, flags=re.M)
                    fallback = 'HALO_COMMON(light_cluster_partition, 12);\n' + fallback
                (work / 'fallback.c').write_text(macro + fallback + '\n')
                executable = work / ('probe.exe' if windows else 'probe')
                abi = ['--target=i686-pc-windows-msvc'] if windows else ['-m32']
                link = ['-Wl,/entry:mainCRTStartup', '-Wl,/subsystem:console'] if windows else [
                    '-Wl,-e,_start', '-Wl,--export-dynamic', '-no-pie', '-static']
                command = [cc, *abi, '-std=gnu99', '-O2', '-fcommon', '-ffreestanding',
                           '-fno-builtin', '-nostdlib', '-fuse-ld=lld', '-I' + str(ROOT / 'source'), *link]
                if lto:
                    command.append('-flto=full')
                command += [str(work / name) for name in ('owner.c', 'caller.c', 'fallback.c')]
                command += ['-o', str(executable)]
                subprocess.run(command, check=True, timeout=60)
                result = subprocess.run([str(executable)], timeout=10)
                expected = 71 if legacy else 0
                assert result.returncode == expected, (command, result.returncode, expected)
                print(f'PASS: {"COFF" if windows else "ELF"} {"full LTO" if lto else "ordinary"}: '
                      f'{"legacy pointer corruption detected" if legacy else "light data and native caches preserved"}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    args = parser.parse_args()
    run(args.cc)
