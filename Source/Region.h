/* Copyright 2011-2013 Arch D. Robison 

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
#pragma once
#ifndef REGION_H
#define REGION_H

#include "Geometry.h"
#include "Outline.h" 

// FIXME - add striping so we can shrink this value
static const int MAX_STRIPE_HEIGHT = 2048;
static const int MAX_CONVEX_REGION = 40;

//! Left and right bounds of a line segment contained inside a region.
class RegionSegment {
public:
    typedef short value_type;
    static const short value_type_max = SHRT_MAX;
    value_type left, right;
    bool empty() const {return right<=left;}
private:
    void assign( value_type l, value_type r ) {
        left=l; 
        right=r;
    }
    friend bool operator<( const RegionSegment& a, const RegionSegment& b ) {
        return a.left<b.left;
    }
    friend class ConvexRegion;
    friend class CompoundRegion;
};

void SetRegionClip( float left, float top, float right, float bottom, int lineWidth=0 );
#if ASSERTIONS
extern int RegionClipBoxLineWidth;
#endif

//! A vector of type T indexed by scan-line y values.
template<typename T, bool InclusiveBottom>
class RowVector {
public:
    bool empty() const {return myBottom<=myTop;}
    int top() const {return myTop;}
    int bottom() const {return myBottom;}
    T& operator[]( int y ) {
        Assert(top()<=y);
        Assert(y<=bottom()+InclusiveBottom);
        return myArray[y+Outline::lineWidth];
    }
    const T& operator[]( int y ) const {
        return (*const_cast<RowVector*>(this))[y];
    }
    void resize( int top, int bottom ) {
        Assert( -RegionClipBoxLineWidth<=top );
        Assert( bottom<=MAX_STRIPE_HEIGHT+RegionClipBoxLineWidth );
        myTop=top;
        myBottom=bottom;
    }

    //! Trim off rows at top and bottom for which isEmptyRow is true.
    template<typename IsEmptyRow>
    void trim( IsEmptyRow isEmptyRow ) {
        while( !empty() && isEmptyRow(myTop) )
            ++myTop;
        while( !empty() && isEmptyRow(myBottom-1) )
            --myBottom;
    }

    //! Clear vector
    void clear() {myTop=0; myBottom=-1;}
private:
    int myTop;
    int myBottom;
    //! Storage is allocated.
    T myArray[Outline::lineWidth+MAX_STRIPE_HEIGHT+InclusiveBottom];
};

//! A region that is convex.
class ConvexRegion {
    RowVector<RegionSegment,false> myVec;
    bool myIsPositive;
public: 
    typedef RegionSegment::value_type value_type;
    bool empty() const {return myVec.empty();}
    value_type top() const {return myVec.top();}
    value_type bottom() const {return myVec.bottom();}
    const RegionSegment& operator[]( int y ) const {return myVec[y];}
    bool isPositive() const {return myIsPositive;}
    void setIsPositive( bool value ) {myIsPositive=value;}
    void makeRectangle( Point upperLeft, Point lowerRight );
    //! Make circular region with given center and radius
    void makeCircle( Point center, float radius );
    //! Make parallelogram with given center and two adjacent corners p and q
    void makeParallelogram( Point center, Point p, Point q );
    //! Make ellipse with given center, extremum point p, and width
    void makeEllipse( Point center, Point p, float halfWidth );
private:
    void trim();
};

struct SignedSegment;

//! A region that can have concavities or disconnects.
class CompoundRegion {
public:
    //! True iff region is empty.
    bool empty() const {return bottom()<=top();}
    int top() const {return myVec.top();}
    int bottom() const {return myVec.bottom();}

    //! True if region is empty for scan line y.
    bool empty( int y ) const {return myVec[y]==myVec[y+1];}
    int left( int y ) const {return begin(y)->left;}
    int right( int y ) const {return end(y)[-1].right;}

    //! Return pointer to first RegionSegment on scan line y.
    RegionSegment* begin( int y ) const {return myVec[y];}

    //! Return pointer to one past last RegionSegment on scan line y.
    RegionSegment* end( int y ) const {return myVec[y+1];}

    //! Build CompoundRegion as union of positive ConvexRegions minus negative ConvexRegions
    void build( const ConvexRegion* first, const ConvexRegion* last );

    //! Build CompoundRegion from complement of union of CompoundRegions.
    void buildComplement( const CompoundRegion* first, const CompoundRegion* last );

    //! Build CompoundRegion that is a rectangle
    /** Calls SetRegionClip before building the CompoundRegion. */
    void buildRectangle( Point upperLeft, Point lowerRight );

#if ASSERTIONS
    bool assertOkay() const;
    //! Zero except for diagrams with cells that have exterior colors
    int lineWidth;
#endif
private:
    RowVector<RegionSegment*,true> myVec;
    static void percolate( SignedSegment* s, SignedSegment* e );
    void trim();
};

#endif /* REGION_H */