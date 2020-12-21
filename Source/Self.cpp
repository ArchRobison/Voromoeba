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

#include "Finale.h"
#include "Self.h"
#include "Sound.h"
#include "World.h"

ColorWobble SelfType::colorWobble(1.0f);

SelfType Self;

static const float TipseyTimeMax = 10.0f;

void SelfType::initialize( NimblePixMap& window ) {
    static OutlinedColor::exteriorColor exSelf = OutlinedColor::newExteriorColor(0xF09000);
    colorWobble.initialize( window, NimbleColor(NimbleColor::FULL,NimbleColor::FULL*7/8,0),
                                    NimbleColor(NimbleColor::FULL,NimbleColor::FULL,NimbleColor::FULL/2));
    lifetime = FLT_MAX;      
    pos = Point(0,0);
    vel = Point(0,0);  
    isInPond = true;
    pondIndex = 0;
    kind = BeetleKind::self;
    orbit = colorWobble.orbit(1.0f,0);
    color = OutlinedColor(colorWobble(orbit),exSelf);
    soundId = 1;
    tipseyTime = FLT_MAX;
}

void SelfType::update( float dt, float forward, float torque ) {
    bool wasAlive = isAlive();
    if( wasAlive ) {
        // Apply torque
        float torqueScale = 8.0f;
        angularVelocity += torque*torqueScale*dt;
        angularPosition += angularVelocity*dt;
    }

    // Apply rotational drag
    float rotationalDrag = 4.0f;
    angularVelocity -= angularVelocity*rotationalDrag*dt;
    Point u = Polar(1,angularPosition);
    myDirectionVector.x = u.y;
    myDirectionVector.y = u.x;
    if( wasAlive ) 
        if( lifetime>deathTime() ) {
            // Normal movement
            vel += dt*forward*2*myDirectionVector;
            color.setInterior( colorWobble( orbit ) );
        } else {
            // Dying - degrade thrust proportionately
            vel += dt*forward*2*lifetime*(1.0f/deathTime())*myDirectionVector;
            // FIXME - interpolate color
            color = ~0u;
        }

    // Apply linear drag
    float linearDrag = 3.0f;
    vel -= linearDrag*dt*vel;

    // Update lifetime
    lifetime -= dt;
    if( wasAlive && !isAlive() ) {
        PlaySound( SOUND_DESTROY_PREDATOR );    // FIXME - have different sound
        Finale::start( "You died" ); 
    }

    World::updateDrivenBeetle( Self, dt );
    if( isAlive() )
        World::checkHit( *this );

    tipseyTime += dt;
}

float SelfType::tipseyScale() const {
    if( tipseyTime<TipseyTimeMax ) 
        //! Adjust scale for tipseyness after being hit
        return 1+0.5f*std::sin(2*3.1415926535f*tipseyTime)*std::exp(-tipseyTime*tipseyTime);
    else 
        return 1;
}
