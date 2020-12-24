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

#ifndef Ant_H
#define Ant_H

#include "Forward.h"
#include "Geometry.h"
#include "Outline.h"
#include <cfloat>
#include <cmath>
#include <limits>

constexpr float AntInfinity = std::numeric_limits<float>::max();

//! Display Voronoi seeds if set
extern bool ShowAnts;

//! A Voronoi generator point and its associated interior/exterior colors. 
//! Also has static members that implement a module for buffering Ants.
class Ant : public Point {
public:
    OutlinedColor color;

    //! Functor for sorting by x coordinate
    struct lessX {
        bool operator()(const Ant& a, const Ant& b) const { return a.x<b.x; }
    };

    //! Functor for sorting by y coordinate
    struct lessY {
        bool operator()(const Ant& a, const Ant& b) const { return a.y<b.y; }
    };

    //! True if Ant is a "bookend" Ant.
    bool isBookend() const {
        return std::fabs(y)==AntInfinity;
    }

    void assignFirstBookend() {
        y = -AntInfinity;
    }

    void assignLastBookend() {
        y = AntInfinity;
    }

    template<typename Color>
    void assign(Point pos_, Color color_) {
        Assert(std::fabs(pos_.y) < AntInfinity);
        static_cast<Point&>(*this) = pos_;
        color = color_;
    }

    //! Return a pointer to the beginning of a buffer to be filled with Ants.  
    static Ant* openBuffer();

    //! Close a buffer and draw the corresponding Voronoi diagram in the given window.
    static void closeBufferAndDraw(NimblePixMap& window, const CompoundRegion& region, Ant* antLast, bool compose=false, bool showAnts=ShowAnts);

    static void clearBuffer();
    static void switchBuffer();
};

//! Maximum number of Ants in a buffer
constexpr size_t N_ANT_MAX = 1<<15;

#endif /* Ant_H */