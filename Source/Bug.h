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
#pragma once
#ifndef Bug_H
#define Bug_H

#include "Ant.h"
#include "Geometry.h"
#include "Color.h"
#include "Outline.h"
#include <cstring>

//! A Bug crawls around on the screen.
class Bug {
public:
    //! Position in world coordinates 
    Point pos; 
    //! Velocity in world units per second
	Point vel;
    //! Color
    OutlinedColor color;
};

//! Array type that never moves its items, constructs them by zeroing, and assumes type T has a trivial destructor.
template<class T>
class BasicArray : NoCopy {
    //! Pointer to storage for items
    T* myArray;
    //! Number of items in the array
    size_t mySize;
#if ASSERTIONS
    //! Maximum number of items that storage can hold
    size_t myPhysicalSize;
#endif
public:
    BasicArray() : myArray(NULL), mySize(0) {}
    ~BasicArray() { delete[] myArray; }
    size_t size() const { return mySize; }
    void reserve(size_t maxSize);
    const T* begin() const { return myArray; }
    const T* end() const { return myArray+mySize; }
    T* begin() { return myArray; }
    T* end() { return myArray+mySize; }
    T& operator[](size_t k) {
        Assert(k<mySize);
        return myArray[k];
    }
    //! Resize array. 
    //! Does not initialize new elements if growing.
    //! The new size must not exceed the reserved sized. 
    void resize(size_t newSize) {
        Assert(newSize<=myPhysicalSize);
        mySize = newSize;
    }
    T& back() {
        return (*this)[size()-1];
    }
    void popBack() {
        resize(size()-1);
    }

    //! Swap ith and jth elements
    void exchange(size_t i, size_t j) {
        std::swap((*this)[i], (*this)[j]);
    }

    Ant* assignAnts(Ant* a, const ViewTransform& v) const;
};

template<typename T>
void BasicArray<T>::reserve(size_t maxSize) {
    delete[] myArray;
    myArray = new T[maxSize];
#if ASSERTIONS
    myPhysicalSize = maxSize;
#endif
    mySize = maxSize;
    std::memset(myArray, 0, maxSize*sizeof(T));
}

template<typename T>
Ant* BasicArray<T>::assignAnts(Ant* a, const ViewTransform& v) const {
    for (const T* b = begin(); b!=end(); ++b, ++a) {
        a->assign(v.transform(b->pos), b->color);
    }
    return a;
}

#endif /* Bug_H */
