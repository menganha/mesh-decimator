//
//  Custom array implementation inspired by bitsquid-foundation library
//
#pragma once

#include "arena.hpp"
#include "log.hpp"

#include <cstdlib>
#include <cstring>
#include <utility>


template<typename T>
struct Array
{
    T*  data;
    int length;
    int capacity;

    inline T&       operator[](int idx) { return data[idx]; }       // TODO: Do a debug bounds check assertion
    inline const T& operator[](int idx) const { return data[idx]; } // TODO: Do a debug bounds check assertion

    inline T*       begin() { return data; }
    inline T*       end() { return data + length; }
    inline const T* begin() const { return data; }
    inline const T* end() const { return data + length; }
    inline T&       front() { return data[0]; }
    inline T&       back() { return data[length - 1]; }
    inline const T& front() const { return data[0]; }
    inline const T& back() const { return data[length - 1]; }
};

// Allocates array on specified arena
template<typename T>
void arraySetCapacity(Array<T>& array, int capacity, Arena& arena)
{
    T* data = arenaAlloc<T>(arena, capacity);
    array.data = data;
    array.capacity = capacity;
}

// Resize
template<typename T>
void arrayResize(Array<T>& array, int new_capacity, Arena& arena)
{
    LASSERT(array.capacity < new_capacity, "Array of capacity %i can only grow. Capacity set %i", array.capacity, new_capacity);
    T* new_data_buffer = arenaAlloc<T>(arena, new_capacity);
    std::memcpy(new_data_buffer, array.data, sizeof(T) * array.length);
    array.data = new_data_buffer;
    array.capacity = new_capacity;
}

template<typename T, typename... Args>
void arrayEmplaceBack(Array<T>& array, Args&&... args)
{
    // if ( array.length == array.capacity )
    // {
    //     int new_capacity = array.capacity * 2 + 8;
    //     LDEBUG("Out of bounds assignment for array of length %i. Resizing", new_capacity);
    //     arrayResize(array, new_capacity);
    // }
    LASSERT(array.length < array.capacity, "Out of bounds emplacement");
    array.data[array.length] = T { std::forward<Args>(args)... };
    ++array.length;
}

template<typename T>
void arrayPushBack(Array<T>& array, T& element)
{
    // if ( array.length == array.capacity )
    // {
    //     int new_capacity = array.capacity * 2 + 8;
    //     LDEBUG("Out of bounds assignment for array of length %i. Resizing", new_capacity);
    //     arrayResize(array, new_capacity);
    // }
    LASSERT(array.length <= array.capacity, "Out of bounds push back");
    array.data[array.length] = element;
    ++array.length;
}

template<typename T>
void arrayPushBack(Array<T>& array, T&& element)
{
    // if ( array.length == array.capacity )
    // {
    //     int new_capacity = array.capacity * 2 + 8;
    //     LDEBUG("Out of bounds assignment for array of length %i. Resizing", new_capacity);
    //     arrayResize(array, new_capacity);
    // }
    LASSERT(array.length <= array.capacity, "Out of bounds push back");
    array.data[array.length] = std::move(element);
    ++array.length;
}

// Removes element at specified index replacing it with the last one.
template<typename T>
void arrayPopAt(Array<T>& array, int index)
{
    LASSERT(index < array.length, "Index %i out of bounds from array of length %i", index, array.length);
    if ( index != array.length - 1 )
    {
        array.data[index] = array.data[array.length - 1];
    }
    array.length--;
}

template<typename T, typename F>
void arraySort(Array<T>& array, F comparison)
{
    std::qsort(array.data, array.length, sizeof(T), comparison);
}

template<typename T, typename F>
static inline int arrayRemoveDuplicates(Array<T>& array, F predicate)
{
    if ( array.length == 0 )
    {
        return 0;
    }

    int i, j;
    int new_length = 1;
    for ( i = 1; i < array.length; i++ )
    {
        for ( j = 0; j < new_length; j++ )
        {
            bool is_duplicated = predicate(array[i], array[j]);
            if ( is_duplicated )
                break;
        }
        // If none of the values in index[0..j] of array is not same as array[i],
        // then copy the current value to corresponding new position in array
        if ( j == new_length )
            array[new_length++] = array[i];
    }

    int duplicates = array.length - new_length;
    array.length = new_length;
    return duplicates;
}

template<typename T>
static inline int arrayRemoveDuplicates(Array<T>& array)
{
    return arrayRemoveDuplicates(array, [](T& a, T& b) { return a == b; });
}

// Linear search
template<typename T, typename F>
static inline T* arrayFind(Array<T>& array, T& needle, F predicate)
{
    for ( T* it = array.begin(); it != array.end(); ++it )
    {
        bool is_equal = predicate(*it, needle);
        if ( is_equal )
        {
            return it;
        }
    }
    return array.end();
}

template<typename T>
static inline T* arrayFind(Array<T>& array, T& needle)
{
    return arrayFind(array, needle, [](T& a, T& b) { return a == b; });
}
