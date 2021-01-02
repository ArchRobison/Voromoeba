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

#ifndef Beetle_H
#define Beetle_H

#include "Bug.h"
#include "Enum.h"
#include "Geometry.h"
#include <cstdint>

enum class BeetleKind : uint8_t {
    water,
    plant,
    orange,
    predator,
    self,
    missile,
    sweetie
};

template<>
constexpr BeetleKind EnumMax<BeetleKind> = BeetleKind::sweetie;

using BeetleSoundId = uint16_t;

//! A Beetle is a Bug in a Pond
class Beetle : public Bug {
public:
    //! Kind of Beetle.
    BeetleKind kind;
    //! Index of pond that beetle currently occupies
    uint8_t pondIndex:7;
    //! True if beetle is in a pond; false if it is in a bridge
    uint8_t isInPond:1;
    //! State of wobbling color
    ColorWobble::orbitType orbit;
    //! Id used for sound.  Zero means the beetle does not make a sound.
    BeetleSoundId soundId;
    //! If this is in a pond in [firstPond,lastPond), append to buffer pointed to by a.  Return pointer to end of buffer.
    inline Ant* assignAntIf(Ant* a, const ViewTransform& v, size_t firstPond, size_t lastPond) {
        if (firstPond<=pondIndex && pondIndex<lastPond) {
            a->assign(v.transform(pos), color);
            ++a;
        }
        return a;
    }
};

#endif /* Beetle_H */