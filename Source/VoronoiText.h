/* Copyright 2011-2013 Arch D. Robison 

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

#include "Voronoi.h"
#include "BuiltFromResource.h"
#include "Bug.h"
#include "Color.h"
#include "Utility.h"
#include <cstdint>

//! A set of points representing Voronoi diagram for a character
//!
//! Has no constructors since it is initialized by memset.
class VoronoiChar {
public:
    // A beetle for a character.
    struct charBeetle {
        uint16_t x:8;
        uint16_t y:7;
        //! 1-->region is white; 0-->region is black
        uint16_t isWhite:1;
    };
    // These dimensions include a 1 pixel border.
    static const int width=37;
    static const int height=51;
    // Number of beetles in the character
    size_t size() const {return mySize;}
    // Maximum number of beetles
    static const size_t maxSize = 25;
    const charBeetle& operator[]( size_t k ) const {
        Assert( k<size() );
        return myArray[k];
    }
    void push_back( int x, int y, bool isWhite ) {
        Assert( char(x)==x );
        Assert( char(y)==y );
        Assert( mySize<maxSize );
        charBeetle& b = myArray[mySize++];
        b.x = x;
        b.y = y;
        b.isWhite = isWhite;
    }
    typedef const charBeetle* const_iterator;
    const_iterator begin() const {return myArray;}
    const_iterator end() const {return myArray+mySize;}
    void adjustBaseline( int delta ) {
        for( int k=0; k<mySize; ++k ) 
            myArray[k].y += delta;
    }
private:
    charBeetle myArray[maxSize];
    char mySize;
};

class VoronoiFont final: BuiltFromResourcePixMap {
public:
    VoronoiFont( const char* resourceName ) : 
        BuiltFromResourcePixMap(resourceName)
    {
    }
    const VoronoiChar& operator[]( int c ) const {
        Assert( 0<=c && c<128 );
        return myCharSet[c];
    }
private:
    static bool isRed( NimblePixel p ) {
        return (p&0xFF0000)>=0xC00000 && (p&0xFF00)<=0x4000 && (p&0xFF) <= 0x40;
    }
    static bool isBlue( NimblePixel p ) {
        return (p&0xFF)>=0xC0 && (p&0xFF0000)<=0x400000 && (p&0xFF00) <= 0x4000;
    }
    // Uses green as a proxy for brightness, since red and blue serve other purposes.
    class brightness {
        int sum;
        int average;
    public:
        brightness() : sum(0), average(0) {}
        void operator+=( NimblePixel p ) {
            sum += p & 0xFF00;
            average += 0x8000;
        }
        bool isBright() const {return sum>=average;}
    };
    void buildFrom( const NimblePixMap& map ) final;
    VoronoiChar myCharSet[128];
};

extern VoronoiFont TheVoronoiFont;

//! Holds text to be drawn as Voronoi diagram.
class VoronoiText {
public:
    VoronoiText();
    ~VoronoiText();
    //! Set line i, position j, to character c
    void setChar(int i, int j, char c);
    //! Set line i to string s.
    void setLine(int i, const char* s);
    //! Return width in characters
    size_t textWidth() const { return myWidth; }
    //! Return height in characters
    int textHeight() const { return myHeight; }
    //! Return width in pixels if scale==1
    int width() const { return myWidth*VoronoiChar::width; }
    //! Return height in pixels if scale==1
    int height() const { return myHeight*VoronoiChar::height; }
    //! Generate ants for *this.
    /** Useful when diplaying text as part of a larger Voronoi diagram */
    Ant* assignAnts(Ant* a, Point upperLeft, float scale=1);
    void drawOn(NimblePixMap& window, const CompoundRegion& region, Point upperLeft, float scale=1);
    void drawOn(NimblePixMap& window, int x, int y, float scale=1, bool compose=false);
    //! Initialize to hold given number of rows and columns of characters.
    void initialize(int rows, int columns);
    //! Initialize with rows=1, columns=strlen(s), and setLine(0,s)
    void initialize(const char* s);
    //! Reset to default-constructed state
    void clear();
    //! Set background and foreground.  
    /** [0] = background, [1] = foreground
        Arguments are not copied - caller must ensure that they remain alive. */
    void bindPalette(ColorStream& background, ColorStream& foreground) {
        myPalette[0] = &background;
        myPalette[1] = &foreground;
    }
    ColorStream& bindForegroundPalette(ColorStream& foreground) {
        ColorStream& old = *myPalette[1];
        myPalette[1] = &foreground;
        return old;
    }
    ColorStream& bindBackgroundPalette(ColorStream& background) {
        ColorStream& old = *myPalette[1];
        myPalette[0] = &background;
        return old;
    }
private:
    //! Width in characters
    int32_t myWidth;
    //! Height in characters
    int32_t myHeight;
    //! Array of text being displayed.  Treated as a matrix.  Not nul terminated.
    char* myText;
    /** Color of the ants */
    NimblePixel* myColor;
    ColorStream* myPalette[2];
    //! Palettes actually used last time characters were set. */
    ColorStream* myOldPalette[2];
    /** Recolor beetles for character at [i][j] */
    void recolor(int i, int j);
    VoronoiText(const VoronoiText&) = delete;
    void operator=(const VoronoiText&) = delete;
};

class VoronoiCounter {
public:
    void initialize( NimblePixMap& window, int width, int height, int initialValue, int upperLimit, int extra, NimbleColor c0, NimbleColor c1 );
    Ant* assignAnts( NimblePixMap& window, CompoundRegion region, Ant* a, int x, int y );
    int operator+=( int addend );
    int value() const {return myValue;}
    int upperLimit() const {return myUpperLimit;}
private:
    BasicArray<Bug> myBug;
    int myValue;
    int myUpperLimit;
    int myExtra;
};

//! Displayed in lower left corner of playing area.
class VoronoiMeter {
    VoronoiText myText;
    VoronoiCounter myLives;     //!< Number of remaining lives
    VoronoiCounter myMissiles;  //!< Number of remaining missiles
    int myScore;                //!< Score
    short myWidth;              //!< Width in pixels
    short myHeight;             //!< Height in pixels
    bool myTextIsOutOfDate;
public:
    VoronoiMeter( int nDigit );

    void initialize( NimblePixMap& window );

    //! Height in pixels
    int height() const {return myHeight;}
    //! Width in pixels
    int width() const {return myWidth;}

    void drawOn( NimblePixMap& window, int x, int y );

    int score() const {return myScore;}
    void addScore( int addend ) {
        myScore = myScore+addend;
        myTextIsOutOfDate = true;
    }
    void multiplyScore( float frac ) {
        myTextIsOutOfDate = true;
        myScore = myScore*frac;
    }

    int lifeCount() const {return myLives.value();}

    void addLife( int delta ) {
        myLives += delta;
    }

    int missileCount() const {return myMissiles.value();}

    void addMissile( int delta ) {
        myMissiles += delta;
    }

    bool reachedMaxMissiles() const {
        return myMissiles.value()>=myMissiles.upperLimit();
    }
};

void InitializeVoronoiText( const NimblePixMap& window );