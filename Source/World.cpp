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

#include "Background.h"
#include "Bridge.h"
#include "Config.h"
#include "Finale.h"
#include "Pond.h"
#include "Missile.h"
#include "Region.h"
#include "Self.h"
#include "Utility.h"
#include "Sound.h"
#include "World.h"
#include "Neighborhood.h"
#include "Outline.h"
#include "Voronoi.h"
#include "VoronoiText.h"
#include "Splash.h"
#include <climits>

namespace {

float TheOriginalViewScale;
float ZoomFactor = 1;

const size_t N_POND_MAX = 10;

static Pond PondSet[N_POND_MAX];
static Point PondCenter[N_POND_MAX];
static Bridge BridgeSet[N_POND_MAX];
static Background PondBackground;

} // (anonymous)

ViewTransform World::viewTransform;

Bridge* World::getEntrance(const Pond& p) {
    size_t k = &p-PondSet;
    Assert(0<k && k<N_POND_MAX);
    return &BridgeSet[k-1];
}

bool World::isInDarkPond(const Beetle& b) {
    return b.isInPond && PondSet[b.pondIndex].isDark();
}

VoronoiMeter TheScoreMeter(4);

namespace {
constexpr int FOOD_PER_MISSILE = 8;
constexpr int POINTS_PER_FOOD = 1;
constexpr int POINTS_PER_KISS = 1000;
static int AccumulatedFood;

uint32_t NumPond;
constexpr float NeighborSeparation = 1.05f;
constexpr float OtherSeparation = 1.5f;

void PlaySoundRelativeToSelf(SoundKind k, Point other) {
    PlaySound(k, UnitVector(World::viewTransform.rotate(other-Self.pos)));
}

//! Open door from pond with index k to next pond if prerequisites are satisfied.
void OpenOrCloseBridgeIfReady(size_t k) {
    const float predatorFracNextLevelClose = 0.02f;
    const float predatorFracNextLevelOpen = 0.025f;
    Assert(k+1<NumPond);
    if (Self.isInPond && k<=Self.pondIndex) {
        float predatorFrac = PondSet[Self.pondIndex].predatorFrac();
        if (k==Self.pondIndex) {
            if (BridgeSet[k].isClosed())
                if (predatorFrac<=predatorFracNextLevelOpen) {
                    BridgeSet[k].setOpeningVelocity(1.0f);
                    PlaySoundRelativeToSelf(SoundKind::openGate, BridgeSet[k].center());
                }
        } else {
            if (BridgeSet[k].isWideOpen()) {
                if (predatorFrac<=predatorFracNextLevelClose) {
                    BridgeSet[k].setOpeningVelocity(-1.0f);
                    PlaySoundRelativeToSelf(SoundKind::closeGate, BridgeSet[k].center());
                }
            }
        }
    }
}

//! Recursive routine that fills in PondSet[k..NumPond)
bool InitializeGeometry(size_t k, float meanRadius) {
    Assert(k<NumPond);
    for (int trial=0; trial<4; ++trial) {
        float r = meanRadius*(0.9f+RandomFloat(0.2f));
        Point c;
        if (k==0) {
            c = Point(0, 0);
        } else {
            // Choose random angle
            float theta = RandomAngle();
            float d = (r+PondSet[k-1].radius())*NeighborSeparation;
            c = PondSet[k-1].center()+Polar(d, theta);
        }
        static_cast<Circle&>(PondSet[k]) = Circle(c, r);
        for (size_t j=0; j+1<k; ++j) {
            float d1 = Distance(PondSet[j].center(), PondSet[k].center());
            float d2 = PondSet[j].radius()+PondSet[k].radius();
            if (d1<d2*OtherSeparation)
                goto nextTrial;
        }
        if (k+1==NumPond)
            return true;
        if (InitializeGeometry(k+1, meanRadius))
            return true;
nextTrial:;
    }
    return false;
}

//! Adjust background beetles so that their Voronoi boundaries bisect the bridges
void AdjustBackground() {
    // Iterative numerical relaxation.
    for (int i=0; i<20; ++i) {
        Point delta[N_POND_MAX];
        for (size_t k=0; k<NumPond; ++k)
            delta[k] = Point(0, 0);
        for (size_t k=0; k+1<NumPond; ++k) {
            Point m = 0.5f*(PondBackground[k].pos+PondBackground[k+1].pos);
            Point residue = BridgeSet[k].center()-m;
            delta[k] += residue;
            delta[k+1] += residue;
        }
        for (size_t k=0; k<NumPond; ++k)
            PondBackground[k].pos += 0.8f*delta[k];
    }
}

void InitializeBackground(NimblePixMap& window) {
    NimbleColor brown[2] ={ NimbleColor(64,32,0), NimbleColor(224,112,0) };

    // Initialize background
    PondBackground.reserve(NumPond);
    for (size_t k=0; k<NumPond; ++k) {
        PondBackground[k].pos = PondSet[k].center();
        float frac = k*1618%1000*.001f; // Fibonacci hash
        NimbleColor tmp(brown[0]);
        tmp.mix(brown[1], frac);
        PondBackground[k].color = window.pixel(tmp);
    }
    PondBackground.resize(NumPond);

    // Adjustbackgound
    AdjustBackground();
}

} //(anonymous)

