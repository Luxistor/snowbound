#include "sb_window.h"
#include "sb_math.h"
#include "sb_common.h"
#include "sb_arena.h"

bool sb_is_key_down(sb_keycode key_code)
{
    return GetKeyState(key_code) & 0x8000;
}

float sb_get_aspect_ratio(sb_window *window)
{
    return window->width / (float) window->height;
}

sb_window *sb_create_window(uint32_t width, uint32_t height, const char *name)
{
    sb_window *window = malloc(sizeof(sb_window));
    assert(window);

    window->instance = GetModuleHandle(NULL);
    window->width = width;
    window->height = height;
    window->event = (sb_window_event) {0};
    window->last_tick_count = (LARGE_INTEGER) {0};
    window->current_tick_count = (LARGE_INTEGER) {0};

    QueryPerformanceFrequency(&window->frequency);

    WNDCLASS window_class = {0};
    window_class.lpfnWndProc = sb_window_proc;
    window_class.hInstance = window->instance;
    window_class.lpszClassName = name;
    RegisterClass(&window_class);

    window->handle = CreateWindowEx(
        0,
        name, name,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL,
        NULL,
        window->instance,
        window
    );

    assert(window->handle);
    ShowWindow(window->handle, 5);
    return window;
}

float sb_get_dt(sb_window *window)
{
    float dt_seconds = (float)(window->current_tick_count.QuadPart - window->last_tick_count.QuadPart) / window->frequency.QuadPart;
    // clamp between 2.5 fps and 3000 fps
    return sb_clamp(dt_seconds, 0.00033f,0.40f);
}

bool sb_poll_events(sb_window *window, sb_window_event *event)
{
    // dt implementation
    {
        window->last_tick_count = window->current_tick_count;
        QueryPerformanceCounter(&window->current_tick_count);
    }

    MSG msg;
    // hwnd is null on purpose
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch(msg.message)
        {
        case(WM_QUIT): return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    *event = window->event;
    window->event = (sb_window_event) {0};
    return true;
}

void sb_wait_events(void)
{
    assert(WaitMessage());
}

VkSurfaceKHR sb_create_surface(sb_window *window, VkInstance vk_instance)
{
    VkWin32SurfaceCreateInfoKHR create_info = {0};
    create_info.hinstance = window->instance;
    create_info.hwnd = window->handle,
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

    VkSurfaceKHR surface;
    VK_CHECK(vkCreateWin32SurfaceKHR(vk_instance, &create_info, NULL, &surface));
    return surface;
}

VkExtent2D sb_get_window_extent(sb_window *window)
{
    return (VkExtent2D) {window->width, window->height};
}

LRESULT CALLBACK sb_window_proc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    sb_window *window = NULL;
    #define get_window() window = (sb_window*) GetWindowLongPtr(window_handle, GWLP_USERDATA)

    switch(message)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_CREATE:
            CREATESTRUCT *create = (CREATESTRUCT*) lparam;
            window = create->lpCreateParams;
            SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR) window);
            return 0;
        case WM_SIZE:
            get_window();
            window->width = LOWORD(lparam);
            window->height = HIWORD(lparam);
            window->event.flags |= SB_WINDOW_RESIZED_FLAG;
            return 0;
        case WM_MOUSEWHEEL:
            get_window();
            window->event.scroll_delta = GET_WHEEL_DELTA_WPARAM(wparam)/WHEEL_DELTA;
            return 0;
        case WM_KEYDOWN:
            get_window();
            if((HIWORD(lparam) & KF_REPEAT) == KF_REPEAT) return 0;
            window->event.keys_just_pressed[wparam] = true;
            return 0;
        case WM_KEYUP:
            get_window();
            window->event.keys_just_released[wparam] = true;
            return 0;
        default:
            return DefWindowProc(window_handle, message, wparam, lparam);
    }

    #undef get_window
}
