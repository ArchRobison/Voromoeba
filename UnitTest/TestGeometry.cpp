// Unit test for classes in Geometry.h

#include "Geometry.h"
#include <algorithm>

bool IsPowerOfTwo(int z) {
    if (z<=0) return false;
    return (z&z-1)==0;
}

const int K=8;
#define FOURTH_POWER(x) ((x)*(x)*(x)*(x))

// Each row is the elements of a 2x2 matrix with a determinant 
// is a power of 2.  The determinant of such a matrix is exactly representable 
// as a floating-point value, which lets exact equality be used in some of the tests.
float Perfect[FOURTH_POWER(2*K+1)][4];
size_t Count;

void FindPerfectPoints() {
    Count = 0;
    for (short a=-K; a<=K; ++a)
        for (short b=-K; b<=K; ++b)
            for (short c=-K; c<=K; ++c)
                for (short d=-K; d<=K; ++d)
                    if (IsPowerOfTwo(a*d-b*c)) {
                        float* z = Perfect[Count++];
                        z[0] = a;
                        z[1] = b;
                        z[2] = c;
                        z[3] = d;
                    }
    Assert(Count==6920);
}

// Return a random point
Point RandPoint() {
    short x = rand() % 63-31;
    short y = rand() % 63-31;
    return Point(x, y);
}

// Return random point that is not at the origin
Point NonzeroRandPoint() {
    Point p;
    do {
        p = RandPoint();
    } while (p.x==0 && p.y==0);
    return p;
}

//! Test routine CenterOfCircle and CenterOfCircleY
void TestCenterOfCircle() {
    for (int trial=0; trial < 10000; ) {
        // Choose point not on origin
        const Point o = NonzeroRandPoint();

        // Construct points a and b that are the same distance from o.
        const float r = Distance(o);
        const Point a = o+Polar(r, RandomAngle());
        const Point b = o+Polar(r, RandomAngle());

        const float epsilon = 2E-3f;
        if (std::fabs(Cross(a, b)) >= epsilon) {
            // Get center of circle though a, b, (0,0). With exact calculations, it would match o.
            Point d = CenterOfCircle(a, b);
            float error = Dist2(d, o);
            Assert(error<=epsilon);

            Point c = RandPoint();
            float y = CenterOfCircleY(a+c, b+c, c);
            error = Square(y-(o+c).y);
            // Error for one dimension should be no worse than half error for two dimensions.
            Assert(error<=0.5f*epsilon);
            ++trial;
        }
    }
}

//! Test routine InCircle 
void TestInCircle() {
    for (int trial=0; trial<10000; ) {
        // Pick random center of circle
        Point c = NonzeroRandPoint();
        float r = std::sqrt(Dist2(c));
        Point a = c+Polar(r, RandomAngle());
        Point b = c+Polar(r, RandomAngle());
        float alpha = Cross(a, b);
        if (std::fabs(alpha)>0) {
            ++trial;
            if (alpha<0) std::swap(a, b);
            Point d = c+NonzeroRandPoint();
            float s = std::sqrt(Dist2(d, c));
            float diff = s-r;
            bool inside = InCircle(a, d, b);
            if (diff<0) {
                Assert(inside);
            } else if (diff>0) {
                Assert(!inside);
            } else {
                // Too close to determine
            }
        }
    }
}

// Test function PseudoAngle
void TestPseudoAngle() {
    const double pi = 3.1415926535f;
    for (int x=-10; x<=10; ++x) {
        for (int y=-10; y<=10; ++y) {
            if (x!=0 || y!=0) {
                float alpha = PseudoAngle(float(x), float(y));
                Assert(0.0f<=alpha && alpha<8.0f);
                // Compute phi as "circular" equivalent of squarish "alpha"
                float phi = float(atan2(double(y), double(x))/(pi/4));
                if (phi<-1.0f)
                    phi += 9;
                else
                    phi += 1;
                Assert(0.0f<=phi && phi<8.0f);
                // Now check that phi and alpha are reasonbly close
                Assert(int(phi)==int(alpha));
                float diff = phi-alpha;
                Assert(-0.0905<=diff);
                Assert(diff<0.0905f);
            }
        }
    }
}

void TestLinearAndAffine() {
    for (size_t i=0; i<1000; ++i) {
        float* z = Perfect[rand()%Count];
        float a = z[0];
        float b = z[1];
        float c = z[2];
        float d = z[3];
        LinearTransform m(a, b, c, d);
        Assert(m.det()!=0);
        LinearTransform n(m.inverse());
        Point p = RandPoint();
        Point q = n(m(p));
        Assert(Dist2(p, q)==0.f);
        Point o = RandPoint();
        AffineTransform t(m, o);
        Assert(Dist2(t(p), m(p)+o)==0.f);
    }
}

