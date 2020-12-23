/* Copyright 2020 Arch D. Robison

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
  General utilities related to enums.
 *******************************************************************************/

#ifndef Enum_H
#define Enum_H

#include <cstdlib>
#include <cstdint>
#include <initializer_list>

template<typename Key>
constexpr Key EnumMax = Key() / 0; // Force build error if primate template is used

//! Array-like object that is subscripted by an Enum
//!
//! \tparam Key enum type used for subscripting. Should be dense 0-origin enum.
//! \tparam KeyCount Number of key values.
//! \tparam T typed mapped to
template<typename Key, typename T>
class EnumMap {
    T array[size_t(EnumMax<Key>) + 1];
public:
    const T* begin() const { return array; }
    const T* end() const { return array + size_t(EnumMax<Key>); }
    T& operator[](Key k) {
        return array[size_t(k)];
    }
    const T& operator[](Key k) const {
        return array[size_t(k)];
    }
    constexpr size_t size() const {
        return size_t(EnumMax<Key>) + 1;
    }
};

//! Set of enum values.
//!
//! \tparam Key enum type used for element type. Should be dense 0-origin enum with not more than 32 values.
//! \tparam KeyCount Number of enum values.
template<typename Key>
class EnumSet {
public:
    constexpr EnumSet() {}
    constexpr EnumSet(Key key) : myMask(1u << uint32_t(key)) {}
    constexpr EnumSet(std::initializer_list<Key> keyList) : myMask(0) {
        for (auto k : keyList)
            myMask |= 1u << uint32_t(k);
    }
    friend EnumSet operator&(const EnumSet& x, const EnumSet& y) {
        return EnumSet(x.myMask & y.myMask);
    }
    friend EnumSet operator|(const EnumSet& x, const EnumSet& y) {
        return EnumSet(x.myMask | y.myMask);
    }
    void operator&=(const EnumSet& y) {
        myMask &= y.myMask;
    }
    void operator|=(const EnumSet& y) {
        myMask |= y.myMask;
    }
    void operator-=(const EnumSet& y) {
        myMask &= ~y.myMask;
    }
    bool operator[](Key k) const {
        return (myMask & 1u << uint32_t(k)) != 0;
    }
private:
    explicit EnumSet(uint32_t mask) : myMask(mask) {}
    uint32_t myMask = 0;
    static_assert(size_t(EnumMax<Key>) < 32, "max keys is 32");
};

#endif /* Enum_H */
