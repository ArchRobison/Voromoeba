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

#include "Region.h"
#include "Utility.h"
#include <cmath>
#include <algorithm>

struct BoundingBox {
    int left, right, top, bottom;
    BoundingBox(float l, float t, float r, float b) {
        left = Round(l);
        top = Round(t);
        right = Round(r);
        bottom = Round(b);
    }
    BoundingBox() {}
    /** If *this and b overlap, set *this to intersection and return true.
        Otherwise return false and leave *this in undefined state. */
    bool clip(const BoundingBox& b);
};

bool BoundingBox::clip(const BoundingBox& b) {
    if (top<b.top)
        top = b.top;
    if (bottom>b.bottom)
        bottom = b.bottom;
    if (top<bottom) {
        if (right>b.right)
            right = b.right;
        if (left<b.left)
            left = b.left;
        if (left<right)
            return true;
    }
    return false;
}

static BoundingBox RegionClipBox;
#if ASSERTIONS
int RegionClipBoxLineWidth;
#endif
static RegionSegment SegmentStorage[MAX_CONVEX_REGION*MAX_STRIPE_HEIGHT];
static RegionSegment* FreeSegmentPtr;

void SetRegionClip(float left, float top, float right, float bottom, int lineWidth) {
    FreeSegmentPtr = SegmentStorage;
    RegionClipBox = BoundingBox(left-lineWidth, top-lineWidth, right+lineWidth, bottom+lineWidth);
#if ASSERTIONS
    RegionClipBoxLineWidth = lineWidth;
#endif
}

void ConvexRegion::trim() {
    myVec.trim([this](int y) {return myVec[y].empty(); });
}