void TestBisectorIntercept() {
    for (size_t trial=0; trial<1000; ++trial) {
        const Point p = RandPoint();
        const Point q = RandPoint();
        if (p.y!=q.y) {
            Point r = RandPoint();
            r.y = BisectorInterceptY(r.x, p, q);
            float diff = fabs(Dist2(p, r)-Dist2(q, r));
            Assert(diff<=0.016f);
        }
        if (p.x!=q.x) {
            Point r = RandPoint();
            r.x = BisectorInterceptX(r.y, p, q);
            float diff = fabs(Dist2(p, r)-Dist2(q, r));
            Assert(diff<=0.016f);
        }
    }
}

//! Find s that minimizes distance between p+s*v and c
float Closest(Point p, Point v, Point c) {
    LinearTransform m(v.x, v.y, v.y, -v.x);
    return m.inverse()(c-p).x;
}

//! Check that point q is on the boundary of circle c, allowing for some round-off error.
void AssertIsOnCircleBoundary(const Circle& c, const Point& q) {
    // Compute distrance to circle's boundary
    float diff = Dist2(q, c.center())-c.radiusSquared();
    Assert(fabs(diff)<=0.003f);
}

void TestCircle() {
    for (int trial=0; trial<10000; ++trial) {
        // Construct random center of a circle
        Point c(RandPoint());

        // Construct a random point not on the circle's center.
        Point p(c+NonzeroRandPoint());

        // Construct random vector representing ray from point p
        Point v(NonzeroRandPoint());

        // Test with circle such that p is inside the circle
        {
            float r = Distance(c, p)*1.25f;
            // Construct a circle with p inside
            Circle big(c, r);
            // Compute intercept coefficient s
            float s = big.intercept(p, v);
            // Check that it is in forward direction
            Assert(s>0);
            // Compute intercept point q
            Point q = p+s*v;
            // Check that intercept is on boundary of circle
            AssertIsOnCircleBoundary(big, q);
            Point w = big.reflect(q, v);
            // Magnitudes of w and v should be the same.
            float error = fabs(Dist2(w)-Dist2(v));
            Assert(fabs(error)<=0.005f);
            // v+w should be perpendicular to q-c.  
            error = Dot(v+w, q-c);
            Assert(fabs(error)<=0.002f);
            // Construct point a outside the circle
            Point a = q+RandomFloat(1.0)*v;
            Assert(big.overlapsSegment(a, p));
            Assert(big.overlapsSegment(p, a));
        }

        // Test with circle such that p is outside the circle
        {
            float r = Distance(c, p)*0.75f;
            Circle small(c, r);
            float t = Closest(p, v, c);
            float s = small.intercept(p, v, false);
            if (IsInfinity(s)) {
                // No intercept
                Assert(t<0 || !small.contains(p+t*v));
                // Construct point a outside the circle
                Point a = p+v;
                Assert(!small.overlapsSegment(a, p));
                Assert(!small.overlapsSegment(p, a));
            } else {
                Assert(s>=0);
                if (s==0) {
                    // There is an intercept, but it is negative
                    Assert(t<0);
                    Point a = p+v;
                    Assert(!small.overlapsSegment(a, p));
                    Assert(!small.overlapsSegment(p, a));
                } else {
                    // Check that "first" intercept was returned.
                    Assert(s<=t);
                    AssertIsOnCircleBoundary(small, p+s*v);
                    Point a = p+(1+s)*v;
                    Assert(small.overlapsSegment(a, p));
                    Assert(small.overlapsSegment(p, a));
                }
            }
        }
    }
}

void TestEllipse() {
    for (int trial=0; trial<10000; ++trial) {
        // Construct random center of ellipse
        Point c(RandPoint());

        // Choose random radii (r is really a vector)
        Point r(float(1+rand()%10), float(1+rand()%10));

        // Choose random unit vector
        Point u = Polar(1.0, RandomFloat(2*3.1415926535f));

        // Generate transform from unit circle to the ellipse
        AffineTransform f(LinearTransform(u.x*r.x, -u.y*r.y,
            u.y*r.x, u.x*r.y), c);

        // Construct the ellipse
        Ellipse e(c, f(Point(1, 0)), r.y);

        // Choose random point that is sometimes inside and sometimes outside of unit circle
        Point p_(RandomFloat(2)-1, RandomFloat(2)-1);

        // Find corresponding point in ellipse space
        Point p(f(p_));

        // Check method "contains"
        float d_ = Dist2(p_);
        Assert(e.contains(p)==d_<=1.0f);

        // Check method "intercept"
        Circle e_(Point(0, 0), 1);
        Point v_ = NonzeroRandPoint();
        Point v = f.linear()(v_);
        bool b = e_.contains(p_);
        float s_ = e_.intercept(p_, v_, b);
        float s = e.intercept(p, v, b);
        if (IsInfinity(s_)) {
            Assert(IsInfinity(s));
        } else {
            Assert(!IsInfinity(s));
            Assert(fabs(s-s_)<=0.0001f);
        }
    }
}

