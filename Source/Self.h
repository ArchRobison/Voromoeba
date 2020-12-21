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

#ifndef Self_H
#define Self_H

#include "Beetle.h"

class SelfType: public Beetle {
    static float deathTime() {return 1.f;}
    float lifetime; 
    float angularVelocity;
    ReducedAngle angularPosition;
    Point myDirectionVector;
    float tipseyTime;
public:
    const Point& directionVector() const {return myDirectionVector;}
    void initialize( NimblePixMap& window );
    void update( float dt, float forward, float torque );
    bool isAlive() const {return lifetime>=0;}
    void kill() {
        if( lifetime>deathTime() )
            lifetime = deathTime();
    }
    void whirlAngularPosition( float deltaTheta ) {
        angularPosition += deltaTheta;
    }
    void startTipsey() {tipseyTime = 0;}
    float tipseyScale() const;
    static ColorWobble colorWobble;
};

extern SelfType Self;

#endif /* Self_H */