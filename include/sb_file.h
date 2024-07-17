#ifndef SB_FILE_H
#define SB_FILE_H

#include <stdio.h>
#include "sb_arena.h"
#include "sb_string.h"

uint64_t sb_get_file_size(FILE *file);

FILE *sb_fopen(const char *name, const char *mode);
uint64_t sb_get_file_size(FILE *file);
void *sb_read_file_bytes(sb_arena *arena, FILE *file, uint64_t bytes);
void *sb_read_file_binary(sb_arena *arena, const char *name, uint64_t *out_size);
sb_str8 sb_read_file_string(sb_arena *arena, const char *name);

#endif
