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

#ifndef Background_H
#define Background_H

#include "Bug.h"
#include "Forward.h"

//! An array of Bug objects used for generating Voronoi backgrounds.
class Background : public BugArray<Bug> {
    typedef void* contextType;
    template<typename PointGenerator>
    static bool generatePoint(Point& p, void* func) {
        auto& pg = *(PointGenerator*)func;
        return pg(p);
    }
    void doInitialize(NimblePixMap& window, size_t n, bool (*f)(Point&, contextType), contextType context);
public:
    //! Initalize background to approximately n points using Point generator f.
    /** bool f(Point&) must assign to the given point and return true, or return false. */
    template<typename PointGeneratorFunc>
    void initialize(NimblePixMap& window, size_t n, PointGeneratorFunc f) {
        doInitialize(window, n, &generatePoint<PointGeneratorFunc>, &f);
    }
};

template<typename R>
static Point Center(const R& rect) {
    return 0.5f*Point(rect.width(), rect.height());
}

#endif /* Backgound_H */