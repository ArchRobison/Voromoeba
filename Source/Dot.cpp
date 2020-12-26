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

#include "Dot.h"
#include "Enum.h"
#include "Pond.h"
#include "Utility.h"
#include "World.h"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>

//! \file Dot.cpp

namespace {

enum class DotKind : int8_t {
    circle,
    ring,
    cross
};

} // (anonymous)

template<>
constexpr DotKind EnumMax<DotKind> = DotKind::cross;

namespace {

//! Minimum radius of a "DotImage". Set to 3 because the crosses render poorly for smaller values.
constexpr int32_t DotRadiusMin = 3;

//! Maximum radius of a "DotImage". Set to 7 so that a DotImage is never more than 16 pixels across.
constexpr int32_t DotRadiusMax = 7;

//! Log2 of "super resolution" factor. 
//! A dot position's x and y are rounded to the nearest multiple of 1/(1<<DotLgSuperRes),
//! and an image optimized for that multiple. This provide subpixel accurate drawing
//! of the dots. The images are NOT anti-aliased, because that can create eye strain.
//! The value 2 enables quarer-pixel accuracy.
constexpr int32_t DotLgSuperRes = 2;

constexpr int32_t DotSuperRes = 1 << DotLgSuperRes;

int32_t DotRadius;

using DotImage = std::bitset<1 + 2*DotRadiusMax + 1>[1 + 2*DotRadiusMax + 1];

#define DEBUG_DOT 0

#if DEBUG_DOT
void printDotImage(const DotImage& image) {
    int count = 0;
    for (int32_t i = 0; i < 1+2*DotRadiusMax+1; ++i) {
        for (int32_t j = 0; j < 1+2*DotRadiusMax+1; ++j) {
            fprintf(stderr, "%c", image[i][j] ? '*' : '.');
            count += image[i][j];
        }
        fprintf(stderr,"\n");
    }
    fprintf(stderr,"%d\n", count);
}
#endif

//! Flattened representation of matrix with size SuperRes x SuperRes
using DotImages = std::array<DotImage,DotSuperRes*DotSuperRes>;

EnumMap<DotKind, DotImages> DotMap;
EnumMap<BeetleKind, const DotImages*> DotOf;

struct DotImageLocator {
    //! (left,top) are screen coordinate corresponding to DotImage[0][0]
    int32_t left, top;
    //! Index into DotImages
    int32_t superResIndex;
    DotImageLocator(float x, float y);
};

DotImageLocator::DotImageLocator(float x, float y) {
    const int64_t xi = static_cast<int64_t>(std::round(x * DotSuperRes));
    const int64_t yi = static_cast<int64_t>(std::round(y * DotSuperRes));
    // Need floor-divide here, so use arithmetic shift
    left = (xi >> DotLgSuperRes) - DotRadius;
    top = (yi >> DotLgSuperRes) - DotRadius;
    const int32_t mask = (1<<DotLgSuperRes) - 1;
    superResIndex = (xi & mask) + DotSuperRes * (yi & mask);
}

int32_t Abs(int32_t i) {
    return i<0 ? -i : i;
}

} // (anonymous)

void Dot::initialize(const NimblePixMap& pixmap) {
    // Compute diagonal of screen
    const float diagonal = Distance(Point(pixmap.width(), pixmap.height()));
    // The 0.00158888 yields a radius of 7 for a standard 4k display.
    DotRadius = Clip<float>(DotRadiusMin, DotRadiusMax, std::round(diagonal*0.0015888f));
#if DEBUG_DOT
    fprintf(stderr,"DotRadius=%d\n",DotRadius);
#endif
    const int32_t r = DotRadius;
    for (int32_t sx = 0; sx < DotSuperRes; ++sx)
        for(int32_t sy = 0; sy < DotSuperRes; ++sy) {
            const float x = float(sx) / DotSuperRes + DotRadius;
            const float y = float(sy) / DotSuperRes + DotRadius;
            DotImageLocator d(x, y);
            Assert(d.left == 0);
            Assert(d.top == 0);
            const int32_t k = d.superResIndex;
            for (int32_t i=0; i<2*r+2; ++i)
                for (int32_t j=0; j<2*r+2; ++j) {
                    const float theta = std::atan2(i-y, j-x);
                    const float d = Dist2(j, i, x, y);
                    DotMap[DotKind::circle][k][i][j] = d <= r*r;
                    DotMap[DotKind::ring][k][i][j] = DotMap[DotKind::circle][k][i][j] && d>=r*r/2;
                    DotMap[DotKind::cross][k][i][j] = d <= 1 || d <= 2*r*r && static_cast<int32_t>(std::round(theta * (8.0f/(2*Pi<float>)))) % 2 != 0;
                }
        }
#if DEBUG_DOT
    for (int k = 0; k < DotSuperRes*DotSuperRes; ++k)
        printDotImage(DotMap[DotKind::cross][k]);
#endif

    DotOf[BeetleKind::water] = &DotMap[DotKind::circle];
    DotOf[BeetleKind::plant] = &DotMap[DotKind::ring];
    DotOf[BeetleKind::orange] = &DotMap[DotKind::ring];
    DotOf[BeetleKind::sweetie] = &DotMap[DotKind::ring];
    DotOf[BeetleKind::predator] = &DotMap[DotKind::cross];
}

void Dot::draw(NimblePixMap& window, const Pond& p) {
    const int32_t r = DotRadius;

    const Point o = World::viewTransform.transform(p.center());
    const float radius = World::viewTransform.scale(p.radius());
    const Point c = 0.5f*Point(window.width(), window.height());
    if (Distance(o, c) > radius+Distance(c))
        // Reject.  Circle around window does not overlap Pond.
        return;

    for (const Beetle& b : p) {
        const Point p = World::viewTransform.transform(b.pos);
        const DotImageLocator d(p.x, p.y);
        const DotImage& image = (*DotOf[b.kind])[d.superResIndex];
        Assert(&d);
        const int32_t xo = d.left;
        const int32_t yo = d.top;
        const int32_t xl = std::max(0, xo);
        const int32_t xr = std::min(window.width(), xo + 2*r + 2);
        const int32_t yt = std::max(0, yo);
        const int32_t yb = std::min(window.height(), yo + 2*r + 2);
        for (int32_t y=yt; y<yb; ++y)
            for (int32_t x=xl; x<xr; ++x)
                if (image[y-yo][x-xo])
                    *(NimblePixel*)window.at(x, y) = b.color.interior();
    }
}