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

#include "Config.h"
#include "Pond.h"
#include "Splash.h"
#include "Missile.h"
#include "World.h"
#include "Host.h"
#include "NimbleDraw.h"
#include "About.h"
#include "BuiltFromResource.h"
#include "Finale.h"
#include "Help.h"
#include "Self.h"
#include "Sound.h"
#include "Utility.h"
#include "Vanity.h"
#include "VoronoiText.h"
#include "Widget.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

enum ShowKind {
    ShowSplash,
    ShowPonds,
    ShowVanity,
    ShowAbout,
    ShowHelp
};

static ShowKind ShowWhat = ShowSplash;

#if WIZARD_ALLOWED
static const bool IsWizard = true;
#endif

void DoShowAbout() {
    Ant::switchBuffer();
    ShowWhat = ShowAbout;
}

void DoShowVanity() {
    Ant::switchBuffer();
    ShowWhat = ShowVanity;
}

void DoShowSplash() {
    Ant::switchBuffer();
    ShowWhat = ShowSplash;
}

void DoShowHelp() {
    Ant::switchBuffer();
    ShowWhat = ShowHelp;
}

void EndPlay() {
    ShowAnts = false;
    if( int score = TheScoreMeter.score() ) {
        TheVanityBoard.newScore(TheScoreMeter.score());
        if( TheVanityBoard.isEnteringName() ) {
            ShowWhat = ShowVanity;
            return;
        }
    }
    ShowWhat = ShowSplash;
}

inline bool IsKeyDown( int key0, int key1 ) {
    return HostIsKeyDown(key0)||HostIsKeyDown(key1);
}

static void UpdateView( NimblePixMap& screen, float dt ) {
    const float Pi = 3.1415926535f;
    float torque = 0;
    if( IsKeyDown( HOST_KEY_RIGHT, 'd') ) {
        torque = -Pi/5;
    } 
    if( IsKeyDown( HOST_KEY_LEFT, 'a' ) ) {
        torque += Pi/5;
    }
    float forward = 0;
    if( IsKeyDown( HOST_KEY_UP,'w' ) ) {
        forward+=1.0f;
    } 
    if( IsKeyDown( HOST_KEY_DOWN, 's' ) ) {
        forward-=1.0f;
    }
    switch( ShowWhat ) {
        case ShowPonds:
            World::update( screen, dt, forward, torque );
            break;
        case ShowHelp:
            Help::update(dt,torque*dt);
            break;
        case ShowSplash:
            Splash::update(dt,torque*dt);
            break;
    }
}

static void Update( NimblePixMap& screen ) {
    static double T0;
    double t1 = HostClockTime();
    float dt = t1-T0;
    T0 = t1;
    if( dt==t1 ) 
        return;
    if( ShowWhat==ShowPonds ) {
    } else {
        // Update slush sounds, otherwise they can be stuck on until the ponds view is shown again.
        UpdateSlush(dt);
    }
    switch( ShowWhat ) {
        case ShowPonds: 
        case ShowSplash:
        case ShowHelp:
            // These are all views that involve motion control.
            UpdateView( screen, dt );
            break;
        case ShowAbout:
            About::update( dt );
            break;
    }
}

static bool ShowCharacterSet = false;

static DigitalMeter FrameRateMeter(5,1);
static bool ShowFrameRate;

static float EstimateFrameRate() {
    static double t0;
    static int count;
    static double estimate;
    ++count;
    double t1 = HostClockTime();
    if( t1-t0>=1.0 ) {
        estimate = count/(t1-t0);
        t0 = t1;
        count = 0;
    } 
    return estimate;
}

static void Draw( NimblePixMap& screen ) {
    Ant::clearBuffer();
    switch( ShowWhat ) {
        case ShowPonds: {
            extern VoronoiMeter TheScoreMeter;
#if !WIZARD_ALLOWED
            ShowAnts &= TheScoreMeter.score()<20;
#endif
            World::draw(screen);
            TheScoreMeter.drawOn( screen, 0, screen.height()-TheScoreMeter.height() );
            break;
        }
        case ShowVanity: {
            TheVanityBoard.draw(screen);
            break;
        }
        case ShowSplash: {
            Splash::draw(screen);
            break;
        }
        case ShowAbout: {
            About::draw(screen);
            break;
        }
        case ShowHelp: {
            Help::draw(screen);
            break;
        }
    }
    if( ShowFrameRate ) {
        FrameRateMeter.setValue( EstimateFrameRate() );
        FrameRateMeter.drawOn( screen, 0, 0 ); 
    }
}

