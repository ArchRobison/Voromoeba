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

#include "AssertLib.h"
#include "BuiltFromResource.h"
#include "Enum.h"
#include "Synthesizer.h"
#include "Sound.h"
#include "Utility.h"
#include <cmath>
#include <vector>
#include <cstdlib>
#include <algorithm>

namespace
{

struct SoundWaveform {
    size_t length;
    float* samples;
};

static EnumMap<SoundKind, Synthesizer::Waveform> TheSounds;

static void ConstructSound(SoundKind k, size_t n) {
    TheSounds[k].resize(n);
    float* ptr = TheSounds[k].begin();
    const size_t m = 5;
    float history[m];
    std::fill_n(history, m, 0.f);
    for (size_t i=0; i<n; ++i, ++ptr) {
        float sum = 0;
        switch (k) {
            case SoundKind::eatPlant:
                for (int s=0; s<=16; ++s)
                    sum+=sin(6.28f*i*(s+8)*.0008f);
                break;
            case SoundKind::destroyPredator:
                for (int s=0; s<=8; ++s)
                    sum+=sin(6.28f*i*(2*s+1)*.0012f);
                break;
            case SoundKind::destroySweetie:
            case SoundKind::sufferHit: {
                int m = 1;
                for (int s=0; s<m; ++s)
                    sum += sin(6.2831853f*i*(s+12)*.0006f*(1+0.2f*sin(6.2831853f*i*0.00001)));// * (1.0f/m) * 0.5*(3+sin(6.2831853f*i*(s+1)/n*8));
                break;
            }
            case SoundKind::self: {
                int m = 1;
                for (int s=0; s<m; ++s)
                    sum += sin(6.2831853f*i*(s+1)/n*160) * (1.0f/m) * 0.5*(3+sin(6.2831853f*i*(s+1)/n*8));
                break;
            }
            case SoundKind::missile: {
                int m = 1;
                for (int s=0; s<m; ++s)
                    sum += sin(6.2831853f*i*(s+1)/n*320) * (1.0f/m) * 0.5*(3+sin(6.2831853f*i*(s+1)/n*8));
                break;
            }
            case SoundKind::closeGate: {
                const float width = 0.1f;
                float t = (float(i)-float(1.0f-width)*n)*(1.0f/Synthesizer::SampleRate);
                if (fabs(t)<=width) {
                    float a = (width-fabs(t))/width;
                    for (int s=8; s<=12; ++s)
                        sum += 4*sin(i*0.001f*s)*a;
                }
                // drop through
            }
            case SoundKind::openGate: {
                sum += sin(6.2831853f*i*.003*(1+0.001*sin(6.2831853f*i*.0001)))*(1+0.25*sin(6.2831853f*i*.00011));
                break;
            }
        }
        float amplitude = 0;
        switch (k) {
            case SoundKind::eatPlant:
            case SoundKind::destroyPredator: {
                int j = i-100;
                amplitude = 0.25f/(exp(-0.01f*j)+exp(j*0.001f));
                break;
            }
            case SoundKind::destroySweetie:
            case SoundKind::sufferHit: {
                amplitude = 0.5f;
                int ramp = Synthesizer::SampleRate*0.1f;
                if (int(i)<ramp) amplitude *= i*(1.0f/ramp);
                else if (int(i)>=n-ramp) amplitude *= float(n-i)*(1.0f/ramp);
                break;
            }
            case SoundKind::self:
            case SoundKind::missile:
                amplitude = 0.5f;
                break;
            case SoundKind::openGate:
            case SoundKind::closeGate: {
                amplitude = 0.5f;
                int ramp = Synthesizer::SampleRate*0.05f;
                if (int(i)<ramp) amplitude *= i*(1.0f/ramp);
                else if (int(i)>=n-ramp) amplitude *= float(n-i)*(1.0f/ramp);
                break;
            }
        }
        // FIXME - merge into single statement
        float value = sum*amplitude;
        *ptr = float(value);
        Assert(std::fabs(*ptr-value)<=1);
    }
    TheSounds[k].complete(/*cyclic=*/false);
}

static const int N_SLUSH_VOICE_MAX = 32;

struct SlushRecord {
    const Beetle* beetle;             //!< BeetleKind::self or BeetleKind::missile
    const Beetle* other;              //!< BeetleKind::water
    float newSegmentLength;
    float newRelativeVolume;
    void assign(const Beetle* b, const Beetle* o, float segmentLength, float relativeVolume) {
        beetle = b;
        other = o;
        newSegmentLength = segmentLength;
        newRelativeVolume = relativeVolume;
    }
};

int NetSlush;

struct SlushVoice {
    Synthesizer::DynamicSource* source;
    float oldSegmentLength;
    float newSegmentLength;
    float relativeVolume;
    float volume(float scale) {
        Assert(scale<1000000000);     // Sanity check
        float v = (newSegmentLength - oldSegmentLength)*scale;
        float x = 4*fabs(v);//v*v;
        if (x>=0.5) x=0.5;
        return x*relativeVolume;
    }
    void begin(const Synthesizer::Waveform& w, float pitch, float scale) {
        Assert(!source);
        source = Synthesizer::DynamicSource::allocate(w, pitch);
        Assert(source);
        Play(source, volume(scale), 0, 1);
        ++NetSlush;
    }
    void end(float dt) {
        --NetSlush;
        Assert(source);
        source->changeVolume(0, dt, true);
        source = NULL;
    }
    bool surelyNull() const { return oldSegmentLength==0 && newSegmentLength==0; }
};

typedef unsigned EdgeSoundId;

static EdgeSoundId EdgeIdOf(const Beetle& b, const Beetle& other) {
    Assert(b.kind==BeetleKind::self || b.kind==BeetleKind::missile);
    Assert(0<b.soundId);
    return b.soundId<<16 | other.soundId;
}

struct SlushLookup {
    EdgeSoundId edgeId;
    unsigned char voiceIndex;
    friend bool operator<(const SlushLookup& x, const SlushLookup& y) {
        return x.edgeId<y.edgeId;
    }
};

static SlushVoice SlushArray[N_SLUSH_VOICE_MAX];
//! Bijection to elements of SlushArray
static SlushLookup SlushLookupBegin[N_SLUSH_VOICE_MAX];
static SlushLookup* SlushLookupEnd;
static SlushRecord SlushRecordBegin[N_SLUSH_VOICE_MAX];
static SlushRecord* SlushRecordEnd;

static void InitSlush() {
    SlushRecordEnd = SlushRecordBegin;
    SlushLookupEnd = SlushLookupBegin;
    for (size_t k=0; k<N_SLUSH_VOICE_MAX; ++k)
        SlushLookupBegin[k].voiceIndex = k;
}

static std::vector<float> SlushPitch;

} // anonymous

