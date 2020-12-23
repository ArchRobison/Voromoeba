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

#include "About.h"
#include "BuiltFromResource.h"
#include "Config.h"
#include "Finale.h"
#include "Game.h"
#include "Help.h"
#include "Host.h"
#include "Missile.h"
#include "NimbleDraw.h"
#include "Pond.h"
#include "Self.h"
#include "Sound.h"
#include "Splash.h"
#include "Synthesizer.h"
#include "Utility.h"
#include "Vanity.h"
#include "VoronoiText.h"
#include "Widget.h"
#include "World.h"

#include <cstdlib>
#include <cmath>
#include <algorithm>

enum class ShowKind : int8_t {
    splash, // Splash screen
    ponds,  // Ponds, i.e. main game play
    vanity, // Vanity board
    about,  // About the author
    help    // Help
};

static ShowKind ShowWhat = ShowKind::splash;

#if WIZARD_ALLOWED
static constexpr bool IsWizard = true;
#endif

void DoShowAbout() {
    Ant::switchBuffer();
    ShowWhat = ShowKind::about;
}

void DoShowVanity() {
    Ant::switchBuffer();
    ShowWhat = ShowKind::vanity;
}

void DoShowSplash() {
    Ant::switchBuffer();
    ShowWhat = ShowKind::splash;
}

void DoShowHelp() {
    Ant::switchBuffer();
    ShowWhat = ShowKind::help;
}

void EndPlay() {
    ShowAnts = false;
    if (int score = TheScoreMeter.score()) {
        TheVanityBoard.newScore(TheScoreMeter.score());
        if (TheVanityBoard.isEnteringName()) {
            ShowWhat = ShowKind::vanity;
            return;
        }
    }
    ShowWhat = ShowKind::splash;
}

inline bool IsKeyDown(int key0, int key1) {
    return HostIsKeyDown(key0)||HostIsKeyDown(key1);
}

static void UpdateView(NimblePixMap& screen, float dt) {
    const float Pi = 3.1415926535f;
    float torque = 0;
    if (IsKeyDown(HOST_KEY_RIGHT, 'd')) {
        torque = -Pi/5;
    }
    if (IsKeyDown(HOST_KEY_LEFT, 'a')) {
        torque += Pi/5;
    }
    float forward = 0;
    if (IsKeyDown(HOST_KEY_UP, 'w')) {
        forward+=1.0f;
    }
    if (IsKeyDown(HOST_KEY_DOWN, 's')) {
        forward-=1.0f;
    }
    switch (ShowWhat) {
        case ShowKind::ponds:
            World::update(screen, dt, forward, torque);
            break;
        case ShowKind::help:
            Help::update(dt, torque*dt);
            break;
        case ShowKind::splash:
            Splash::update(dt, torque*dt);
            break;
    }
}

static void Update(NimblePixMap& screen) {
    static double T0;
    double t1 = HostClockTime();
    float dt = t1-T0;
    T0 = t1;
    if (dt==t1)
        return;
    if (ShowWhat==ShowKind::ponds) {
    } else {
        // Update slush sounds, otherwise they can be stuck on until the ponds view is shown again.
        UpdateSlush(dt);
    }
    switch (ShowWhat) {
        case ShowKind::ponds:
        case ShowKind::splash:
        case ShowKind::help:
            // These are all views that involve motion control.
            UpdateView(screen, dt);
            break;
        case ShowKind::about:
            About::update(dt);
            break;
    }
}

static bool ShowCharacterSet = false;

static DigitalMeter FrameRateMeter(5, 1);
static bool ShowFrameRate;

static float EstimateFrameRate() {
    static double t0;
    static int count;
    static double estimate;
    ++count;
    double t1 = HostClockTime();
    if (t1-t0>=1.0) {
        estimate = count/(t1-t0);
        t0 = t1;
        count = 0;
    }
    return estimate;
}

static void Draw(NimblePixMap& screen) {
    Ant::clearBuffer();
    switch (ShowWhat) {
        case ShowKind::ponds: {
            extern VoronoiMeter TheScoreMeter;
#if !WIZARD_ALLOWED
            ShowAnts &= TheScoreMeter.score()<20;
#endif
            World::draw(screen);
            TheScoreMeter.drawOn(screen, 0, screen.height()-TheScoreMeter.height());
            break;
        }
        case ShowKind::vanity: {
            TheVanityBoard.draw(screen);
            break;
        }
        case ShowKind::splash: {
            Splash::draw(screen);
            break;
        }
        case ShowKind::about: {
            About::draw(screen);
            break;
        }
        case ShowKind::help: {
            Help::draw(screen);
            break;
        }
    }
    if (ShowFrameRate) {
        FrameRateMeter.setValue(EstimateFrameRate());
        FrameRateMeter.drawOn(screen, 0, 0);
    }
}

static bool InitWorldFlag =false;

void GameUpdateDraw(NimblePixMap& screen, NimbleRequest request) {
    if (InitWorldFlag) {
        Finale::reset();
        World::initialize(screen);
        InitWorldFlag = false;
    }
    if (has(request, NimbleRequest::update)) {
        Update(screen);
    }
    if (has(request, NimbleRequest::draw)) {
#if 0
        // Clear screen - used during development
        screen.draw(NimbleRect(0, 0, screen.width(), screen.height()), NimblePixel(-1));
#endif
        Draw(screen);
    }
}

