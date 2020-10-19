#include "lvl5_types.h"
#include <memory.h>

extern "C" void OutputDebugStringA(const char *);

globalvar f64 foo_total = 0;
globalvar f64 foo_count = 0;


struct Timed_Block {
  u64 m_stamp;
  u64 m_count;

  Timed_Block(u64 count = 1) {
    m_stamp = __rdtsc();
    m_count = count;
  }

  ~Timed_Block() {
    f64 time_per_count = (f64)(__rdtsc() - m_stamp)/(f64)m_count;
    foo_total += time_per_count;
    foo_count++;

    char buffer[64];
    sprintf_s(buffer, array_count(buffer), "%0.02f\n", foo_total/foo_count);
    OutputDebugStringA(buffer);
  }
};

#define TIMED_BLOCK(name, count) Timed_Block __##name(count)

union Pixel {
  u32 rgba;
  struct {
    u8 b;
    u8 g;
    u8 r;
    u8 a;
  };
};

struct Bitmap {
  i32 width;
  i32 height;
  i32 pitch;
  Pixel *data;
};


struct Grid_Props {
  i32 cols, rows;
};

struct Element {
  Rect2 rect;
  Element *children;

  Pixel color;
  Grid_Props grid_props;
};

struct Layout {
  Element root;
  Element *current_parent;
};

void draw_rect(Bitmap screen, Rect2 rect, Pixel color) {
  Rect2i screen_rect = {{0, 0}, {screen.width, screen.height}};
  Rect2i paint_rect = intersect(screen_rect, rect2i(rect));
  
  if (has_area(paint_rect)) {
    for (i32 y = paint_rect.min.y; y < paint_rect.max.y; y++) {
      for (i32 x = paint_rect.min.x; x < paint_rect.max.x; x++) {
        screen.data[y*screen.pitch + x] = color;
      }
    }
  }
}

#define RED Pixel{0xFFFF0000}
#define BLUE Pixel{0xFF0000FF}
#define GREEN Pixel{0xFF0000FF}
#define BLACK Pixel{0xFF000000}
#define WHITE Pixel{0xFFFFFFFF}
#define YELLOW Pixel{0xFFFF00FF}
#define PINK Pixel{0xFFff63ed}

globalvar Layout _layout = {};


globalvar bool loaded = false;

void ui_grid_begin(Layout *layout, Grid_Props props) {
  Element e = {
    .children = sb_make(Element, 16),
  };

  // if (!layout->root) {
  //   layout->root = e;
  //   layout->current_parent = &layout->root;
  // } else {
  //   sb_push(layout->current_parent->children, e);
  // }
}

Pixel lerp(Pixel a, Pixel b, f32 c) {
  Pixel result = {
    .r = (u8)(a.r*(1 - c) + b.r*c),
    .g = (u8)(a.g*(1 - c) + b.g*c),
    .b = (u8)(a.b*(1 - c) + b.b*c),
    .a = (u8)(a.a*(1 - c) + b.a*c),
  };
  return result;
}

struct Bilinear_Sample {
  Pixel a, b, c, d;
};

Bilinear_Sample get_bilinear_sample(Bitmap bmp, V2i coords) {
  Pixel *texel_ptr = bmp.data + coords.y*bmp.pitch + coords.x;
  Bilinear_Sample result = {
    *texel_ptr,
    *(texel_ptr + 1),
    *(texel_ptr + bmp.pitch),
    *(texel_ptr + bmp.pitch + 1),
  };
  return result;
}

V4_8x pixel_u32_to_v4_8x(i32_8x p) {
  V4_8x result;
  result.a = to_f32_8x(p >> 24);
  result.r = to_f32_8x((p >> 16) & 0xFF);
  result.g = to_f32_8x((p >> 8) & 0xFF);
  result.b = to_f32_8x(p & 0xFF);
  return result;
}

i32_8x pixel_v4_to_u32_8x(V4_8x p) {
  i32_8x a = to_i32_8x(p.a) << 24;
  i32_8x r = to_i32_8x(p.r) << 16;
  i32_8x g = to_i32_8x(p.g) << 8;
  i32_8x b = to_i32_8x(p.b);
  i32_8x result = r | g | b | a;
  return result;
}

struct Bilinear_Sample_8x {
  V4_8x a, b, c, d;
};
Bilinear_Sample_8x get_bilinear_sample(Bitmap bmp, V2i_8x coords) {
  i32_8x offset = coords.y*bmp.pitch + coords.x;
  i32_8x a = gather_i32(bmp.data, offset);
  i32_8x b = gather_i32(bmp.data + 1, offset);
  i32_8x c = gather_i32(bmp.data + bmp.pitch, offset);
  i32_8x d = gather_i32(bmp.data + bmp.pitch + 1, offset);

  Bilinear_Sample_8x result;
  result.a = pixel_u32_to_v4_8x(a);
  result.b = pixel_u32_to_v4_8x(b);
  result.c = pixel_u32_to_v4_8x(c);
  result.d = pixel_u32_to_v4_8x(d);
  return result;
}

