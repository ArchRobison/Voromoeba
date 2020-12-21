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

#ifndef Missile_H
#define Missile_H

class Ant;
class Beetle;
class NimblePixMap;

//! Module for operations on missiles.
class Missiles {
public:
    static void initialize( NimblePixMap& window );
    static void update( float dt );
    static void tryFire();
    static void tallyHit( Beetle& b );
    //! Fill Ant buffer with Ants for any Missiles in ponds [firstPond,lastPond)
    static Ant* assignAnts( Ant* a, size_t firstPond, size_t lastPond );
};

#endif /* Missile_H */