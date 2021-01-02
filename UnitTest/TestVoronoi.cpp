#include "Voronoi.h"
#include "Region.h"

void TestVoronoi() {
    NimblePixel pixels[100][100];
    NimblePixMap window( 100, 100, 32, pixels, sizeof(pixels[100]) );

    ConvexRegion r;
    r.makeRectangle(Point(10,20),Point(90,80));  

    SetRegionClip(0,0,window.width(),window.height());
    CompoundRegion region;
    region.build(&r,&r+1);

    for( int trial=0; trial<1000; ++trial ) {
        static Ant ants[N_ANT_MAX];
        NimblePixel color[N_ANT_MAX];
        Ant* a = ants;
        a->assignFirstBookend();
        ++a;
        size_t n = 100;
        for( size_t k=0; k<n; ) {
            float x = RandomFloat(100);
            float y = RandomFloat(100);
            float dx = RandomFloat(0.001f);
            float dy = RandomFloat(0.0f);
            a->assign(Point(x,y),color[k]); 
            ++a; ++k;
            a->assign(Point(x+dx,y+dy),color[k]);
            ++a; ++k;
        }
        a->assignLastBookend();
        ++a;
        DrawVoronoi( window, region, ants, a );
    }
}