Pixel bilinear_blend(Bilinear_Sample sample, V2 c) {
  Pixel ab = lerp(sample.a, sample.b, c.x);
  Pixel cd = lerp(sample.c, sample.d, c.x);
  Pixel abcd = lerp(ab, cd, c.y);
  return abcd;
}

V4_8x lerp(V4_8x a, V4_8x b, f32_8x c) {
  V4_8x result = a*(1 - c) + b*c;
  return result;
}


V4_8x bilinear_blend(Bilinear_Sample_8x sample, V2_8x c) {
  V4_8x ab = lerp(sample.a, sample.b, c.x);
  V4_8x cd = lerp(sample.c, sample.d, c.x);
  V4_8x abcd = lerp(ab, cd, c.y);
  return abcd;
}

void draw_bitmap_avx(Bitmap screen, Rect2 rect, f32 angle, Bitmap bmp) {
  Rect2i screen_rect = {{0, 0}, {screen.width, screen.height}};

  Rect2 bitmap_rect = add_radius(rect, {1, 1});
  V2 bitmap_rect_size = get_size(bitmap_rect);

  V2 x_axis = V2{cosf(angle), -sinf(angle)}*bitmap_rect_size.x;
  V2 y_axis = V2{sinf(angle), cosf(angle)}*bitmap_rect_size.y;
  V2 origin = get_center(bitmap_rect) - x_axis*0.5f - y_axis*0.5f;

  V2 vertices[] = {origin, origin + x_axis, origin + y_axis, origin + x_axis + y_axis};

  Rect2 drawn_rect = inverted_infinity_rect();
  for (u32 vertex_index = 0; vertex_index < array_count(vertices); vertex_index++) {
    V2 vertex = vertices[vertex_index];

    drawn_rect.min.x = min(drawn_rect.min.x, vertex.x);
    drawn_rect.min.y = min(drawn_rect.min.y, vertex.y);
    drawn_rect.max.x = max(drawn_rect.max.x, vertex.x);
    drawn_rect.max.y = max(drawn_rect.max.y, vertex.y);
  }

  Rect2i paint_rect = intersect(screen_rect, rect2i(drawn_rect));
  V2 rect_size = get_size(drawn_rect);

  if (paint_rect.min.x & 7) {
    paint_rect.min.x = paint_rect.min.x & (~7);
  }

  if (paint_rect.max.x & 7) {
    paint_rect.max.x = (paint_rect.max.x & (~7)) + 8;
  }
  V2i paint_size = get_size(paint_rect);


  V2 texture_size = v2(bmp.width, bmp.height);
  V2 pixel_scale = rect_size/texture_size;

  V2 inverse_axis_length_sqr = 1/sqr(bitmap_rect_size);

  V2_8x x_axis_8x = set8(x_axis);
  V2_8x y_axis_8x = set8(y_axis);
  V2_8x inverse_axis_length_sqr_8x = set8(inverse_axis_length_sqr);

  V2_8x texture_size_8x = set8(texture_size);
  V2_8x pixel_scale_8x = set8(pixel_scale);
  V2_8x origin_8x = set8(origin);

  f32_8x pixel_x_offsets = {0, 1, 2, 3, 4, 5, 6, 7};

  V2_8x texture_size_with_apron = texture_size_8x + 2/pixel_scale_8x;
  
  if (has_area(paint_rect)) {
    TIMED_BLOCK(draw_pixel_avx, (u64)get_area(paint_rect));

    for (i32 y = paint_rect.min.y; y < paint_rect.max.y; y++) {
      for (i32 x = paint_rect.min.x; x < paint_rect.max.x; x += 8) {
        f32_8x x_float = set8((f32)x);
        f32_8x y_float = set8((f32)y);
        V2_8x d = V2_8x{x_float + pixel_x_offsets, y_float} - origin_8x;

        V2_8x uv01 = V2_8x{dot(d, x_axis_8x), dot(d, y_axis_8x)}*inverse_axis_length_sqr_8x;
        V2_8x uv = uv01*texture_size_with_apron;

        V2_8x floored_uv = floor(uv);
        V2_8x fract_uv = clamp01((uv - floored_uv)*pixel_scale_8x);

        Bilinear_Sample_8x sample = get_bilinear_sample(bmp, v2i_8x(floored_uv));
        V4_8x texel = bilinear_blend(sample, fract_uv);

        Pixel *pixel_ptr = screen.data + y*screen.pitch + x;
        i32_8x pixel_u32 = load_i32_8x(pixel_ptr);
        V4_8x pixel = pixel_u32_to_v4_8x(pixel_u32);

        V3_8x result_rgb = texel.rgb + pixel.rgb*(1 - texel.a/255.0f);
        i32_8x result = pixel_v4_to_u32_8x(v4_8x(result_rgb, set8(255)));

        i32_8x write_mask = to_i32_8x((uv01.x >= 0) & (uv01.x < 1) & (uv01.y >= 0) & (uv01.y < 1));
        mask_store_i32_8x(pixel_ptr, write_mask, result);
      }
    }
  }
}

