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

#include <algorithm>
#include <functional>
#include <cmath>
#include "Config.h"
#include "AssertLib.h"
#include "Region.h"
#include "Voronoi.h"
#include "Outline.h"

class WalkByY {
    Ant* myL;
    Ant* myU;
    static float subtract( float a, float b ) {
        Assert( a>=b );
        return a-b;
    }
#if ASSERTIONS
    Ant* myBegin;
    Ant* myEnd;
#endif /* ASSERTIONS */
public:
    WalkByY() {}
    const Ant& WalkByY::startWalk( float y, Ant* first, Ant* last );

    /** Get next Ant that is above or below lineY if it is within distance d of lineY.
        Otherwise return NULL. */
    const Ant* getNextAboveOrBelowIf( float lineY, float d );

    /** Get next Ant in downwards direction if it is within distance d of lineY.  
        Otherwise return NULL. */
    const Ant* getNextBelowIf( float lineY, float d ) {
        Assert( myU->y>=lineY );
        return myU->y-lineY<=d ? myU++ : NULL;
    }

#if ASSERTIONS
    size_t remaining() const {
        Assert( myL>=myBegin );
        Assert( myU<=myEnd-1 );
        return (myL-myBegin)+(myEnd-1-myU);
    }
#endif /* ASSERTIONS */
};

inline const Ant& WalkByY::startWalk( float y, Ant* first, Ant* last ) {
    // Next 5 assertions are caller's responsibility
    Assert(first[0].y==-AntInfinity);   
    Assert(last[-1].y==AntInfinity);    
    Assert(first[1].y>-AntInfinity);
    Assert(last[-2].y<AntInfinity);
    Assert( -AntInfinity<y && y<AntInfinity );
#if ASSERTIONS
    myBegin = first;
    myEnd = last;
#endif /* ASSERTIONS */
    Ant tmp;
    tmp.y = y;
    Ant* u = std::upper_bound( first, last, tmp, Ant::lessY() ); 
    Ant* l = u-1;
    Assert( first<l || u<last );
    float dl = subtract(y,l->y); 
    float du = subtract(u->y,y); 
    Ant* a = dl<du ? l : u;
    myL=a-1;
    myU=a+1;
    Assert(myL>=myBegin);
    Assert(myU<myEnd);
    return *a;
}

inline const Ant* WalkByY::getNextAboveOrBelowIf( float y, float d ) {
    Assert(myL>=myBegin);
    Assert(myU<myEnd);
    Assert(0<=d);
    Assert(d<AntInfinity);
    // Choose remaining ant that is closest to y.
    float dl = subtract(y,myL->y);
    float du = fabs(myU->y-y);
    if( dl<du )
        return dl<d ? myL-- : NULL;
    else
        return du<d ? myU++ : NULL;
}

struct DeferredAnt: Ant {
    float top;
    struct greaterTop {
        bool operator()( const DeferredAnt& a, const DeferredAnt& b ) const {return a.top>b.top;}
    };
};

//! Base class is coordinates of the ant.
struct VoronoiSegment: public Ant {
    // Next segment in list of live segments
    VoronoiSegment* next;
    // Previous segment in list of live segments
    VoronoiSegment* prev;
    // Left end of segment
    float left;
    // Change in left per scan line.
    float slope;
    Outline::idType outlineId;
#if ASSERTIONS
    float yWhenSlopeWasSet;
    float originalLeft;
#endif /* ASSERTIONS */
    void assign( const Ant& a ) {
        static_cast<Ant&>(*this) = a;
        // Skip initialization of left. It is computed later.
        if( a.color.hasExterior() )
            outlineId = Outline::newId();
        else
            outlineId = 0;
    }
};


#define STATISTICS 0

#if STATISTICS
struct Stats {
    unsigned long mergeCount;
    unsigned long liveTotal;
    unsigned long frontierTotal;
    unsigned long emptyFrontierCount;
} TheStats;
#define STAT(fieldExpr) (TheStats.fieldExpr)
#else
#define STAT(expr) ((void)0)
#endif

