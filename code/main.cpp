#include "lvl5_math.h"
#include "lvl5_context.h"
#include "game.cpp"

#include "Windows.h"

#define TARGET_FPS 60

globalvar bool running = true;
globalvar LARGE_INTEGER performance_frequency = {};
globalvar Bitmap screen = {};
globalvar BITMAPINFO screen_info = {};




struct File_Buffer {
    byte *data;
    u32 size;
};

File_Buffer win32_read_entire_file(char *file_name) {
    HANDLE file = CreateFileA(
        file_name,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    assert(file != INVALID_HANDLE_VALUE);

    DWORD file_size_high;
    DWORD file_size = GetFileSize(file, &file_size_high);

    byte *file_memory = (byte *)memalloc(file_size);
    DWORD bytes_read = 0;

    ReadFile(file, file_memory, file_size, &bytes_read, nullptr);

    File_Buffer result;
    result.data = file_memory;
    result.size = file_size;
    return result;
}

#pragma pack(push, 1)
struct Bmp_Header {
    u16 signature;
    u32 file_size;
    u32 unused;
    u32 data_offset;
};

struct Bmp_Info {
    u32 header_size;
    u32 width;
    u32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 image_size;
    u32 x_pixels_per_meter;
    u32 y_pixels_per_meter;
    u32 colors_used;
    u32 important_colors;
};
#pragma pack(pop)

Bitmap win32_read_bmp(char *file_name) {
    File_Buffer file = win32_read_entire_file(file_name);
    Bmp_Header *header = (Bmp_Header *)file.data;
    Bmp_Info *info = (Bmp_Info *)(file.data + sizeof(Bmp_Header));
    Pixel *pixels = (Pixel *)(file.data + header->data_offset);

    // AA RR GG BB
    Mem_Size data_size = sizeof(Pixel)*(info->width + 2)*(info->height + 2);
    Bitmap result = {
      .data = (Pixel *)memalloc(data_size),
      .width = (i32)info->width,
      .height = (i32)info->height,
      .pitch = (i32)info->width + 2,
    };

    memset(result.data, 0, data_size);

    for (u32 y = 0; y < info->height; y++) {
      for (u32 x = 0; x < info->width; x++) {
        Pixel pixel = pixels[y*info->width + x];
        f32 alpha = (f32)pixel.a/255.0f;
        pixel.r = (u8)(alpha*pixel.r);
        pixel.g = (u8)(alpha*pixel.g);
        pixel.b = (u8)(alpha*pixel.b);

        result.data[(y + 1)*(info->width + 2) + (x + 1)] = pixel;
      }
    }

    return result;
}

f64 win32_get_time() {
  LARGE_INTEGER counter;
  assert(QueryPerformanceCounter(&counter));
  f64 result = (f64)counter.QuadPart/(f64)performance_frequency.QuadPart;
  return result;
}

void win32_resize_screen_buffer(HWND window) {
  RECT window_rect;
  assert(GetClientRect(window, &window_rect));

  screen.width = window_rect.right;
  screen.height = window_rect.bottom;
  screen.pitch = screen.width;
  if (screen.data) {
    memfree(screen.data);
  }
  screen.data = (Pixel *)memalloc(sizeof(u32)*(u32)window_rect.right*(u32)window_rect.bottom);

  screen_info = {
    .bmiHeader = {
      .biSize = sizeof(BITMAPINFOHEADER),
      .biWidth = screen.width,
      .biHeight = screen.height,
      .biPlanes = 1,
      .biBitCount = 32,
      .biCompression = BI_RGB,
    },
  };
}

LRESULT CALLBACK WindowProc(
  HWND window,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
  switch (message) {
    case WM_CLOSE:
    case WM_QUIT: {
      running = false;
    } break;

    case WM_SIZE: {
      win32_resize_screen_buffer(window);
    } break;
  }

  return DefWindowProcA(window, message, w_param, l_param);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR command_line, int show_command_line) {
  // program start
  assert(timeBeginPeriod(1) == TIMERR_NOERROR);
  assert(QueryPerformanceFrequency(&performance_frequency));

  char *class_name = "window_class_name";
  WNDCLASSA window_class = {
    .style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC,
    .lpfnWndProc = WindowProc,
    .hInstance = instance,
    .hCursor = LoadCursor(nullptr, IDC_ARROW),
    .lpszClassName = class_name,
  };

  ATOM registered_class = RegisterClassA(&window_class);
  assert(registered_class);

  HWND window = CreateWindowExA(
    0, //ex styles
    class_name,
    "Cool window name",
    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    CW_USEDEFAULT, //x
    CW_USEDEFAULT, //y
    CW_USEDEFAULT, //width
    CW_USEDEFAULT, //height
    nullptr,
    nullptr,
    instance,
    nullptr
  );
  assert(window);
  // SetProcessDPIAware();
  
  HDC device_context = GetDC(window);
  win32_resize_screen_buffer(window);

  f64 prev_time = win32_get_time();


  while (running) { 
    MSG message;

    while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {
      switch (message.message) {
        default: {
          DefWindowProcA(window, message.message, message.wParam, message.lParam);
        } break;
      }
    }

    game_update(screen);
  
    StretchDIBits(
      device_context,
      0, 0, screen.width, screen.height,
      0, 0, screen.width, screen.height,
      screen.data,
      &screen_info,
      DIB_RGB_COLORS,
      SRCCOPY
    );

    f64 target_time = 1.0/TARGET_FPS;

    #define get_frame_time() (win32_get_time() - prev_time)
    f64 frame_time = get_frame_time();

    char buffer[256];
    sprintf_s(buffer, array_count(buffer), "ms: %0.2f", frame_time*1000);
    OutputDebugStringA(buffer);

    f64 seconds_left = target_time - frame_time;
    if (seconds_left > 0) {
      u32 sleep_ms = (u32)(seconds_left*1000);
      Sleep(sleep_ms);

      frame_time = get_frame_time();
      while (frame_time < target_time) {
        frame_time = get_frame_time();
      }
    }

    prev_time = win32_get_time();
  }

  return 0;
}
