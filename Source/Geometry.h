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

#ifndef Geometry_H
#define Geometry_H

 //! \file Geometry.h
 //! Functions related to geometry.

#include "Utility.h"
#include "AssertLib.h"
#include <cmath>
#include <limits>

 //! Return square of distance of (x,y) from origin.
inline float Dist2(float x, float y) {
    return Square(x)+Square(y);
}

//! Return square of distance between (x0,y0) and (x1,y1)
inline float Dist2(float x0, float y0, float x1, float y1) {
    return Dist2(x0-x1, y0-y1);
}

//! A 2D point, vector, or complex number.  Meaning depends on context.
class Point {
public:
    float x, y;
    Point() {}
    Point(float x_, float y_) : x(x_), y(y_) {}
    void reflectAbout(const Point& c) {
        x = 2*c.x-x;
        y = 2*c.y-y;
    }
    void operator+=(const Point& b) {
        x += b.x;
        y += b.y;
    }
    void operator-=(const Point& b) {
        x -= b.x;
        y -= b.y;
    }
};

//! Vector addition of two points.
inline Point operator+(const Point& a, const Point& b) {
    return Point(a.x+b.x, a.y+b.y);
}

//! Vector subtraction of two points.
inline Point operator-(const Point& a, const Point& b) {
    return Point(a.x-b.x, a.y-b.y);
}

//! Mirror point about origin.
inline Point operator-(const Point& a) {
    return Point(-a.x, -a.y);
}

//! Scalar times vector 
inline Point operator*(float s, const Point& a) {
    return Point(a.x*s, a.y*s);
}

//! Vector times multiplicative inverse of scalar
inline Point operator/(const Point& a, float s) {
    return (1/s)*a;
}

//! Dot product of two points
inline float Dot(const Point& a, const Point& b) {
    return a.x*b.x + a.y*b.y;
}

//! Cross product of two points
inline float Cross(const Point& a, const Point& b) {
    return a.x*b.y - a.y*b.x;
}