void World::initialize(NimblePixMap& window) {
    TheOriginalViewScale = 0.8f*std::sqrt(float(window.width()*window.height()));
    ZoomFactor = 1;

    float meanRadius = 2;
    NumPond = 8;
    Assert(NumPond<=N_POND_MAX);
    // Initialize the geometry
    while (!InitializeGeometry(0, meanRadius))
        continue;

    // Initialize bridges
    for (size_t k=0; k+1<NumPond; ++k)
        BridgeSet[k].initialize(PondSet[k], PondSet[k+1]);

    Self.initialize(window);
    Missiles::initialize(window);

    // Choose where oranges will go (uniform distribution across first 7 ponds)
    int8_t numOrange[N_POND_MAX] = {};
    constexpr uint32_t totalNumOrange = 5;
    for (size_t i=0; i<totalNumOrange; ++i)
        numOrange[rand()%7u]++;

    // Allocate and initialize ponds
    ResetSlush();
    for (size_t k=0; k<NumPond; ++k) {
        PondOptionSet po;
        if (k>=1) po|=PondOption::predatorMoves;
        if (k>=2) po|=PondOption::waterMoves;
        if (k>=3) po|=PondOption::foodMoves;
        if (k==4) po|=PondOption::whirlpool;
        if (k==5) po|=PondOption::dark;
        if (k==6) po|=PondOption::zoneOfDoom;
        if (k==7) { po|=PondOption::crystalline; po|=PondOption::zinger; }
        PondSet[k].initialize(window, 600, po, numOrange[k]);
    }

    // Initialize WorldViewTransform by doing dummy update.
    updateSelfAndMissiles(window, 0, 0, 0);

    // Initialize the background
    InitializeBackground(window);

    // Initialize the score area
    TheScoreMeter.initialize(window);
    AccumulatedFood = 0;
}

namespace {

void DrawBackground(NimblePixMap& window, CompoundRegion& region) {
    if (!region.empty()) {
        Ant* a = Ant::openBuffer();
        a = PondBackground.copyToAnts(a, World::viewTransform);
        Ant::closeBufferAndDraw(window, region, a, false);
    }
}

//! Draw Ponds with indices [first,last) in given region
void DrawPondGroup(NimblePixMap& window, CompoundRegion& region, size_t first, size_t last) {
    Ant* a = Ant::openBuffer();
    // Draw self if alive and in given pond
    if (Self.isAlive())
        a = Self.assignAntIf(a, World::viewTransform, first, last);
    if (Finale::isRunning())
        // Write text above "self"
        a = Finale::copyToAnts(a, window, Self.pos, PondSet[Self.pondIndex].center());
    a = Missiles::copyToAnts(a, first, last);
    for (size_t k=first; k<last; ++k)
        if (PondSet[k].isDark())
            a = PondSet[k].assignDarkAnts(a, World::viewTransform);
        else
            a = PondSet[k].copyToAnts(a, World::viewTransform);
    Ant::closeBufferAndDraw(window, region, a, first==0);
}

} // (anonymous)

