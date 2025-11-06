#ifndef MINUI_HARDWARE_H
#define MINUI_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

// Input button definitions
#define BTN_UP      (1 << 0)
#define BTN_DOWN    (1 << 1)
#define BTN_LEFT    (1 << 2)
#define BTN_RIGHT   (1 << 3)
#define BTN_A       (1 << 4)
#define BTN_B       (1 << 5)
#define BTN_X       (1 << 6)
#define BTN_Y       (1 << 7)
#define BTN_MENU    (1 << 8)

// Display definitions (these match your code)
#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

#ifdef __cplusplus
}
#endif

#endif // MINUI_HARDWARE_H