static bool InitWorldFlag =false;

void GameUpdateDraw( NimblePixMap& screen, NimbleRequest request ) {
    if( InitWorldFlag ) { 
        Finale::reset(); 
        World::initialize( screen );
        InitWorldFlag = false;
    }
    if( request &  NimbleUpdate ) {
        Update(screen);
    }
    if( request&NimbleDraw ) {
#if 0
        // Clear screen - used during development
        screen.draw( NimbleRect(0,0,screen.width(),screen.height()), NimblePixel(-1));
#endif
        Draw(screen);
    }
}

void GameMouseMove( const NimblePoint& point ) {
    // FIXME
}

void GameMouseButtonDown( const NimblePoint& point, int k ) {
    // FIXME
}

void GameMouseButtonUp( const NimblePoint& point, int k ) {
    // FIXME
}

void GameKeyDown( int key ) {
    if( key==HOST_KEY_ESCAPE ) {
        HostExit();
        return;
    }
    if( TheVanityBoard.isEnteringName() ) {
        Assert( ShowWhat==ShowVanity );
        TheVanityBoard.enterNextCharacterOfName( key );
        return;
    }
    // Canonicalize to lower case
    if( 'A'<=key && key<='Z' ) 
        key = key-'A'+'a';
    // Keys whose meaning is independent what is being shown
    switch( key ) {
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
            if( IsWizard ) {
                static unsigned x = 0;
                switch( x++%3 ) {
                case 0: 
                    PlaySound( SOUND_OPEN_GATE );
                    break;
                case 1:
                    PlaySound( SOUND_CLOSE_GATE );
                    break;
                case 2:
                    PlaySound( SOUND_SUFFERED_HIT );
                    break;
                }
            }
            break;
#endif
#if 0
        case 'i':           // Need different letter
            if( IsWizard ) {
                ShowWhat = ShowVanity;
                TheVanityBoard.showTestImage(true);
            }
            break;
#endif
        case 'o': {
            PlaySound(SOUND_DESTROY_ORANGE);
            break;
        }
        case 'l': {
            TheScoreMeter.addLife(1);
            PlaySound(SOUND_SMOOCH);
            break;
        }
        case 'm': {
            TheScoreMeter.addMissile(1);
            break;
        }
#if 0 
        case 't':           // Need different letter
            if( IsWizard ) {
                Self.startTipsey(); 
            }
            break;
#endif
        case 'y': {
            PlaySound(SOUND_EAT_ORANGE);
            break;
        }
        case '1': {
            // For testing end of game
            if( IsWizard ) {
                DoShowVanity();
                TheVanityBoard.newScore(rand()%100);
            }
            break;
        }
#endif
    }
    // Keys whose meaning depends on what is being shown
    switch( ShowWhat ) {
        case ShowSplash:
            switch( key ) {
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
        case ShowPonds:
            switch( key ) {
#if WIZARD_ALLOWED
                case 'j':
                case 'k':
                    if( IsWizard )
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
                    if( IsWizard )
                        Zoom( key=='0' ? 0.0f : key=='-' ? 0.5f : 2.0f );
                    break;
#endif
                case ' ': {
                    if( !Finale::isRunning() )
                        Missiles::tryFire();
                    else if( Finale::pastMourning() )
                        EndPlay();
                    break;
                }
            }
            break;
        case ShowAbout: 
        case ShowVanity: 
        case ShowHelp: {
            switch( key ) {
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
    ShowWhat = ShowPonds;
    InitWorldFlag = true;
    Ant::switchBuffer();
}

void GameResizeOrMove( NimblePixMap& window ) {
    InitializeVoronoiText( window );
    Splash::initialize( window );
    InitializeVanity();
    About::initialize( window );
    Help::initialize( window );
    Finale::initialize( window );
    HostShowCursor(false);
}

const char* GameTitle() {
    return "Voromoeba 1.2"
#if ASSERTIONS
           " ASSERTIONS"
#endif
    ;
}

bool GameInitialize() {
    BuiltFromResource::loadAll();
    ConstructSounds();
    return true;
}
