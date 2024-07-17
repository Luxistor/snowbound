#include <stdbool.h>

typedef struct
{
    float duration;
    float time;

    bool just_finished;
} sb_timer;

sb_timer sb_create_timer(float duration);
bool sb_timer_finished(sb_timer *timer);
float sb_timer_elapsed(const sb_timer *timer);
bool sb_timer_just_finished(const sb_timer *timer);
bool sb_timer_tick(sb_timer *timer, float dt);
float sb_timer_percent_left(sb_timer *timer);
