#include "sb_common.h"
#include "sb_window.h"
#include "sb_arena.h"
#include "sb_math.h"

#define SCRATCH_POOL_COUNT 2

static __declspec(thread) sb_arena *SCRATCH_POOL[SCRATCH_POOL_COUNT] = {0};

#if defined(SB_WINDOWS_OS_FLAG)
uint32_t get_page_size(void)
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	return sys_info.dwPageSize;
}

bool commit_memory(void *memory, size_t size)
{
    return VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

void *reserve_memory(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
}
#endif

sb_arena *sb_arena_alloc_custom(size_t reserve_size, size_t commit_size)
{
    sb_arena *arena = reserve_memory(reserve_size);
	assert(arena);

	size_t rounded_commit_size = sb_round_up(commit_size, get_page_size());
	assert(commit_memory(arena, rounded_commit_size));

	arena->offset = sizeof(sb_arena);
	arena->commit_pos = rounded_commit_size;
	arena->reserve_size = reserve_size;
	arena->commit_size = rounded_commit_size;
    return arena;
}

void *sb_arena_push_aligned(sb_arena *arena, size_t size, size_t align)
{
    uintptr_t current_ptr = (uintptr_t)arena + (uintptr_t)arena->offset;
	uintptr_t aligned_ptr = sb_align_forward_power_of_two(current_ptr, align);
	uintptr_t offset = aligned_ptr - (uintptr_t) arena;

	size_t new_offset = offset + size;
	assert(new_offset < arena->reserve_size);
	if (new_offset > arena->commit_pos)
    {
		size_t new_commit_pos = sb_round_up(new_offset, arena->commit_size);
		assert(commit_memory(arena, new_commit_pos));
		arena->commit_pos = new_commit_pos;
	}
	arena->offset = new_offset;
	return memset((char*)arena + offset, 0, size);
}

void sb_reset_arena(sb_arena *arena)
{
	arena->offset = sizeof(sb_arena);
}

sb_arena_temp sb_arena_temp_begin(sb_arena *arena)
{
    return (sb_arena_temp) {
        .arena = arena,
        .offset = arena->offset
    };
}

void sb_arena_temp_end(sb_arena_temp *temp)
{
    temp->arena->offset = temp->offset;
}


void initialize_scratch_pool(void)
{
    for (size_t i = 0; i < SCRATCH_POOL_COUNT; i++)
    {
        SCRATCH_POOL[i] = sb_arena_alloc();
    }
}

sb_arena_temp sb_get_scratch(void)
{
    if (SCRATCH_POOL[0] == NULL)
    {
		initialize_scratch_pool();
	}

	return sb_arena_temp_begin(SCRATCH_POOL[0]);
}

sb_arena_temp sb_get_scratch_with_conflicts(sb_arena **conflicts, size_t conflict_count)
{
	if (SCRATCH_POOL[0] == NULL)
	{
		initialize_scratch_pool();
	}

	for (size_t i = 0; i < SCRATCH_POOL_COUNT; i++)
	{
		sb_arena *scratch = SCRATCH_POOL[i];
		bool conflict_found = false;
		for (size_t j = 0; j < conflict_count; j++)
		{
			sb_arena *conflict = conflicts[j];
			if (conflict == scratch)
			{
				conflict_found = true;
				break;
			}
		}
		if (!conflict_found) return sb_arena_temp_begin(scratch);
	}

	SB_PANIC("Too many conflicts passed into scratch arena!");
}
