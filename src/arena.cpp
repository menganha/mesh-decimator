#include "arena.hpp"

#include "cstdlib"

Arena arenaMake(int buffer_size)
{
    Arena arena {};
    arena.buffer = static_cast<std::byte*>(std::malloc(buffer_size));
    arena.offset = 0;
    arena.size = buffer_size;
    return arena;
}

void arenaReset(Arena& arena) { arena.offset = 0; }

void arenaFree(Arena& arena)
{
    std::free(arena.buffer);
    arena.buffer = nullptr;
    arena.offset = 0;
    arena.size = 0;
}