void World::draw(NimblePixMap& window) {
#if 0 
    // Blank out background 
    window.draw(NimbleRect(0, 0, window.width(), window.height()), 0xFFFFFF);
#endif
    // "*4" accounts for one bridge per pond
    // Each bridge requires one positive shape and two negative shapes.
    static ConvexRegion RegionStorage[N_POND_MAX*4];
    static CompoundRegion CompoundRegionStorage[N_POND_MAX];

    size_t k;
    CompoundRegion* cFirst=CompoundRegionStorage;
    CompoundRegion* cLast=cFirst;
    ConvexRegion* rFirst=RegionStorage;
    ConvexRegion* rLast=rFirst;
    SetRegionClip(0, 0, window.width(), window.height(), Outline::lineWidth);
    for (size_t start=0; start<NumPond; start=k) {
        // Find contiguous sequence of ponds
        for (k=start; k<NumPond; ) {
            const Pond& p = PondSet[k];
            Point c = viewTransform.transform(p.center());
            rLast->makeCircle(c, viewTransform.scale(p.radius()));
            if (!rLast->empty()) {
                ++rLast;
            }
            if (BridgeSet[k++].isClosed()) break;
            Assert(k<NumPond);
            rLast = BridgeSet[k-1].pushVisibleRegions(rLast);
        }
        if (rFirst!=rLast) {
            cLast->build(rFirst, rLast);
            // Even if a region was clipped, we must include its beetles if any region shows, because
            // when ponds connect, beetles's voronoi domains can leak into other ponds.
            DrawPondGroup(window, *cLast, start, k);
            rFirst = rLast;
            ++cLast;
        }
    }
    for (size_t k=0; k<NumPond; ++k)
        if (PondSet[k].isDark()) {
            extern void DrawDots(NimblePixMap& window, const Pond& p);
            DrawDots(window, PondSet[k]);
        }

    CompoundRegion background;
    background.buildComplement(cFirst, cLast);
    DrawBackground(window, background);
}

void World::updatePonds(float dt) {
    ColorWobble::updateTime(dt);
    for (size_t k=0; k<NumPond; ++k) {
        PondSet[k].update(dt);
        if (k+1<NumPond) {
            OpenOrCloseBridgeIfReady(k);
            BridgeSet[k].update(dt);
        }
    }
}

static const float Pi = 3.14159265f;

