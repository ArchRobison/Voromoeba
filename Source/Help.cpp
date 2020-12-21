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

#include "Background.h"
#include "NimbleDraw.h"
#include "Help.h"
#include "Region.h"
#include "Widget.h"

static Background HelpBackground;
static ReducedAngle Theta;
static ViewTransform HelpViewTransform;

void Help::initialize( NimblePixMap& window ) {
	Circle windowCircle = Circle(Point(0,0),std::sqrt(Dist2(Center(window))));
    HelpBackground.initialize( window, 200, [&](Point& p)->bool {
        p = windowCircle.radius()*(Point(RandomFloat(2),RandomFloat(2))-Point(1,1));
        return windowCircle.contains(p);
    });
    HelpViewTransform.setScaleAndRotation(1,Theta);
	HelpViewTransform.setOffset(0.5f*Point(window.width(),window.height()));
}

void Help::update( float dt, float deltaTheta ) {
    Theta += deltaTheta;
    HelpViewTransform.setRotation(Theta);
}

static InkOverlay TheHelp("Help");

void Help::draw( NimblePixMap& window ) {
    CompoundRegion region;
    region.buildRectangle( Point(0,0), Point(window.width(),window.height()) ); 
    Ant* a = Ant::openBuffer();
    a = HelpBackground.assignAnts(a,HelpViewTransform);
    Ant::closeBufferAndDraw( window, region, a, true );
    TheHelp.drawOn(window,Max((window.width()-TheHelp.width())/2,0),Max((window.height()-TheHelp.height())/2,0));
}