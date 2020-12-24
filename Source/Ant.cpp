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

#include "Ant.h"
#include "Host.h"
#include "Voronoi.h"
#include <algorithm>

namespace {

// The "two buffer" logic here is necessary for the scene cut effect, where ants from the new scene
// march in from the perimeter and ants from the old scene march out to the perimeter.

//! Specifies which row of AntArray is the "new" buffer.
//! Always 0 or 1
uint8_t CurrentHalf;

//! "Old" and "new" buffers. 
Ant AntArray[2][N_ANT_MAX];

//! Positions where ants in "old" buffer are going to.
Point To[N_ANT_MAX];

//! Positions where ants in "new" buffer are coming from.
Point From[N_ANT_MAX];

//! Pointer to first non-bookend Ant in new half of AntArray
Ant* BufferFirst;

//! Pointer to on e past last non-bookend And in new half of Array
Ant* BufferPtr;

//! Pointer to first non-bookend Ant in old half of AntArray
Ant* OldFirst;

//! Pointer to one past last non-bookend Ant in old half of AntArray
Ant* OldLast;

bool CutFlag;

} // (anonymous)

bool ShowAnts;

void Ant::clearBuffer() {
    BufferPtr = AntArray[CurrentHalf];
}

Ant* Ant::openBuffer() {
    Assert(BufferPtr);
    Ant* a = BufferFirst = BufferPtr;
    (a++)->assignFirstBookend();
    return a;
}

void Ant::switchBuffer() {
    Assert(CurrentHalf<2);
    // Save pointers to beginning and ending of current buffer
    OldFirst = AntArray[CurrentHalf]+1;
    OldLast = BufferPtr-1;
    Assert(OldFirst[-1].y==-AntInfinity);
    Assert(OldLast[0].y==AntInfinity);
    Assert(OldFirst<=OldLast);
    // Switch to the other buffer
    CurrentHalf ^= 1;
    CutFlag = true;
}

//! For each Ant a in [first,last), set corresponding point of outer to where position
//! on perimeter where Ant will come from (if disappearing) or go to (if appearing).
static void SetPoints(NimblePixMap& window, const Ant* first, const Ant* last, Point outer[]) {
    Assert(first<=last);
    // Calculations are in window coordinates.
    const Point center = Point(window.width()/2, window.height()/2);
    const float r = 0.6f * std::sqrt(Dist2(window.width(), window.height()));
    Point* o = outer;
    for (const Ant* a=first; a!=last; ++a) {
        const Point p = *a - center;
        *o++ = center + Polar(r, RandomFloat(1.0f) + std::atan2(p.y, p.x));
    }
}

static Ant* AntCutCompose(NimblePixMap& window, Ant* antLast) {
    Assert(BufferFirst->y == -AntInfinity);
    static double baseTime;
    double globalTime = HostClockTime();
    if (CutFlag) {
        CutFlag = false;
        baseTime = globalTime;
        Assert(OldFirst[-1].y==-AntInfinity);
        Assert(OldFirst<OldLast);
        SetPoints(window, OldFirst, OldLast, To);
        SetPoints(window, BufferFirst+1, antLast, From);
    }
    const float t = 1.0f*(globalTime-baseTime);
#if 1   
    // Interpolate current buffer with From
    if (t < 1.0f) {
        const float f = std::min(t, 1.0f);
        const Point* p = From;
        for (Ant* a = BufferFirst+1; a!=antLast; ++a, ++p)
            static_cast<Point&>(*a) = f**a + (1.0f-f)**p;
    }
#endif
#if 1
    if (t<2.0f) {
        // Interpolate old buffer with to 
        const float f = Min(1.0f, 2.0f-t);
        const Point* p = To;
        Ant* a = antLast;
        for (Ant* old=OldFirst; old!=OldLast; ++old, ++p) {
            if (!old->isBookend()) {
                a->color = old->color;
                static_cast<Point&>(*a) = f**old + (1-f)**p;
                ++a;
            }
        }
        antLast = a;
    }
#endif
    return antLast;
}

static void DrawAnts(NimblePixMap& window, const Ant* antFirst, const Ant* antLast) {
    // Draw black dot for each beetle
    for (const Ant* a=antFirst; a<antLast; ++a) {
        NimblePixel c = a->color.interior();
        c = (c & 0xFF) + (c>>8&0xFF) + (c>>16&0xFF) >=0x180 ? 0 : ~0;

        int xl = Max(int(a->x-1), 0);
        int yl = Max(int(a->y-1), 0);
        int xu = Min(int(a->x+2), window.width());
        int yu = Min(int(a->y+2), window.height());
        if (xl<xu && yl<yu) {
            NimbleRect r(xl, yl, xu, yu);
            window.draw(r, c);
        }
    }
}

void Ant::closeBufferAndDraw(NimblePixMap& window, const CompoundRegion& region, Ant* antLast, bool compose, bool showAnts) {
    if (compose)
        antLast = AntCutCompose(window, antLast);
    Assert(AntArray[CurrentHalf] < antLast && antLast<AntArray[CurrentHalf]+N_ANT_MAX);
    (antLast++)->assignLastBookend();
    BufferPtr = antLast;
    DrawVoronoi(window, region, BufferFirst, antLast);
    if (showAnts)
        DrawAnts(window, BufferFirst+1, antLast-1);
}