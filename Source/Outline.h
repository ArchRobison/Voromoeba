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

#ifndef Outline_H
#define Outline_H

#include "AssertLib.h"
#include "NimbleDraw.h"
#include <cstdint>

 //! Compact representation of a 24-bit interior and 8-bit indexed exterior color.
class OutlinedColor {
    //! Low 24 bits are RBG color; upper 8 bits are index into color table.  index=0 means there is no outline.
    uint32_t myData;
    //! Mask for interior color bits
    static constexpr uint32_t interiorMask = 0xFFFFFF;
    //! Maximum number of distinct exterior colors 
    static constexpr uint32_t exteriorNumColorMax = 255;
    static uint32_t exteriorColorCount;
    static NimblePixel exteriorTable[exteriorNumColorMax+1];
public:
    //! Index of an exterior color.  Zero means "no exterior color".
    using exteriorColor = uint8_t;
    //! Create undefined OutlineColor
    OutlinedColor() {}
    //! Create interior color with given exterior color.  Default is no exterior color.
    OutlinedColor(NimblePixel color, exteriorColor outside=0) : myData(color& interiorMask|outside<<24) {}
    //! Set the interior color
    void setInterior(NimblePixel color) { myData = color&interiorMask | myData&~interiorMask; }
    //! True if *this has an exterior color
    bool hasExterior() const { return myData&~interiorMask; }
    //! Interior color of *this
    NimblePixel interior() const {
        return myData&interiorMask;
    }
    //! Exterior color of *this.  Valid only if hasExterior()==true.
    NimblePixel exterior() const {
        Assert(hasExterior());
        return exteriorTable[myData>>24];
    }
    //! Clear the global exterior color counter
    static void clearExteriorColors() { exteriorColorCount=0; }
    //! Create a new exterior color index.  Increments the global exterior color counter.
    static exteriorColor newExteriorColor(NimblePixel c) {
        unsigned i = ++exteriorColorCount;
        Assert(i<=exteriorNumColorMax);
        exteriorTable[i] = c;
        return i;
    }
    friend class Outline;
};

class Outline {
public:
    enum class idType : uint16_t {
        null = 0
    };
private:
    static const size_t nSegmentMax = 1<<15;

    //! Horizontal segment on display
    struct segment {
        idType id;
        int16_t y;
        int16_t left, right;
        OutlinedColor color;
        friend bool operator<(const segment& a, const segment& b) {
            return a.id<b.id;
        }
    };

    static segment* ptr;
    static segment array[nSegmentMax];

    static const size_t nIdMax = 1<<12; // FIXME - determine sensible value
    static segment sorted[nSegmentMax];
    static segment* sortIntoBins();

    static void loadCache(NimblePixel interior, NimblePixel exterior);
    template<bool Left, bool Right>
    static void gradient(const segment* s, const segment* jmin, const segment* jmax, int xleft, int xright, NimblePixel* out);
    static unsigned idCount;
public:
    static const int lineWidth = 5;
    static idType newId() {
        Assert(idCount+1 < nIdMax);
        return static_cast<idType>(++idCount);
    }
    static void start() {
        array[0].id = idType::null;    // Initialize sentinel
        ptr = &array[1];
        idCount = 0;
    }
    static void addSegment(idType id, short left, short right, short y, const OutlinedColor& color) {
        Assert(color.hasExterior());
        Assert(ptr<&array[nSegmentMax-1]);
        if (ptr<&array[nSegmentMax-1]) {
            segment* s = ptr++;
            s->id = id;
            s->y = y;
            s->left = left;
            s->right = right;
            s->color = color;
        }
    }
    static void finishAndDraw(NimblePixMap& window);
};

#endif /* Outline_H */