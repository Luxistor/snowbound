#include "sb_string.h"

#include <stdio.h>

sb_str8 sb_str8_concat(sb_arena *arena, sb_str8 s1, sb_str8 s2)
{
    sb_str8 ret;
    ret.size = s1.size + s2.size;
    ret.str = sb_arena_push(arena, char, ret.size + 1);

    memcpy(ret.str, s1.str, s1.size);
    memcpy(ret.str + s1.size, s2.str, s2.size);

    ret.str[ret.size] = 0;
    return ret;
}

sb_str8 sb_u8_to_str8(sb_arena *arena, uint8_t number)
{
    size_t len = snprintf(NULL, 0, "%d", number);

    char *str = sb_arena_push(arena, char, len+1);
    snprintf(str, len+1, "%d", number);

    return (sb_str8) {str, len};
}