class VoronoiRasterizer {
    //! Root of list of free segments
    VoronoiSegment* freeList;
    //! Next free segment
    VoronoiSegment* freePtr;
    //! Pointer to array of segments
    VoronoiSegment* liveArray;

    //! Dummy segment on left.  
    VoronoiSegment leftDummy;
    //! Dummy segment on right
    VoronoiSegment rightDummy;

    Ant* const frontierFirst;
    Ant* frontierLast;
    DeferredAnt* const heapFirst;
    DeferredAnt* heapLast;
    float minX, maxX, minY, maxY;
    float lineY;

    //! Insert segment between i and k.
    /** Does not require that i and k be linked. */
    VoronoiSegment* insert( VoronoiSegment* i, VoronoiSegment* k ) {
        // Grab a segment off the freelist if possible, otherwise get from buffer.
        VoronoiSegment* j = freeList;
        if( j ) {
            freeList = j->next;
        } else {
            j = freePtr++;
        }
        i->next = j;
        k->prev = j;
        j->prev = i;
        j->next = k;
        return j;
    }

    //! Erase segment between i and k.
    void erase( VoronoiSegment* i, VoronoiSegment* k ) {
        Assert( i->next->next==k );
        Assert( i==k->prev->prev );
        VoronoiSegment* j = i->next;
        i->next = k;
        k->prev = i;
        j->next = freeList;
        freeList = j;
    }

    //! Replace segments in linked open interval (i,k) with a a fresh segment, and return a pointer to the latter.
    VoronoiSegment* replaceSegmentsBetweenWithNewSegment( VoronoiSegment* i, VoronoiSegment* k ) {
        if( i->next!=k ) {
            // Move replaced segments to freelist
            k->prev->next = freeList;
            freeList = i->next;
        }
        return insert(i,k);
     }

    //! Put segment on heap if it might become visible later
    // FIXME - use array of linked lists instead of a heap
    void defer( const Ant& a, float top ) {
        // Following assertion is written in ! form so that it tolerates case where top is a NaN.
        Assert( !(top<lineY) );
        if( top<=maxY ) {
            // FIXME - ignore segment if it is beyond bottom of screen
            DeferredAnt& d = *heapLast++;
            static_cast<Ant&>(d) = a; 
            d.top = top;
            std::push_heap( heapFirst, heapLast, DeferredAnt::greaterTop() );
        }
    }

#if ASSERTIONS
    static bool assertOrderedByX( const VoronoiSegment* first, const VoronoiSegment* last ) {
        Assert( first<=last );
        for( ; first+2<last; ++first ) {
            Assert( first[0].x<=first[1].x );
        }
        return true;
    }

    bool assertLeftOfFrontier( float x, const Ant& k ) const {
        Assert( frontierFirst<=frontierLast );
        return frontierFirst==frontierLast || x<=frontierFirst[0].x;
    }

    bool assertLiveIsOkay() const;

 #endif /* ASSERTIONS */

    // Return true iff cell for j does not intersect current scan line
    // Puts j on heap if it will intersect future scan line
    bool processTriplet( const Point& i, const Ant& j, const Point& k );

    // Set left and slope of r to be boundary between cells for l and r
   void setBoundary( const Point& l, VoronoiSegment& r ) const;
 
    void forceBoundaryOrder( VoronoiSegment& j ) const;

public:
    struct bufferType {
        DeferredAnt heap[N_ANT_MAX+2];
        Ant frontier[N_ANT_MAX+2];
        VoronoiSegment segment[N_ANT_MAX+2];
    };

    VoronoiRasterizer( bufferType& buffer );

    void setBoundingBox( const CompoundRegion& region );

    float top() const {return minY;}
    float bottom() const {return maxY;}

    void setLine( float y ) {
        lineY = y;
    }

