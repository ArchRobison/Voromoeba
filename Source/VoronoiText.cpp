#include "Region.h"
#include "VoronoiText.h"
#include "Beetle.h"
#include "Host.h"
#include <cmath>
#include <cstring>

void VoronoiFont::buildFrom( const NimblePixMap& map ) {
    Assert( map.width()==16*VoronoiChar::width );
    Assert( map.height()==8*VoronoiChar::height );
    std::memset( myCharSet, 0, sizeof(myCharSet) );
    int defaultBaseline = -1;
    for( unsigned char k=0; k<128; ++k ) {
        int charBaseline = -1;
        int row = k/16;
        int col = k%16;
        VoronoiChar& c = myCharSet[k];
        for( int i=0; i<VoronoiChar::height; ++i ) {
            for( int j=0; j<VoronoiChar::width; ++j ) {
                int x = col*VoronoiChar::width+j;
                int y = row*VoronoiChar::height+i;
                NimblePixel p = map.pixelAt(x,y);
                if( isBlue(p) ) {
                    // If there is character baseline, it must be consistent across the character
                    Assert( charBaseline==-1 || charBaseline==i );
                    charBaseline = i;
                }
                if( isRed(p) ) {
                    brightness sum ;
                    if( j>0 ) sum += map.pixelAt(x-1,y);
                    if( i>0 ) sum += map.pixelAt(x,y-1);
                    if( j+1<VoronoiChar::width ) sum += map.pixelAt(x+1,y);
                    if( i+1<VoronoiChar::height ) sum += map.pixelAt(x,y+1);
                    c.push_back(j,i,sum.isBright());
                }
            }
        }
        if( charBaseline!=-1 ) {
            if( defaultBaseline==-1 ) {
                // First baseline that we've seen.  Make it the default.
                defaultBaseline = charBaseline;
            } else if( charBaseline!=defaultBaseline ) {
                // Character's baseline differs from default baseline.
                c.adjustBaseline(defaultBaseline-charBaseline);
            }
        }
    }
}

VoronoiFont TheVoronoiFont("VoronoiFont");

static ColorStream DefaultBackground, DefaultForeground;

void InitializeVoronoiText( const NimblePixMap& window ) {
    DefaultBackground.initialize( window, NimbleColor(0,0,255), NimbleColor(0,0,128) );
    DefaultForeground.initialize( window, NimbleColor(255,255,0), NimbleColor(255,128,0) );
}

VoronoiText::VoronoiText() : myWidth(0), myHeight(0), myText(NULL), myColor(NULL) 
{
   bindPalette( DefaultBackground, DefaultForeground );
   myOldPalette[0] = NULL;
   myOldPalette[1] = NULL;
}

void VoronoiText::clear() {
    delete[] myText;
    delete[] myColor;
    myText = NULL;
    myColor = NULL;
    myWidth = 0;
    myHeight = 0;
    bindPalette( DefaultBackground, DefaultForeground );
    myOldPalette[0] = NULL;
    myOldPalette[1] = NULL;
}

void VoronoiText::initialize(int32_t height, int32_t width) {
    Assert(height >= 0);
    Assert(width >= 0);
    clear();
    myWidth = width;
    myHeight = height;
    size_t n = height*width;
    myText = new char[n];
    memset(myText, 0, n);
    size_t m = textHeight()*textWidth()*VoronoiChar::maxSize;
    Assert(!myColor);
    myColor = new NimblePixel[m];
}

void VoronoiText::initialize(const char* s) {
    // Compute the dimensions of the block of text
    int32_t rows = 0;
    int32_t cols = 0;
    int32_t currentLineLength = 0;
    for (const char* t=s; ; ++t) {
        if (*t=='\n' || *t=='\0') {
            if (currentLineLength>cols) cols=currentLineLength;
            currentLineLength=0;
            ++rows;
            if (*t==0) break;
        } else {
            ++currentLineLength;
        }
    }
    initialize(rows, cols);
    int32_t i = 0;
    int32_t j = 0;
    for (const char* t=s; ; ++t) {
        if (*t=='\n' || *t=='\0') {
            for (; j<textWidth(); ++j)
                setChar(i, j, ' ');
            if (*t==0) break;
            ++i;
            j = 0;
        } else {
            setChar(i, j, *t);
            ++j;
        }
    }
    setLine(0, s);
}

