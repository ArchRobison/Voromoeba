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

#ifndef World_H
#define World_H

#include "Forward.h"

extern VoronoiMeter TheScoreMeter;

#if WIZARD_ALLOWED
void OpenBridgeToNextPond();
void JumpToPond(int delta);
void Zoom(float factor);
#endif

//! Module for representing World (ponds + bridges)
class World {
    static void updatePonds(float dt);
    static void updateSelfAndMissiles(NimblePixMap& window, float dt, float forward, float torque);
public:
    //! Initialize world to start a new game
    static void initialize(NimblePixMap& window);
    //! Draw world (ponds, self, missiles) on given window.
    static void draw(NimblePixMap& window);
    /** torque = torque to apply to self (not scaled by dt).
        forward = forward force to apply (not scaled by dt). */
    static void update(NimblePixMap& window, float dt, float forward, float torque);
    //! Account for given beetle (self or missile) hitting another beetle.
    static void checkHit(Beetle& b);
    //! Advance a beetle that is not bound to a particular pond.
    static void updateDrivenBeetle(Beetle& b, float dt);
    //! Get Bridge that is the entrance to the given Pond
    static Bridge* getEntrance(const Pond& p);
    //! True if given beetle is in a dark pond.
    static bool isInDarkPond(const Beetle& b);
    //! Transform from World coordinates onto window coordinates
    static ViewTransform viewTransform;
};

#endif /* World_H */