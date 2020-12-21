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

#pragma once
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
    void drawOn(NimblePixMap& map, int top, int bottom) const;
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

    //! Describes horizontal run of pixels with same color.
    struct runType {
        static const unsigned dyMax = 255;
        NimblePixel color;
        //! x coordinate where run starts
        uint32_t x:12;
        //! delta for y coordinate relative to previous run
        uint32_t dy:8;
        //! Length of run
        uint32_t len:12;
    };
    runType* myArray;
    //! Number of elements in myArray
    size_t mySize;
    //! Dimensions of the overlay in pixels.
    int32_t myWidth, myHeight;
};

#endif /* Widget_H */