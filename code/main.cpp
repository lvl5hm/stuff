#include "lvl5_math.h"
#include "game.cpp"

#include "Windows.h"
#include <intrin.h>

#define TARGET_FPS 60

globalvar bool running = true;
globalvar LARGE_INTEGER performance_frequency = {};
globalvar Bitmap screen = {};
globalvar BITMAPINFO screen_info = {};
globalvar Mouse mouse;

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
  u32 red_mask;
  u32 green_mask;
  u32 blue_mask;
  u32 alpha_mask;
  u32 cs_type;
  i32 red_x;
  i32 red_y;
  i32 red_z;
  i32 green_x;
  i32 green_y;
  i32 green_z;
  i32 blue_x;
  i32 blue_y;
  i32 blue_z;
  u32 gamma_red;
  u32 gamma_green;
  u32 gamma_blue;
};

struct Bmp_Header {
  u16 signature;
  u32 file_size;
  u32 reserved;
  u32 data_offset;
  Bmp_Info info;
};
#pragma pack(pop)

#define BMP_SIGNATURE (('B' << 0) | ('M' << 8))

void win32_save_bmp(char *file_name, Bitmap bmp) {
  Bmp_Info info = {0};
  info.header_size = 124;
  info.width = (u32)bmp.width;
  info.height = (u32)bmp.height;
  info.planes = 1;
  info.bits_per_pixel = 32;
  info.compression = 3;
  info.image_size = (u32)bmp.width*(u32)bmp.height*4;
  info.x_pixels_per_meter = 11811;
  info.y_pixels_per_meter = 11811;
  info.colors_used = 0;
  info.important_colors = 0;
  info.red_mask = 0x00FF0000;
  info.green_mask = 0x0000FF00;
  info.blue_mask = 0x000000FF;
  info.alpha_mask = 0xFF000000;
  info.cs_type = 0x73524742;
	
  Bmp_Header header = {0};
  header.signature = BMP_SIGNATURE;
  header.data_offset = 138;
  header.file_size = header.data_offset + info.image_size;
  header.reserved = 0;
  header.info = info;
  
  FILE *file;
  fopen_s(&file, file_name, "wb");
  fwrite(&header, sizeof(header), 1, file);
  fseek(file, (i32)header.data_offset, SEEK_SET);
  fwrite(bmp.data, info.image_size, 1, file);
  
  fclose(file);
}

