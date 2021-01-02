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

#ifndef NonblockingQueue_H
#define NonblockingQueue_H

#include "AssertLib.h"
#include <atomic>
#include <cstdint>

 //! Nonblocking queue for single producer and single consumer.
template<typename T>
class NonblockingQueue {
    std::atomic<uint32_t> myPush;    // Count of pushes.  May wrap. 
    std::atomic<uint32_t> myPop;     // Count of pops.  May wrap. 
    T* myArray;
    T* myEnd;
    //! Updated by consumer
    T* myHead;
    //! Updated by producer
    T* myTail;
    size_t myCapacity;
public:
    NonblockingQueue(size_t maxSize) {
        myPush = myPop = 0;
        myHead = myTail = myArray = new T[maxSize];
        myEnd = myArray+maxSize;
        myCapacity = static_cast<uint32_t>(maxSize);
    }

    // Methods for producer

    //! Return pointer to fresh slot, or return nullptr if queue is full.
    //! Caller must call finishPush after filling in slot. 
    T* startPush() {
        auto d = myPush.load(std::memory_order_acquire) - myPop.load(std::memory_order_acquire);          
        return d < myCapacity ? myTail : nullptr;
    }

    //! Push slot returned by previous call to startPush.
    void finishPush() {
        myPush.fetch_add(1, std::memory_order_release);                            // FIXME - write of myPush needs load-with-acquire semantics
        if (++myTail==myEnd)
            myTail=myArray;
    }

    // Methods for consumer

    //! Return pointer to slot at head of queue, or return NULL if queue is empty.
    T* startPop() {
        auto d = myPush.load(std::memory_order_acquire) - myPop.load(std::memory_order_acquire);
        Assert(d>=0);
        return d > 0 ? myHead : nullptr;
    }

    void finishPop() {
        myPop.fetch_add(1, std::memory_order_release);
        if (++myHead==myEnd)
            myHead=myArray;
    }
};

#endif /* NonblockingQueue */