    //! Move all segments s with s.top<=top into the frontier.
    void popHeapToFrontier( float top ) {
        while( heapFirst<heapLast && heapFirst->top<=top ) {
            std::pop_heap( heapFirst, heapLast, DeferredAnt::greaterTop() );
            *frontierLast++ = *--heapLast;
        }
    }

    //! Append segment to the frontier
    void appendToFrontier( const Ant& a ) {
        *frontierLast++ = a;
    }

    //! True if frontier is empty
    bool frontierIsEmpty() const {
        return frontierFirst==frontierLast;
    }

    //! True if live list is empty
    bool liveIsEmpty() const {
        return leftDummy.next == &rightDummy;
    };

    void mergeIntoEmptyLive( const Ant& a );

    //! Merge frontier into live
    void mergeFrontierIntoLive();

    // Compute maximum distance for a segment in the live list, and set n to length of live list (not including dummies)
    float computeLiveMaxDist( size_t& n ) const;

    // Draw live segments and advance their "left" to the next line
    void drawLive( NimblePixMap& window, const CompoundRegion& region );

    void advanceLive();
};

#if ASSERTIONS
bool VoronoiRasterizer::assertLiveIsOkay() const {
    const VoronoiSegment& i = *leftDummy.next;
    const VoronoiSegment& j = *i.next;
    // Leftmost real segment should have artificial boundary
    Assert( i.left==minX );
    Assert( i.slope==0 );

    // Right dummy segment should have artificial boundary
    const VoronoiSegment& l = rightDummy;
    Assert( l.left==maxX );
    // Check that boundaries are correct
    for( VoronoiSegment* t = leftDummy.next; t; t=t->next ) {
        if( t==leftDummy.next ) {
            Assert( t->left==minX );
        } else if( t==&rightDummy ) {
            Assert( t->left==maxX );
        } else {
            VoronoiSegment* s = t->prev;
            float x = BisectorInterceptX(lineY,*s,*t);
            float error = t->left - x;
            // The error is typically much smaller.  
            // However very long sides can accumulate 1/2 ulp error per scan line.
            // For example, for a 2000x2000 pixel display, the accumulated error could be 1000 ulp.
            // Furthermore, the monotonicty hacks can further bloat this.
            /* FIXME - use fixed-point for "left" and "slope".  Use about 14 bits for integer part and 18 for fraction
               That's better than current situation, which has only about 14 bits for the fraction. */
            Assert( fabs(error)<=.6f || TolerateRoundoffErrors );
        }
    }
    // Check that boundaries occur left to right.
    VoronoiSegment* t;
    for( VoronoiSegment* s = leftDummy.next; t=s->next; s=t ) {
        Assert( s->left <= t->left );
    }
    return true;
}
#endif /* ASSERTIONS */

VoronoiRasterizer::VoronoiRasterizer( bufferType& buffer ) :
    frontierFirst(buffer.frontier),
    frontierLast(buffer.frontier),
    freePtr( buffer.segment ),
    freeList( NULL ),
    heapFirst(buffer.heap),
    heapLast(buffer.heap)
{
    // Create two-element list of dummy segments
    VoronoiSegment& l = leftDummy;
    VoronoiSegment& r = rightDummy;

    l.x = -FLT_MAX;
    l.y = 0;
    l.color = 0;
    l.left = -FLT_MAX;
    l.slope = 0;
    l.next = &r;
    l.prev = NULL;

    r.x = FLT_MAX;
    r.y = 0;
    r.color = 0;
    r.left = FLT_MAX;
    r.slope = 0;
    r.next = NULL;
    r.prev = &l;
}

void VoronoiRasterizer::setBoundingBox( const CompoundRegion& region ) {
    // FIXME - CompoundRegion should be returning a top() that is non-empty
    minY = FLT_MAX;
    maxY = -FLT_MAX;
    minX = FLT_MAX;
    maxX = -FLT_MAX;
    for( int y=region.top(); y<region.bottom(); ++y ) {
        if( !region.empty(y) ) {
            if( y<minY ) minY = y;
            if( y>maxY ) maxY = y;
            float l = region.left(y);
            Assert( -region.lineWidth<=l );
            if( l<minX ) minX = l;
            float r = region.right(y);
            if( r>maxX ) maxX = r;
        }
    }
}

