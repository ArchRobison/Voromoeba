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

#include "Outline.h"
#include "Utility.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

uint32_t OutlinedColor::exteriorColorCount;
NimblePixel OutlinedColor::exteriorTable[OutlinedColor::exteriorNumColorMax+1];

uint32_t Outline::idCount;

Outline::segment Outline::array[Outline::nSegmentMax];
Outline::segment* Outline::ptr = Outline::array;

inline int Red(NimblePixel c) {
    return c>>16&0xFF;
}

inline int Green(NimblePixel c) {
    return c>>8&0xFF;
}

inline int Blue(NimblePixel c) {
    return c>>0&0xFF;
}

inline int InterpolateInt(float frac, int a, int b) {
    return (1.f-frac)*a + frac*b;
}

// FIXME - consider using SSE
inline NimblePixel InterpolateColor(float frac, NimblePixel c0, NimblePixel c1) {
    return InterpolateInt(frac, Red(c0), Red(c1))<<16 |
        InterpolateInt(frac, Green(c0), Green(c1))<<8 |
        InterpolateInt(frac, Blue(c0), Blue(c1));
}

const int cacheSize = (Outline::lineWidth+1)*(Outline::lineWidth+1)+2;
static NimblePixel interiorCache, exteriorCache, cache[cacheSize];

void Outline::loadCache(NimblePixel interior, NimblePixel exterior) {
    if (cache[0]!=exterior || cache[(lineWidth+1)*(lineWidth+1)+1] != interior) {
        cache[0] = cache[1] = exterior;
        for (int d2=2; d2<=(lineWidth+1)*(lineWidth+1); ++d2) {
            float f = (std::sqrt(float(d2))-1)*(1.0f/lineWidth);
            cache[d2] = InterpolateColor(f, exterior, interior);
        }
        cache[(lineWidth+1)*(lineWidth+1)+1] = interior;
    }
}

template<bool Left, bool Right>
void Outline::gradient(const segment* s, const segment* jmin, const segment* jmax, int xleft, int xright, NimblePixel* out) {
    Assert(jmin[0].id==s->id);
    Assert(jmax[0].id==s->id);
    Assert(jmin<=s);
    Assert(s<=jmax);
    Assert(!(Left && Right));
    Assert(0<=xleft);
    Assert(jmin[0].y<=s->y && s->y<=jmax[0].y);
    const int y = s->y;
    int mind2init = Min<int>(Square(1+Min(y-jmin[0].y, jmax[0].y-y)), cacheSize-1);
    if (Left) {
        // Trim search region
        while (jmin<s && jmin[0].left<=s[0].left)
            ++jmin;
        while (jmax>s && jmax[0].left<=s[0].left)
            --jmax;
        if (xleft==s->left)
            out[xleft++] = cache[1];
    }
    if (Right) {
        // Trim search region
        while (jmin<s && jmin[0].right>=s[0].right)
            ++jmin;
        while (jmax>s && jmax[0].right>=s[0].right)
            --jmax;
        if (xright==s->right)
            out[--xright] = cache[1];
    }
    for (int x=xleft; x<xright; ++x) {
        int mind2 = mind2init;
        for (const segment* j=jmin; ; ++j) {
            int dy = j[0].y-y;
            int d2 = dy*dy;
            if (Left) {
                if (x>=j[0].left) {
                    int dx = x-(j[0].left-1);
                    d2 += dx*dx;
                }
            } else if (Right) {
                if (x<j[0].right) {
                    int dx = x-j[0].right;
                    d2 += dx*dx;
                }
            } else {
                if (x<j[0].left) {
                    if (j[-1].y==j[0].y && j[-1].id==j[0].id && x<j[-1].right)
                        goto next;
                } else if (x>=j[0].right) {
                    if (j[1].y==j[0].y && j[1].id==j[0].id && x>=j[1].left)
                        goto next;
                } else {
                    int m2 = (j[0].left-1)+j[0].right;
                    int dx;
                    if (2*x<m2) dx = x-(j[0].left-1);
                    else dx = x-j[0].right;
                    d2 += dx*dx;
                }
            }
            if (d2<mind2)
                mind2=d2;
next:
            if (j==jmax) break;
        }
        out[x] = cache[mind2];
#if 0
        out[x] = Left && Right ? 0xFFFFFF : Left ? 0xFF0000 : Right ? 0x0000FF : 0;
#endif
    }
}

