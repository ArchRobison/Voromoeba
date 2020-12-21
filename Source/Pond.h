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

#include "NimbleDraw.h"
#include "Utility.h"
#include "Geometry.h"
#include "Beetle.h"

enum PondOptions {
    PO_None=0,
    PO_WaterMoves=1,
    PO_PredatorMoves=2,
    PO_FoodMoves=4,
    PO_Whirlpool=8,
    PO_ZoneOfDoom=0x10,
    PO_Dark=0x20,
    PO_Crystalline=0x40,
    PO_Zinger=0x80      // Predators are fast
};

inline PondOptions operator|( PondOptions x, PondOptions y ) {
    return PondOptions(int(x)|int(y));
}

inline void operator|=( PondOptions& x, PondOptions y ) {
    x = x|y;
}

inline void operator-=( PondOptions& x, PondOptions y ) {
    x = PondOptions(x&~y);
}

class Pond: public Circle, public BasicArray<Beetle> {
public:
    Pond() : Circle(), myPondOptions(PO_None), myWhirlSpeed(0) {}
    void initialize( NimblePixMap& window, size_t size, PondOptions pondOptions, size_t numOrange );
    void update( float dt );
    void drawOn( NimblePixMap& window );
    // Schedule ith beetle for destruction
    void kill( size_t i );
    void whirl( Beetle& b, float dt );
    bool hasWhirlpool() const {return myPondOptions&PO_Whirlpool;}
    bool hasZoneOfDoom() const {return myPondOptions&PO_ZoneOfDoom;}
    bool isDark() const {return myPondOptions&PO_Dark;}
    void melt() {myPondOptions -= PO_Crystalline;}
    size_t safeSize() const {return mySafeSize;}
    Ant* assignDarkAnts( Ant* a, const ViewTransform& v ) const;
    //! Fraction of cells occupied by predators
    float predatorFrac() const {return float(myPredatorCount)/size();}
private:
    void initializeColorAndSound( NimblePixMap& window, size_t first, size_t last, NimbleColor c0, NimbleColor c1, BeetleKind kind, OutlinedColor::exteriorColor=0 );
    PondOptions myPondOptions;
    Grating myZoneOfDoom;
    //! Beetles from [safeSize,size()) are doomed.
    size_t mySafeSize;
    //! Count of BK_PREDATOR beetles in this pond
    int myPredatorCount;
    //! 0 if no whirling, 1 if clockwise whirl, -1 if counterclockwise whirl.
    signed char myWhirlSpeed;
};