#include "sb_timer.h"

sb_timer sb_create_timer(float duration)
{
    return (sb_timer) {
        .duration = duration,
        .time = duration,
    };
}

bool sb_timer_finished(sb_timer *timer)
{
    return timer->time < 0.001f;
}

bool sb_timer_tick(sb_timer *timer, float dt)
{
    timer->just_finished = false;
    timer->time -= dt;
    if(sb_timer_finished(timer))
    {
        timer->just_finished = true;
        timer->time = timer->duration;
        return true;
    }
    return false;
}

float sb_timer_elapsed(const sb_timer *timer)
{
    return timer->duration - timer->time; 
}

bool sb_timer_just_finished(const sb_timer *timer)
{
    return timer->just_finished;
}

float sb_timer_percent_left(sb_timer *timer)
{
    return timer->time / timer->duration;
}
