/* Copyright 1996-2020 Arch D. Robison 

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
 Configuration parameters for Voromoeba
*******************************************************************************/

#ifndef Config_H
#define Config_H

//! Minimum display width
const int DISPLAY_WIDTH_MIN = 1024;

//! Minimum display height
const int DISPLAY_HEIGHT_MIN = 600;

//! True if wizard mode allowed
#ifndef WIZARD_ALLOWED
#define WIZARD_ALLOWED 1
#endif

#endif /*Config_H*/