VoronoiText::~VoronoiText() {
    clear();
}

void VoronoiText::recolor( int i, int j ) {
    Assert(0<=i && i<myHeight);
    Assert(0<=j && j<myWidth);
    Assert( myPalette[0] );
    Assert( myPalette[1] );
    size_t m = i*myWidth+j;
    char c = myText[m];
    Assert(0<=c && c<128);
    const VoronoiChar& v = TheVoronoiFont[c];
    ColorStream::seedType s[2] = {0,0};
    NimblePixel* p = &myColor[m*VoronoiChar::maxSize];
    for( VoronoiChar::const_iterator c=v.begin(); c!=v.end(); ++c ) {
        size_t k = c->isWhite ? 1 : 0;
        *p++ = myPalette[k]->get(s[k]); 
    }
}

void VoronoiText::setChar( int i, int j, char c ) {
    Assert(0<=i && i<myHeight);
    Assert(0<=j && j<myWidth);
    Assert(0<=c && c<128);
    size_t m = i*myWidth+j;
    myText[m] = c==' ' ? (i^j)%7 : c;
    recolor( i, j );
}

void VoronoiText::setLine( int i, const char* s ) {
    for( int j=0; j<myWidth; ++j ) 
        setChar( i, j,*s ? *s++ : ' ' );
}

Ant* VoronoiText::assignAnts( Ant* a, Point upperLeft, float scale ) {
    if( myOldPalette[0]!=myPalette[0] || myOldPalette[1]!=myPalette[1] ) {
        for( int i=0; i<myHeight; ++i )
            for( int j=0; j<myWidth; ++j ) 
                recolor(i,j);
        myOldPalette[0] = myPalette[0];
        myOldPalette[1] = myPalette[1];
    }
    // Assertion checks that method initialize() was called.
    Assert( myColor );
    const double fundamentalPeriod = 90; // Units = seconds
    float omega = fmod(HostClockTime(),fundamentalPeriod)*(2*3.1415926535/fundamentalPeriod);
    for( int i=0; i<myHeight; ++i )
        for( int j=0; j<myWidth; ++j ) {
            int dx = j*VoronoiChar::width;
            int dy = i*VoronoiChar::height;
            size_t k = i*myWidth+j;
            const VoronoiChar& v = TheVoronoiFont[myText[k]];
            size_t index = k*VoronoiChar::maxSize;
            for( VoronoiChar::const_iterator c=v.begin(); c!=v.end(); ++c, ++index ) {
                int x = c->x + dx;
                int y = c->y + dy;
                float theta = (16+index%16)*omega;
                float wobble = 0.5f;
                Point p( x+wobble*cos(theta), y+wobble*sin(theta) );
                (a++)->assign(scale*p+upperLeft,myColor[index]);
            }
        }
    return a;
}

void VoronoiText::drawOn( NimblePixMap& window, const CompoundRegion& region, Point upperLeft, float scale ) {
    Ant* a = Ant::openBuffer();
    a = assignAnts( a, upperLeft, scale );
    Ant::closeBufferAndDraw( window, region, a, false );
}

void VoronoiText::drawOn( NimblePixMap& window, int x, int y, float scale, bool compose ) {
    CompoundRegion region;
    region.buildRectangle(Point(0,0),Point(window.width(),window.height()));
    Assert( region.assertOkay() );
    Ant* a = Ant::openBuffer();
    a = assignAnts( a, Point(x,y), scale );
    Assert( region.assertOkay() );
    Ant::closeBufferAndDraw( window, region, a, compose );
}

