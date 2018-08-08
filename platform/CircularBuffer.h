/* mbed Microcontroller Library
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MBED_CIRCULARBUFFER_H
#define MBED_CIRCULARBUFFER_H

#include "platform/mbed_critical.h"
#include "platform/mbed_assert.h"

namespace mbed {

namespace internal {
/* Detect if CounterType of the Circular buffer is of unsigned type. */
template<typename T>
struct is_unsigned {
    static const bool value = false;
};
template<>
struct is_unsigned<unsigned char> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned short> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned int> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned long> {
    static const bool value = true;
};
template<>
struct is_unsigned<unsigned long long> {
    static const bool value = true;
};
};

/** \addtogroup platform */
/** @{*/
/**
 * \defgroup platform_CircularBuffer CircularBuffer functions
 * @{
 */

/** Templated Circular buffer class
 *
 *  @note Synchronization level: Interrupt safe
 *  @note CounterType must be unsigned and consistent with BufferSize
 */
template<typename T, uint32_t BufferSize, typename CounterType = uint32_t>
class CircularBuffer {
public:
    CircularBuffer() : _head(0), _tail(0), _full(false)
    {
        MBED_STATIC_ASSERT(
            internal::is_unsigned<CounterType>::value,
            "CounterType must be unsigned"
        );

        MBED_STATIC_ASSERT(
            (sizeof(CounterType) >= sizeof(uint32_t)) ||
            (BufferSize < (((uint64_t) 1) << (sizeof(CounterType) * 8))),
            "Invalid BufferSize for the CounterType"
        );
    }

    ~CircularBuffer()
    {
    }

    /** Push the transaction to the buffer. This overwrites the buffer if it's
     *  full
     *
     * @param data Data to be pushed to the buffer
     */
    void push(const T &data)
    {
        core_util_critical_section_enter();

        //Copy _head and _tail to local variables to allow for optimization
        CounterType tail = _tail;
        CounterType head = _head;
        
        if (_full) {
            tail++;
            tail %= BufferSize;
            _tail = tail;
        }
        _pool[head++] = data;
        head %= BufferSize;
        if (head == tail) {
            _full = true;
        }
        _head = head;

        core_util_critical_section_exit();
    }

    /** Pop the transaction from the buffer
     *
     * @param data Data to be popped from the buffer
     * @return True if the buffer is not empty and data contains a transaction, false otherwise
     */
    bool pop(T &data)
    {
        bool data_popped = false;
        
        core_util_critical_section_enter();

        //Copy _head and _tail to local variables to allow for optimization
        CounterType tail = _tail
        CounterType head = _head;

        if (_head != _tail || _full) {
            data = _pool[tail++];
            tail %= BufferSize;
            _tail = tail;
            _full = false;
            data_popped = true;
        }
        core_util_critical_section_exit();
        return data_popped;
    }

    /** Check if the buffer is empty
     *
     * @return True if the buffer is empty, false if not
     */
    bool empty() const
    {
        core_util_critical_section_enter();
        bool is_empty = (_head == _tail) && !_full;
        core_util_critical_section_exit();
        return is_empty;
    }

    /** Check if the buffer is full
     *
     * @return True if the buffer is full, false if not
     */
    bool full() const
    {
        core_util_critical_section_enter();
        bool full = _full;
        core_util_critical_section_exit();
        return full;
    }

    /** Reset the buffer
     *
     */
    void reset()
    {
        core_util_critical_section_enter();
        _head = 0;
        _tail = 0;
        _full = false;
        core_util_critical_section_exit();
    }

    /** Get the number of elements currently stored in the circular_buffer */
    CounterType size() const
    {
        core_util_critical_section_enter();
        //Copy _head and _tail to local variables to allow for optimization
        CounterType tail = _tail
        CounterType head = _head;
        CounterType elements;
        if (!_full) {
            if (head < tail) {
                elements = BufferSize + head - tail;
            } else {
                elements = head - tail;
            }
        } else {
            elements = BufferSize;
        }
        core_util_critical_section_exit();
        return elements;
    }

    /** Peek into circular buffer without popping
     *
     * @param data Data to be peeked from the buffer
     * @return True if the buffer is not empty and data contains a transaction, false otherwise
     */
    bool peek(T &data) const
    {
        bool data_updated = false;
        core_util_critical_section_enter();
        //Copy _head to local variable to allow for optimization
        CounterType tail = _tail
        if (_head != tail || _full) {
            data = _pool[tail];
            data_updated = true;
        }
        core_util_critical_section_exit();
        return data_updated;
    }

private:
    volatile T _pool[BufferSize];
    volatile CounterType _head;
    volatile CounterType _tail;
    volatile bool _full;
};

/**@}*/

/**@}*/

}

#endif
