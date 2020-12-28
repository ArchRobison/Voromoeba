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

#include <algorithm>
#include "AssertLib.h"
#include "Geometry.h"

 //! A neighbor of a target point, as computed by class Neighborhood.
class Neighbor : public Point {
public:
    using indexType = uint32_t;
    indexType index;
    static constexpr indexType ghostIndex = ~indexType(0);
private:
    float alpha;
    void assign(Point pos, float alpha_, indexType index_) {
        *static_cast<Point*>(this) = pos;
        alpha = alpha_;
        index = index_;
    }
    static friend bool operator<(const Neighbor& a, const Neighbor& b) {
        return a.alpha<b.alpha;
    }
    friend class Neighborhood;
};

//! Used to compute neighbors of a target generator point in a Voronoi diagram.
//! The target generator point is presumed to be at Point(0,0).
//! In the game, the generator point is "self" or "missile", and other
//! generator point's coordinates are relative to the target point.
//! 
//! The intended usage is:
//! 
//!      Neighborhood(buffer,...);
//!      start();
//!      for(...) addPoint(...);
//!      n = finish();
//!      buffer[0..n-1] contains Neighbors of (0,0) in a  Voronoi diagram.
//!
class Neighborhood {
public:
    //! Construct an empty Neighborhood with room for bufSize ants.
    Neighborhood(Neighbor buffer[], size_t bufSize);

    //! Start/reste collection of generator points.
    void start();

    //! Add a generator point.
    void addPoint(Point p, Neighbor::indexType index);

    //! Finish computing neighbors and return number 'n' of neighbors.
    //! Afterwards, buffer[0..n-1] contain the neighbors.
    //! Some may be "ghosts" with index==Neighbor::ghostIndex.
    size_t finish();
private:
    //! Pointer to beginning of buffer.
    Neighbor* const mySortedBegin;

    //! Pointer to end of portion of buffer that is sorted by alpha.
    Neighbor* mySortedEnd;

    Neighbor* myExtraEnd;
    Neighbor* myExtraLimit;
    Neighbor* const myPhysicalEnd;
    // Merge [mySortedBegin,mySortedEnd) and [mySortedEnd,myExtraEnd)
    void merge();
    void setEnd(Neighbor* s);

    //! Return fale if s can be rejected as a possible neighbor,
    //! based on points currently in the buffer.
    bool tentativeAccept(const Neighbor& s) const;

    Neighbor* trim(Neighbor* e);
};

inline void Neighborhood::addPoint(Point p, Neighbor::indexType index) {
    const float alpha = PseudoAngle(p.x, p.y);
    myExtraEnd->assign(p, alpha, index);
    if (tentativeAccept(*myExtraEnd))
        if (++myExtraEnd >= myExtraLimit)
            merge();
}