void VoronoiCounter::initialize( NimblePixMap& window, int width, int height, int initialValue, int upperLimit, int extra, NimbleColor c0, NimbleColor c1 ) {
    Assert( 0<=initialValue );
    Assert( initialValue<=upperLimit );
    Assert( 1<=upperLimit );
    Assert( 0<=extra );
    myValue = initialValue;
    myUpperLimit = upperLimit;
    myExtra = extra;
    int n = myExtra+myUpperLimit;
    myBug.reserve(n);
    float j = 0;
    for( int k=0; k<n; ++k ) {
        auto& b = myBug[k];
        float x = RandomFloat(1);
        float y = RandomFloat(1);
        if( x>y ) Swap(x,y);
        b.pos = Point(x*width,y*height);
        NimbleColor c;
        if( k<myExtra ) {
            float value = float(k)/(myExtra-1);
            c = NimbleColor(NimbleColor::FULL*value*0.5f);
        } else {
            float f = float(k-myExtra)/(myUpperLimit-1);
            c = c0;
            c.mix(c1,f);
        }
        myBug[k].color = window.pixel(c);
    }
    myBug.resize(myExtra+myValue);
}

int VoronoiCounter::operator+=( int addend ) {
    myValue = Clip(0,myUpperLimit,myValue+addend);
    myBug.resize(myExtra+myValue);
    return myValue;
}

Ant* VoronoiCounter::assignAnts( NimblePixMap& window, CompoundRegion region, Ant* a, int x_, int y_ ) {
    for( const auto* b = myBug.begin(); b!=myBug.end(); ++b, ++a ) 
        a->assign( b->pos+Point(x_,y_), b->color );
    return a;
}

VoronoiMeter::VoronoiMeter( int nDigit ) {
    myWidth = nDigit*VoronoiChar::width*2;
    myHeight = VoronoiChar::height*2;
    myScore = 0;
    myTextIsOutOfDate = true;
}

static ColorStream MeterBackgroundPalette, MeterForegroundPalette;

void VoronoiMeter::initialize( NimblePixMap& window ) {
    MeterBackgroundPalette.initialize( window, NimbleColor(0,0,0), NimbleColor(64) );
    MeterForegroundPalette.initialize( window, NimbleColor(255,255,0), NimbleColor(255,128,0) );
    myText.bindPalette( MeterBackgroundPalette, MeterForegroundPalette );
    myText.initialize(1,myWidth/(VoronoiChar::width*2));
    myLives.initialize( window, myWidth/2, myHeight/2, 0, 10, 3, NimbleColor(255,128,0), NimbleColor(255,255,0) );
    myMissiles.initialize( window, myWidth/2, myHeight/2, 2, 12, 5, NimbleColor(128,0,128), NimbleColor(255,0,255) );
    myScore = 0;
}

void VoronoiMeter::drawOn( NimblePixMap& window, int x, int y ) {
    SetRegionClip(0,0,window.width(),window.height());
    ConvexRegion r;
    r.makeParallelogram( Point(0, window.height()), Point(0,window.height()-height()), Point(width(),window.height()));
    CompoundRegion region;
    region.build( &r, &r+1 );
    if( myTextIsOutOfDate ) {
        int j = myText.textWidth();
        int k = myScore;
        Assert(myScore>=0); // Negative numbers not yet implemented
        while( k>0 && j>0 ) {
            int d = k%10;
            k/=10;
            --j;
            myText.setChar(0,j,'0'+d);
        }
        while( j>0 ) {
            --j;
            myText.setChar(0,j,j+1==myText.textWidth()?'0':' ');
        }
        myTextIsOutOfDate = false;
    }

    Ant* a = Ant::openBuffer();
    a = myText.assignAnts( a, Point(x, y+height()/2) );
    a = myLives.assignAnts( window, region, a, x, y );
    a = myMissiles.assignAnts( window, region, a, x+width()/2, y+height()/2 );
    Ant::closeBufferAndDraw( window, region, a, false );
}