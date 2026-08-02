#include <fb/fbtext.h>
#include <font/cp866.h>

uint8_t (*g_font)[FBFONT_HEIGHT] = (uint8_t (*)[FBFONT_HEIGHT])g_CP866_F16;

struct limine_framebuffer *g_fb = NULL;

static size_t cursor_x = 0;
static size_t cursor_y = 0;

void _putchar(char c) {
#ifdef FB_BLUE_THEME
  fb_putchar(c, 0xD7D7DA, 0x070792);
#else
  fb_putchar(c, 0xFFFFFF, 0x000000);
#endif
}

void fb_putchar(char c, uint32_t fg, uint32_t bg) {
  if (!g_fb || !g_font)
    return;

  size_t rows = g_fb->height / FBFONT_HEIGHT;

  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= rows) {
      fb_scroll(bg);
      cursor_y = rows - 1;
    }
    return;
  }
  if (c == '\r') {
    cursor_x = 0;
    return;
  }
  if (c == '\t') {
    cursor_x += 4;
    return;
  }

  uint8_t *glyph = g_font[(uint8_t)c];
  size_t px = cursor_x * FBFONT_WIDTH;
  size_t py = cursor_y * FBFONT_HEIGHT;
  for (int row = 0; row < FBFONT_HEIGHT; row++) {
    uint8_t bits = glyph[row];
    for (int col = 0; col < FBFONT_WIDTH; col++) {
      if (bits & (0x80 >> col))
        fb_putpixel(px + col, py + row, fg);
      else
        fb_putpixel(px + col, py + row, bg);
    }
  }

  cursor_x++;
  size_t cols = g_fb->width / FBFONT_WIDTH;
  if (cursor_x >= cols) {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= rows) {
      fb_scroll(bg);
      cursor_y = rows - 1;
    }
  }
}

void fb_scroll(uint32_t bg) {
  if (!g_fb)
    return;

  uint8_t *fb_bytes = (uint8_t *)g_fb->address;
  size_t pitch = g_fb->pitch; // bytes per row

  size_t bytes_to_move = pitch * (g_fb->height - FBFONT_HEIGHT);
  for (size_t i = 0; i < bytes_to_move; i++) {
    fb_bytes[i] = fb_bytes[i + pitch * FBFONT_HEIGHT];
  }

  uint32_t *pixels = g_fb->address;
  size_t pitch_pixels = pitch / 4;
  for (size_t y = g_fb->height - FBFONT_HEIGHT; y < g_fb->height; y++) {
    for (size_t x = 0; x < g_fb->width; x++) {
      pixels[y * pitch_pixels + x] = bg;
    }
  }
}

void fb_clear() {
  uint32_t color = fb_get_background_color();

  struct limine_framebuffer *fb = g_fb;
  uint32_t *fb_ptr = (uint32_t *)fb->address;

  size_t pitch_pixels = fb->pitch / 4;

  for (size_t y = 0; y < fb->height; y++) {
    for (size_t x = 0; x < fb->width; x++) {
      fb_ptr[y * pitch_pixels + x] = color;
    }
  }
}

void fb_puts(const char *c, uint32_t fg, uint32_t bg) {
  while (*c)
    fb_putchar(*c++, fg, bg);
}