float VoronoiRasterizer::computeLiveMaxDist( size_t& n ) const {
    VoronoiSegment* i = leftDummy.next;
    VoronoiSegment* k = rightDummy.prev;
    Assert( i!=&rightDummy );
    Assert( k!=&leftDummy );
    // Special rules apply to leftmost and rightmost real segments.
    float maxD2 = Max( Dist2( i->x, i->y, minX, lineY ),
                       Dist2( k->x, k->y, maxX, lineY ) );
    float count = 1;
    while( i!=k ) {
        i = i->next;
        float d2 = Dist2( i->x, i->y, i->left, lineY );
        if( d2>maxD2 )
            maxD2 = d2;
        ++count;
    }
    n = count;
    return std::sqrt(maxD2);
}

bool VoronoiRasterizer::processTriplet( const Point& i, const Ant& j, const Point& k ) {
    Assert( i.x<=j.x );
    Assert( j.x<=k.x );
    float o;
    if( i.x==-FLT_MAX ) {
        Assert( k.x!=FLT_MAX );
        if( j.y==k.y ) {
            // Bisector is vertical line
            return j.x+k.x <= 2*minX;              
        } else {
            o = BisectorInterceptY(minX,j,k);
            if( o < lineY ) 
                return j.y < k.y;
            else if( j.y < k.y ) 
                return false;
        }
    } else if( k.x==FLT_MAX ) {
        if( j.y==i.y ) {
            // Bisector is vertical line
            return i.x+j.x >= 2*maxX;
        } else {
            o = BisectorInterceptY(maxX,i,j);
            if( o < lineY ) 
                return j.y < i.y;
            else if( j.y < i.y ) 
                return false;
        }
    } else {
        o = CenterOfCircleY( i, j, k );
        if( o < lineY ) 
            // If j.y <= o <= lineY, cell is convex-up and above current scan line and should be squashed.
            return j.y < o; 
        else if( j.y < o ) 
            return false;
        // lineY < o < j.y.  Cell for j is convex-down and below current scan line.  Defer j
    }
    defer(j,o);
    return true;
}

void VoronoiRasterizer::setBoundary( const Point& l, VoronoiSegment& r ) const {
    Assert( l.x<r.x );
    if( l.x==-FLT_MAX ) {
        r.left = minX;
        r.slope = 0;
    } else if( r.x==FLT_MAX ) {
        r.left = maxX;
        r.slope = 0;
    } else {
        // Compute slope with respect to y-axis of perpendicular bisector of l--r
        float slope = (l.y-r.y)/(r.x-l.x);
        r.slope = slope;
        // Compute intersection with current y
        r.left = 0.5f*((l.x+r.x) + slope*(2.0f*lineY-(l.y+r.y)));
        // Check for culling errors
        Assert( -RegionSegment::value_type_max < r.left );
        Assert( r.left < RegionSegment::value_type_max );
    }
#if ASSERTIONS
    r.yWhenSlopeWasSet = lineY;
    r.originalLeft = r.left;
#endif /* ASSERTIONS */
}

void VoronoiRasterizer::forceBoundaryOrder( VoronoiSegment& j ) const {
	float array[4] = {j.prev->left,j.left,j.next->left,1.0E6};
	if( j.next->next ) {
		array[3] = j.next->next->left;
	}

    VoronoiSegment& k = *j.next;
    if( j.left>k.left ) {
        Assert( j.left-k.left <= 0.125f );
        // Segment j is backwards.  
        // Use "left" of neighbor with smallest slope, since it's likely to be the most accurate.
        j.left = k.left = fabs(j.slope)<fabs(k.slope) ? j.left : k.left;
    }
    VoronoiSegment& i = *j.prev;
    if( i.left>j.left ) {
        // Segment i is backwards.  
        Assert( i.left-j.left <= 0.25f || TolerateRoundoffErrors );
        j.left = i.left;
		if( j.left>k.left ) {
			Assert( j.left-k.left <= 0.0001f );
			k.left = j.left;
		}
    }
    if( VoronoiSegment* l = k.next ) {
        if( k.left>l->left ) {
            // Segment k is backwards. 
            Assert( k.left-l->left <= 0.60f || TolerateRoundoffErrors );
            k.left = l->left;
			if( j.left>k.left ) {
				Assert( j.left-k.left < 0.0001f );
				j.left = k.left;
			}
        }
    }
	Assert( i.left<=j.left );
	Assert( j.left<=k.left );
}

