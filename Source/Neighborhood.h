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
class Neighbor: public Point {
public:
    typedef unsigned indexType;
    indexType index;
    static const indexType ghostIndex = -1;
private:
    float alpha;
    void assign( Point pos, float alpha_, indexType index_ ) {
        *static_cast<Point*>(this) = pos;
        alpha = alpha_;
        index = index_;
    }
    static friend bool operator<( const Neighbor& a, const Neighbor& b ) {
        return a.alpha<b.alpha;
    }
    friend class Neighborhood;
};

//! Computes neighbors of target point in a Voronoi diagram
class Neighborhood {
    Neighbor* const mySortedBegin;
    Neighbor* mySortedEnd;
    Neighbor* myExtraEnd;
    Neighbor* myExtraLimit;
    Neighbor* const myPhysicalEnd;
    // Merge [mySortedBegin,mySortedEnd) and [mySortedEnd,myExtraEnd)
    void merge();
    void setEnd( Neighbor* s );
    bool tentativeAccept( const Neighbor& s ) const;
    Neighbor* trim( Neighbor* e );
public:
    Neighborhood( Neighbor buffer[], size_t bufSize );
    void start();
    void addPoint( Point p, Neighbor::indexType index );
    //! Finish computing neighbors and return number 'n' of neighbors
    /** Afterwards, buffer[0..n-1] contain the neighbors.  
        Some may be "ghosts" with index==ghostIndex. */
    size_t finish();
};

inline void Neighborhood::addPoint( Point p, Neighbor::indexType index ) {
    float alpha = PseudoAngle(p.x,p.y);
    myExtraEnd->assign( p, alpha, index );
    if( tentativeAccept(*myExtraEnd) ) 
        if( ++myExtraEnd>=myExtraLimit ) 
            merge();
}