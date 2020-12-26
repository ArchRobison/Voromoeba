/* Copyright 1996-2012 Arch D. Robison

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

 /******************************************************************************
  Various widgets for the screen
 *******************************************************************************/

#ifndef Widget_H
#define Widget_H

#include "BuiltFromResource.h"
#include "Nimbledraw.h"
#include <cstdint>

class Widget : BuiltFromResourcePixMap {
public:
    Widget(const char* resourceName) :
        BuiltFromResourcePixMap(resourceName) {
    }
protected:
    NimblePixMapWithOwnership myPixMap;
    void buildFrom(const NimblePixMap& map) override;
private:
    Widget(const Widget&) = delete;
    void operator=(const Widget&) = delete;
};

//! Image that can be stretched to fit a rectangle.
//!
//! Stretching does not distort the borders.
class RubberImage : public Widget {
public:
    RubberImage(const char* resourceName);
    void drawOn(NimblePixMap& map) const { drawOn(map, 0, map.height()); }
    void drawOn(NimblePixMap& map, int top, int bottom) const;
};

class Font : BuiltFromResourcePixMap {
public:
    Font(const char* resourceName) :
        BuiltFromResourcePixMap(resourceName),
        storage(NULL), myHeight(0) {
    }
    ~Font() { delete[] storage; }
    Font(const Widget&) = delete;
    void operator=(const Font&) = delete;
    /** Returns x coordinate that next character would have. */
    int drawOn(NimblePixMap& map, int x, int y, const char* s, NimblePixel ink);
    int height() const { return myHeight; }
    int width(const char* string) const;
private:
    static const char charMin=32;
    static const char charMax=127;
    byte* storage;
    byte myHeight;
    unsigned short start[charMax-charMin+2];
    void buildFrom(const NimblePixMap& map) override;
    static bool isBlankColumn(const NimblePixMap& map, int x);
};

//! Displays a floating-point number, not using the Voronoi font.
//! Used for displaying information such as frame rate.
class DigitalMeter : public Widget {
public:
    //! nDigit is total number of digits, including decimal point.
    //! and the nDecimal digits after the point.
    DigitalMeter(int nDigit, int nDecimal);
    void drawOn(NimblePixMap& map, int x, int y) const;
    void setValue(float value) { myValue=value; }
    float value() const { return myValue; }
    void operator+=(float addend) { myValue+=addend; }
    int width() const { return myPixMap.width()*myNdigit; }
    int height() const { return myPixMap.height()/13; }
private:
    float myValue;
    int8_t myNdigit;
    int8_t myNdecimal;
};

//! Image that is overlayed on top of another image.
//!
//! Optimized for long transparent runs.
class InkOverlay final : public BuiltFromResourcePixMap {
public:
    //! Upper left pixel of resource defines the transparent color.
    //! Resource must not be more than 4095 pixels wide.
    InkOverlay(const char* resourceName) :
        BuiltFromResourcePixMap(resourceName),
        myWidth(0),
        myHeight(0) {
    }
    ~InkOverlay() { delete[] myArray; }

    //! Draw this InkOverlay, with upper left corner positioned at (left,top).
    //! The InkOverlay must fit in the map.
    void drawOn(NimblePixMap& map, int left, int top) const;
    int height() const {
        Assert(myWidth>=32);
        return myHeight;
    }
    int width() const {
        Assert(myWidth>=32);
        return myWidth;
    }
protected:
    void buildFrom(const NimblePixMap& map) final;

private:
    InkOverlay(const InkOverlay&) = delete;
    void operator=(const InkOverlay&) = delete;
    static constexpr NimblePixel rgbMask = 0xFFFFFF;

    //! Describes horizontal run of pixels with same color.
    class elementType {
        //! Upper byte determines meaning.
        //! 0 -> lower 24 bits are split into 12 bits each of (x,y) coordinate
        //! nonzero -> lower 24 bits are RGB portion of NimblePixel, upper 8 bits are run length.
        uint32_t bits;
        explicit elementType(uint32_t bits_) : bits(bits_) {}
    public:
        elementType() = default;\
        //! True if element defines (x,y) start coordinate, false if element defines a run.
        bool isStart() const { return len() == 0; }
        //! Color of a run
        NimblePixel color() const { return bits & rgbMask; }
        //! Length of run
        uint32_t len() const { return bits >> 24; }

        uint32_t x() const { return bits & 0xFFF; }
        uint32_t y() const { return bits >> 12 & 0xFFF; }

        static elementType makeStart(uint32_t x, uint32_t y) {
            Assert(x < 0x1000);
            Assert(y < 0x1000);
            return elementType(y << 12 | x);
        }

        static elementType makeRun(NimblePixel color, uint32_t len) {
            Assert(0 < len);
            Assert(len < 0x100);
            return elementType(color & 0xFFFFFF | len << 24);
        }
    };
    elementType* myArray;
    //! Number of elements in myArray
    size_t mySize;
    //! Dimensions of the overlay in pixels.
    int32_t myWidth, myHeight;
};

#endif /* Widget_H */