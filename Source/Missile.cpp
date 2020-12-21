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
#include "Missile.h"
#include "Self.h"
#include "VoronoiText.h"
#include "World.h"

//! Represents one missile
struct MissileType: Beetle {
    //! Time that Missile has been in flight so far
    float clock;
    //! Number of points that a Missile hitting a target is worth, or 0 if Missile is not in flight. 
    int value;
    ColorGradient colorDecay;
    void update( float dt );
    void fire();
};

//! Max number of missiles allowed
static const size_t N_Missile = 12;

//! Storage for missiles.
static MissileType Missile[N_Missile];

//! Missile lifetime, in seconds
const float MissileLifetime = 2;

void MissileType::update( float dt ) {
    if( value ) {
        color = colorDecay( clock/MissileLifetime );
        World::updateDrivenBeetle( *this, dt );
        clock += dt;
        if( clock>MissileLifetime ) 
            // Deactivate missile
            value = 0;
        else
            World::checkHit( *this );
    }
}

void MissileType::fire() {
    Assert(!value);
    clock = 0;
    value = 2;
    vel = Self.directionVector();
    const float recoil = 0.5f;
    Self.vel -= recoil*Self.directionVector();
    pondIndex = Self.pondIndex;
    isInPond = Self.isInPond;
    pos = Self.pos;
}

void Missiles::initialize( NimblePixMap& window ) {
    for( size_t k=0; k<N_Missile; ++k ) {
        MissileType& b = Missile[k];
        b.kind = BeetleKind::missile;
        int adjustBlue = (k%2)*32;
        int adjustRed = (k/2%2)*32;
        b.colorDecay.initialize( window, NimbleColor( 255-adjustRed, 0, 255-adjustBlue ),
                                         NimbleColor( 100, 0, 100 ));
        b.value = 0;
        b.soundId = static_cast<BeetleSoundId>(2+k);
    }
}

void Missiles::update( float dt ) {
    for( size_t k=0; k<N_Missile; ++k ) 
        Missile[k].update(dt);
}

Ant* Missiles::copyToAnts(Ant* a, size_t firstPond, size_t lastPond ) {
    for( size_t k=0; k<N_Missile; ++k ) 
        if( Missile[k].value )
            a = Missile[k].assignAntIf( a, World::viewTransform, firstPond, lastPond );
    return a;
}

void Missiles::tryFire() {  
    if( Self.isAlive() && TheScoreMeter.missileCount()>0 )
        for( size_t k=0; k<N_Missile; ++k ) 
            if( !Missile[k].value ) {
                TheScoreMeter.addMissile(-1);
                Missile[k].fire();
                break;
            }
}

void Missiles::tallyHit(Beetle& b) {
    Assert( b.kind == BeetleKind::missile );
    MissileType& m = static_cast<MissileType&>(b);
    Assert( Missile<=&m && &m<Missile+N_Missile );
    TheScoreMeter.addScore( m.value );
    if( m.value<128 )
        m.value *= 2;
}