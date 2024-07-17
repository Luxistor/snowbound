#include "sb_file.h"

#include <assert.h>

uint64_t sb_get_file_size(FILE *file)
{
	assert(fseek(file, 0, SEEK_END) == 0);
	uint64_t file_size = ftell(file);
	assert(file_size > 0);
	assert(fseek(file, 0, SEEK_SET) == 0);
	return file_size;
}


FILE *sb_fopen(const char *name, const char *mode)
{
	FILE *file = fopen(name, mode);
	assert(file);
	return file;
}

void *sb_read_file_bytes(sb_arena *arena, FILE *file, uint64_t bytes)
{
	void *buffer = sb_arena_push_aligned(arena, bytes, 64);
	fread(buffer, sizeof(char), bytes, file);
	return buffer;
}

void *sb_read_file_binary(sb_arena *arena, const char *name, uint64_t *out_size)
{
	FILE* file = fopen(name, "rb");
	assert(file);

    *out_size = sb_get_file_size(file);
	void *buffer = sb_read_file_bytes(arena, file, *out_size);

	assert(fclose(file) == 0);
	return buffer;
}

sb_str8 sb_read_file_string(sb_arena *arena, const char *name)
{
	FILE* file = fopen(name, "r");
	assert(file);

    uint64_t file_size = sb_get_file_size(file);
	char *buffer = sb_arena_push_aligned(arena, file_size + 1, 64);

	fread(buffer, sizeof(char), file_size, file);
	fclose(file);

	buffer[file_size+1] = 0;
	return (sb_str8) {buffer, file_size};
}
