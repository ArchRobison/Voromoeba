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

//! Array of Bug or subclass thereof.
//!
//! Class T should be Bug or a subclass of Bug, and can be default-constructed via zero initialization.
template<class T>
class BugArray : NoCopy {
    //! Pointer to storage for items
    T* myArray = nullptr;
    //! Number of items in the array
    size_t mySize = 0;
    //! Maximum number of items that storage can hold
    size_t myPhysicalSize = 0;
public:
    BugArray() = default;
    ~BugArray() { delete[] myArray; }
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
    //! The new size must not exceed the reserved size. 
    void resize(size_t newSize) {
        Assert(newSize<=myPhysicalSize);
        mySize = newSize;
    }
    T& back() {
        return (*this)[size()-1];
    }
    void popBack() {
        Assert(mySize > 0);
        --mySize;
    }

    //! Swap ith and jth elements
    void exchange(size_t i, size_t j) {
        std::swap((*this)[i], (*this)[j]);
    }

    //! Copy to Ants using given transform.
    Ant* copyToAnts(Ant* a, const ViewTransform& v) const;
};

template<typename T>
void BugArray<T>::reserve(size_t maxSize) {
    delete[] myArray;
    myArray = new T[maxSize];
    myPhysicalSize = maxSize;
    mySize = maxSize;
    std::memset(myArray, 0, maxSize*sizeof(T));
}

template<typename T>
Ant* BugArray<T>::copyToAnts(Ant* a, const ViewTransform& v) const {
    for (const auto& b : *this)
    {
        a->assign(v.transform(b.pos), b.color);
        ++a;
    }
    return a;
}

#endif /* Bug_H */
