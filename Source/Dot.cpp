/* Copyright 2011-2020 Arch D. Robison

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
#include "Utility.h"
#include "World.h"
#include <cstdint>

namespace {

constexpr int32_t DotRadius = 3;

enum class DotKind : int8_t {
    circle,
    ring,
    cross
};

constexpr size_t N_DotKind = size_t(DotKind::cross) + 1;

typedef bool DotImage[2*DotRadius+1][2*DotRadius+1];
EnumMap<DotKind, N_DotKind, DotImage> DotSet;
EnumMap<BeetleKind, N_BeetleKind, const DotImage*> DotOf;

int Abs(int i) {
    return i<0 ? -i : i;
}

void InitializeDot() {
    const int r = DotRadius;
    for (int i=0; i<2*r+1; ++i)
        for (int j=0; j<2*r+1; ++j) {
            float d = Dist2(i, j, r, r);
            DotSet[DotKind::circle][i][j] = d<=(r*r+2);
            DotSet[DotKind::ring][i][j] = DotSet[DotKind::circle][i][j] && d>=r*r/2;
            DotSet[DotKind::cross][i][j] = Abs(i-j)<=1 || Abs(i+j-2*r)<=1;
        }
    DotOf[BeetleKind::water] = &DotSet[DotKind::circle];
    DotOf[BeetleKind::plant] = &DotSet[DotKind::ring];
    DotOf[BeetleKind::orange] = &DotSet[DotKind::ring];
    DotOf[BeetleKind::sweetie] = &DotSet[DotKind::ring];
    DotOf[BeetleKind::predator] = &DotSet[DotKind::cross];
    // Self and missile do not show up as Dots
}

} // (anonymous)

void DrawDots(NimblePixMap& window, const Pond& p) {
    const int r = DotRadius;
    static bool isInitialized;
    if (!isInitialized) {
        InitializeDot();
        isInitialized = true;
    }

    Point o = World::viewTransform.transform(p.center());
    float radius = World::viewTransform.scale(p.radius());
    Point c = 0.5f*Point(window.width(), window.height());
    if (Distance(o, c)>radius+Distance(c))
        // Reject.  Circle around window does not overlap Pond.
        return;

    for (const Beetle* b = p.begin(); b!=p.end(); ++b) {
        Point p = World::viewTransform.transform(b->pos);
        const DotImage& d = *DotOf[b->kind];
        Assert(&d);
        int i=Round(p.y);
        int j=Round(p.x);
        int xl = Max(0, j-r);
        int xr = Min(window.width(), j+r+1);
        int yt = Max(0, i-r);
        int yb = Min(window.height(), i+r+1);
        for (int y=yt; y<yb; ++y)
            for (int x=xl; x<xr; ++x)
                if (d[y-i+r][x-j+r])
                    *(NimblePixel*)window.at(x, y) = b->color.interior();
    }
}