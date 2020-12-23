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

#ifndef Finale_H
#define Finale_H

class NimblePixMap;
class Point;
class Ant;

class Finale {
public:
    static void start( const char* reason );
    static bool isRunning() {return myTime>0;}
    static bool pastMourning() {return myTime>=1.0f;}
    static void initialize( NimblePixMap& window );
    static void reset() {myTime=0;}
    static void update( float dt );
    /** The window, p, and q arguments are used to determine where to place the text in the Window.
        Point p should be a point not on land.  
        Point q should be the center of the pond containing p or connected to a bridge containing p. */ 
    static Ant* copyToAnts( Ant* a, NimblePixMap& window, Point p, Point q );
private:
    static float myTime;
};

#endif /* Finale_H */