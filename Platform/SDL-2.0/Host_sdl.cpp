/* Copyright 2014-2020 Arch D. Robison

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

#include "SDL.h"
#include "SDL_image.h"
#include "Host.h"
#include "Game.h"
#include "BuiltFromResource.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

static SDL_PixelFormat* ScreenFormat;

#ifdef __APPLE__
// MacOS/SDL-2 have a texture synchronization bug on MacBook Airs running MacOS 10.11.1.  
// The code works around the bug by manual double-buffering.
const int N_TEXTURE = 2;
#else
const int N_TEXTURE = 1;
#endif

static void ReportResourceError(const char* routine, const char* resourceName, const char* error) {
    std::printf("Internal error: %s failed %s: %s\n", routine, resourceName, error);
    HostExit();
}

void HostLoadResource(BuiltFromResourcePixMap& item) {
#if defined(HOST_RESOURCE_PATH)
    std::string path(HOST_RESOURCE_PATH);
#else
    std::string path("../../../Resource");
#endif
    path = path + "/" + item.resourceName() + ".png";
    if (SDL_Surface* raw = IMG_Load(path.c_str())) {
        if (SDL_Surface* image = SDL_ConvertSurface(raw, ScreenFormat, 0)) {
            SDL_FreeSurface(raw);
            if (SDL_LockSurface(image)==0) {
                NimblePixMap map(image->w, image->h, 8*sizeof(NimblePixel), image->pixels, image->pitch);
                item.buildFrom(map);
                SDL_UnlockSurface(image);
            } else {
                ReportResourceError("SDL_LockSurface", item.resourceName(), SDL_GetError());
            }
            SDL_FreeSurface(image);
        } else {
            ReportResourceError("SDL_ConvertSurface", item.resourceName(), SDL_GetError());
        }
    } else {
#ifdef _WIN32
        char* cwd = _getcwd(nullptr, 0);
#else
        char* cwd = getcwd(nullptr, 0);
#endif /*__APPLE__*/
        std::printf("cwd = %s\n", cwd);
        std::free(cwd);
        ReportResourceError("IMG_Load", path.c_str(), IMG_GetError());
    }
}

void HostLoadResource(BuiltFromResourceWaveform& item) {
#if defined(HOST_RESOURCE_PATH)
    std::string path(HOST_RESOURCE_PATH);
#else
    std::string path("../../../Resource");
#endif
    path = path + "/" + item.resourceName() + ".wav";
    FILE* f = std::fopen(path.c_str(), "rb");
    Assert(f); // FIXME - should issue error message
    std::fseek(f, 0L, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0L, SEEK_SET);
    std::vector<char> buffer(size);
    const size_t n = std::fread(buffer.data(), 1, buffer.size(), f);
    Assert(n == size); // FIXME - should issue error message
    std::fclose(f);
    item.buildFrom(buffer.data(), size);
}

double HostClockTime() {
    return SDL_GetTicks()*0.001;
}

static float BusyFrac;

float HostBusyFrac() {
    return BusyFrac;
}

static int NewFrameIntervalRate=1, OldFrameIntervalRate=-1;

void HostSetFrameIntervalRate(int limit) {
    NewFrameIntervalRate = limit;
}

static bool Quit;
static bool Resize = true;

void HostExit() {
    Quit = true;
}

// [i] has SDL_Scancode corresponding to HOST_KEY_...
static unsigned short ScanCodeFromHostKey[HOST_KEY_LAST];

// [i] has HOST_KEY_... corresponding to SDL_ScanCode
static unsigned short HostKeyFromScanCode[SDL_NUM_SCANCODES];

inline void Associate(SDL_Scancode code, int key) {
    Assert(unsigned(code)<SDL_NUM_SCANCODES);
    Assert(unsigned(key)<HOST_KEY_LAST);
    ScanCodeFromHostKey[key] = code;
    HostKeyFromScanCode[code] =key;
}

bool HostIsKeyDown(int key) {
    static int numKeys;
    static const Uint8* state = SDL_GetKeyboardState(&numKeys);
    int i = ScanCodeFromHostKey[key];
    return i<numKeys ? state[i] : false;
}

static void InitializeKeyTranslationTables() {
#if 0
    for (int i=' '; i<='@'; ++i)
        KeyTranslate[i] = i;
#endif
    Associate(SDL_SCANCODE_SPACE, ' ');
    for (int i='a'; i<='z'; ++i)
        Associate(SDL_Scancode(SDL_SCANCODE_A+(i-'a')), i);
    Associate(SDL_SCANCODE_RETURN, HOST_KEY_RETURN);
    Associate(SDL_SCANCODE_ESCAPE, HOST_KEY_ESCAPE);
    Associate(SDL_SCANCODE_LEFT, HOST_KEY_LEFT);
    Associate(SDL_SCANCODE_RIGHT, HOST_KEY_RIGHT);
    Associate(SDL_SCANCODE_UP, HOST_KEY_UP);
    Associate(SDL_SCANCODE_DOWN, HOST_KEY_DOWN);
}