void VoronoiRasterizer::mergeIntoEmptyLive( const Ant& j ) {
    Assert( liveIsEmpty() );
    VoronoiSegment* s = insert(&leftDummy,&rightDummy);
    s->assign(j);
    setBoundary(leftDummy,*s);
    setBoundary(*s,rightDummy);
    Assert( assertLiveIsOkay() );
}

void VoronoiRasterizer::mergeFrontierIntoLive() { 
    STAT(mergeCount++);
    STAT(liveTotal+=liveLast-liveFirst);
    STAT(frontierTotal+=frontierLast-frontierFirst);
    STAT(emptyFrontierCount+=(frontierLast==frontierFirst));
    std::sort( frontierFirst, frontierLast, Ant::lessX() );
    Assert( assertLiveIsOkay() );
    VoronoiSegment* i = &leftDummy;
    for( Ant* j = frontierFirst; j!=frontierLast; ++j ) {
        VoronoiSegment* k;
        // Advance i (and compute k) so that i->x <= j->x <= k->x
        while( j->x >= (k=i->next)->x ) 
            i=k;
        Assert( i->x <= j->x );
        Assert( j->x < k->x );
        if( !processTriplet(*i,*j,*k) ) {
            // j should be inserted.  
            // See what it squashes/defers to its left
            if( i->x == j->x )  {
                // Perpendicular bisector of i--j is horizontal.  Since j is being inserted, i must be removed.
                // FIXME - if two points are identical, have deterministic rule to resolve the fight
                if(i->y>j->y) {
                    defer(*i,(i->y+j->y)*0.5f);
                }
                i=i->prev;
            }
            while( i!=&leftDummy && processTriplet(*i->prev,*i,*j) )
                i=i->prev;
            // See what it squashes/defers to its right
            while( k!=&rightDummy && processTriplet(*j,*k,*k->next) )
                k=k->next;
            Assert( i->x < j->x );
            Assert( j->x < k->x );
            VoronoiSegment* s = replaceSegmentsBetweenWithNewSegment(i,k);
            s->assign(*j);
            setBoundary( *i, *s );
            setBoundary( *s, *k );
            forceBoundaryOrder( *s );
            Assert( assertLiveIsOkay() );
            i = s;
        }
    }
    frontierLast = frontierFirst;
}

