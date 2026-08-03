#ifndef FBTEXT_H_
#define FBTEXT_H_

#include <limine/limine.h>
#include <stddef.h>
#include <stdint.h>

#include <font/cp866.h>

// #define FB_BLUE_THEME

#define FBFONT_WIDTH 8
#define FBFONT_HEIGHT 16

extern uint8_t (*g_font)[FBFONT_HEIGHT];

extern struct limine_framebuffer *g_fb;

static inline void fb_putpixel(size_t x, size_t y, uint32_t color) {
  if (x >= g_fb->width || y >= g_fb->height)
    return;

  volatile uint32_t *pixels = g_fb->address;
  pixels[y * (g_fb->pitch / 4) + x] = color;
}

static inline void fb_set_limine_framebuffer(struct limine_framebuffer *fb) {
  g_fb = fb;
}

static inline uint32_t fb_get_background_color() {
#ifdef FB_BLUE_THEME
  return 0x070792;
#else
  return 0x000000;
#endif
}

void fb_init();
void fb_clear();
void fb_putchar(char c, uint32_t fg, uint32_t bg);
void fb_puts(const char *c, uint32_t fg, uint32_t bg);
void fb_scroll(uint32_t bg);

#endif
