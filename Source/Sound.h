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
#include "Geometry.h"
#include <cstdint>

enum class SoundKind : int8_t {
    destroyOrange,
    destroyPredator,
    destroySweetie,
    eatPlant,
    eatOrange,
    sufferHit,
    self,
    missile,
    openGate,
    closeGate,
    smooch
};

template<>
constexpr SoundKind EnumMax<SoundKind> = SoundKind::smooch;

//! Play sound of kind k coming from point p.
void PlaySound(SoundKind k, Point p=Point(0, 1));

void ResetSlush();
BeetleSoundId AllocateSlush(float u);
void AppendSlush(const Beetle& b, const Beetle& other, float segmentLength, float relativeVolume);
void UpdateSlush(float dt);
void ConstructSounds();