Outline::segment Outline::sorted[nSegmentMax];

Outline::segment* Outline::sortIntoBins() {
    static uint32_t binCount[nIdMax];
    static segment* binPtr[nIdMax];

#if ASSERTIONS
    for (unsigned i=0; i<=idCount; ++i)
        Assert(binCount[i]==0);
#endif
    for (const segment* s = array; s<ptr; ++s) {
        const uint32_t i = static_cast<uint32_t>(s->id);
        Assert(i < nIdMax);
        binCount[i]++;
    }
    segment* total = sorted;
    for (uint32_t i=0; i<=idCount; ++i) {
        binPtr[i] = total;
        total += binCount[i];
        binCount[i] = 0;        // Clear for next time around
    }
    Assert(total-sorted == ptr-array);
    for (const segment* s = array; s<ptr; ++s) {
        const uint32_t i = static_cast<uint32_t>(s->id);
        Assert(sorted<=binPtr[i] && binPtr[i]<sorted+nSegmentMax);
        *binPtr[i]++ = *s;
    }
    Assert(binPtr[idCount]-sorted == ptr-array);
#if 0
    static int maxBinCount, maxIdCount;
    maxIdCount = Max<int>(maxIdCount, idCount);
    maxBinCount = Max<int>(maxBinCount, ptr-array);
#endif
    return binPtr[idCount];
}

void Outline::finishAndDraw(NimblePixMap& window) {
    // FIXME - consider using radix sort
    segment* sortedEnd = sortIntoBins();
    sortedEnd->id = idType::null;

    int prevId = 0;
    for (const segment* s = sorted+1; s<sortedEnd; ++s) {
        if (unsigned(s->y)>=unsigned(window.height()))
            continue;
        Assert(s->color.hasExterior());
        const int x0 = Max<int>(s->left, 0);
        const int x1 = Min<int>(s->right, window.width());
        const segment* jmin = s;
        while (jmin[-1].id==s->id && jmin[-1].y>=s->y-lineWidth)
            --jmin;
        const segment* jmax = s;
        while (jmax[1].id==s->id && jmax[1].y<=s->y+lineWidth)
            ++jmax;
        int d = Min(jmax[0].y-s->y, s->y-jmin[0].y);
        // Now set xleft and xright to bounds of segment that is all of interior color
        int xleft = s->left;
        int xright = s->right;
        if (d>=lineWidth) {
            for (const segment* j=jmin; j<=jmax; ++j) {
                xleft = Max(xleft, +j[0].left);
                xright = Min(xright, +j[0].right);
            }
            xleft += lineWidth;
            xright -= lineWidth;
            Assert(xleft>=0);
            if (xleft>=xright)
                goto empty;
        } else {
empty:
            // Homogeneous part is empty.  Put empty homogeneous part to right of left inhomogeneous part.
            xleft = xright = s->right;
        }
        NimblePixel c = s->color.interior();
        loadCache(c, s->color.exterior());
        NimblePixel* out = (NimblePixel*)window.at(0, s->y);
        if (xleft<xright) {
            gradient<true, false>(s, jmin, jmax, x0, xleft, out);
            // Do homogeneous part
            for (int x=xleft; x<xright; ++x)
                out[x] = c;
            gradient<false, true>(s, jmin, jmax, Max(xright, 0), x1, out);
        } else {
            gradient<false, false>(s, jmin, jmax, x0, x1, out);
        }
    }
#if ASSERTIONS
    ptr = nullptr;
#endif
}