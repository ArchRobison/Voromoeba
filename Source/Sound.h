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

#pragma once
#include "Geometry.h"
#include "Beetle.h"

enum SoundKind {
    SOUND_DESTROY_ORANGE,
    SOUND_DESTROY_PREDATOR,
    SOUND_DESTROY_SWEETIE,
    SOUND_EAT_PLANT,
    SOUND_EAT_ORANGE,
    SOUND_SUFFERED_HIT,
    SOUND_SELF,
    SOUND_MISSILE,
    SOUND_OPEN_GATE,
    SOUND_CLOSE_GATE,
    SOUND_SMOOCH,
    N_SOUND
};

//! Play sound of kind k coming from point p.
void PlaySound( SoundKind k, Point p=Point(0,1) ); 

void ResetSlush();
BeetleSoundId AllocateSlush( float u );
void AppendSlush( const Beetle& b, const Beetle& other, float segmentLength, float relativeVolume );
void UpdateSlush( float dt );
void ConstructSounds();