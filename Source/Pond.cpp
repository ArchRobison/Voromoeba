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
#include "Pond.h"
#include "Region.h"
#include "Self.h"
#include "Voronoi.h"
#include "World.h"
#include "Sound.h"
#include <algorithm>

static bool ColorWobbleInitialized = false;

static ColorWobble PredatorColorWobble(0.25f);

void Pond::initializeColorAndSound(NimblePixMap& window, size_t first, size_t last, NimbleColor c0, NimbleColor c1, BeetleKind kind, OutlinedColor::exteriorColor exterior) {
    if (!ColorWobbleInitialized) {
        PredatorColorWobble.initialize(window, NimbleColor(0x80, 0, 0xF), NimbleColor(0xE0, 0x0, 0x40));
        ColorWobbleInitialized = true;
    }
    for (size_t k=first; k<last; ++k) {
        Beetle& b = (*this)[k];
        b.kind = kind;
        NimbleColor c2;
        c2 = c0;
        float f;
        if (last-first-1==0)
            f = 0;
        else
            f = float(k-first)/(last-first-1);
        c2.mix(c1, f);
        b.color = OutlinedColor(window.pixel(c2), exterior);
        if (kind==BeetleKind::water) {
            b.soundId = AllocateSlush(f);
        }
    }
}

