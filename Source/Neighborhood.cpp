/* Copyright 2011-2021 Arch D. Robison

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

#include "Neighborhood.h"
#include <algorithm>

namespace {

inline bool ClockwiseOrInCircle(const Neighbor* a, const Neighbor* b, const Neighbor* c) {
    Assert(Cross(*a, *b)>=0);
    Assert(Cross(*b, *c)>=0);
    return Cross(*a, *c)<=0 || InCircle(*a, *b, *c);
}

} // (anonymous)

inline void Neighborhood::setEnd(Neighbor* s) {
    mySortedEnd = myExtraEnd = s;
    myExtraLimit = s+(s-mySortedBegin);
    Assert(myExtraLimit<=myPhysicalEnd);
}

Neighborhood::Neighborhood(Neighbor buffer[], size_t bufSize) :
    mySortedBegin(buffer),
    myPhysicalEnd(buffer+bufSize) {
    Assert(bufSize>=4);
    mySortedEnd = nullptr;
    myExtraEnd = nullptr;
    myExtraLimit = nullptr;
}

bool Neighborhood::tentativeAccept(const Neighbor& x) const {
    const Neighbor* s1 = std::upper_bound(mySortedBegin, mySortedEnd, x);
    const Neighbor* s0 = s1==mySortedBegin ? mySortedEnd-1 : s1-1;
    if (s1==mySortedEnd)
        s1 = mySortedBegin;
    return InCircle(*s0, x, *s1);
}

Neighbor* Neighborhood::trim(Neighbor* e) {
    while (!ClockwiseOrInCircle(e-2, e-1, mySortedBegin)) {
        --e;
        Assert(e>=mySortedBegin+2);
    }
    return e;
}

void Neighborhood::merge() {
    std::sort(mySortedBegin, myExtraEnd);
    Neighbor* e = trim(myExtraEnd);
    const Neighbor* s = mySortedBegin+1;
    Neighbor* d = mySortedBegin+1;
    while (s<e) {
        *d++ = *s++;
        Assert(d>=mySortedBegin+2);
        do {
            Neighbor* p = d>=mySortedBegin+3 ? d-3 : e-1;
            if (ClockwiseOrInCircle(p, d-2, d-1))
                break;
            // Cell d-2 is not a neighbor of the Voronoi cell at (0,0), so delete it.
            d[-2] = d[-1];
            --d;
        } while (d!=mySortedBegin+1);
    }
    d = trim(d);
    setEnd(d);
}

void Neighborhood::start() {
    // Initialize buffer with huge triangle surrounding the origin.
    for (int k=0; k<3; ++k) {
        static constexpr float huge = 1E6;
        const Point p = Polar(huge, k*(2*Pi<float>/3));
        mySortedBegin[k].assign(p, PseudoAngle(p.x, p.y), Neighbor::ghostIndex);
        Assert(k==0 || mySortedBegin[k].alpha > mySortedBegin[k-1].alpha);
    }
    setEnd(mySortedBegin+3);
}

size_t Neighborhood::finish() {
    Assert(mySortedEnd <= myExtraEnd);
    if (mySortedEnd < myExtraEnd)
        merge();
    return mySortedEnd-mySortedBegin;
}

// FIXME - figure out where to put the clipping against regions