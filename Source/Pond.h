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

#include "Beetle.h"
#include "Enum.h"
#include "NimbleDraw.h"
#include "Utility.h"
#include "Geometry.h"
#include <cstdint>

enum class PondOption : uint8_t {
    waterMoves,
    predatorMoves,
    foodMoves,
    whirlpool,
    zoneOfDoom,
    dark,
    crystalline,
    zinger      // Predators are fast
};

template<>
constexpr PondOption EnumMax<PondOption> = PondOption::zinger;

using PondOptionSet = EnumSet<PondOption>;

class Pond : public Circle, public BugArray<Beetle> {
public:
    Pond() : Circle() {}
    void initialize(NimblePixMap& window, size_t size, PondOptionSet pondOptions, uint32_t numOrange);
    void update(float dt);
    void drawOn(NimblePixMap& window);
    // Schedule ith beetle for destruction
    void kill(size_t i);
    void whirl(Beetle& b, float dt);
    bool hasWhirlpool() const { return myPondOptions[PondOption::whirlpool]; }
    bool hasZoneOfDoom() const { return myPondOptions[PondOption::zoneOfDoom]; }
    bool isDark() const { return myPondOptions[PondOption::dark]; }
    void melt() { myPondOptions -= PondOption::crystalline; }
    size_t safeSize() const { return mySafeSize; }
    Ant* assignDarkAnts(Ant* a, const ViewTransform& v) const;
    //! Fraction of cells occupied by predators
    float predatorFrac() const { return float(myPredatorCount)/size(); }
private:
    void initializeColorAndSound(NimblePixMap& window, size_t first, size_t last, NimbleColor c0, NimbleColor c1, BeetleKind kind, OutlinedColor::exteriorColor=0);
    PondOptionSet myPondOptions;
    Grating myZoneOfDoom;
    //! Beetles from [safeSize,size()) are doomed.
    size_t mySafeSize = 0;
    //! Count of BK_PREDATOR beetles in this pond
    int myPredatorCount = 0;
    //! 0 if no whirling, 1 if clockwise whirl, -1 if counterclockwise whirl.
    int8_t myWhirlSpeed = 0;
};