#ifndef SB_ARENA_H
#define SB_ARENA_H

#include "sb_math.h"

#define SB_DEFAULT_RESERVE_SIZE GB(1)
#define SB_DEFAULT_COMMIT_SIZE KB(4)

#include <stdint.h>
#include <stdbool.h>

static uint32_t get_page_size(void);
static bool commit_memory(void *memory, size_t size);
static void *reserve_memory(size_t size);

typedef struct
{
    size_t offset;
    size_t commit_pos;
    size_t commit_size;
    size_t reserve_size;
} sb_arena;

sb_arena *sb_arena_alloc_custom(size_t reserve_size, size_t commit_size);
#define sb_arena_alloc() sb_arena_alloc_custom(SB_DEFAULT_RESERVE_SIZE, SB_DEFAULT_COMMIT_SIZE)

void *sb_arena_push_aligned(sb_arena *arena, size_t size, size_t align);
#define sb_arena_push(arena, type, count) sb_arena_push_aligned(arena, sizeof(type)*(count), _Alignof(type))
#define sb_arena_one(arena, type) sb_arena_push(arena, type, 1)

void sb_reset_arena(sb_arena *arena);

typedef struct
{
    sb_arena *arena;
    size_t offset;
} sb_arena_temp;

sb_arena_temp sb_arena_temp_begin(sb_arena *arena);
void sb_arena_temp_end(sb_arena_temp *temp);

static void initialize_scratch_pool(void);
sb_arena_temp sb_get_scratch(void);
sb_arena_temp sb_get_scratch_with_conflicts(sb_arena **conflicts, size_t conflict_count);
#define sb_release_scratch(scratch) sb_arena_temp_end(scratch)

#endif
