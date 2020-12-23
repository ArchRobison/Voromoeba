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
  OS-independent game code that is called from OS specific services on the host.
 *******************************************************************************/

#ifndef GAME_H
#define GAME_H

#include "NimbleDraw.h"
#include <cstdint>

//! Must be called by host to initialize game for window of given size.
//!
//! Initializes game-specific data structures.
//! Return true if successful, otherwise false. 
bool GameInitialize(int width, int height);

//! Called when user presses keyboard key.
/** See HostIsKeyDown in Host.h for key codes. */
void GameKeyDown(int key);

//! Return null-terminated string with name of game and version.
/** Defined by client. */
const char* GameTitle();

//! Update and/or draw game state, depending on flags set in request.
void GameUpdateDraw(NimblePixMap& map, NimbleRequest request);

//! Called when main window has been resized or moved.
/** map contains the new size and position of the window. */
void GameResizeOrMove(NimblePixMap& map);

//! Default sample rate
constexpr size_t GameSoundSamplesPerSec = 44100;

//! Max possible value of nSamples parameter of GameGetSoundSamples.
constexpr size_t GameGetSoundSamplesMax = 8192;

//! Called by host layer.
void GameGetSoundSamples(float samples[], uint32_t nSamples);

#if HAVE_GAME_FILES

//! File that can be written
class GameOutFile : public OutBitFile {
    HANDLE hFile;
    /*override*/ void write(const void* buffer, size_t nbyte);
public:
    bool open();
    void close();
};

//! File that can be read
class GameInFile : public InBitFile {
    HANDLE hFile;
    /*override*/ size_t read(void* buffer, size_t nbyte);
public:
    bool open();
    void close();
};

#endif /* HAVE_GAME_FILES */

#endif /* GAME_H */