//! Advance position of "self" or "missile" by time dt.
void World::updateDrivenBeetle(Beetle& b, float dt) {
    if (b.isInPond && PondSet[b.pondIndex].hasWhirlpool()) {
        PondSet[b.pondIndex].whirl(b, dt);
    }
    const float epsilon = 1e-6f;
    Point dir = dt*b.vel;
    if (std::sqrt(Dist2(dir)) >= epsilon) {
        // Invariant: self is always in a circle or bridge.  
        float s = 1.0f;
        // Limit of three iterations prevents infinite loop caused by numerical roundoff issues.
        for (int count=0; count<3 && s>=0; ++count) {
            if (b.isInPond) {
                // Find intercept with circle
                float deltaS = PondSet[b.pondIndex].intercept(b.pos, dir);
                if (deltaS>=s) {
                    // Did not go past boundary of circle
                    b.pos += s*dir;
                    Assert(PondSet[b.pondIndex].contains(b.pos));
                    break;
                } else {
                    // Tried to go past boundary of circle.  Advance to boundary.
                    b.pos += deltaS*dir;
                    Assert(PondSet[b.pondIndex].fuzzyContains(b.pos));
                    // Use up that much forward motion.
                    s -= deltaS;
                    // Check if inside a neighboring bridge
                    int kMin = Max(0, b.pondIndex-1);
                    int kMax = b.pondIndex;
                    for (int k=kMin; k<=kMax; ++k) {
                        if (!BridgeSet[k].isClosed())
                            if (BridgeSet[k].contains(b.pos)) {
                                b.pondIndex = k;
                                b.isInPond = false;
                                break;
                            }
                    }
                    if (b.isInPond) {
                        if (b.kind==BeetleKind::self)
                            // Bumped into pond boundary with no connecting bridge.
                            // Use velocity component that is tangential to circle.
                            b.pos = PondSet[b.pondIndex].projectOntoPerimeter(b.pos+s*dir);
                        return;
                    }
                }
            }
            Assert(!b.isInPond);
            if (float deltaS = Min(s, BridgeSet[b.pondIndex].intercept(b.pos, dir))) {
                // Advance through bridge
                b.pos += deltaS*dir;
                s -= deltaS;
                // Check if we are back in a circle
                int kMin = b.pondIndex;
                int kMax = b.pondIndex+1;
                for (int k=kMin; k<=kMax; ++k) {
                    if (PondSet[k].contains(b.pos)) {
                        b.pondIndex = k;
                        b.isInPond = true;
                        break;
                    }
                }
            }
            if (!b.isInPond) {
                // Bumped into bridge boundary or deltaS is zero.
                if (b.kind==BeetleKind::self) {
                    b.pos = BridgeSet[b.pondIndex].plough(b.pos, s*dir);
                    if (!BridgeSet[b.pondIndex].contains(b.pos)) {
                        // Scoot back into closest circle
                        int kMin = b.pondIndex;
                        int kMax = b.pondIndex+1;
                        int j = -1;
                        float dj = FLT_MAX;
                        for (int k=kMin; k<=kMax; ++k) {
                            // Compute distance from circle k
                            float d = Dist2(PondSet[k].center(), b.pos)-PondSet[k].radiusSquared();
                            if (d<dj) {
                                j = k;
                                d = dj;
                                break;
                            }
                        }
                        if (!PondSet[j].contains(b.pos)) {
                            b.pos = PondSet[j].projectOntoPerimeter(b.pos);
                            Assert(PondSet[j].fuzzyContains(b.pos));
                        }
                        b.pondIndex = j;
                        b.isInPond = true;
                    }
                }
                return;
            }
        }
    }
}

static bool SegmentOverlapsPondOrBridge(size_t kMin, size_t kMax, Point a, Point c) {
    for (size_t k=kMin; k<=kMax; ++k)
        if (PondSet[k].overlapsSegment(a, c))
            return true;
    for (size_t k=kMin; k<=kMax; ++k)
        if (BridgeSet[k].overlapsSegment(a, c))
            return true;

    return false;
}

static bool IsBumpable(BeetleKind selfOrMissile, BeetleKind other) {
    Assert(selfOrMissile==BeetleKind::self || selfOrMissile==BeetleKind::missile);
    return other==BeetleKind::predator || other==BeetleKind::sweetie || other==BeetleKind::orange || selfOrMissile==BeetleKind::self && other==BeetleKind::plant;
}

/** Called when self or missile touches another beetle.
    Returns true if "other" should be destroyed; false otherwise. */
