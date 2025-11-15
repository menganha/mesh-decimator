#pragma once
#include "log.hpp"

#include <cstddef>
#include <new>  // IWYU pragma: keep. For some reason clang is not picking up the use of the placement new operator

#define KILOBYTES(value) ((value)*1024LL)
#define MEGABYTES(value) (KILOBYTES(value)*1024LL)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)
#define TERABYTES(value) (GIGABYTES(value)*1024LL)

struct Arena
{
    std::byte* buffer;
    int        offset; // Cursor to check how much of the arena is being currently used
    int        size;
};

Arena arenaMake(int buffer_size);
void  arenaReset(Arena& arena);
void  arenaFree(Arena& arena);

// TODO Return an arena pointer and make all function take pointer as inputs. Also remove the new placement operator.
//      The placement operator would make sense when working with a more modern c++ style. Now it just makes it more
//      complicated to read.
template<typename T>
T* arenaAlloc(Arena& arena, int number = 1)
{
    int alignment = alignof(T);
    int size = sizeof(T) * number;

    LASSERT(((alignment & (alignment - 1)) == 0), "Alignment is not a power of two:  %i", alignment);

    std::ptrdiff_t padding = -arena.offset & (alignment - 1);

    arena.offset += padding;

    LASSERT(((arena.offset + size > arena.size) == 0), "Not enough memory to allocate object");

    T* result = reinterpret_cast<T*>(&arena.buffer[arena.offset]);

    // TODO: Do not construct the objects. Just reserve the memory and let the construction be handled
    // elsewhere. This leaves no room for flexibility as we can only default construct the elements.
    for ( int idx = 0; idx < number; ++idx ) {
        new (static_cast<void*>(result + idx)) T {};
    }

    arena.offset += size;

    return result;
}