void ResetSlush() {
    SlushPitch.clear();
}

BeetleSoundId AllocateSlush(float u) {
    SlushPitch.push_back(pow(2.f, u));
    return SlushPitch.size();
}

void AppendSlush(const Beetle& b, const Beetle& other, float segmentLength, float relativeVolume) {
    Assert(b.kind==BeetleKind::self || b.kind==BeetleKind::missile);
    Assert(other.kind==BeetleKind::water);
    SlushLookup r;
    r.edgeId = EdgeIdOf(b, other);
    SlushLookup* d = std::lower_bound(SlushLookupBegin, SlushLookupEnd, r);
    if (d<SlushLookupEnd && d->edgeId==r.edgeId) {
        // Edge was present in previous frame
        SlushVoice& v = SlushArray[d->voiceIndex];
        Assert(v.newSegmentLength==0);    // Failure indicates duplicate id or failure to call UpdateSlush.
        v.newSegmentLength = segmentLength;
        v.relativeVolume = relativeVolume;
    } else if (SlushRecordEnd < &SlushRecordBegin[N_SLUSH_VOICE_MAX]) {
        // Edge was not present in previous frame
        SlushRecordEnd++->assign(&b, &other, segmentLength, relativeVolume);
    } else {
        // Buffer overflow - just drop the record.   The counter here is just for satisfying curiousity with a debugger.
        static unsigned dropCount;
        ++dropCount;
    }
}

