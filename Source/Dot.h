/* Copyright 2020-2021 Arch D. Robison

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

#ifndef Dot_H
#define Dot_H

#include "Forward.h"

//! Module for drawing generator points for a Voronoi diagram,
//! with different shapes for different kinds of points.
class Dot {
public:
    static void initialize(const NimblePixMap& map);

    //! Draw dots for ants in given pond.
    static void draw(NimblePixMap& window, const Pond& p);
};

#endif /* Dot_H */
