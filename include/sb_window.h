#ifndef SB_WINDOW_H
#define SB_WINDOW_H

#define SB_WINDOWS_OS_FLAG
#define SB_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
#define VK_USE_PLATFORM_WIN32_KHR

#include "sb_arena.h"
#include "sb_common.h"

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef enum
{
    SB_KEY_CODE_NULL = 0,
    SB_KEY_CODE_W = 'W',
    SB_KEY_CODE_A = 'A',
    SB_KEY_CODE_S = 'S',
    SB_KEY_CODE_D = 'D',
    SB_KEY_CODE_Q = 'Q',
    SB_KEY_CODE_E = 'E',
    SB_KEY_CODE_R = 'R',
    SB_KEY_CODE_T = 'T',
    SB_KEY_CODE_I = 'I',
    SB_KEY_CODE_O = 'O',
    SB_KEY_CODE_COUNT,
} sb_keycode;

bool sb_is_key_down(sb_keycode key_code);

typedef enum
{
    SB_WINDOW_RESIZED_FLAG = (1 << 0),
} sb_window_event_flags;

typedef struct
{
    uint64_t flags;
    bool keys_just_pressed[UINT8_MAX];
    bool keys_just_released[UINT8_MAX];
    int scroll_delta;
} sb_window_event;

typedef struct
{
    HWND handle;
    HINSTANCE instance;

    sb_window_event event;
    uint32_t width, height;

    LARGE_INTEGER frequency;
    LARGE_INTEGER last_tick_count;
    LARGE_INTEGER current_tick_count;
} sb_window;

float sb_get_dt(sb_window *window);
float sb_get_aspect_ratio(sb_window *window);

sb_window *sb_create_window(uint32_t width, uint32_t height, const char *name);
bool sb_poll_events(sb_window *window, sb_window_event *event);
void sb_wait_events(void);
VkSurfaceKHR sb_create_surface(sb_window *window, VkInstance vk_instance);
VkExtent2D sb_get_window_extent(sb_window *window);

LRESULT CALLBACK sb_window_proc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam);

#endif