void Pond::initialize(NimblePixMap& window, size_t n, PondOptions pondOptions, size_t numOrange) {
    myPondOptions = pondOptions;

    if (hasWhirlpool()) {
        myWhirlSpeed = rand()>=RAND_MAX/2 ? 1 : -1;
    } else {
        myWhirlSpeed = 0;
    }

    if (hasZoneOfDoom()) {
        myZoneOfDoom = Grating(Point(1.f, 0.f), 0.5f);
    }

    // Initialize position of each Beetle as position in a honeycomb pattern
    float m = std::sqrt(float(n));
    float base = 1.9*radius()/m;
    float alt = std::sqrt(3.0f)/2*base;
    reserve(2*n);
    size_t k = 0;
    for (int i=-m; i<m; ++i) {
        for (int j=-m; j<m; ++j) {
            Point p((i&1) ? j*base : (j+0.5f)*base, i*alt);
            if (!(myPondOptions&PO_Crystalline))
                p += Polar(base/4.f, RandomAngle());
            if (Dist2(p)>=radiusSquared())
                continue;
            Beetle& b = (*this)[k++];
            b.pos = center()+p;
            Assert(Distance(b.pos, center())<=radius()*1.001);
        }
    }
    resize(k);
    mySafeSize = k;

    // Randomly permute the beetles
    // FIXME - really just need to permute positions
    for (size_t i=size(); --i>0; ) {
        // Choose a beetle in the range [0..i)
        size_t j = rand() % i;
        Swap((*this)[i], (*this)[j]);
    }
    size_t sweetieEnd = 0;
    float predatorFrac=0, foodFrac=0;
    if (myPondOptions&PO_Zinger) {
        // Reserve cell for "sweetie" that is at least a pond's radius away from the pond's entrance.
        Point entrance = World::getEntrance(*this)->center();
        size_t i=size();
        while (--i>0)
            if (Dist2((*this)[i].pos, entrance)>=radiusSquared())
                break;
        Swap((*this)[0], (*this)[i]);
        sweetieEnd = 1;
        // Inject "zingers" as extra Beetles in addition to the crystalline water.
        const size_t numZinger = 3;
        Assert(size()>=sweetieEnd+numZinger);
        resize(k+numZinger);
        mySafeSize = k+numZinger;
        for (size_t j=0; j<numZinger; ++j) {
            Beetle& b = (*this)[k+j];
            b.pos = center()+Polar(0.25f*radius(), RandomAngle());
            Swap((*this)[sweetieEnd+j], (*this)[k+j]);
        }
        myPredatorCount = numZinger;
    } else {
        predatorFrac = 0.05f;
        foodFrac = 0.2f;
        myPredatorCount = predatorFrac*size();
    }
    size_t predatorEnd = sweetieEnd+myPredatorCount;
    size_t foodEnd = predatorEnd+foodFrac*size();
    size_t orangeEnd = foodEnd+numOrange;

    // Set sweetie color wobble parameters
    if (sweetieEnd) {
        static OutlinedColor::exteriorColor exSweetie = OutlinedColor::newExteriorColor(0xF0A000);
        Beetle& b = (*this)[0];
        b.kind = BeetleKind::sweetie;
        b.orbit = SelfType::colorWobble.orbit(1.0f, 4);
        b.color = OutlinedColor(SelfType::colorWobble(b.orbit), exSweetie);
        Assert(b.color.hasExterior());
    }
    // Set predator color wobble parameters.
    for (size_t k=sweetieEnd; k<predatorEnd; ++k) {
        Beetle& b = (*this)[k];
        b.orbit = PredatorColorWobble.orbit((predatorEnd-sweetieEnd)>1 ? float(k-sweetieEnd)/(predatorEnd-sweetieEnd-1) : 0.5f, k);
    }

    static OutlinedColor::exteriorColor exPredator = OutlinedColor::newExteriorColor(0xFF40E0);
    static OutlinedColor::exteriorColor exPlant = OutlinedColor::newExteriorColor(0x006000);
    static OutlinedColor::exteriorColor exOrange = OutlinedColor::newExteriorColor(0x603000);
#if WIZARD_ALLOWED && 0
    // For stress testing
    static OutlinedColor::exteriorColor exWater = OutlinedColor::newExteriorColor(0x000080);
#else
    static const OutlinedColor::exteriorColor exWater = 0;
#endif

    // Put predators into pond
    initializeColorAndSound(window, sweetieEnd, predatorEnd, NimbleColor(0x80, 0, 0), NimbleColor(0xC0, 0x00, 0x00), BeetleKind::predator, exPredator);
    // Put food into pond
    initializeColorAndSound(window, predatorEnd, foodEnd, NimbleColor(0, 0xC0, 30), NimbleColor(0, 0xFF, 0x40), BeetleKind::plant, exPlant);
    // Put oranges into pond
    initializeColorAndSound(window, foodEnd, orangeEnd, NimbleColor(0xFF, 0xB0, 0), NimbleColor(0xFF, 0xD0, 0), BeetleKind::orange, exOrange);
    // Put water into pond
    initializeColorAndSound(window, orangeEnd, size(), NimbleColor(0, 0, 0x40), NimbleColor(0, 0, 0xFF), BeetleKind::water, exWater);

    if (contains(Point(0, 0))) {
        // This is the initial pond where "self" will be placed at (0,0). 
        // Move any close food, oranges, or predators away from (0,0), so that points will not 
        // be immediately gained or lost when the game starts.
        std::sort(begin(), end(), [](Beetle& b, Beetle& c) {return Dist2(b.pos)<Dist2(c.pos); });
        const size_t n_close = 12;
        for (size_t k=0; k<n_close; ++k) {
            Beetle& b = (*this)[k];
            if (b.kind!=BeetleKind::water) {
                // Find water to swap it with
                size_t j;
                do {
                    j = rand() % (size()-n_close) + n_close;
                } while ((*this)[j].kind!=BeetleKind::water);
                Beetle& c = (*this)[j];
                // Swap( b, c );
                Swap(b.pos, c.pos);
                ++j;
            }
        }
    }

    // Initialize velocities
    for (size_t k=0; k<size(); ++k) {
        Beetle& b = (*this)[k];
        PondOptions isMoving = PondOptions(0);
        switch (b.kind) {
            case BeetleKind::predator: isMoving = PO_PredatorMoves; break;
            case BeetleKind::plant:    isMoving = PO_FoodMoves; break;
            case BeetleKind::water:    isMoving = PO_WaterMoves; break;
        }
        if (myPondOptions & isMoving) {
            float theta = RandomAngle();
            float speed;
            if ((myPondOptions & PO_Zinger) && b.kind==BeetleKind::predator)
                speed = 0.15f*radius()*(RandomFloat(3)+1);
            else if ((myPondOptions & PO_Crystalline) && b.kind==BeetleKind::water)
                speed = 0.01f*radius()*(RandomFloat(2)+1);
            else
                speed = 0.03f*radius()*(RandomFloat(2)+1);
            b.vel = Polar(speed, theta);
        } else {
            b.vel = Point(0, 0);
        }
    }
}