void GameKeyDown(int key) {
    if (key==HOST_KEY_ESCAPE) {
        HostExit();
        return;
    }
    if (TheVanityBoard.isEnteringName()) {
        Assert(ShowWhat==ShowKind::vanity);
        TheVanityBoard.enterNextCharacterOfName(key);
        return;
    }
    // Canonicalize to lower case
    if ('A'<=key && key<='Z')
        key = key-'A'+'a';
    // Keys whose meaning is independent what is being shown
    switch (key) {
        case 'f':
            ShowFrameRate = !ShowFrameRate;
            break;
        case 'g':
            ShowAnts = !ShowAnts;
            break;
#if WIZARD_ALLOWED
        case 'u': {
            static int frameIntervalRate=1;
            frameIntervalRate=!frameIntervalRate;
            HostSetFrameIntervalRate(frameIntervalRate);
            break;
        }
#endif
#if WIZARD_ALLOWED
#if 0 /* Need different letter */
        case 'g':
            if (IsWizard) {
                static unsigned x = 0;
                switch (x++%3) {
                    case 0:
                        PlaySound(SoundKind::openGate);
                        break;
                    case 1:
                        PlaySound(SoundKind::closeGate);
                        break;
                    case 2:
                        PlaySound(SoundKind::sufferHit);
                        break;
                }
            }
            break;
#endif
#if 0
        case 'i':           // Need different letter
            if (IsWizard) {
                ShowWhat = ShowKind::vanity;
                TheVanityBoard.showTestImage(true);
            }
            break;
#endif
        case 'o': {
            PlaySound(SoundKind::destroyOrange);
            break;
        }
        case 'l': {
            TheScoreMeter.addLife(1);
            PlaySound(SoundKind::smooch);
            break;
        }
        case 'm': {
            TheScoreMeter.addMissile(1);
            break;
        }
#if 0 
        case 't':           // Need different letter
            if (IsWizard) {
                Self.startTipsey();
            }
            break;
#endif
        case 'y': {
            PlaySound(SoundKind::eatOrange);
            break;
        }
        case '1': {
            // For testing end of game
            if (IsWizard) {
                DoShowVanity();
                TheVanityBoard.newScore(rand()%100);
            }
            break;
        }
#endif
    }
    // Keys whose meaning depends on what is being shown
    switch (ShowWhat) {
        case ShowKind::splash:
            switch (key) {
                case HOST_KEY_RETURN:
                case ' ':
                    Splash::doSelectedAction();
                    break;
                case 'i':           // "info"
                    DoShowAbout();
                    break;
                case 'h':           // "help"
                    DoShowHelp();
                    break;
                case 'v':           // "vanity"
                    DoShowVanity();
                    break;
            }
            break;
        case ShowKind::ponds:
            switch (key) {
#if WIZARD_ALLOWED
                case 'j':
                case 'k':
                    if (IsWizard)
                        JumpToPond(key=='j'? 1 : 5);
                    break;
                case 'h':           // Hari-kiri
                    Self.kill();
                    break;
#endif
                case 't':
                    EndPlay();
                    break;
#if WIZARD_ALLOWED
                case '0':
                case '-':
                case '=':
                    if (IsWizard)
                        Zoom(key=='0' ? 0.0f : key=='-' ? 0.5f : 2.0f);
                    break;
#endif
                case ' ': {
                    if (!Finale::isRunning())
                        Missiles::tryFire();
                    else if (Finale::pastMourning())
                        EndPlay();
                    break;
                }
            }
            break;
        case ShowKind::about:
        case ShowKind::vanity:
        case ShowKind::help: {
            switch (key) {
                case ' ':
                case 't':
                    DoShowSplash();
                    break;
            }
            break;
        }
    }
}

void DoStartPlaying() {
    ShowWhat = ShowKind::ponds;
    InitWorldFlag = true;
    Ant::switchBuffer();
}

void GameResizeOrMove(NimblePixMap& window) {
    InitializeVoronoiText(window);
    Splash::initialize(window);
    InitializeVanity();
    About::initialize(window);
    Help::initialize(window);
    Finale::initialize(window);
}

const char* GameTitle() {
    return "Voromoeba 1.2"
#if ASSERTIONS
        " ASSERTIONS"
#endif
        ;
}

bool GameInitialize(int /*width*/, int /*height*/) {
    BuiltFromResource::loadAll();
    ConstructSounds();
    return true;
}

namespace {

const size_t N_OutputChannel = 2;

typedef float Accumulator[N_OutputChannel][GameGetSoundSamplesMax/N_OutputChannel];

//! Convert floating-point samples in src[0:N_Channel-1][0:n-1] to interleaved samples in dst[0:n-1],
//! and clear src.
void ConvertAccumulatorToSamples(float dst[], Accumulator& src, uint32_t n) {
    float* d = dst;
    for (uint32_t i=0; i<n; ++i)
        for (uint32_t j=0; j<N_OutputChannel; ++j) {
            *d++ = src[j][i];
            src[j][i] = 0;
        }
}

} // (anonymous)

void GameGetSoundSamples(float* samples, uint32_t nSamples) {
    Assert(nSamples % 2 == 0);
    Assert(nSamples <= GameGetSoundSamplesMax);
    const uint32_t n = nSamples / 2;
    static Accumulator temp;
    Synthesizer::OutputInterruptHandler(temp[0], temp[1], n);
    ConvertAccumulatorToSamples(samples, temp, n);
}