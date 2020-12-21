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

#include "Finale.h"
#include "Self.h"
#include "VoronoiText.h"
#include "World.h"

static VoronoiText FinaleText;
static ColorStream DarkBackground;
float Finale::myTime;

void Finale::start( const char* reason ) {
    FinaleText.initialize(reason);
    if( World::isInDarkPond( Self ) )
        FinaleText.bindBackgroundPalette(DarkBackground);
    myTime = 0.0001f;
}

void Finale::initialize( NimblePixMap& window )  {
    DarkBackground.initialize( window, NimbleColor(0,0,0), NimbleColor(0,0,0));
}

void Finale::update( float dt ) {
    if( isRunning() ) {
        myTime += dt;
        if( myTime >= 10.f ) {
            extern void EndPlay();
            EndPlay();
        }
    }
}

Ant* Finale::copyToAnts( Ant* a, NimblePixMap& window, Point p, Point q ) {
    Parallelogram r(Point(0,0),Point(window.width(),0),Point(window.width(),window.height()));
    p = World::viewTransform.transform(p);
    Assert( r.contains(p) );
    q = World::viewTransform.transform(q);
    Point v = q-p;
    if( Dist2(v)==0 ) 
        v = Point(0,0.001f);
    Point o = p+r.intercept( p, v )*0.5f*v;
    return FinaleText.copyToAnts(a,o-0.5f*Point(FinaleText.width(),FinaleText.height()));
}