void Pond::whirl(Beetle& b, float dt) {
    Assert(fuzzyContains(b.pos));
    Assert(myWhirlSpeed!=0);
    Point local = b.pos-center();
    float r2 = Dist2(local);
    const float inner = 0.01f*radius();
    if (r2>inner*inner) {
        float r = std::sqrt(r2);
        float omega = (1/r-1/radius())*myWhirlSpeed;
        PreciseRotation rotate(omega*dt);
        b.pos = rotate(local)+center();
        b.vel = rotate(b.vel);
        Assert(Distance(b.pos, center())<=radius()*1.001);
        if (b.kind==BeetleKind::self)
            Self.whirlAngularPosition(-omega*dt);
    } else {
        // Put point on circumference
        float rho = RandomAngle();
        b.pos = center()+Polar(radius()*0.9, rho);
    }
    Assert(fuzzyContains(b.pos));
}

void Pond::kill(size_t i) {
    Assert(i<size());
    // FIXME - do something flashier than just turning it to white
    (*this)[i].color.setInterior(~0);
    --mySafeSize;
    exchange(i, mySafeSize);
}

static Point InterceptVec(Point a, Point u, Point b, float v) {
    float t = Distance(b, a)/v;
    for (int i=0; i<8; ++i)
        t = Distance(b, a+t*u)/v;
    return v*UnitVector((a+t*u)-b);
}

void Pond::update(float dt) {
    if (safeSize()<size()) {
        // FIXME - use timer or queue
        if (back().kind==BeetleKind::predator)
            --myPredatorCount;
        popBack();
    }
    for (size_t i=0; i<size(); ++i) {
        Beetle& b = (*this)[i];
        Assert(b.color.hasExterior() || !(b.kind==BeetleKind::predator||b.kind==BeetleKind::plant||b.kind==BeetleKind::orange));
        if (b.kind==BeetleKind::predator) {
            b.color.setInterior(PredatorColorWobble(b.orbit));
            Assert(b.color.hasExterior());
        } else if (b.kind==BeetleKind::sweetie) {
            b.color.setInterior(SelfType::colorWobble(b.orbit));
            Assert(b.color.hasExterior());
        } else if (b.kind==BeetleKind::water) {
            if (myPondOptions & PO_Crystalline)
                continue;
        }
        float s = dt;
        const float epsilon = 1e-6f;
        Assert(fuzzyContains(b.pos));
        // Count serves to prevent infinite loops
        for (int count=20; epsilon<s && epsilon<Dist2(b.vel) && count>0; --count) {
            float deltaS = intercept(b.pos, b.vel);
            if (hasZoneOfDoom()) {
                float deltaZ = myZoneOfDoom.intercept(b.pos, b.vel);
                Assert(deltaZ>=0);
                if (deltaZ<=deltaS && s>=deltaZ) {
                    b.pos += deltaZ*b.vel;
                    Assert(fuzzyContains(b.pos));
                    s -= deltaZ;
                    b.vel = myZoneOfDoom.reflect(b.pos, b.vel);
                    continue;
                }
            }
            Assert(deltaS==deltaS);
            if (s<deltaS) {
                b.pos += s*b.vel;
                break;
            } else {
                // Advance to boundary
                b.pos += deltaS*b.vel;
                s -= deltaS;
                if ((b.kind==BeetleKind::predator) && (myPondOptions & PO_Zinger) && Dist2(Self.pos, center())<radiusSquared()) {
                    Point v = InterceptVec(Self.pos, Self.vel, b.pos, Distance(b.vel));
                    if (Dot(v, b.pos-center())<0) {
                        b.vel = v;
                        continue;
                    }
                }
                // Reflect veclocity
                b.vel = reflect(b.pos, b.vel);
            }
        }
        Assert(fuzzyContains(b.pos));
        if (hasWhirlpool()) {
            whirl(b, dt);
        }
    }
}

Ant* Pond::assignDarkAnts(Ant* a, const ViewTransform& v) const {
    size_t k=0;
    for (const Beetle* b = begin(); b!=end(); ++b, ++a, ++k)
        if (k<safeSize())
            a->assign(v.transform(b->pos), 0);
        else
            a->assign(v.transform(b->pos), b->color);
    return a;
}