void TestParallelogram() {
    // Loop over trials
    for (int t=0; t<1000; ++t) {
        float* z = Perfect[rand()%Count];
        Point b = RandPoint();
        Point a = b+Point(z[1], z[3]);
        Point c = b+Point(z[0], z[2]);
        Parallelogram p(a, b, c);
        Point imageB = p.mySquare(b);
        Point imageA = p.mySquare(a);
        Point imageC = p.mySquare(c);
        Assert(Dist2(imageB, Point(0, 0))==0.f);
        Assert(Dist2(imageA, Point(0, 1))==0.f);
        Assert(Dist2(imageC, Point(1, 0))==0.f);
        for (float i=-2; i<=2; i+=0.125f)
            for (float j=-2; j<=2; j+=.125f) {
                Point q = b+j*(a-b)+i*(c-b);
                bool isInside = 0<=i && i<=1 && 0<=j && j<=1;
                Assert(p.contains(q)==isInside);
                bool isStrictInside = 0<i && i<1 && 0<j && j<1;
                if (isStrictInside) {
                    Point v = NonzeroRandPoint();
                    float s = p.intercept(q, v);
                    Assert(s>=0);
                    Point r = q+s*v;
                    float epsilon = 0.001f/std::sqrt(Dist2(v));
                    Assert(p.contains(r-epsilon*v));
                    Assert(!p.contains(r+epsilon*v));
                    // Generate point e outside the parallelogram
                    Point e = q+2*s*v;
                    Point f = q;
                    bool overlaps = p.clipSegment(e, f);
                    Assert(overlaps);
                    Assert(Dist2(f, q)==0);
                    Assert(Dist2(e, r)<=epsilon);
                    // Generate points e and f outside the parallelogram 
                    e = q+2*s*v;
                    f = q+3*s*v;
                    overlaps = p.clipSegment(e, f);
                    Assert(!overlaps);
                    // Generate points e and f on opposite sides
                    e = q+2*s*v;
                    f = q;
                    while (p.contains(f)) {
                        f -= s*v;
                    }
                    overlaps = p.clipSegment(e, f);
                    Assert(overlaps);
                    Assert(Dist2(e, r)<=epsilon);
                    Point g = q+p.intercept(q, -v)*-v;
                    Assert(Dist2(g, f)<=epsilon);
                }
            }
    }
}

void TestGrating() {
    for (int trial=0; trial<10000; ++trial) {
        Point lambda = NonzeroRandPoint();
        float h = 0.25f + RandomFloat(0.75f);
        Grating grating(lambda, h);
        Point v = NonzeroRandPoint();
        Point p = RandPoint();
        // Find point on grate
        float epsilon = 0.001f;
        float d = (1-epsilon)*Min(h, 1-h);
        int n = int((1-h)/d+2.0f);
        int k = 0;
        while (!grating.contains(p+(d*k)*lambda)) {
            ++k;
            Assert(k<n);
        }
        if (k) {
            // p is off of grating.  Check that method intercept does not return backwards value. 
            float s = grating.intercept(p, v);
            Assert(s>=0.0f);
        }
        // Move p to be on grating.  
        p += (d*k)*lambda;
        float s = grating.intercept(p, v);
        Assert(s>=-5E-8);
        if (IsInfinity(s)) {
            // Should only happen if ray and lambda are perpendicular
            // Exact comparison against zero is safe here, because v and lambda have small integer coordinates.
            Assert(Dot(v, lambda)==0);
        } else {
            Point q = p+s*v;
            Point w = grating.reflect(q, v);
            // Magnitudes of w and v should be the same
            float error = fabs(Dist2(w)-Dist2(v));
            Assert(error <= 0.0010f);
            // v+w should be perpendicular to lambda.  
            error = Dot(v+w, lambda);
            Assert(fabs(error)<=0.0005f);
            Assert(fabs(error)<=0.0005f);
        }
    }
}

void TestGeometry() {
    srand(2);
    FindPerfectPoints();
    TestLinearAndAffine();
    TestBisectorIntercept();
    TestCenterOfCircle();
    TestInCircle();
    TestPseudoAngle();
    TestGrating();
    TestCircle();
    TestEllipse();
    TestParallelogram();
}