static bool TallyBump(Beetle& selfOrMissile, const Beetle& other) {
    Assert(IsBumpable(BeetleKind(selfOrMissile.kind), BeetleKind(other.kind)));
    switch (selfOrMissile.kind) {
        case BeetleKind::missile:
            switch (other.kind) {
                case BeetleKind::predator: {
                    // Destroyed a predator with a missile
                    PlaySoundRelativeToSelf(SoundKind::destroyPredator, other.pos);
                    Missiles::tallyHit(selfOrMissile);
                    return true;
                }
                case BeetleKind::orange:
                    PlaySoundRelativeToSelf(SoundKind::destroyOrange, other.pos);
                    return true;

                case BeetleKind::sweetie:
                    PlaySoundRelativeToSelf(SoundKind::destroySweetie, other.pos);
                    Finale::start("Sweetie died");
                    return true;
                default:
                    Assert(false);
                    return false;
            }
        case BeetleKind::self:
            switch (other.kind) {
                case BeetleKind::plant:
                    if (TheScoreMeter.reachedMaxMissiles())
                        return false;
                    // Ate some food
                    PlaySoundRelativeToSelf(SoundKind::eatPlant, other.pos);
                    TheScoreMeter.addScore(POINTS_PER_FOOD);
                    AccumulatedFood+=1;
                    if (AccumulatedFood>=FOOD_PER_MISSILE) {
                        AccumulatedFood = 0;
                        TheScoreMeter.addMissile(1);
                    }
                    return true;
                case BeetleKind::orange:
                    PlaySoundRelativeToSelf(SoundKind::eatOrange, other.pos);
                    TheScoreMeter.addLife(1);
                    return true;
                case BeetleKind::predator:
                    if (Finale::isRunning()) {
                        // Game is over, so just destroy predator.
                        PlaySoundRelativeToSelf(SoundKind::destroyPredator, other.pos);
                    } else {
                        // Predator bumped me!
                        Self.startTipsey();
                        PlaySoundRelativeToSelf(SoundKind::sufferHit, other.pos);
                        // Pay 10% tax for being bumped
                        TheScoreMeter.multiplyScore(.90f);
                        // Predator dies and I lose a life.
                        if (TheScoreMeter.lifeCount()>0) {
                            TheScoreMeter.addLife(-1);
                        } else {
                            // I am out of lives.  Start death sequence.
                            Self.kill();
                        }
                    }
                    return true;
                case BeetleKind::sweetie:
                    if (!Finale::isRunning()) {
                        PlaySound(SoundKind::smooch);
                        TheScoreMeter.addScore(POINTS_PER_KISS);
                        Finale::start("Smooch!");
                        PondSet[NumPond-1].melt();
                    }
                    return false;
            }
        default:
            Assert(false);
            return false;
    }
}

//! Record for tracking a kill
class KillRec {
    //! Pond that contains victim
    Pond* pond;
    //! Index of victim within pond
    size_t localIndex;
public:
    friend bool operator<(const KillRec& x, const KillRec& y) {
        return x.localIndex < y.localIndex;
    }
    void assign(Pond* p, size_t i) {
        pond = p;
        localIndex = i;
    }
    void doKill() {
        pond->kill(localIndex);
    }
};

static const size_t N_KILL_MAX = 64;
static KillRec KillBuf[N_KILL_MAX];
static KillRec* KillPtr = KillBuf;

