#include "platform.h"
#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>

// Stub implementations for desktop builds
bool platform_init(void) {
    fprintf(stderr, "Warning: Using MinUI platform stub\n");
    return true;
}

void platform_quit(void) {
}

uint32_t platform_poll_input(void) {
    return 0;
}

bool display_init(void) {
    return true;
}

void display_quit(void) {
}

void display_flip(void) {
}

void display_fill(int x, int y, int w, int h, uint16_t color) {
}

void display_rect(int x, int y, int w, int h, uint16_t color) {
}

void display_text(int x, int y, const char* text, uint16_t color) {
}

void display_blit(const void* data, int x, int y, int w, int h) {
}

bool image_load(Image* img, const char* path) {
    if (img) {
        img->data = NULL;
        img->width = 0;
        img->height = 0;
    }
    return false;
}

void image_free(Image* img) {
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
    }
}

void display_image(const Image* img, int x, int y, int w, int h) {
}