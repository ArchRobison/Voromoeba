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

#ifndef Bridge_H
#define Bridge_H

#include "Geometry.h"
#include "Region.h"

 //! A Bridge is a rectangle with two elliptical cutouts
class Bridge {
    const Circle* pond[2];
    Parallelogram parallelogram;
    Ellipse ellipse[2];
    float opening;          //!< How far bridge is open.  0 = closed.  1 = wide op en.
    float openingVelocity;
    void computeGeometry(float scale);
private:
    Point rectCenter;
    Point rectUpperLeft;
    Point rectUpperRight;
    Point ellipseCenter[2];
    Point ellipseA[2];
    //! Half-width of ellipses
    float ellipseB;
public:
    // Initialize connection between circles p and q.
    void initialize(const Circle& p, const Circle& q) {
        pond[0] = &p;
        pond[1] = &q;
        computeGeometry(1.0f);
        openingVelocity = 0;
        opening = 0;
    }
    Point center() const { return rectCenter; }
    bool contains(Point p) const { return parallelogram.contains(p); }
    bool isWideOpen() const { return opening>=1.0f; }
    bool isClosed() const { return opening<=0.0f; }
    void setOpeningVelocity(float v) { openingVelocity=v; }
    void update(float dt);
    float intercept(const Point& p, const Point& s) const;
    Point plough(const Point& p, const Point& s) const;
    bool overlapsSegment(Point a, Point c) const;
    ConvexRegion* pushVisibleRegions(ConvexRegion* rLast) const;
};

#endif /* Bridge_H */