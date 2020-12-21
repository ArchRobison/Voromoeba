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
  Utility functions for Seismic Duck and Voromoeba
 *******************************************************************************/

#ifndef UTILITY_H
#define UTILITY_H

#include <cstdlib>
#include <cstdint>
#include <limits>
#include "AssertLib.h"

typedef unsigned char byte;

template<typename T>
inline T Min(T a, T b) {
    return a<b ? a : b;
}

template<typename T>
inline T Max(T a, T b) {
    return a<b ? b : a;
}

//! Return x clipped to closed interval [a,b]
template<typename T>
inline T Clip(T a, T b, T x) {
    Assert(!(b<a));
    return x<a ? a : b<x ? b : x;
}

template<typename T>
inline void Swap(T& x, T& y) {
    T tmp(x);
    x = y;
    y = tmp;
}

inline int Round(float x) {   // FIXME - add SSE version
    x += x<0 ? -0.5f : 0.5f;
    return int(x);
}

inline int Ceiling(float x) { // FIXME - add SSE version
    int t = int(x);
    if (x>0 && x!=t) ++t;
    return t;
}

inline int Floor(float x) { // FIXME - add SSE version
    int t = int(x);
    if (x<0 && x!=t) --t;
    return t;
}

inline float Square(float z) {
    return z*z;
}

//! Return true if z is an IEEE "Infinity"
inline bool IsInfinity(float z) {
    return z==std::numeric_limits<float>::infinity();
}

//! Return random real in [0,a]
inline float RandomFloat(float a) {
    return rand()*(a*(1.0f/RAND_MAX));
}

//! Return random angle
inline float RandomAngle() {
    return rand()*(2*3.1415926535f/(RAND_MAX+1.f));
}

//! Base class for non-copiable non-assignable classes
class NoCopy {
    NoCopy(const NoCopy&) = delete;
    void operator=(const NoCopy&) = delete;
public:
    NoCopy() {}
};

//! No-frills array class
//!
//! Extra is number of additional elements to add beyond reported size.
//! These additional elements may be accessed via operator[].
//! E.g., AdditiveSynthesizer uses Extra=1 to simplify logic. */
template<typename T, size_t Extra=0>
class SimpleArray : NoCopy {
    T* myStart;
    size_t mySize;
public:
    SimpleArray() : myStart(0), mySize(0) {}
    SimpleArray(size_t n) {
        myStart = new T[n+Extra];
        mySize = n;
    }
    ~SimpleArray() { clear(); }
    size_t size() const { return mySize; }
    void clear() {
        if (myStart) {
            delete[] myStart;
            myStart = nullptr;
            mySize = 0;
        }
    }
    void resize(size_t n) {
        clear();
        if (n+Extra>0) {
            myStart = new T[n+Extra];
            mySize = n;
        }
    }
    template<typename U>
    void assign(const U* array, size_t count) {
        clear();
        if (count>0) {
            myStart = new T[count+Extra];
            for (size_t i=0; i<count; ++i)
                myStart[i] = array[i];
            mySize = count;
        }
    }
    void fill(const T& value) {
        for (size_t i=0; i<mySize; ++i)
            myStart[i] = value;
    }
    T* begin() { return myStart; }
    const T* begin() const { return myStart; }
    T* end() { return myStart+mySize; }
    const T* end() const { return myStart+mySize; }
    //! Subscript is checked against size()+Extra.
    T& operator[](size_t k) {
        Assert(k<mySize+Extra);
        return myStart[k];
    }
    const T& operator[](size_t k) const {
        Assert(k<mySize+Extra);
        return myStart[k];
    }
};

//! No-frills bag class
template<typename T>
class SimpleBag {
    T* myBegin;
    T* myEnd;
    T* myLimit;
public:
    //! Create bag and reserve space for maxSize items.
    SimpleBag(size_t maxSize) {
        myEnd = myBegin = (T*)operator new(sizeof(T)*maxSize);
        myLimit = myBegin + maxSize;
    }
    ~SimpleBag() {
        operator delete(myBegin);
    }
    bool isEmpty() {
        Assert(myBegin<=myEnd);
        return myEnd==myBegin;
    }
    //! Pop item from end of bag
    void pop(T& dst) {
        Assert(!isEmpty());
        dst = *--myEnd;
        myEnd->~T();
    }
    //! Add item to end of bag
    void push(T i) {
        Assert(myEnd<myLimit);
        new(myEnd++) T(i);
    }

    typedef T* iterator;
    iterator begin() { return myBegin; }
    iterator end() { return myEnd; }

    //! Erase *i.  Afterwards i points to the next unvisited item.
    void erase(iterator i) {
        Assert(myBegin<=i && i<myEnd);
        pop(*i);
    }
};

//! Array-like object that is subscripted by an Enum
//!
//! \tparam Key enum type used for subscripting. Should be dense 0-origin enum.
//! \tparam KeyCount Number of key values.
//! \tparam T typed mapped to
template<typename Key, size_t KeyCount, typename T>
class EnumMap {
    T array[KeyCount];
public:
    T& operator[](Key k) {
        return array[size_t(k)];
    }
    const T& operator[](Key k) const {
        return array[size_t(k)];
    }
};

#endif /* UTILITY_H */