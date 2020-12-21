/* Copyright 2011-2013 Arch D. Robison 

   Licensed under the Apache License, Version 2.0 (the "License"); 
   you may not use this file except in compliance with the License. 
   You may obtain a copy of the License at 
   
       http://www.apache.org/licenses/LICENSE-2.0 
       
   Unless required by applicable law or agreed to in writing, software 
   distributed under the License is distributed on an "AS IS" BASIS, 
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
   See the License for the specific language governing permissions and 
   limitations under the License.
 */

#include "Pond.h"
#include "World.h"

const int DotRadius = 3;

enum DotKind {
    DK_Circle,
    DK_Ring,
    DK_Cross,
    N_DotKind
};

typedef bool DotImage[2*DotRadius+1][2*DotRadius+1];
static DotImage DotSet[N_DotKind];
static const DotImage* DotOf[N_BeetleKind];

static inline int Abs( int i ) {
    return i<0 ? -i : i;
}

static void InitializeDot() {
    const int r = DotRadius;
    for( int i=0; i<2*r+1; ++i ) 
        for( int j=0; j<2*r+1; ++j ) {
            float d = Dist2(i,j,r,r);
            DotSet[DK_Circle][i][j] = d<=(r*r+2);
            DotSet[DK_Ring][i][j] = DotSet[DK_Circle][i][j] && d>=r*r/2;
            DotSet[DK_Cross][i][j] = Abs(i-j)<=1 || Abs(i+j-2*r)<=1;
        }
    DotOf[BK_WATER] = &DotSet[DK_Circle];
    DotOf[BK_PLANT] = &DotSet[DK_Ring];
    DotOf[BK_ORANGE] = &DotSet[DK_Ring];
    DotOf[BK_SWEETIE] = &DotSet[DK_Ring];
    DotOf[BK_PREDATOR] = &DotSet[DK_Cross];
    // Self and missile do not show up as Dots
}

void DrawDots( NimblePixMap& window, const Pond& p ) {
    const int r = DotRadius;
    static bool isInitialize;
    if( !isInitialize ) {
        InitializeDot();
        isInitialize = true;
    }

    Point o = World::viewTransform.transform( p.center() );
    float radius = World::viewTransform.scale( p.radius() );
    Point c = 0.5f*Point(window.width(),window.height());
    if( Distance(o,c)>radius+Distance(c) )
        // Reject.  Circle around window does not overlap Pond.
        return;

    for( const Beetle* b = p.begin(); b!=p.end(); ++b ) {
        Point p = World::viewTransform.transform( b->pos );
        const DotImage& d = *DotOf[b->kind];
        Assert(&d);
        int i=Round(p.y);
        int j=Round(p.x);
        int xl = Max( 0, j-r );
        int xr = Min( window.width(), j+r+1 );
        int yt = Max( 0, i-r );
        int yb = Min( window.height(), i+r+1 );
        for( int y=yt; y<yb; ++y ) {
            for( int x=xl; x<xr; ++x ) {
                if( d[y-i+r][x-j+r] )
                    *(NimblePixel*)window.at(x,y) = b->color.interior();
            }
        }
    }
}