static void PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            /* Keyboard event */
            /* Pass the event data onto PrintKeyInfo() */
            case SDL_KEYDOWN:
                /*case SDL_KEYUP:*/
                GameKeyDown(HostKeyFromScanCode[event.key.keysym.scancode]);
                break;

            case SDL_QUIT:
                Quit = true;
                break;

            default:
                break;
        }
    }
}

static void CallbackAdaptor(void* userdata, Uint8* stream, int len) {
    Assert(len % (2*sizeof(float)) == 0);
    GameGetSoundSamples((float*)stream, len / sizeof(float));
}

static void InitializeSound() {
    // Install audio callback
    SDL_AudioSpec spec{};
    spec.freq =  GameSoundSamplesPerSec;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 4096;
    spec.callback = CallbackAdaptor;
    SDL_AudioSpec obtained;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &spec, &obtained, /*allowed_changes=*/SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (dev == 0) {
        const char* msg = SDL_GetError();
        fprintf(stderr, "SDL_OpenAudioDevice: %s\n", msg);
    }
    Assert(dev >= 2);
    // Start playing
    SDL_PauseAudioDevice(dev, 0);
}

static const bool UseRendererForUnlimitedRate = true;

//! Destroy renderer and texture, then recreate them if they are to be used.  Return true if success; false if error occurs.
static bool RebuildRendererAndTexture(SDL_Window* window, int w, int h, SDL_Renderer*& renderer, SDL_Texture* texture[N_TEXTURE]) {
    for (int i=0; i<N_TEXTURE; ++i)
        if (texture[i]) {
            SDL_DestroyTexture(texture[i]);
            texture[i] = nullptr;
        }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (UseRendererForUnlimitedRate || NewFrameIntervalRate>0) {
        Uint32 flags = SDL_RENDERER_ACCELERATED;
        if (NewFrameIntervalRate>0)
            flags |= SDL_RENDERER_PRESENTVSYNC;
        renderer = SDL_CreateRenderer(window, -1, flags);
        if (!renderer) {
            printf("Internal error: SDL_CreateRenderer failed: %s\n", SDL_GetError());
            return false;
        }
        for (int i=0; i<N_TEXTURE; ++i) {
            texture[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!texture[i]) {
                printf("Internal error: SDL_CreateRenderer failed: %s\n", SDL_GetError());
                return false;
            }
        }
    }
    OldFrameIntervalRate = NewFrameIntervalRate;
    return true;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1) {
        printf("Internal error: SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode)) {
        printf("Internal error: SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        exit(1);
    }
    int w = displayMode.w;
    int h = displayMode.h;
#if !EXCLUSIVE_MODE
    w = 1024;
    h = 600;
#endif
    SDL_Window* window = SDL_CreateWindow(
        GameTitle(),
        SDL_WINDOWPOS_UNDEFINED,    // initial x position
        SDL_WINDOWPOS_UNDEFINED,    // initial y position
        w,
        h,
#if EXCLUSIVE_MODE
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN
#else
        SDL_WINDOW_SHOWN
#endif
    );
    InitializeSound();
    InitializeKeyTranslationTables();
    ScreenFormat = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
    if (GameInitialize(w, h)) {
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture[N_TEXTURE];
        for (int i=0; i<N_TEXTURE; ++i)
            texture[i] = nullptr;
        int textureIndex = 0;
        while (!Quit) {
            if (NewFrameIntervalRate!=OldFrameIntervalRate)
                if (!RebuildRendererAndTexture(window, w, h, renderer, texture))
                    break;
            void* pixels;
            int pitch;
            if (!texture[textureIndex]) {
                fprintf(stderr, "No texture!\n");
                abort();
            }
            if (SDL_LockTexture(texture[textureIndex], nullptr, &pixels, &pitch)) {
                printf("Internal eror: SDL_LockTexture failed: %s\n", SDL_GetError());
                break;
            }
            double t0 = HostClockTime();
            NimblePixMap screen(w, h, 8*sizeof(NimblePixel), pixels, pitch);
            if (Resize) {
                GameResizeOrMove(screen);
                Resize = false;
            }
            GameUpdateDraw(screen, NimbleRequest::update|NimbleRequest::draw);

            SDL_UnlockTexture(texture[textureIndex]);
            SDL_RenderClear(renderer);
            // Assume 60 Hz update rate.  Simulate slower refresh rate by presenting texture twice.
            // At least one trip trhough the loop is required because a rate of 0 indicates "unlimited".
            int i = 0;
            do {
                SDL_RenderCopy(renderer, texture[textureIndex], nullptr, nullptr);
                SDL_RenderPresent(renderer);
            } while (++i<OldFrameIntervalRate);
            PollEvents();
            textureIndex = (textureIndex + 1) & N_TEXTURE-1;
        }
        for (int i=0; i<N_TEXTURE; ++i)
            if (texture[i])
                SDL_DestroyTexture(texture[i]);
        if (renderer) SDL_DestroyRenderer(renderer);
    } else {
        printf("GameInitialize() failed\n");
        return 1;
    }

    SDL_DestroyWindow(window);
    return 0;
}

void HostWarning(const char* message) {
    fprintf(stderr, message);
    abort();
}

const char* HostGetCommonAppData(const char* pathSuffix) {
    return "C:\\tmp\\tmp.dat";
}