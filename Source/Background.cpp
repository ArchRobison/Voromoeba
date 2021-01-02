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

#include "Background.h"

void Background::doInitialize(NimblePixMap& window, size_t n, bool (*f)(Point&, contextType), contextType context) {
    // Try to create n bugs                    
    reserve(n);
    size_t k = 0;
    for (size_t trial=0; trial<20*n && k<n; ++trial) {
        Point p;
        if (f(p, context)) {
            auto& b = (*this)[k++];
            b.pos.x = p.x;
            b.pos.y = p.y;
            b.vel = Point();
        }
    }
    // Created k bugs
    resize(k);

    // Color the bugs
    NimbleColor c0(0, 0, 0xFF);
    NimbleColor c1(0, 0, 0x80);
    for (size_t j=0; j<k; ++j) {
        NimbleColor c2;
        c2 = c0;
        if (k-1==0)
            c2 = c0;
        else
            c2.mix(c1, float(j)/(k-1));
        (*this)[j].color = window.pixel(c2);
    }
}