void World::checkHit(Beetle& b) {
    Assert(b.kind==BeetleKind::self || b.kind==BeetleKind::missile);
    size_t kMin = b.pondIndex;
    size_t kMax = b.pondIndex;
    while (kMin>0 && !BridgeSet[kMin-1].isClosed())
        --kMin;
    while (kMax<NumPond-1 && !BridgeSet[kMax].isClosed())
        ++kMax;
    static Neighbor buffer[N_ANT_MAX];
    Neighborhood neighborhood(buffer, sizeof(buffer)/sizeof(buffer[0]));
    neighborhood.start();
    Neighbor::indexType beginIndex[N_POND_MAX+1];
    Neighbor::indexType index=0;
    for (size_t k=kMin; k<=kMax; ++k) {
        beginIndex[k] = index;
        const Pond& p = PondSet[k];
        for (const auto& p: PondSet[k]) {
            neighborhood.addPoint(p.pos-b.pos, index++);
        }
    }
    beginIndex[kMax+1] = index;

    // Iterate over neighboring Voronoi cells
    Neighbor* e = buffer + neighborhood.finish();
    for (Neighbor* s = buffer; s<e; ++s) {
        if (s->index==Neighbor::ghostIndex)
            continue;
        // Find which pond the cell belongs to
        size_t which=kMin;
        while (s->index >= beginIndex[which+1]) {
            ++which;
            Assert(which<=kMax);
        }
        Pond* p = &PondSet[which];
        size_t localIndex = s->index-beginIndex[which];
        if (localIndex < p->safeSize()) {
            Beetle& other = (*p)[localIndex];
            Point a = CenterOfCircle(*s, *(s==buffer?e-1:s-1)) + b.pos;
            Point c = CenterOfCircle(*s, *(s+1==e?buffer:s+1)) + b.pos;
            if (SegmentOverlapsPondOrBridge(kMin, kMax, a, c)) {
                if (IsBumpable(BeetleKind(b.kind), BeetleKind(other.kind))) {
                    // The order of the next two if conditions is important.
                    // If the KillBuf is full, then "other" is considered unkillable
                    // for the moment.  Such a situation is theoretically rare,
                    // but if it happens, we don't want to TallyBump something twice.
                    // In practice, KillBuf has never been seen to have more than 2 items.
                    if (KillPtr<&KillBuf[N_KILL_MAX])
                        if (TallyBump(b, other))
                            (KillPtr++)->assign(p, localIndex);
                }
                if (other.kind == BeetleKind::water) {
                    float r = 1.0f;
                    if (b.kind==BeetleKind::missile) {
                        float d = std::sqrt(Dist2(Self.pos, other.pos));
                        float dMin = 0.1f;
                        if (d>=dMin)
                            r = dMin/d;
                    }
                    if (b.kind == BeetleKind::self)
                        r = 1.0f;
                    // FIXME - clip segment against Pond/Bridge. 
                    // The maxD limit is a hack in lieu of proper clipping.
                    const float maxD = 0.25f;
                    AppendSlush(b, other, Min(Dist2(a, c), maxD), r);
                }
            }
        }
    }
}

void World::updateSelfAndMissiles(NimblePixMap& window, float dt, float forward, float torque) {
    // Update self
    Assert(KillPtr == KillBuf);
    Self.update(dt, forward, torque);
    // Set WorldViewTransform so that self is about 1/4 from bottom center of screen and pointed upwards.
    float scale = TheOriginalViewScale*ZoomFactor*Self.tipseyScale();
    viewTransform.setScaleAndRotation(scale*Point(-Self.directionVector().y, -Self.directionVector().x));
    viewTransform.setOffset(Point(window.width()/2, window.height()*0.75) - viewTransform.rotate(Self.pos));
    Missiles::update(dt);
    // Do UpdateSlush before doing kills, because AppendSlush captured pointers to Beetle
    // objects in ponds that doKill will move.
    UpdateSlush(dt);
    // Do kills in decreasing index order, because of the way "Pond::kill" moves items
    std::sort(KillBuf, KillPtr);
    while (KillPtr > KillBuf)
        (--KillPtr)->doKill();
}

void World::update(NimblePixMap& window, float dt, float forward, float torque) {
    updatePonds(dt);
    Finale::update(dt);
    updateSelfAndMissiles(window, dt, forward, torque);
}

#if WIZARD_ALLOWED
void Zoom(float factor) {
    if (factor>0)
        ZoomFactor *= factor;
    else
        ZoomFactor = 1;
}

void JumpToPond(int delta) {
    const int32_t i = Clip<int32_t>(0, NumPond-1, Self.pondIndex+delta);
    if (i!=Self.pondIndex) {
        Self.pondIndex = i;
        Self.pos = PondSet[i].center();
    }
}
#endif
