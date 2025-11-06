#ifndef MINUI_PLATFORM_H
#define MINUI_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Platform initialization
bool platform_init(void);
void platform_quit(void);

// Input handling
uint32_t platform_poll_input(void);

// Display functions
bool display_init(void);
void display_quit(void);
void display_flip(void);
void display_fill(int x, int y, int w, int h, uint16_t color);
void display_rect(int x, int y, int w, int h, uint16_t color);
void display_text(int x, int y, const char* text, uint16_t color);
void display_blit(const void* data, int x, int y, int w, int h);

// Image handling
typedef struct {
    uint16_t* data;
    int width;
    int height;
} Image;

bool image_load(Image* img, const char* path);
void image_free(Image* img);
void display_image(const Image* img, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif

#endif // MINUI_PLATFORM_H