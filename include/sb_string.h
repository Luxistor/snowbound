#ifndef SB_STRING_H
#define SB_STRING_H

#include "sb_arena.h"

typedef struct
{
    char *str;
    size_t size;
} sb_str8;

#define sb_str8_lit(s) (sb_str8){(char*)(s), sizeof(s) - 1}

sb_str8 sb_str8_concat(sb_arena *arena, sb_str8 s1, sb_str8 s2);
sb_str8 sb_u8_to_str8(sb_arena *arena, uint8_t number);

#endif
