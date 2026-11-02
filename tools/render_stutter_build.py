"""Build a Windows timing probe from scratch copies of an already built checkout.

The optional honeycomb omission is an experiment, not a shipping renderer.
No production source or object is overwritten. Use equal settings and fixed
simulation ticks when comparing checkouts; driver caches remain uncontrolled.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def omit_honeycomb(source):
    for statement in ['xgpu_text_append(&text, "%s", shield_honeycomb_glsl);',
                      'xgpu_text_append(&text, "\\tresult = shield_honeycomb_surface(result);\\n");']:
        assert source.count(statement) == 1
        guarded = '\tif (key->shield_honeycomb)\n\t\t' + statement + '\n'
        plain = '\t' + statement + '\n'
        source = source.replace(guarded, '') if guarded in source else source.replace(plain, '')
    return source


def build(root, output, honeycomb=False, deterministic=False, record_prepared=False):
    root = root.resolve(); output = output.resolve()
    assert output.is_relative_to(root/'build')
    output.mkdir(parents=True, exist_ok=True)
    entries = json.loads(subprocess.check_output(['ninja', '-t', 'compdb', 'windows_cc'], cwd=root, text=True))
    link = json.loads(subprocess.check_output(['ninja', '-t', 'compdb', '-x', 'windows_link'], cwd=root, text=True))[0]['command']
    backend = (root/'port/linux/src/d3d8_gl.c').read_text()
    timer = r'''
static Uint64 stutter_started[12],stutter_time[12];
static unsigned long stutter_calls[12];
void stutter_begin(int k) {stutter_started[k]=SDL_GetTicksNS();}
void stutter_end(int k) {stutter_time[k]+=SDL_GetTicksNS()-stutter_started[k];stutter_calls[k]++;}
struct stutter_sample {Uint64 ns,time[12];unsigned long calls[12];long tick;int playing;};
static struct stutter_sample stutter_samples[40000];
static unsigned long stutter_count;
static void stutter_dump(void) {
    FILE *file=fopen("frames.csv","w");unsigned long i;int j;
    if(!file)return;
    fprintf(file,"ns,tick,playing,compile_ns,compile_calls,link_ns,link_calls,load_ns,load_calls,store_ns,store_calls,texture_ns,texture_calls,all_texture_ns,all_texture_calls,watch_ns,watch_calls,upload_ns,upload_calls,generation_ns,generation_calls,override_ns,override_calls,result_ns,result_calls,texture_create_ns,texture_create_calls\n");
    for(i=0;i<stutter_count;i++) {
        struct stutter_sample *s=&stutter_samples[i];
        fprintf(file,"%llu,%ld,%d",s->ns,s->tick,s->playing);
        for(j=0;j<12;j++)fprintf(file,",%llu,%lu",s->time[j],s->calls[j]);
        fprintf(file,"\n");
    }
    fclose(file);
}
void stutter_frame(long tick,int playing) {
    if(!stutter_count)atexit(stutter_dump);
    if(stutter_count<40000) {
        struct stutter_sample *s=&stutter_samples[stutter_count++];
        s->ns=SDL_GetTicksNS();s->tick=tick;s->playing=playing;
        memcpy(s->time,stutter_time,sizeof(stutter_time));memcpy(s->calls,stutter_calls,sizeof(stutter_calls));
    }
    memset(stutter_time,0,sizeof(stutter_time));memset(stutter_calls,0,sizeof(stutter_calls));
}
'''
    assert backend.count('#include <time.h>') == 1
    backend = backend.replace('#include <time.h>', '#include <time.h>\n'+timer)
    if 'if (!material_preparing)' in backend:
        anchor = '\tif (!material_preparing)\n\t\tshader_prepare_record'
        assert backend.count(anchor) == 1
        backend = backend.replace(anchor, '''\tif (material_preparing) {
        static FILE *pairs;
        if (!pairs) pairs=fopen("prepared-pairs.csv","w");
        if (pairs) fprintf(pairs,"%016llx,%016llx\\n",
            (unsigned long long)shader_hash(vertex_shader),(unsigned long long)shader_hash(fragment_shader));
    }
'''+anchor)
        if record_prepared:
            backend = backend.replace(anchor, '\tshader_prepare_record')
    for kind, call, status in [(0, 'glCompileShader(shader);', 'glGetShaderiv(shader, GL_COMPILE_STATUS, &status);'),
                              (1, 'glLinkProgram(entry->program);', 'glGetProgramiv(entry->program, GL_LINK_STATUS, &status);')]:
        assert backend.count(call) == 1
        start = backend.index(call); end = backend.index(status, start)+len(status)
        backend = backend[:start]+f'stutter_begin({kind});'+backend[start:end]+f'stutter_end({kind});'+backend[end:]
    for kind, call in [(2, 'entry->program = program_cache_load(shader_hash(vertex_shader), shader_hash(fragment_shader));'),
                       (3, 'program_cache_store(entry->program, shader_hash(vertex_shader), shader_hash(fragment_shader));')]:
        assert backend.count(call) == 1
        backend = backend.replace(call, '{'+f'stutter_begin({kind});'+call+f'stutter_end({kind});'+'}')
    main = (root/'source/main/main.c').read_text()
    main = 'void stutter_frame(long,int);\n'+main
    anchor = '\t\tmain_frame_rate_debug();'
    assert main.count(anchor) == 1
    main = main.replace(anchor, anchor+'\n stutter_frame(game_in_progress()?game_time_get():-1,game_in_progress()&&!main_globals.main_menu_scenario_loaded);')
    sources = {'port/linux/src/d3d8_gl.c': backend, 'source/main/main.c': main}
    texture = (root/'port/linux/src/xbox_textures.c').read_text()
    assert texture.count('GLuint xgpu_texture_get(') == 1
    texture = 'void stutter_begin(int);void stutter_end(int);\n'+texture.replace(
        'GLuint xgpu_texture_get(', 'static GLuint measured_texture_get(')
    for kind, call in [(6, 'memory_watch_protect(entry->address, entry->size);'),
                       (7, 'upload(entry->texture, entry->target, &entry->description, (const unsigned char *)entry->address, palette,\n\t\t\t\tcustom_edition_texels_order(entry->address));'),
                       (9, 'entry->override = hud_hires_override_find(entry->address, entry->description.width,\n\t\t\t\tentry->description.height, entry->description.levels > 1 ?\n\t\t\t\txgpu_texture_level_offset(&entry->description, 1) : xgpu_texture_face_size(&entry->description));'),
                       (11, 'entry->texture = xgpu_texture_name_new();' if 'xgpu_texture_name_new' in texture else 'glGenTextures(1, &entry->texture);')]:
        assert texture.count(call) == 1
        texture = texture.replace(call, f'stutter_begin({kind});'+call+f'stutter_end({kind});')
    texture = texture.replace('static GLuint texture_entry_result(', 'static GLuint measured_texture_result(')
    texture = texture.replace('memory_watch_generation(entry->address, entry->size)', 'measured_generation(entry->address, entry->size)')
    texture = texture.replace('static GLuint measured_texture_get(', '''
static unsigned long measured_generation(unsigned long address,unsigned long size) {
    unsigned long result;stutter_begin(8);result=memory_watch_generation(address,size);stutter_end(8);return result;
}
static GLuint texture_entry_result(struct texture_entry *entry,GLenum *target,struct xgpu_texture_description *description) {
    GLuint result;stutter_begin(10);result=measured_texture_result(entry,target,description);stutter_end(10);return result;
}
static GLuint measured_texture_get(''')
    texture += '''
GLuint xgpu_texture_get(DWORD const *resource,D3DCOLOR const *palette,
    GLenum *target,struct xgpu_texture_description *description) {
    GLuint result;stutter_begin(5);
    result=measured_texture_get(resource,palette,target,description);
    stutter_end(5);return result;
}
'''
    sources['port/linux/src/xbox_textures.c'] = texture
    if deterministic:
        # Split-screen test matches still use the network-server path, where
        # stock fixed_step is intentionally inactive. This isolated probe is
        # for one process only; never use it for network compatibility tests.
        condition = 'halo_fixed_step() && main_globals.connection == _game_connection_local'
        assert main.count(condition) == 2
        sources['source/main/main.c'] = main.replace(condition, 'halo_fixed_step() /* port: diagnostic fixed clock */')
        timing = (root/'source/game/game_time.c').read_text()
        condition = 'halo_fixed_step() && game_connection() == _game_connection_local'
        assert timing.count(condition) == 1
        sources['source/game/game_time.c'] = timing.replace(condition, 'halo_fixed_step() /* port: diagnostic fixed clock */')
        game = (root/'source/game/game.c').read_text()
        condition = 'set_random_seed(game_globals->options.random_seed);'
        assert game.count(condition) == 1
        sources['source/game/game.c'] = game.replace(condition, 'set_random_seed(0xDEADBEEF); /* port: diagnostic fixed seed */')
        bots = (root/'port/linux/game/bots.c').read_text()
        assert bots.count('network_game_get_random_seed()') == 1
        sources['port/linux/game/bots.c'] = bots.replace('network_game_get_random_seed()',
            '0xDEADBEEF /* port: diagnostic bot seed */')
    if (root/'port/linux/src/mcc_texture_bridge.c').exists():
        texture = (root/'port/linux/src/mcc_texture_bridge.c').read_text()
        texture = 'void stutter_begin(int);void stutter_end(int);\n'+texture.replace('BOOL xgpu_mcc_texture_get(', 'static BOOL measured_mcc_texture_get(')
        texture += '''
BOOL xgpu_mcc_texture_get(DWORD const *resource,D3DCOLOR const *palette,
    GLuint *texture,GLenum *target,struct xgpu_texture_description *description) {
    BOOL result;stutter_begin(4);
    result=measured_mcc_texture_get(resource,palette,texture,target,description);
    stutter_end(4);return result;
}
'''
        sources['port/linux/src/mcc_texture_bridge.c'] = texture
    if honeycomb:
        sources['port/linux/src/nv2a_psh.c'] = omit_honeycomb((root/'port/linux/src/nv2a_psh.c').read_text())
    manifest = dict(source_root=str(root), honeycomb_omitted=honeycomb, deterministic=deterministic,
                    record_prepared=record_prepared, source_hashes={})
    for relative, source in sources.items():
        entry = next(e for e in entries if e['file'].replace('\\', '/') == relative)
        path = output/Path(relative).name; obj = path.with_suffix('.o')
        path.write_text(source)
        manifest['source_hashes'][relative] = hashlib.sha256((root/relative).read_bytes()).hexdigest()
        command = entry['command']
        for old, new in [(entry['file'], str(path)), (entry['output'], str(obj))]:
            command = command.replace(old, new).replace(old.replace('/', '\\'), new)
        subprocess.run(command+' -Iport/linux/src -Iport/linux/game -Isource/main -Isource/game', cwd=root, check=True)
        link = link.replace(entry['output'], str(obj)).replace(entry['output'].replace('/', '\\'), str(obj))
    link = link.replace('build\\windows\\halo.exe', str(output/'halo.exe'))
    marker = ' -o '+str(output/'halo.exe')+' '
    head, tail = link.split(marker, 1)
    response = output/'link.rsp'; response.write_text(tail)
    subprocess.run(head+' --rsp-quoting=windows'+marker+'@'+str(response), cwd=root, check=True)
    shutil.copy2(root/'build/windows/SDL3.dll', output/'SDL3.dll')
    manifest['executable_sha256'] = hashlib.sha256((output/'halo.exe').read_bytes()).hexdigest()
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--omit-honeycomb', action='store_true')
    parser.add_argument('--deterministic', action='store_true', help='Single-process benchmark only: fixed clock and seed')
    parser.add_argument('--record-prepared', action='store_true', help='Diagnostic recipe capture includes speculative materials')
    args = parser.parse_args()
    build(args.source, args.output, args.omit_honeycomb, args.deterministic, args.record_prepared)