void ConvexRegion::makeRectangle(Point upperLeft, Point lowerRight) {
    myIsPositive = true;
    BoundingBox b(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
    myVec.resize(b.top, b.bottom);
    for (int y=b.top; y<b.bottom; ++y)
        myVec[y].assign(b.left, b.right);
}

void ConvexRegion::makeCircle(Point center, float radius) {
    myIsPositive = true;
    BoundingBox b(center.x-radius, center.y-radius, center.x+radius, center.y+radius);
    if (b.clip(RegionClipBox)) {
        myVec.resize(b.top, b.bottom);
        float r2 = radius*radius;
        for (int y=b.top; y<b.bottom; ++y) {
            float h2 = r2 - Square(y-center.y);
            float h = h2>0 ? std::sqrt(h2) : 0;
            myVec[y].assign(Max(RegionClipBox.left, Round(center.x-h)),
                Min(RegionClipBox.right, Round(center.x+h)));
        }
        trim();
    } else {
        myVec.clear();
    }
}

void ConvexRegion::makeEllipse(Point center, Point p, float halfWidth) {
    myIsPositive = true;
    // a = length of major radius
    float a = std::sqrt(Dist2(p, center));
    // b = length of minor radius
    float b = halfWidth;
    // u = unit vector of major radius
    Point u = (p-center)/a;
    float A = Dist2(u.x/a, u.y/b);
    float B = 2*u.x*u.y*(1/(a*a)-1/(b*b));
    float C = Dist2(u.y/a, u.x/b);
    float dy = std::sqrt(4*A/(4*A*C-B*B));
    float dx = std::sqrt(4*C/(4*A*C-B*B));
    BoundingBox box(center.x-dx, center.y-dy, center.x+dx, center.y+dy);
    if (box.clip(RegionClipBox)) {
        myVec.resize(box.top, box.bottom);
        float d0 = 1/A;
        float d1 = -B/(2*A);
        float d2 = Square(B/(2*A))-C/A;
        for (int y=box.top; y<box.bottom; ++y) {
            float h2 = d2*Square(y-center.y)+d0;
            float h = h2>0 ? std::sqrt(h2) : 0;
            float g = d1*(y-center.y);
            myVec[y].assign(Max(RegionClipBox.left, Round(center.x+g-h)),
                Min(RegionClipBox.right, Round(center.x+g+h)));
        }
        trim();
    } else {
        myVec.clear();
    }
}

void ConvexRegion::makeParallelogram(Point c, Point p, Point q) {
    myIsPositive = true;
    // Normalize arguments so that p is a topmost point and q is a leftmost point, 
    // where convention is that y axis points downwards.
    if (p.y>c.y)
        p.reflectAbout(c);  // Reflect p above c
    if (q.y>c.y)
        q.reflectAbout(c);  // Reflect q above c
    if (q.y<p.y)
        std::swap(p, q);          // Make p an uppermost point
    if (q.x>c.x)
        q.reflectAbout(c);  // Make q a leftmost point
    // Compute parameters from normalized p and q
    // FIXME - investigate whether a zero divisor (and consequent infinite slope) cause problems.
    BoundingBox b;
    b.top = Round(p.y);
    b.left = Round(q.x);
    Point leftIntercept = q;
    float inverseSlope[2];
    inverseSlope[0] = (p.x-q.x)/(p.y-q.y);
    p.reflectAbout(c);
    inverseSlope[1] = (p.x-q.x)/(p.y-q.y);
    q.reflectAbout(c);
    Point rightIntercept = q;
    b.right = Round(q.x);
    b.bottom = Round(p.y);
    if (b.clip(RegionClipBox)) {
        myVec.resize(b.top, b.bottom);
        for (int y=b.top; y<b.bottom; ++y) {
            float u = y-leftIntercept.y;
            float v = y-rightIntercept.y;
            myVec[y].assign(Max(b.left, Round(leftIntercept.x+inverseSlope[u>=0]*u)),
                Min(b.right, Round(rightIntercept.x+inverseSlope[v<=0]*v)));
        }
        trim();
    } else {
        myVec.clear();
    }
}

struct SignedSegment : RegionSegment {
    bool isPositive;
    void assign(const RegionSegment& s, bool isPositive_) {
        left = s.left;
        right = s.right;
        isPositive = isPositive_;
    }
};

void CompoundRegion::percolate(SignedSegment* s, SignedSegment* e) {
    for (; s+1<e && s[1].left<s[0].left; ++s)
        std::swap(s[0], s[1]);
}

void CompoundRegion::trim() {
    myVec.trim([this](int y) {return begin(y)==end(y); });
}

static const size_t MaxSegmentPerScanLine = 4000;

#if ASSERTIONS
#define EXPENSIVE_CHECK 1
#endif

#if EXPENSIVE_CHECK
class BuildChecker {
    SignedSegment buffer[MaxSegmentPerScanLine];
    SignedSegment* end;
public:
    BuildChecker(const SignedSegment* first, const SignedSegment* last) { init(first, last); }
    void init(const SignedSegment* first, const SignedSegment* last) {
        end = std::copy(first, last, buffer);
    }
    void assertOkay(const RegionSegment& b) const;
};

void BuildChecker::assertOkay(const RegionSegment& b) const {
    for (const SignedSegment* s=buffer; s!=end; ++s)
        if (!s->isPositive) {
            const RegionSegment& a = *s;
            // Check that b does not overlap segment a.
            Assert(a.left<=a.right);
            Assert(b.left<=b.right);
            if (a.left<=b.left)
                Assert(a.right<=b.left);
            else
                Assert(b.right<=a.left);
        }
}
#define DECLARE_CHECK(tmp,e) BuildChecker checker(tmp,e)
#define LOAD_CHECK(tmp,e) checker.init(tmp,e)
#define CHECK checker.assertOkay(out[-1])
#else
#define DECLARE_CHECK(tmp,e) ((void)0)
#define CHECK ((void)0)
#define LOAD_CHECK(tmp,e) ((void)0)
#endif /* Checker */

void CompoundRegion::build(const ConvexRegion* first, const ConvexRegion* last) {
#if ASSERTIONS
    lineWidth = RegionClipBoxLineWidth;
#endif
    Assert(last-first<=MaxSegmentPerScanLine);

    // Find extrema
    int32_t top = std::numeric_limits<int32_t>::max();
    int32_t bottom = std::numeric_limits<int32_t>::min();
    for (const ConvexRegion* r = first; r!=last; ++r) {
        if (r->isPositive()) {
            if (r->top() < top)
                top = r->top();
            if (r->bottom() > bottom)
                bottom = r->bottom();
        }
    }

    const bool CHECK_FOR_CREATION_OF_EMPTY_SEGMENTS = true;

    myVec.resize(top, bottom);
    RegionSegment* out = FreeSegmentPtr;
    // For each scan line
    for (int y=top; y<bottom; ++y) {
        SignedSegment tmp[MaxSegmentPerScanLine];
        SignedSegment* e = tmp;
        // Collect non-empty segments from each convex region
        // Ideally, all segments would be non-empty, because the region is convex, but because 
        // of roundoff error, occasionally empty segments do occur because of one-pixel concavities.
        for (const ConvexRegion* r = first; r!=last; ++r)
            if (r->top()<=y && y<r->bottom()) {
                Assert(e<tmp+MaxSegmentPerScanLine);
                const RegionSegment& s = (*r)[y];
                if (!CHECK_FOR_CREATION_OF_EMPTY_SEGMENTS || !s.empty())
                    (e++)->assign(s, r->isPositive());
            }
        DECLARE_CHECK(tmp, e);
        myVec[y] = out;
        if (tmp<e) {
            SignedSegment* s = tmp;
            if (s+1<e) {
                Assert(e-tmp < MaxSegmentPerScanLine);
                std::sort(tmp, e);
                LOAD_CHECK(tmp, e);
                do {
                    Assert(!CHECK_FOR_CREATION_OF_EMPTY_SEGMENTS || !s[0].empty());
                    Assert(s[0].left<=s[0].right);
                    Assert(s[0].left<=s[1].left);
                    if (s[0].right<=s[1].left) {
                        // Segments are disjoint.  
                        if (s[0].isPositive) {
                            *out++ = *s;
                            CHECK;
                        }
                    } else if (s[0].right<=s[1].right) {
                        // s0 and s1 partially overlap but not in a donut way
                        if (s[0].isPositive==s[1].isPositive) {
                            s[1].left = s[0].left;                              // Extend s1 to cover s0
                        } else if (s[0].isPositive) {
                            // s1 erases right portion of s0
                            if (s[0].left<s[1].left) {
                                (out++)->assign(s[0].left, s[1].left);           // Emit left portion of s0
                                CHECK;
                            }
                        } else {
                            if (s[0].right<s[1].right) {
                                s[1].left=s[0].right;                           // s0 clips s1
                                Assert(!s[1].empty());
                                percolate(s+1, e);
                                goto retry;
                            } else {
                                s[1] = s[0];                                    // s0 erases s1
                            }
                        }
                    } else {
                        // s0 completely covers s1
                        if (!s[0].isPositive || s[1].isPositive) {
                            s[1] = s[0];                                        // s0 subsumes s1
                        } else {
                            if (s[0].left<s[1].left) {
                                // s1 splits s0 into two segments.  
                                (out++)->assign(s[0].left, s[1].left);        // Output left segment.
                                CHECK;
                            }
                            // s1 erases left portion of s0
                            s[0].left = s[1].right;                             // Set s0 to right segment
                            Assert(!s[0].empty());
                            percolate(s, e);
                            goto retry;
                        }
                    }
                    ++s;
retry:;
                } while (s+1<e);
            }
            // Deal with last segment
            if (s->isPositive) {
                *out++ = s[0];
                CHECK;
            }
#if ASSERTIONS
            for (const RegionSegment* t = myVec[y]; t<out; ++t)
                Assert(!t->empty());
#endif
        }
    }
    myVec[bottom] = out;
    trim();
    Assert(out <= &SegmentStorage[MAX_CONVEX_REGION*MAX_STRIPE_HEIGHT]);
    FreeSegmentPtr = out;
}

void CompoundRegion::buildComplement(const CompoundRegion* first, const CompoundRegion* last) {
#if ASSERTIONS
    lineWidth = RegionClipBoxLineWidth;
#endif
    // Find extrema
    int top = INT_MAX;
    int bottom = INT_MIN;
    for (const CompoundRegion* r = first; r!=last; ++r) {
        if (r->top() < top)
            top = r->top();
        if (r->bottom() > bottom)
            bottom = r->bottom();
    }

    myVec.resize(RegionClipBox.top, RegionClipBox.bottom);
    RegionSegment all;
    all.assign(RegionClipBox.left, RegionClipBox.right);
    RegionSegment* out = FreeSegmentPtr;
    if (top>=bottom)
        top=bottom=0;
    for (int y=this->top(); y<top; ++y) {
        myVec[y] = out;
        *out++ = all;
    }
    for (int y=top; y<bottom; ++y) {
        myVec[y] = out;
        RegionSegment tmp[MaxSegmentPerScanLine];
        RegionSegment* e = tmp;
        // Collect segments from each compound region
        for (const CompoundRegion* r = first; r!=last; ++r)
            if (r->top()<=y && y<r->bottom()) {
                Assert(e+(r->end(y)-r->begin(y)) < tmp+MaxSegmentPerScanLine);
                e = std::copy(r->begin(y), r->end(y), e);
            }
        if (tmp+1<e) {
            Assert(e<tmp+MaxSegmentPerScanLine);
            std::sort(tmp, e);
        }
        int l = RegionClipBox.left;
        for (RegionSegment* s = tmp; s<e; ++s) {
            if (l<s->left)
                (out++)->assign(l, s->left);
            l = s->right;
        }
        if (l<RegionClipBox.right)
            (out++)->assign(l, RegionClipBox.right);
    }
    for (int y=bottom; y<this->bottom(); ++y) {
        myVec[y] = out;
        *out++ = all;
    }
    myVec[this->bottom()] = out;
    Assert(out <= &SegmentStorage[MAX_CONVEX_REGION*MAX_STRIPE_HEIGHT]);
    FreeSegmentPtr = out;
}

void CompoundRegion::buildRectangle(Point upperLeft, Point lowerRight) {
    SetRegionClip(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
    ConvexRegion r;
    r.makeRectangle(upperLeft, lowerRight);
    build(&r, &r+1);
}

#if ASSERTIONS
#include "Outline.h"

bool CompoundRegion::assertOkay() const {
    Assert(top()>=-lineWidth);
    for (int y=top(); y<bottom(); ++y) {
        if (!empty(y)) {
            float l = left(y);
            Assert(-lineWidth<=l);
        }
    }
    return true;
}
#endif /*ASSERTIONS*/