void UpdateSlush(float dt) {
    if (dt==0)
        // Fake update during initialization
        return;
    float scale = 1/dt;  // FIXME - add normalization factor
    // Update volumes and delete records that have a new and old segment length of zero
    SlushLookup* dst = SlushLookupBegin;
    for (SlushLookup* s = SlushLookupBegin; s<SlushLookupEnd; ) {
        SlushVoice& v = SlushArray[s->voiceIndex];
        if (v.surelyNull()) {
            v.end(dt);
            // Do deletion in way that preserves bijection
            std::swap(*s, *--SlushLookupEnd);
        } else {
            // Update volume
            v.source->changeVolume(v.volume(scale), dt);
            v.oldSegmentLength = v.newSegmentLength;
            v.newSegmentLength = 0;
            ++s;
        }
    }
    // Append new records (or at least as many that will fit)
    for (SlushRecord* s=SlushRecordBegin; s<SlushRecordEnd && SlushLookupEnd < SlushLookupBegin+N_SLUSH_VOICE_MAX; ++s) {
        SlushLookup& d = *SlushLookupEnd++;
        d.edgeId = EdgeIdOf(*s->beetle, *s->other);
        SlushVoice& v = SlushArray[d.voiceIndex];
        v.newSegmentLength = s->newSegmentLength;
        v.oldSegmentLength = 0;
        Assert(0<s->other->soundId);
        Assert(s->other->soundId<=SlushPitch.size());
        float pitch = SlushPitch[s->other->soundId-1];
        v.relativeVolume = s->newRelativeVolume;
        Assert(s->beetle->kind==BeetleKind::self || s->beetle->kind==BeetleKind::missile);
        SoundKind k = s->beetle->kind==BeetleKind::self ? SoundKind::self : SoundKind::missile;
        v.begin(TheSounds[k], pitch, scale);
        v.oldSegmentLength = v.newSegmentLength;
        v.newSegmentLength = 0;
    }
    SlushRecordEnd = SlushRecordBegin;
    // Sort by id so that subsequent calls to AppendSlush work
    std::sort(SlushLookupBegin, SlushLookupEnd);
#if ASSERTIONS
    for (const SlushLookup* s = SlushLookupBegin; s<SlushLookupEnd; ++s) {
        const SlushVoice& v = SlushArray[s->voiceIndex];
        Assert(v.newSegmentLength==0);
    }
#endif
}

class ResourceSound : BuiltFromResourceWaveform, public Synthesizer::Waveform {
    void buildFrom(const char* data, size_t size) override;
public:
    ResourceSound(const char* resourceName) : BuiltFromResourceWaveform(resourceName) {}
};

void ResourceSound::buildFrom(const char* data, size_t size) {
    readFromMemory(data, size);
}

static ResourceSound SquishSound("Squish.wav");
static ResourceSound SmoochSound("Smooch.wav");
static ResourceSound YumSound("Yum.wav");

void ConstructSounds() {
    size_t n = Synthesizer::SampleRate;
    ConstructSound(SoundKind::eatPlant, n/4);
    ConstructSound(SoundKind::destroyPredator, n/4);
    ConstructSound(SoundKind::destroySweetie, n);
    ConstructSound(SoundKind::sufferHit, n);
    ConstructSound(SoundKind::self, n);
    ConstructSound(SoundKind::missile, n);
    ConstructSound(SoundKind::openGate, n);
    ConstructSound(SoundKind::closeGate, n);
    // Initialize slush structures
    InitSlush();
}

void PlaySound(SoundKind k, Point p) {
    float relativePitch=1.0f;
    switch (k) {
        case SoundKind::openGate:
        case SoundKind::closeGate:
        case SoundKind::eatOrange:
        case SoundKind::destroyOrange:
            break;
        default:
            relativePitch = pow(1.059463f, RandomUInt(13));
    }
    Synthesizer::Waveform* w;
    switch (k) {
        case SoundKind::smooch: w = &SmoochSound; break;
        case SoundKind::eatOrange: w = &YumSound; break;
        case SoundKind::destroyOrange: w = &SquishSound; break;
        default: w = &TheSounds[k]; break;
    }
    Synthesizer::SimpleSource* s = Synthesizer::SimpleSource::allocate(*w, relativePitch);
    Play(s, 1.0f, p.x, p.y);
};
