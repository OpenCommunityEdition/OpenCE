"""Every menu combination survives the production profile codec and process restart."""
import itertools
import json
import re
import subprocess
from pathlib import Path

from tools.test_infection import compile_run, production, read


def test_saved_infection_menu_combinations(tmp_path):
    menu = read('port/linux/game/menu_functions.c')
    fields = ('infection_initial_zombies', 'infection_speed_percent', 'infection_damage_percent',
              'infection_respawn_seconds', 'infection_countdown_seconds', 'infection_zombie_shields', 'time_limit')
    choices = []
    for field in fields:
        spinner = 'infection_time_limit' if field == 'time_limit' else field
        match = re.search(r'"' + spinner + r'_spinner"[^\n]*OPTIONS_FIELD\(' + field +
                          r'\)[^\n]*\{ ([0-9, ]+) \}', menu)
        assert match, field + ': missing menu binding'
        choices.append([int(v.strip()) for v in match[1].split(',')])
    cases = list(itertools.product(*choices))
    data = ',\n'.join('{' + ','.join(map(str, case)) + '}' for case in cases)
    assignments = '\n'.join(f'want.{field} = cases[i][{n}];' for n,field in enumerate(fields))
    path = tmp_path/'saved-options.bin'
    main = r'''
    static const short cases[][FIELD_COUNT] = { CASE_DATA };
    int main(int argc, char **argv) {
        FILE *file; unsigned long i, n=sizeof(cases)/sizeof(cases[0]);
        byte block[512]; struct game_variant v={2,{1}};
        struct game_variant_options want, got; int j;
        file=fopen(FILE_NAME, argc>1 ? "rb" : "wb"); if(!file) return 2;
        for(i=0;i<n;i++) {
            memset(&want,0,sizeof(want)); infection_options_default(&want);
            want.infection_enabled=1; ASSIGNMENTS
            infection_options_validate(&v,&want);
            if(argc==1) {
                memset(block,0xA5,sizeof(block)); playlist_profile_options_to_block(block,&want);
                for(j=0;j<256;j++) if(block[j]!=0xA5) return 3;
                if(fwrite(block,1,sizeof(block),file)!=sizeof(block)) return 4;
            } else {
                if(fread(block,1,sizeof(block),file)!=sizeof(block)) return 5;
                memset(&got,0xEE,sizeof(got));
                if(!playlist_profile_options_from_block(block,&got)) return 6;
                if(memcmp(&want,&got,sizeof(want))) return 7;
                /* A stored Infection option cannot enable Infection in FFA. */
                v.universal_variant.teams=0; infection_options_validate(&v,&got);
                if(got.infection_enabled) return 8;
                v.universal_variant.teams=1;
            }
        }
        if(argc>1 && fgetc(file)!=EOF) return 9;
        fclose(file); return 0;
    }
    '''.replace('FIELD_COUNT',str(len(fields))).replace('CASE_DATA',data).replace('FILE_NAME',json.dumps(path.as_posix())).replace('ASSIGNMENTS',assignments)
    source = '#define _CRT_SECURE_NO_WARNINGS 1\n#include <stdio.h>\n' + production() + main
    assert compile_run(tmp_path, source) == 0
    assert path.stat().st_size == len(cases)*512
    exe = tmp_path/('test.exe' if __import__('os').name=='nt' else 'test')
    reopened = subprocess.run([str(exe),'read'],capture_output=True,timeout=15)
    assert reopened.returncode == 0, reopened.stderr
    # A different process must reject corruption in the saved extension.
    with path.open('r+b') as file:
        file.seek(268); old=file.read(1); file.seek(268); file.write(bytes([old[0]^1]))
    corrupted = subprocess.run([str(exe),'read'],capture_output=True,timeout=15)
    assert corrupted.returncode == 6
    (tmp_path/'case-count.txt').write_text(str(len(cases)))