void VoronoiRasterizer::drawLive( NimblePixMap& window, const CompoundRegion& region ) {
    Assert( assertLiveIsOkay() );
    Assert( -region.lineWidth<=minX );
    Assert( maxX<=window.width()+region.lineWidth );
    Assert( -region.lineWidth<=minY );
    Assert( minY<=window.height()+region.lineWidth );

    // FIXME - do not do infill.  That needs to be done after all Voronoi regions are drawn.
    // FIXME - split out advancement to next line to separate routine
    // FIXME compute maxDist2 after update
    RegionSegment* s = region.begin(lineY);
    RegionSegment* e = region.end(lineY);
    Assert( s<e ); // Caller should reject empty scan lines
    VoronoiSegment* j = leftDummy.next;
    for(;;) {
        Assert( s->left < s->right );
        Assert( -RegionSegment::value_type_max <= j->left );
        Assert( j->left <= RegionSegment::value_type_max );
        RegionSegment::value_type l=j->left;
        RegionSegment::value_type r=j->next->left;
        int u = Max(s->left,l);
        auto c = j->color;
        int v = Min(s->right,r);
        if( j->outlineId ) {
            if( u<v ) 
               Outline::addSegment(j->outlineId,u,v,lineY,c);
        } else if( unsigned(lineY)<unsigned(window.height()) ) {
            if( u<0 ) 
                // FIXME - assert that we're dealing with outlined diagram
                u = 0;
            if( v>=window.width() )
                // FIXME - assert that we're dealing with outlined diagram
                v = window.width();
            if( u<v ) {
                NimblePixel* out = (NimblePixel*)window.at(u,lineY);                   
                do {
                    *out++ = c.interior();
                } while( ++u<v );
            }
        }
        if( j->next->left>=s->right ) {
            // Current Voronoi segment reached end of current RegionSegment
            if( ++s==e )
                break;
        } else {
            // Current VoronoiSegment did not reach end of current RegionSegment
            j = j->next;
            if( j==&rightDummy ) 
                break;
        }
    }
}

void VoronoiRasterizer::advanceLive() {
    Assert( assertLiveIsOkay() );
    Assert(leftDummy.next->slope==0);
    lineY += 1;
    for(VoronoiSegment* j = leftDummy.next; VoronoiSegment* k = j->next; j=k) {
        k->left += k->slope;
        while( j->left >= k->left ) {
            // j is squashed
            VoronoiSegment* i = j->prev;
            erase(i,k);
            setBoundary(*i,*k);
            j = i;
        }
        Assert( j->next==k );
    }
    Assert( assertLiveIsOkay() );
}

void DrawVoronoi( NimblePixMap& window, const CompoundRegion& region, Ant* antFirst, Ant* antLast ) {
    Assert( antFirst+3<=antLast ); // Must have at least two bookends and one ant
    Assert( antLast-antFirst<=N_ANT_MAX+2 );
    Assert( antFirst[0].y==-AntInfinity );
    Assert( antLast[-1].y==AntInfinity );
    Assert( region.bottom()<=window.height()+region.lineWidth );
    Assert( region.assertOkay() );
#if ASSERTIONS
    for( Ant* a = antFirst+1; a<antLast-1; ++a ) {
        Assert( a->y>-AntInfinity );
        Assert( a->y<AntInfinity );
    }
#endif
	std::sort( antFirst+1, antLast-1, Ant::lessY() );

    size_t nAnt = (antLast-antFirst)-2;

    static VoronoiRasterizer::bufferType buffer;
    VoronoiRasterizer v(buffer);
    v.setBoundingBox(region);
    Outline::start();

    // Start with Ant closest to scan line
    WalkByY yOrder;

	// For each scan line
	for( int lineY=v.top(); lineY<=v.bottom(); ++lineY ) {

        // FIXME - move this line before loop, because "advanceLive" makes it redundant here
        v.setLine( lineY );
        if( !region.empty(lineY) ) {

            if( v.liveIsEmpty() ) {
                // Starting from scratch.  
                v.mergeIntoEmptyLive( yOrder.startWalk( lineY, antFirst, antLast ) );
            }
            v.popHeapToFrontier( lineY );
            bool endOfIncoming = false;
            for(;;) {
                // Following step is required even if frontier is empty, so that "left" field is computed for each segment.
                v.mergeFrontierIntoLive();
                if( endOfIncoming ) break;
                size_t n;
                float d = v.computeLiveMaxDist(n);
                for( ; n>0; --n ) {
                    const Ant* a = yOrder.getNextAboveOrBelowIf(lineY,d);
                    if( !a ) {
                        endOfIncoming = true;
                        break;
                    }
                    v.appendToFrontier(*a);
                }
                if( v.frontierIsEmpty() ) 
                    break;
            }
            v.drawLive( window, region ); 
        }
        v.advanceLive();
    }
    Outline::finishAndDraw( window );
#if STATISTICS
    Stats& s = TheStats;
#endif /* STATISTICS */
}