//! Complex multiplication of two points
inline Point Multiply(const Point& a, const Point& b) {
    return Point(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

//! Point with polar coordinates f, theta
inline Point Polar(float r, float theta) {
    return Point(r*std::cos(theta), r*std::sin(theta));
}

//! Square of distance between point a and origin.
inline float Dist2(const Point& a) {
    return Dist2(a.x, a.y);
}

//! Square of distance between points a and b.
inline float Dist2(const Point& a, const Point& b) {
    return Dist2(a.x, a.y, b.x, b.y);
}

//! Distance between a and origin.
inline float Distance(const Point& a) {
    return std::sqrt(Dist2(a));
}

//! Distance between points a and b
inline float Distance(const Point& a, const Point& b) {
    return std::sqrt(Dist2(a, b));
}

//! Unit vector in same direction as vector a.
inline Point UnitVector(const Point& a) {
    return a/Distance(a);
}

//! Return x such that (x,y) lies on perpendicular bisector of segment (lx,ly)--(rx,ry) 
inline float BisectorInterceptX(float y, Point l, Point r) {
    return 0.5f*((l.x+r.x) - ((2.0f*y-(l.y+r.y))*(l.y-r.y))/(l.x-r.x));
}

//! Return y such that (x,y) lies on perpendicular bisector of segment (lx,ly)--(rx,ry) 
inline float BisectorInterceptY(float x, Point l, Point r) {
    return 0.5f*((l.y+r.y) - ((2*x-(l.x+r.x))*(l.x-r.x))/(l.y-r.y));
}

//! Return center of circle passing through points (0,0), a, and b.
inline Point CenterOfCircle(Point a, Point b) {
    const float d = Cross(a, b);
    Assert(d!=0);
    const float ar = Dist2(a);
    const float br = Dist2(b);
    const float e = 0.5f/d;
    return Point((ar*b.y-br*a.y)*e, (br*a.x-ar*b.x)*e);
}

//! Return Y coordinate of center of circle through points a, b, and c.
inline float CenterOfCircleY(Point a, Point b, Point c) {
    a -= b;
    c -= b;
    const float d = Cross(a, c);
    return (Dist2(c)*a.x-Dist2(a)*c.x)*0.5f/d + b.y;
}

//! \brief Return if point d is inside a circle passing through points (0,0), a, and b.
//!
//! The latter three points must be in counter-clockwise order. 
inline bool InCircle(Point a, Point d, Point b) {
    Assert(Cross(a, b)>0);
    const float m00 = a.x, m01 = a.y, m02 = a.x*a.x+a.y*a.y;
    const float m10 = b.x, m11 = b.y, m12 = b.x*b.x+b.y*b.y;
    const float m20 = d.x, m21 = d.y, m22 = d.x*d.x+d.y*d.y;
    const float det = m00*(m11*m22-m21*m12) + m10*(m21*m02-m01*m22) + m20*(m01*m12-m11*m02);
    return det<=0;
}

//! \brief Return pseudo-angle of point(x,y) with respect to orgin.
//!
//! A full circle has 8 pseudo-radians.
//! The corners of a square correspond to 0, 2, 4, 6 in counterclockwise order, starting with the lower right corner.
//! The midpoints of the sides correspond to 1, 3, 5, 7 in counterlockwise order, starting with the right side.
//! PseudoAngles have the same total order as regular angles, but are faster to compute. 
inline float PseudoAngle(float x, float y) {
    Assert(Dist2(x, y)>0);
    if (x>=y) {
        // p is on or below main diagonal
        if (x>=-y)
            // p is above counter diagonal
            return 1+y/x;
        else {
            // p is below counter diagonal
            Assert(y<=0);
            return 7-x/y;
        }
    } else {
        // p is above main diagonal
        if (x>=-y)
            // p is above both diagonals
            return 3-x/y;
        else
            // p is below counter diagonal
            return 5+y/x;
    }
}

//! Representation of angle that always returns value reduced to [-pi,pi)
class ReducedAngle {
    //! Fixed-point representation of the angle.
    int32_t a;
    // Value equal to quarter angle
    static constexpr float scale() { return float(1.5707963268 / (1<<(sizeof(int32_t)*8-2))); }
public:
    // Add value in radians
    void operator+=(float delta) {
        // Assumes that int32_t arithmetic wraps around
        a += int32_t(delta*(1/scale()));
    }
    // Return value in radians
    operator float() {
        return a*scale();
    }
};

//! \brief Transform that rotates a vector with high precision
//!
//! Objective is to minimize change to the vector's length, which is useful
//! in scenarios where the rotation is applied many times to the same vector. 
class PreciseRotation {
    double x, y;
public:
    PreciseRotation(double theta) : x(std::cos(theta)), y(std::sin(theta)) {}
    Point operator()(Point p) const {
        return Point(float(x*p.x-y*p.y), float(x*p.y+y*p.x));
    }
};

//! Linear transform in a plane
class LinearTransform {
    float a, b, c, d;
public:
    // Construct undefined transform
    LinearTransform() {}
    // Construct linear transform  represented by matrix a b 
    //                                                   c d
    LinearTransform(float a_, float b_, float c_, float d_) : a(a_), b(b_), c(c_), d(d_) {}
    // Determinant
    float det() const {
        return a*d-b*c;
    }
    //! Inverse transform
    LinearTransform inverse() const {
        const float dinv = 1.0f/det();
        return LinearTransform(dinv*d, -dinv*b, -dinv*c, dinv*a);
    }
    //! Apply transform
    Point operator()(Point p) const {
        return Point(a*p.x+b*p.y, c*p.x+d*p.y);
    }
    //! Apply inverse transform
    Point inverse(Point p) const {
        return inverse()(p);
    }
};

//! Affine transform in a plane
class AffineTransform {
    LinearTransform myLinear;
    Point myOffset;
public:
    //! Construct undefined affine transform
    AffineTransform() = default;
    //! Construct transform x=>ax+b
    AffineTransform(const LinearTransform& a, const Point& b) : myLinear(a), myOffset(b) {}
    //! Construct affine transform that maps a=>(0,1), b=>(0,0), c=>(1,0)
    AffineTransform(Point a, Point b, Point c);
    //! Apply transform to a point
    Point operator()(Point p) const {
        return myLinear(p)+myOffset;
    }
    //! Return linear portion of transform
    const LinearTransform& linear() const { return myLinear; }
    //! Apply inverse transform to a point
    Point inverse(Point p) const {
        return myLinear.inverse(p-myOffset);
    }
};

inline AffineTransform::AffineTransform(Point a, Point b, Point c) {
    a-=b;
    c-=b;
    LinearTransform m(c.x, a.x, c.y, a.y);
    // Failure of following assertion indicates the points are not counterclockwise order, or are colinear.
    Assert(m.det()>0);
    myLinear = m.inverse();
    myOffset = myLinear(-b);
}

//! A circle in the Cartesian plane
class Circle {
    //! Center point
    Point myCenter;
    //! Radius
    float myRadius;
    //! Square of radius
    float myRadiusSquared;
public:
    //! Construct undefined circle.
    Circle() {}
    //! Construct circle with given center and radius
    Circle(Point center, float r) : myCenter(center), myRadius(r), myRadiusSquared(r* r) {}
    const Point& center() const { return myCenter; }
    float radius() const { return myRadius; }
    float radiusSquared() const { return myRadiusSquared; }
    //! Area of the circle
    float area() const { return Pi<float>*myRadiusSquared; }
    //! True if circle contains point p
    bool contains(Point p) const {
        return Dist2(p, myCenter)<=myRadiusSquared;
    }
    //! True if interior (or boundary) of a circle and a line segment p-q overlap.
    bool overlapsSegment(Point p, Point q) const;
#if ASSERTIONS
    //! True if circle contains or almost contains point p.
    bool fuzzyContains(Point p) const;
#endif /* ASSERTIONS */
    /** Given point p and vector v, return s such that p+sv intersects the circle.
        When there is not a unique intersection, the flag "fromInside" controls the result,

        If fromInside==true, point p is assumed to be inside the circle, and
        the maximum possible s is returned, or zero if there is no intersection.
        The "no intersection" case can happen if numerical roundoff has caused p
        to be slightly the circle and v points away from the circle.  The value
        of s is never negative if fromInside is true.

        If fromInside==false, point p is assumed to be ouside the circle, and
        the minimum possible s is returned, or infinity if there is no intersection.
        If p is slightly inside the circle because of numerical roundoff, then s is
        zero if v points into the circle and infinity if v points out of the circle. */
    float intercept(Point p, Point v, bool fromInside=true) const;

    //! Given a point p on the perimeter and a direction vector v, return the reflected v.
    Point reflect(Point p, Point v) const;

    //! Return projection of p on perimeter of the circle
    Point projectOntoPerimeter(Point p) const {
        return myCenter + radius()*UnitVector(p-myCenter);
    }
};

inline bool Circle::overlapsSegment(Point p, Point q) const {
    if (contains(p) || contains(q))
        return true;
    else {
        const float a = Dot(myCenter-p, q-p);
        if (a<=0)
            return false;
        else {
            const float b = Dist2(p, q);
            return a<=b && contains(p + (a/b)*(q-p));
        }
    }
}

#if ASSERTIONS
inline bool Circle::fuzzyContains(Point p) const {
    return Dist2(p, myCenter)<=radiusSquared()*(1+32*FLT_EPSILON);
}
#endif /* ASSERTIONS */

inline float Circle::intercept(Point p, Point v, bool fromInside) const {
    Assert(Dist2(v)>0);
    Point u = p-myCenter;
    // Compute coefficients of quadratic equation.  "b" is 1/2 the usual coefficient.
    float a = Dist2(v);
    float b = Dot(u, v);
    float c = Dist2(u)-radiusSquared();
    // Compute discriminant.   
    float disc = b*b-a*c;
    if (disc>=0) {
        // There is an intersection
        float d = std::sqrt(disc);
        if (fromInside) {
            // Want the maximum root of the quadratic equation
        } else {
            // Want the minimum root of the quadratric equation
            d=-d;
        }
        if (d>=b) {
            // Have intersection in forwards direction
            return (d - b)/a;
        } else {
            // Intersections are in backwards direction
            return fromInside || b<0 ? 0 : std::numeric_limits<float>::infinity();
        }
    } else {
        // There is no intersection.
        return fromInside ? 0 : std::numeric_limits<float>::infinity();
    }
}

inline Point Circle::reflect(Point p, Point v) const {
    // Compute unit direction vector in direction from center to (x,y)
    Point u = (p-center())/radius();
    // Flip v component and multiply by square of u.
    v.x = -v.x;
    return Multiply(v, Multiply(u, u));
}

//! A parallelogram in the Cartesian plane
class Parallelogram {
    // Given 0<=q<=1, find positive s such that q+s*w==0 or 1
    static float unitSolve(float q, float w) {
        Assert(0<=q && q<=1);
        return w>0 ? (1-q)/w : w<0 ? -q/w : std::numeric_limits<float>::infinity();
    }
    static void unitSquareIntercept(float a, float b, float& s, float& t);
    // Linear transform that maps parallelogram onto unit square.
    AffineTransform mySquare;
    friend void TestParallelogram();
public:
    // Construct undefined paralleogram
    Parallelogram() {}
    // Construct parallelogram with given three consecutive corners (in counterclockwise order)
    Parallelogram(Point a, Point b, Point c) : mySquare(a, b, c) {}
    //! Return true if parallogram contains p
    bool contains(Point p) const;
    //! Given point p inside the parallelogram and vector v, return non-negative s such that p+sv intersects the parallelogram.
    float intercept(Point p, Point v) const;
    /** If line segment p-q overlaps this parallelogram, clip p-q to portion that fits and return true
        Otherwise leave p-q unaltered and return false. */
    bool clipSegment(Point& p, Point& q) const;
};

inline bool Parallelogram::contains(Point p) const {
    Point q = mySquare(p);
    return 0<=q.x && q.x<=1 && 0<=q.y && q.y<=1;
}

inline float Parallelogram::intercept(Point p, Point v) const {
    Assert(Dist2(v)>0);
    // Transform problem to coordinates where parallelogram is a square
    Point q = mySquare(p);
    Point w = mySquare.linear()(v);
    Assert(contains(p));
    // Now find s such that q+s*w intercepts the square
    return Min(unitSolve(q.x, w.x), unitSolve(q.y, w.y));
}

//! An ellipse in the Cartesian plane
class Ellipse {
    //! Affine transform that maps parallelogram onto unit circle.
    AffineTransform myCircle;
    friend void TestEllipse();
public:
    //! Construct undefined paralleogram
    Ellipse() {}
    //! Construct ellipse with given center, extremum point p, and given width perpendicular to p-center.
    Ellipse(Point center, Point p, float width);
    // Return true if ellipse contains p
    bool contains(Point p) const;
    //! See description of Circle::intercept
    float intercept(Point p, Point v, bool fromInside) const;
    //! True if ellipse completely covers segment p-q
    bool coversSegment(Point p, Point q) const {
        return contains(p) && contains(q);
    }
    //! Return projection of p on perimeter of ellipse
    Point projectOntoPerimeter(Point p) const;
};

inline Ellipse::Ellipse(Point center, Point p, float width) {
    Assert(width>0);
    Point d = width*UnitVector(p-center);
    myCircle = AffineTransform(center+Point(-d.y, d.x), center, p);
}

inline bool Ellipse::contains(Point p) const {
    return Dist2(myCircle(p))<=1.0f;
}

inline float Ellipse::intercept(Point p, Point v, bool fromInside) const {
    Assert(Dist2(v)>0);
    // Transform problem to coordinates where ellipse is a unit circle
    Point q = myCircle(p);
    Point w = myCircle.linear()(v);
    Circle unit(Point(0.0f, 0.0f), 1.f);
    return unit.intercept(q, w, fromInside);
}

inline Point Ellipse::projectOntoPerimeter(Point p) const {
    // Transform problem to coordinates where ellipse is a unit circle
    Point q = myCircle(p);
    return myCircle.inverse(UnitVector(q));
}

//! An infinite repetition of evenly spaced stripes in the Cartesian plane
class Grating {
    Point myOmega;
    float myOffset;
    // Half height of each bar in unit space.  Always between 0 and 0.5.
    float myHeight;
public:
    //! Construct unitialized grating
    Grating() {}
    //! Construct grating with wavevector lambda and duty-cycle h.
    /** The lines of the grate are perpendicular to the grate with spacing |lambda|. */
    Grating(Point lambda, float h);
    //! Return true if grate contains point p
    bool contains(Point p) const;
    //! Given a point p inside the grate, find s such that p+v*s.
    /** Returns infinity if v is parallel to the grating
        If point p is outside grate, returns 0 if v points away from nearest slat, otherwise
        the far side of nearest slat.  This intent is to deal with cases where p is slightly
        off the grating because of numerical roundoff. */
    float intercept(Point p, Point v);
    // Given a point p on a boundary of the grate, and a direction vector v, return the reflected v.
    Point reflect(Point p, Point v) const;
};

inline Grating::Grating(Point lambda, float h) : myHeight(h), myOffset(0) {
    myOmega = lambda/Dist2(lambda);
}

inline bool Grating::contains(Point p) const {
    float q = Dot(p, myOmega)+myOffset;
    q -= floor(q);
    return q<=myHeight;
}

inline float Grating::intercept(Point p, Point v) {
    float q = Dot(p, myOmega)+myOffset;
    // Adjustment by e deals with points that are slightly outside the grate.
    float e = (1-myHeight)*0.5f;
    q += e;
    q -= floor(q);
    q -= e;
    float w = Dot(v, myOmega);
    float t = w < 0 ? -q : myHeight-q;
    return Max(t/w, float(0));
}

inline Point Grating::reflect(Point /*p*/, Point v) const {
    // Compute unit direction vector normal to boundary
    Point u = UnitVector(myOmega);
    // Flip v component and multiply by square of u.
    v.x = -v.x;
    Point w = Multiply(v, Multiply(u, u));
    return w;
}

//! A geometric transform combining scaling, rotation and translation.
class ViewTransform {
    Point srot;         //!< Scale and rotation (as a complex factor)
    Point offset;       //!< Offset applied after scale and rotation
    float myScale;      //!< Scale.  Should be same as Distance(srot).
public:
    //! Create identity transform
    ViewTransform() : srot(1, 0), offset(0, 0), myScale(1) {}
    //! Change angle of rotation
    void setRotation(float theta) {
        srot = Polar(myScale, theta);
    }
    //! Change scale and angle of rotation
    void setScaleAndRotation(float scale, float theta) {
        myScale = scale;
        srot = Polar(scale, theta);
    }
    //! Change scale and rotation according equivalent to complex multiplication by p
    void setScaleAndRotation(Point p) {
        srot = p;
        myScale = Distance(p);
    }
    //! Set offset
    void setOffset(Point p) {
        offset = p;
    }
    //! Return transform of input point (x,y)
    /* t10 is implicitly -t10
       t11 is implicitly t00 */
    template<typename T>
    T transform(const T& in) const {
        return T(in.x*srot.x - in.y*srot.y + offset.x,
            in.y*srot.x + in.x*srot.y + offset.y);
    }
    //! Do only rotation/scaling part of transform.
    /** This is useful for transforming relative vectors. */
    template<typename T>
    T rotate(const T& in) const {
        return T(in.x*srot.x - in.y*srot.y,
            in.y*srot.x + in.x*srot.y);
    }
    //! Do only scaling part of transform.
    float scale(float z) const {
        return myScale*z;
    }
};

#if ASSERTIONS
//! If true, ignore assertions that might fail because of rare roundoff errors.
constexpr bool TolerateRoundoffErrors = true;
#endif

#endif /* Geometry_H */