void draw_bitmap(Bitmap screen, Rect2 rect, f32 angle, Bitmap bmp) {
  Rect2i screen_rect = {{0, 0}, {screen.width, screen.height}};

  Rect2 bitmap_rect = add_radius(rect, {1, 1});
  V2 bitmap_rect_size = get_size(bitmap_rect);

  V2 x_axis = V2{cosf(angle), -sinf(angle)}*bitmap_rect_size.x;
  V2 y_axis = V2{sinf(angle), cosf(angle)}*bitmap_rect_size.y;
  V2 origin = get_center(bitmap_rect) - x_axis*0.5f - y_axis*0.5f;

  V2 vertices[] = {origin, origin + x_axis, origin + y_axis, origin + x_axis + y_axis};

  Rect2 drawn_rect = inverted_infinity_rect();
  for (u32 vertex_index = 0; vertex_index < array_count(vertices); vertex_index++) {
    V2 vertex = vertices[vertex_index];

    drawn_rect.min.x = min(drawn_rect.min.x, vertex.x);
    drawn_rect.min.y = min(drawn_rect.min.y, vertex.y);
    drawn_rect.max.x = max(drawn_rect.max.x, vertex.x);
    drawn_rect.max.y = max(drawn_rect.max.y, vertex.y);
  }

  Rect2i paint_rect = intersect(screen_rect, rect2i(drawn_rect));

  V2 rect_size = get_size(drawn_rect);
  V2i paint_size = get_size(paint_rect);

  V2 texture_size = v2(bmp.width, bmp.height);
  V2 pixel_scale = rect_size/texture_size;

  V2 inverse_axis_length_sqr = 1/sqr(bitmap_rect_size);
  
  if (has_area(paint_rect)) {
    TIMED_BLOCK(draw_pixel_slow, (u64)get_area(paint_rect));

    for (i32 y = paint_rect.min.y; y < paint_rect.max.y; y++) {
      for (i32 x = paint_rect.min.x; x < paint_rect.max.x; x++) {
        V2 d = v2(x, y) - origin;
        V2 uv01 = V2{dot(d, x_axis), dot(d, y_axis)}*inverse_axis_length_sqr;

        if (uv01.x >= 0 && uv01.y >= 0 && uv01.x < 1 && uv01.y < 1) {
          V2 uv = uv01*(texture_size + 2/pixel_scale);
          V2 uv_floored = floor(uv);
          V2 uv_fract = clamp01((uv - uv_floored)*pixel_scale);

          Bilinear_Sample sample = get_bilinear_sample(bmp, v2i(uv_floored));
          Pixel mixed_sample = bilinear_blend(sample, uv_fract);

          Pixel pixel = screen.data[y*screen.pitch + x];
          f32 alpha = (f32)mixed_sample.a/255.0f;
          Pixel result = {
            .r = (u8)(mixed_sample.r + pixel.r*(1 - alpha)),
            .g = (u8)(mixed_sample.g + pixel.g*(1 - alpha)),
            .b = (u8)(mixed_sample.b + pixel.b*(1 - alpha)),
          };
          screen.data[y*screen.pitch + x] = result;
        }
      }
    }
  }
}

Bitmap win32_read_bmp(char *);
globalvar Bitmap test_bmp;

void game_update(Bitmap screen) {
  if (!loaded) {
    loaded = true;
    test_bmp = win32_read_bmp("test.bmp");

    {
      // TIMED_BLOCK(haha, 1);

      // #define TEST_SIZE 10000
      // i32 *foo = (i32 *)memalloc(sizeof(i32)*TEST_SIZE*TEST_SIZE);
      // for (i32 y = 0; y < TEST_SIZE; y++) {
      //   for (i32 x = 0; x < TEST_SIZE; x++) {
      //     foo[y*TEST_SIZE + x] = 69;
      //   }
      // }
    }
  }


  static f32 t = 0;
  t += 0.001f;
  f32 scale = (sinf(t) + 1)*10 + 10;
  memset(screen.data, (i32)0xFF333333, (size_t)(screen.height*screen.pitch*(i32)sizeof(Pixel)));

  draw_bitmap_avx(screen, rect2_center_size(v2(screen.width, screen.height)*0.5f, v2(test_bmp.width*2, test_bmp.height)*scale), t*10, test_bmp);


#if 0
  Layout *layout = &_layout;

  ui_grid_begin(layout, {
    .cols = 2,
    .rows = 2,
  }); {
    ui_block(layout, RED);
    ui_block(layout, GREEN);
    ui_block(layout, YELLOW);
  } ui_grid_end(layout);

  ui_layout_end(layout);
#endif
}