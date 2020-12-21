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
#include "VoronoiText.h"
#include "Region.h"
#include "Splash.h"
#include "Host.h"
#include "Background.h"
#include <cstring>

static const size_t N_Button = 6;

// In counter-clockwise order
static const char* ButtonName[N_Button] = {"Play","Exit","Scores","About","Help","Voromoeba"};
static Circle ButtonCircle[N_Button];

static VoronoiText ButtonText[N_Button];
static Background SplashBackground;
static ViewTransform SplashViewTransform;

// Always between 0 and N_Button-1
static int SelectedButtonIndex = -1;
static ReducedAngle Theta;
static ColorStream SelectedColor, *OldColor;

void Splash::update( float dt, float deltaTheta ) {
    Theta += deltaTheta;
    float pi = 3.1415926535;
    SplashViewTransform.setRotation(Theta);
    float omega = pi/(N_Button-1);
    int j = Round((Theta+2*pi)/omega) % (2*(N_Button-1));
    Assert( 0<=j && j<2*(N_Button-1) );
    int k = j&1 ? -1 : j/2;
    if( k!=SelectedButtonIndex ) {
        if( SelectedButtonIndex!=-1 )
            ButtonText[SelectedButtonIndex].bindForegroundPalette(*OldColor);
        SelectedButtonIndex = k;
        if( SelectedButtonIndex!=-1 )
            ButtonText[SelectedButtonIndex].bindForegroundPalette(SelectedColor);
    }
}

void Splash::doSelectedAction() {
    extern void DoStartPlaying();
    extern void DoShowVanity();
    extern void DoShowAbout();
    extern void DoShowHelp();
    switch( SelectedButtonIndex ) {
        case 0:
            DoStartPlaying();
            break;
        case 1:
            HostExit();
            break;
        case 2:
            DoShowVanity();
            break;
        case 3:
            DoShowAbout();
            break;
        case 4:
            DoShowHelp();
            break;
        default:
            break;
    }
}

void Splash::initialize( NimblePixMap& window ) {
    SelectedColor.initialize( window, NimbleColor(255,0,0), NimbleColor(255,0,255));
    float pi = 3.1415926545;
    float omega = 2*pi/(N_Button-1);
	float radius = Min(window.width(),window.height())*0.4f;
	float buttonArea = 0;
    for( size_t k=0; k<N_Button; ++k ) {
        if( k==0 ) {
            OldColor = &ButtonText[0].bindForegroundPalette(SelectedColor);
        }
        ButtonText[k].initialize(ButtonName[k]);
		Point center = k==N_Button-1 ? Point(0,0) : Polar(radius,-0.5*pi - k*omega);
		float textRadius = std::sqrt(Dist2(ButtonText[k].width(),ButtonText[k].height()))/2;
		ButtonCircle[k] = Circle(center,textRadius);
		buttonArea += ButtonCircle[k].area();
    }
	Circle windowCircle = Circle(Point(0,0),std::sqrt(Dist2(Center(window))));
	float windowArea = windowCircle.area();
	size_t m = (windowArea/buttonArea-1)*25*32/8;
	if( m>2000 ) m=2000;
    SplashBackground.initialize( window, m, [&](Point& p) -> bool {
        p = windowCircle.radius()*(Point(RandomFloat(2),RandomFloat(2))-Point(1,1));
        return windowCircle.contains(p);
    });
    SplashViewTransform.setScaleAndRotation(1,Theta);
	SplashViewTransform.setOffset(0.5f*Point(window.width(),window.height()));
}

void Splash::draw( NimblePixMap& window ) {
    Ant* a = Ant::openBuffer();
    // FIXME - add shorthand constructor to CompoundRegion for rectangular regions.
    CompoundRegion region;
    region.buildRectangle( Point(0,0), Point(window.width(),window.height()) );
    for( size_t k=0; k<N_Button; ++k ) {
		VoronoiText& b = ButtonText[k];
        Point p = SplashViewTransform.transform(ButtonCircle[k].center()) - Center(b);
        a = ButtonText[k].copyToAnts( a, p );
	}
	a = SplashBackground.copyToAnts(a,SplashViewTransform);
    Ant::closeBufferAndDraw( window, region, a, true );
#if 0
	// Code for showing centers
	for( size_t k=0; k<N_Button; ++k ) {
		Point p = SplashViewTransform.transform(ButtonCircle[k].center());
		int x = Round(p.x);
		int y = Round(p.y);
		window.draw(NimbleRect(x,y,x+1,y+1),~0u);
	}
#endif
}