/* Copyright 2011-2021 Arch D. Robison

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

 //! Module for splash screen (top-level menu)
class Splash {
public:
    static void initialize(NimblePixMap& window);
    //! Update splash scren for given time interval and rotation delta.
    static void update(float dt, float deltaTheta);
    static void draw(NimblePixMap& window);
    static void doSelectedAction();
};