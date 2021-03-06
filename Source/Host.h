/* Copyright 1996-2021 Arch D. Robison

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

#ifndef Host_H
#define Host_H

//! \file Host.h
//! 
//! OS specific services on the host that are called from OS-independent game code.
//! See Game.h for game routines that are called from host-specific code.

#include <string>

//! Set to 1 if platform supports playing sounds.
#define HAVE_SOUND_OUTPUT 1

//! Set to 1 if platform supports shared application data, e.g. a score file.
#define HAVE_APPLICATION_DATA 1

 //! Return current absolute time in seconds.
 /** Only the difference between two calls are meaningful, because the
     definition of 0 is platform dependent. */
double HostClockTime();

/** 0 = no limit
    1 = one per frame
    2 = every two frames */
void HostSetFrameIntervalRate(int limit);

//! Enumeration of keys corresponding to non-printing characters. 
enum : int32_t {
    HOST_KEY_BACKSPACE=8,
    HOST_KEY_RETURN=0xD,
    HOST_KEY_ESCAPE=0x1B,
    HOST_KEY_DELETE=0x7F,
    HOST_KEY_LEFT = 256,
    HOST_KEY_RIGHT,
    HOST_KEY_UP,
    HOST_KEY_DOWN,
    HOST_KEY_LSHIFT,
    HOST_KEY_RSHIFT,
    HOST_KEY_LAST           // Value for declaring arrays
};

//! Return true if specified key is down, false otherwise.
//!
//! Use one of the HOST_KEY_* enumeration for keys that do not correspond to printable ASCII characters.
//! The value of key should be lowercase if it is alphabetic. 
bool HostIsKeyDown(int key);

//! Request termination of the game.
void HostExit();

class BuiltFromResourcePixMap;

//! Load a bitmap resource.
//!
//! Construct map from resource item.resourceName() and invoke item.buildFrom(map)
void HostLoadResource(BuiltFromResourcePixMap& item);

#if HAVE_SOUND_OUTPUT
class BuiltFromResourceWaveform;

//! Load waveform resource.
//! 
//! Construct waveform from resource item.resourceName().
//! Call item.buildFrom(waveform,size)
void HostLoadResource(BuiltFromResourceWaveform& item);
#endif

#if HAVE_APPLICATION_DATA
//! Get path to application data directory to be shared across multiple users.
std::string HostApplicationDataDir();
#endif

//! Print warning message. 
//!
//! Intended for warning during startup. The message should end in "\n".
void HostWarning(const char* message);

//! Show cursor iff show is true.
void HostShowCursor(bool show);

#endif /* Host_H */