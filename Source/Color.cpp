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

#include "Color.h"

float ColorWobble::omega;

void ColorGradient::initialize( const NimblePixMap& window, NimbleColor c0, NimbleColor c1 ) {
    Assert( size>1 );
    for( size_t k=0; k<size; ++k ) {
        NimbleColor c2 = c0;
        c2.mix(c1,float(k)/(size-1));
        color[k] = window.pixel(c2);
    }
}

void ColorWobble::updateTime( float dt ) {
    static const unsigned scale = 1<<24;
    static unsigned time;
    time += dt*scale;
    // Convert to 2pi periodicity
    omega = (3.1415926535/(1u<<31))*time;
}
