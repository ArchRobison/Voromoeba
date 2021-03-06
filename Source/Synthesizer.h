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

#ifndef Synthesizer_H
#define Synthesizer_H

#include "Utility.h"
#include <cstdint>
#include <new>

namespace Synthesizer {

static const size_t SampleRate = 44100;

template<typename T, int Shift>
class SampledSignalBase : public SimpleArray<T, 1> {
public:
    typedef T sampleType;
    typedef uint32_t timeType;	                            // Unsigned type       
    static const int timeShift = Shift;
    static constexpr uint32_t unitTime = (1<<timeShift);
    timeType limit() const { return static_cast<timeType>(size()<<timeShift); }
    float interpolate(const T* w, timeType t) const {
        size_t i = t>>timeShift;
        Assert(w+i<end());
        sampleType s0 = w[i];
        sampleType s1 = w[i+1];
        float f = (t & unitTime-1)*(1.0f/unitTime);
        return s0+(s1-s0)*f;
    }
};

class Waveform : public SampledSignalBase<float, 16> {
public:
    Waveform() : myIsCyclic(2) {}
    Waveform(size_t n) : myIsCyclic(2) { resize(n); }

    ~Waveform() {}
    bool isCyclic() const {
        Assert(isCompleted());
        return myIsCyclic!=0;
    }
    void complete(bool cyclic=true) {
        myIsCyclic = cyclic;
        *end() = cyclic ? *begin() : 0;
    }
    bool isCompleted() const { return myIsCyclic<2; }
    //! Read from a ".wav" file
    void readFromFile(const char* filename);
    //! Write to a ".wav" file
    void writeToFile(const char* filename);
    //! Read from a ".wav" file in memory
    void readFromMemory(const char* data, size_t size);
private:
    class inputType;
    void readFromInput(inputType& f);
    /** 0-> non-cyclic waveform (conceptually tailed by zeros).
        1-> cyclic waveform
        2-> method complete has not been called yet */
    char myIsCyclic;
};

class Envelope : public SampledSignalBase<float, 20> {
public:
    Envelope() : myIsSustain(2) {}
    ~Envelope() {}
    bool isSustain() const {
        Assert(isCompleted());
        return myIsSustain!=0;
    }
    void complete(bool sustain) {
        sampleType* e = end();
        e[0] = sustain ? e[-1] : 0;
        myIsSustain = sustain;
    }
    bool isCompleted() const { return myIsSustain<2; }
private:
    /** 0-> release channel when reaching end of envelope.
        1-> sustain channel when reaching end of envelope.
        2-> method complete has not been called yet */
    char myIsSustain;
};

class Player;
class PlayerMessage;

class Source : NoCopy {
protected:
    Player* player;
    friend void Play(Source* src, float volume, float x, float y);
    friend void OutputInterruptHandler(Waveform::sampleType* left, Waveform::sampleType* right, unsigned n);
    friend class Player;
    //! Set acc[0:n] to next n samples (or fewer if src has reached its end).  Returns nmber of samples created
    virtual uint32_t update(float* acc, uint32_t n) = 0;
    virtual void destroy() = 0;
    virtual void receive(const PlayerMessage& m) = 0;
public:
};

//! Source
class SimpleSource final : public Source {
    const Waveform* waveform;
    unsigned short waveLowIndex;    // Low 16 bits, i.e. bits after the fixed-point
    unsigned int waveHighIndex;     // Upper 32 bits, i.e. integer part of index
    Waveform::timeType waveDelta;
    uint32_t update(float* acc, uint32_t n) final;
    void destroy() final;
    void receive(const PlayerMessage& m) final;
public:
    static SimpleSource* allocate(const Waveform& w, float freq=1.0f);
};

class DynamicSource final : public Source {
private:
    const Waveform* waveform;
    Waveform::timeType waveIndex;
    Waveform::timeType waveDelta;
    float currentVolume;
    float targetVolume;
    unsigned deadline:31;
    unsigned release:1;
    uint32_t update(float* acc, uint32_t n) final;
    void destroy() final;
    void receive(const PlayerMessage& m) final;
public:
    static DynamicSource* allocate(const Waveform& w, float freq=1.0f);
    //! Cause volume to change smoothly to given value by given deadline.
    /** Deadline measured in sec. */
    void changeVolume(float newVolume, float deadline, bool release=false);
};

class MidiSource : public Source {
    const Waveform* waveform;
    Waveform::timeType waveIndex;
    Waveform::timeType waveDelta;
    const Envelope* envelope;
    Envelope::timeType envIndex;
    Envelope::timeType envDelta;
    const Envelope* relEnvelope;
    Envelope::timeType relEnvDelta;
    uint32_t update(float* acc, uint32_t n) final;
    void destroy() final;
    void receive(const PlayerMessage& m) final;
public:
    static MidiSource* allocate(const Waveform& w, float freq, const Envelope& attack, float speed=1.0f);
    void changeEnvelope(Envelope& e, float speed=1.0f);
};

//! Intialize synthesizer global structures.
void Initialize();

void OutputInterruptHandler(Waveform::sampleType* left, Waveform::sampleType* right, unsigned n);
void InputInterruptHandler(const Waveform::sampleType* wave, size_t n);

//! Start playing src.  Method src->destroy() will be invoked after src->update() returns.
/** No-op if src is NULL.  Doing so allows clients to SimplesSource to not have to check
    whether SimpleSource::allocate returns NULL. */
void Play(Source* src, float volume=1.0f, float x=0, float y=1.0f);

} // namespace Synthesizer

#define INJECT_SOUND 0

class SoundInjector {
public:
    void inject(float* p, size_t n) {
#if INJECT_SOUND
        for (size_t i=0; i<n; ++i) {
            float y = sin(0.0625f*k++);
            p[i] = y;
        }
#endif
    }
    void check(const float* p, size_t n) {
#if INJECT_SOUND
        for (size_t i=0; i<n; ++i) {
            float y = sin(0.0625f*k++);
            Assert(p[i]==y);
        }
#endif
    }
    SoundInjector() : k(0) {}
private:
    unsigned k;
};

#endif /* Synthesizer_H */