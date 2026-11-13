"""Discovery retries changed exact paths, not an unchanged rejected fingerprint."""
from pathlib import Path
import subprocess
from harness import function
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]


def test_identity_candidates(tmp_path):
    source = (ROOT / "port/linux/game/mcc_identity.c").read_text()
    begin = source.index("static struct {\n    char wire")
    end = source.index("} mcc_identity_job;", begin) + len("} mcc_identity_job;")
    code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mcc_validation.h"
#define MCC_IDENTITY_MAX_CANDIDATES 16
#define FALSE 0
#define csstrcasecmp strcmp
/* JOB */
static struct mcc_precache_snapshot files;
static unsigned char metadata[32];
static int scans,readable=1,metadata_valid=1,maps=2;
void mcc_maps_rescan(void) {scans++;}
short mcc_maps_count(int multiplayer) {(void)multiplayer;return (short)maps;}
char const *mcc_maps_level_name(short i) {return i ? "mcc_maps\\other\\arena" : "mcc_maps\\p\\arena";}
int mcc_precache_snapshot(char const *name,struct mcc_precache_snapshot *out) {
    (void)name;*out=files;return readable;
}
static int mcc_identity_metadata(char const *name,unsigned char out[32]) {
    (void)name;memcpy(out,metadata,32);return metadata_valid;
}
/* CANDIDATES */
int main(void) {
    strcpy(mcc_identity_job.session,"mcc_maps\\p\\arena");
    mcc_identity_job.attempted=1;
    assert(mcc_identity_candidates() && scans==1 && mcc_identity_job.count==1);
    assert(!strcmp(mcc_identity_job.candidates[0],"mcc_maps\\other\\arena"));
    files.files[0].written_low++;
    assert(mcc_identity_candidates() && mcc_identity_job.count==2);
    assert(!strcmp(mcc_identity_job.candidates[0],mcc_identity_job.session));
    files.files[0].written_low=0;metadata[0]=1;
    assert(mcc_identity_candidates() && mcc_identity_job.count==2);
    metadata[0]=0;readable=0;
    assert(mcc_identity_candidates() && mcc_identity_job.count==2);
    readable=1;metadata_valid=0;
    assert(mcc_identity_candidates() && mcc_identity_job.count==2);
    metadata_valid=1;mcc_identity_job.attempted=0;
    assert(mcc_identity_candidates() && mcc_identity_job.count==2);
    maps=17;assert(!mcc_identity_candidates());
    return 0;
}
'''
    code = code.replace("/* JOB */", source[begin:end]).replace("/* CANDIDATES */",
        function(source, "mcc_identity_candidates"))
    path = tmp_path / "identity.c"
    path.write_text(code)
    exe = compile_tool(tmp_path, [path])
    subprocess.run([str(exe)], check=True, capture_output=True)
