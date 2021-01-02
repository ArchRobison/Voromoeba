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
#include "Geometry.h"

inline void Parallelogram::unitSquareIntercept(float a, float b, float& s, float& t) {
    s = 0;
    t = 0;
    if (a<b) {
        Assert(a<=1 && 0<=b);
        if (a<0)
            s = -a/(b-a);
        if (b>1)
            t = (b-1)/(b-a);
    } else {
        Assert(b<=1 && 0<=a);
        if (a>1)
            s = (1-a)/(b-a);
        if (b<0)
            t = b/(b-a);
    }
    Assert(0<=s && s<=1);
    Assert(0<=t && t<=1);
    Assert(s+t<=1);
}

bool Parallelogram::clipSegment(Point& p, Point& q) const {
    Point a = mySquare(p);
    Point b = mySquare(q);
    // Do fast rejection test
    if (a.x<0 && b.x<0 || a.x>1 && b.x>1 || a.y<0 && b.y<0 || a.y>1 && b.y>1) {
        return false;
    } else {
        float sx, tx, sy, ty;
        unitSquareIntercept(a.x, b.x, sx, tx);
        unitSquareIntercept(a.y, b.y, sy, ty);
        float s = Max(sx, sy);
        float t = Max(tx, ty);
        if (s+t<1.0f) {
            Point v = q-p;
            p += s*v;
            q -= t*v;
            return true;
        } else {
            return false;
        }
    }
}