Bitmap win32_read_bmp(char *file_name) {
    File_Buffer file = win32_read_entire_file(file_name);
    Bmp_Header *header = (Bmp_Header *)file.data;
    Bmp_Info *info = &header->info;
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


Texture_Atlas texture_atlas_make_from_bitmaps(Bitmap *bitmaps, i32 bitmap_count, i32 atlas_width) {
  Texture_Atlas result = {0};
  result.rects = (Rect2i *)memalloc(sizeof(Rect2i)*(u32)bitmap_count);
  result.count = bitmap_count;
  
  i32 total_height = 0;
  
  i32 row_height = 0;
  i32 row_width = 0;
  
  i32 PADDING = 4;
  
  for (i32 i = 0; i < bitmap_count; i++) {
    Bitmap bmp = bitmaps[i];
    if (row_width + (i32)bmp.width > atlas_width) {
      total_height += row_height;
      row_width = 0;
      row_height = 0;
    }
    
    result.rects[i] = rect2i_min_size({row_width, total_height},
                                      {(i32)bmp.width, (i32)bmp.height});
    row_width += (i32)bmp.width + PADDING;
    if (bmp.height > row_height) {
      row_height = (i32)bmp.height;
    }
  }
  
  total_height += row_height;
  result.bmp = make_empty_bitmap((i32)atlas_width, (i32)total_height);
  
  for (i32 i = 0; i < bitmap_count; i++) {
    Rect2i rect = result.rects[i];
    Bitmap bmp = bitmaps[i];
    
    for (i32 y = 0; y < bmp.height; y++) {
      for (i32 x = 0; x < bmp.width; x++) {
        result.bmp.data[(rect.min.y + y)*result.bmp.width + rect.min.x + x] = bmp.data[y*bmp.width + x];
      }
    }
  }

  win32_save_bmp("foo.bmp", result.bmp);
  
  return result;
}

Font os_load_font(char *file_name, char *font_name, i32 font_size) {
  Font font = {0};
  
  HDC device_context = CreateCompatibleDC(GetDC(nullptr));
  bool font_added = AddFontResourceExA(file_name, FR_PRIVATE, nullptr);
  assert(font_added);

  Mem_Size scratch_mark = scratch_get_mark();
  
  HFONT win_font = CreateFontA(
    font_size, 0,
    0, 0,
    FW_NORMAL, //weight
    FALSE, //italic
    FALSE, //underline
    FALSE, // strikeout
    DEFAULT_CHARSET,
    OUT_DEFAULT_PRECIS,
    CLIP_DEFAULT_PRECIS,
    ANTIALIASED_QUALITY,
    DEFAULT_PITCH|FF_DONTCARE,
    font_name);
  
  u32 *font_buffer_pixels = nullptr;
  
  i32 font_buffer_width = 256;
  i32 font_buffer_height = 256;
  
  BITMAPINFO font_buffer_bi = {0};
  font_buffer_bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  font_buffer_bi.bmiHeader.biWidth = font_buffer_width;
  font_buffer_bi.bmiHeader.biHeight = font_buffer_height;
  font_buffer_bi.bmiHeader.biPlanes = 1;
  font_buffer_bi.bmiHeader.biBitCount = 32;
  font_buffer_bi.bmiHeader.biCompression = BI_RGB;
  font_buffer_bi.bmiHeader.biSizeImage = 0;
  font_buffer_bi.bmiHeader.biXPelsPerMeter = 0;
  font_buffer_bi.bmiHeader.biYPelsPerMeter = 0;
  font_buffer_bi.bmiHeader.biClrUsed = 0;
  font_buffer_bi.bmiHeader.biClrImportant = 9;
  
  HBITMAP font_buffer = CreateDIBSection(
    device_context,
    &font_buffer_bi,
    DIB_RGB_COLORS,
    (void **)&font_buffer_pixels,
    nullptr, 0);
  SelectObject(device_context, font_buffer);
  SelectObject(device_context, win_font);
  
  
  b32 metrics_bytes = GetOutlineTextMetrics(device_context, 0, nullptr);
  OUTLINETEXTMETRIC *metric = (OUTLINETEXTMETRIC *)scratch_alloc(metrics_bytes);
  GetOutlineTextMetrics(device_context, metrics_bytes, metric);
  
  
  
  SetBkColor(device_context, RGB(0, 0, 0));
  
  char first_codepoint = 0;
  char last_codepoint = '~';
  
  i32 codepoint_count = (last_codepoint - first_codepoint + 1);
  Bitmap *codepoint_bitmaps = (Bitmap*)scratch_alloc(sizeof(Bitmap)*(u32)(codepoint_count + 1));
  // 1 extra bitmap for white pixel
  
  font = {
    .first_codepoint = first_codepoint,
    .advance = (i8 *)memalloc(sizeof(i8)*(u32)codepoint_count),
    .kerning = (i8 *)memalloc(sizeof(i8)*(u32)(codepoint_count*codepoint_count)),
    .origins = (V2 *)memalloc(sizeof(V2)*(u32)codepoint_count),
    .codepoint_count = codepoint_count,
  };
  memset(font.advance, 0, sizeof(i8)*(u32)codepoint_count);
  memset(font.kerning, 0, sizeof(i8)*(u32)(codepoint_count*codepoint_count));
  
  ABC *abcs = (ABC *)scratch_alloc(sizeof(ABC)*(u32)font.codepoint_count);
  GetCharABCWidthsW(device_context, (u32)first_codepoint, (u32)first_codepoint + (u32)codepoint_count, abcs);
  
  for (i32 codepoint_index = 0;
        codepoint_index < codepoint_count;
        codepoint_index++)
  {
    wchar_t codepoint = (wchar_t)(first_codepoint + codepoint_index);
    
    bool bg_blitted = PatBlt(device_context, 
                            0, 0, font_buffer_width, font_buffer_height, BLACKNESS);
    assert(bg_blitted);
    SetTextColor(device_context, RGB(255, 255, 255));
    bool written_text = TextOutW(device_context, 0, 0, &codepoint, 1);
    
    i32 min_x = 10000;
    i32 min_y = 10000;
    i32 max_x = -10000;
    i32 max_y = -10000;
    
    
    u32 *pixel = font_buffer_pixels;
    for (i32 y = 0; y < font_buffer_height; y++) {
      for (i32 x = 0; x < font_buffer_width; x++) {
        u32 color_ref = *(pixel++);
        if (color_ref != 0) {
          if (x < min_x) min_x = x;
          if (x > max_x) max_x = x;
          if (y < min_y) min_y = y;
          if (y > max_y) max_y = y;
        }
      }
    }
    
    if (min_x == 10000) {
      min_x = 0;
      min_y = 0;
      max_x = 0;
      max_y = 0;
    } else {
      min_x--;
      min_y--;
      max_x++;
      max_y++;
    }
    
    
    Bitmap bmp = make_empty_bitmap(max_x - min_x, max_y - min_y);
    
    for (i32 y = 0; y < bmp.height; y++) {
      for (i32 x = 0; x < bmp.width; x++) {
        u32 src_pixel = font_buffer_pixels[(min_y + y)*font_buffer_width + min_x + x];
        u8 intensity = (u8)((src_pixel & 0x00FF0000) >> 16);
        Pixel new_pixel = pixel_u32(0xFF, 0xFF, 0xFF, intensity);
        bmp.data[y*bmp.width + x] = new_pixel;
      }
    }
    
    codepoint_bitmaps[codepoint_index] = bmp;
    
    ABC abc = abcs[codepoint_index];
    i8 total_width = (i8)(abc.abcA + (i32)abc.abcB + (i32)abc.abcC);
    font.advance[codepoint_index] = total_width;
    font.origins[codepoint_index] = {(f32)min_x, (f32)(min_y - font_buffer_height)};
  }
  
  font.advance[first_codepoint] = font.advance[' ' - first_codepoint];
  
  Bitmap white_bitmap = make_empty_bitmap(2, 2);
  white_bitmap.data[0] = {0xFFFFFFFF};
  white_bitmap.data[1] = {0xFFFFFFFF};
  white_bitmap.data[2] = {0xFFFFFFFF};
  white_bitmap.data[3] = {0xFFFFFFFF};
  codepoint_bitmaps[codepoint_count] = white_bitmap;
  
  font.line_spacing = (i8)((i32)metric->otmLineGap + metric->otmAscent - (i32)metric->otmDescent);
  font.line_height = (i8)metric->otmTextMetrics.tmHeight;
  font.descent = (i8)metric->otmTextMetrics.tmDescent;
  font.atlas = texture_atlas_make_from_bitmaps(codepoint_bitmaps, codepoint_count+1, 512);
  
  DWORD kerning_pair_count = GetKerningPairs(device_context, I32_MAX, nullptr);
  KERNINGPAIR *kerning_pairs = (KERNINGPAIR *)memalloc(sizeof(KERNINGPAIR)*kerning_pair_count);
  GetKerningPairs(device_context, kerning_pair_count, kerning_pairs);
  for (DWORD i = 0; i < kerning_pair_count; i++) {
    KERNINGPAIR pair = kerning_pairs[i];
    assert(pair.wFirst >= font.first_codepoint);
    
    char first = (char)pair.wFirst - font.first_codepoint;
    char second = (char)pair.wSecond - font.first_codepoint;
    assert(pair.iKernAmount < I8_MAX);
    assert(pair.iKernAmount > I8_MIN);
    font.kerning[first*codepoint_count + second] += (i8)pair.iKernAmount;
  }
  
  scratch_set_mark(scratch_mark);

  return font;
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

void win32_process_button(Button *button, bool is_down) {
  if (is_down && !button->is_down) {
    button->went_down = true;
  } else if (!is_down && button->is_down) {
    button->went_up = true;
  }
  button->is_down = is_down;
}

LRESULT CALLBACK WindowProc(
  HWND window,
  UINT message,
  WPARAM w_param,
  LPARAM l_param
) {
  LRESULT result = 0;

  switch (message) {
    case WM_CLOSE:
    case WM_QUIT: {
      running = false;
    } break;

    case WM_SIZE: {
      win32_resize_screen_buffer(window);
    } break;

    default: {
      result = DefWindowProcA(window, message, w_param, l_param);
    }
  }
  return result;
}



struct Thread_Queue_Item {
  Worker_Fn fn;
  void *data;
};

#define THREAD_QUEUE_CAPACITY 64

struct Thread_Queue {
  Thread_Queue_Item items[THREAD_QUEUE_CAPACITY];
  volatile u32 write_cursor;
  volatile u32 read_cursor;
  volatile u32 completion_cursor;
  volatile u32 target_cursor;
  HANDLE semaphore;
};

struct Thread {
  Thread_Queue *queue;
  u32 thread_index;
};

globalvar bool __run_threads = true;

bool do_thread_task(Thread_Queue *queue) {
  bool result = true;

  u32 old_read_cursor = queue->read_cursor;
  if (old_read_cursor != queue->write_cursor) {
    u32 old_write_cursor = queue->write_cursor;
    u32 new_read_cursor = (old_read_cursor + 1) % THREAD_QUEUE_CAPACITY;
    if (InterlockedCompareExchange(&queue->read_cursor, new_read_cursor, old_read_cursor) == old_read_cursor) {
      Thread_Queue_Item *item = queue->items + old_read_cursor;
      item->fn(item->data);
      InterlockedIncrement(&queue->completion_cursor);
    }
  } else {
    result = false;
  }

  return result;
}

DWORD WINAPI thread_proc(void *data) {
  Thread *thread = (Thread *)data;
  Thread_Queue *queue = thread->queue;
  while (__run_threads) {
    bool did_task = do_thread_task(queue);
    if (!did_task) {
      WaitForSingleObject(queue->semaphore, INFINITE);
    }
  }

  return 0;
}

void print_some_shit(u64 num) {
  char buffer[128];
  sprintf_s(buffer, 128, "%lld\n", num);
  OutputDebugStringA(buffer);
}

void add_thread_task(Thread_Queue *queue, Worker_Fn fn, void *data) {
  Thread_Queue_Item item = {
    .fn = fn,
    .data = data,
  };

  u32 old_write_cursor = queue->write_cursor;
  u32 old_read_cursor = queue->read_cursor;

  u32 new_write_cursor = (old_write_cursor + 1) % THREAD_QUEUE_CAPACITY;
  assert(new_write_cursor != old_read_cursor);

  queue->items[old_write_cursor] = item;

  _WriteBarrier();
  queue->write_cursor = new_write_cursor;
  queue->target_cursor++;
  ReleaseSemaphore(queue->semaphore, 1, nullptr);
}

void wait_for_all_tasks(Thread_Queue *queue) {
  while (queue->target_cursor != queue->completion_cursor) {
    do_thread_task(queue);
  }
  queue->target_cursor = 0;
  queue->completion_cursor = 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR command_line, int show_command_line) {
  init_default_context();

  Font font = os_load_font("ubuntu_mono.ttf", "Ubuntu Mono", 32);
  font.atlas.bmp = win32_read_bmp("foo.bmp");
  // win32_save_bmp("foo2.bmp", font.atlas.bmp);

  u32 logical_core_count = 5;

  Thread *threads = (Thread *)memalloc(sizeof(Thread)*logical_core_count);
  Thread_Queue thread_queue = {
    .semaphore = CreateSemaphoreA(nullptr, 0, THREAD_QUEUE_CAPACITY, nullptr),
  };


  for (u32 thread_index = 0; thread_index < logical_core_count - 1; thread_index++) {
    Thread *thread = threads + thread_index;
    thread->thread_index = thread_index;
    thread->queue = &thread_queue;

    u32 thread_id;
    HANDLE thread_handle = CreateThread(nullptr, 0, thread_proc, thread, 0, &thread_id);
  }


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
    scratch_reset();

    MSG message;

    POINT mouse_p;
    assert(GetCursorPos(&mouse_p));
    ScreenToClient(window, &mouse_p);


    mouse.p = {(f32)mouse_p.x, (f32)(screen.height - mouse_p.y)};
    mouse.left.went_down = false;
    mouse.left.went_up = false;
    mouse.right.went_down = false;
    mouse.right.went_up = false;

    while (PeekMessageA(&message, window, 0, 0, PM_REMOVE)) {
      switch (message.message) {
        case WM_LBUTTONDOWN: {
          win32_process_button(&mouse.left, true);
        } break;
        case WM_LBUTTONUP: {
          win32_process_button(&mouse.left, false);
        } break;
        case WM_RBUTTONDOWN: {
          win32_process_button(&mouse.right, true);
        } break;
        case WM_RBUTTONUP: {
          win32_process_button(&mouse.right, false);
        } break;

        default: {
          DefWindowProcA(window, message.message, message.wParam, message.lParam);
        } break;
      }
    }

    Input input = { mouse };

    game_update(screen, input, &thread_queue, &font);
  
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

    // char buffer[256];
    // sprintf_s(buffer, array_count(buffer), "ms: %0.2f", frame_time*1000);
    // OutputDebugStringA(buffer);

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
