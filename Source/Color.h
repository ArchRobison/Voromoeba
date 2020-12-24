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

#ifndef Color_H
#define Color_H

#include <cmath>
#include "NimbleDraw.h"
#include "Utility.h"

 //! Represent a gradient of colors
class ColorGradient {
protected:
    static const size_t size = 256;
    NimblePixel color[size];
public:
    ColorGradient() {}
    //! Initialize the gradient to go from c0 to c1.  
    /** The window argument is required because the internal lookup table a table of NimblePixel
        converted from the NimbleColor arguments. */
    void initialize(const NimblePixMap& window, NimbleColor c0, NimbleColor c1);
    //! Return color interpolated from value x, where x is in range [0,1]
    NimblePixel operator()(float x) const {
        size_t k = Round(x*(size-1));
        Assert(k<size);
        return color[k];
    }
};

//! Generates a stream of colors chosen semi-randomly from a color gradient. 
class ColorStream : private ColorGradient {
public:
    typedef unsigned seedType;
    NimblePixel get(seedType& seed) const {
        seed += 157;                // 157 is about phi*size, and relatively prime to size
        return color[seed % size];
    }
    using ColorGradient::initialize;
};

//! Lookup table for a color that periodically wobbles
/** Different wobbly colors can share the same table by choosing different means. */
class ColorWobble : private ColorGradient {
    //! Angle in wobble space
    static float omega;
    float amplitude;
public:
    //! Specification of a wobbling color that cannot be factored out into a ColorWobble.
    class orbitType {
        byte mean;
        byte freq;
        friend class ColorWobble;
    };
    //! Create orbit for *this with given mean and frequency.
    /** The input mean should be in the range [0.0,1.0] */
    orbitType orbit(float mean, int32_t freq) {
        Assert(0<=mean && mean<=1);
        orbitType o;
        o.freq = freq;
        int y = amplitude + mean*((size-1)-2*amplitude);
        if (y<amplitude) {
            // Increment mean to ensure that calculations in operator() do not generate "negative" k.
            ++y;
        }
        o.mean = y;
        return o;
    }
    //! Update time (advance angle in wobble space)
    static void updateTime(float dt);

    //! Get current color of o
    NimblePixel operator()(const orbitType& o) const {
        float y = o.mean + amplitude*std::sin(omega*(o.freq+128));
        size_t k = Round(y);
        Assert(k<=size);
        return color[k];
    }
    //! Construct ColorWobble with given peak-to-peak amplitude and undefined gradient.
    /** The peak-to-peak amplitude should be 1.0 if the wobble is over the entire gradient
        and proportionally smaller if over a fraction of the gradient.  The Caller should
        subsequently call method initialize() to initialize the gradient. */
    ColorWobble(float peakToPeak) : amplitude(peakToPeak* ((size-1)/2)) {}
    using ColorGradient::initialize;
};

#endif /* Color_H */