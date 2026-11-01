#include "harness.h"
struct glow_datum { short number_of_markers; real marker_time_index[4]; };
struct glow_particle { real t; short parent_marker_index; };
#define PIN(v,lo,hi) ((v)<(lo)?(lo):(v)>(hi)?(hi):(v))
#define match_vassert(file,line,test,message) CHECK(test, "marker interval missing")
#include "under_test.inc"
int main(int argc, char **argv)
{
    struct glow_datum glow;
    struct glow_particle particle;
    short count, index;
    CHECK(argc==2,"case");
    for (count=2; count<=4; count++)
    {
        glow.number_of_markers=count;
        for (index=0; index<count; index++) glow.marker_time_index[index]=(real)index;
        if (!strcmp(argv[1],"endpoint"))
        {
            particle.t=(real)(count-1);
            CHECK(marker_at(&glow,&particle)==count-2,"final endpoint used a nonexistent segment");
        }
        else if (!strcmp(argv[1],"interior"))
        {
            for (index=0; index<count-1; index++)
            {
                particle.t=(real)index;
                CHECK(marker_at(&glow,&particle)==index,"interior boundary changed");
                particle.t+=0.5f;
                CHECK(marker_at(&glow,&particle)==index,"interior interval changed");
            }
        }
        else
        {
            CHECK(!strcmp(argv[1],"invalid"),"case");
            particle.t=(real)count;
            marker_at(&glow,&particle);
            CHECK(0,"out-of-range time accepted");
        }
    }
    return 0;
}
