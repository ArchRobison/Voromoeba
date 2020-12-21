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

#include "Bridge.h"
#include "World.h"

bool Bridge::overlapsSegment(Point a, Point c) const {
    if (!isClosed()) {
        if (parallelogram.clipSegment(a, c)) {
            for (size_t k=0; k<2; ++k)
                if (ellipse[k].coversSegment(a, c))
                    return false;
            return true;
        }
    }
    return false;
}

void Bridge::update(float dt) {
    opening = Clip(0.0f, 1.0f, opening+dt*openingVelocity);
    computeGeometry(opening);
}

void Bridge::computeGeometry(float scale) {
    // Half-width is proportional to geometric mean of radii.
    const Circle& p = *pond[0];
    const Circle& q = *pond[1];
    float halfWidth = std::sqrt(p.radius()*q.radius())*0.1f*scale;
    if (halfWidth<0.001f) halfWidth=0.001f;
    Point u = (q.center()-p.center())/Distance(p.center(), q.center());
    Point v = halfWidth*Point(-u.y, u.x);
    // "nudge" is a half-pixel adjustment that is necessary for covering up
    // rounding of pixels.
    float nudge = 0.0f;
    rectUpperLeft = (p.center()+v)+(p.intercept(p.center()+v, u)-nudge)*u;
    rectUpperRight = (q.center()+v)-(q.intercept(q.center()+v, -u)-nudge)*u;
    rectCenter = (rectUpperLeft+rectUpperRight)/2-v;
    Point rectLowerLeft = 2*rectCenter - rectUpperRight;
    parallelogram = Parallelogram(rectUpperRight, rectUpperLeft, rectLowerLeft);
    ellipseB = halfWidth/2;
    ellipseCenter[0] = rectCenter + v;
    ellipseA[0] = rectUpperLeft;
    ellipseCenter[1] = rectCenter - v;
    ellipseA[1] = rectUpperLeft - 2*v;
    for (size_t k=0; k<2; ++k)
        ellipse[k] = Ellipse(ellipseCenter[k], ellipseA[k], ellipseB);
}

float Bridge::intercept(const Point& p, const Point& v) const {
    float s0 = parallelogram.intercept(p, v);
    float s1 = ellipse[0].intercept(p, v, false);
    float s2 = ellipse[1].intercept(p, v, false);
    return Min(s0, Min(s1, s2));
}

Point Bridge::plough(const Point& p, const Point& s) const {
    // Find the closer ellipse
    int k = Dist2(ellipseCenter[1], p)<Dist2(ellipseCenter[0], p);
    Point q = p+s;
    if (ellipse[k].contains(q))
        q = ellipse[k].projectOntoPerimeter(q);
    return q;
}

ConvexRegion* Bridge::pushVisibleRegions(ConvexRegion* rLast) const {
    const auto& v = World::viewTransform;
    rLast->makeParallelogram(v.transform(center()),
        v.transform(rectUpperLeft),
        v.transform(rectUpperRight));
    if (!rLast->empty()) {
        ++rLast;
        for (int i=0; i<2; ++i) {
            rLast->makeEllipse(v.transform(ellipseCenter[i]),
                v.transform(ellipseA[i]),
                v.scale(ellipseB));
            if (!rLast->empty()) {
                rLast->setIsPositive(false);
                ++rLast;
            }
        }
    }
    return rLast;
}
