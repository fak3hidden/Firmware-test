#pragma once
/*
 * T-Flipper GUI asset types.
 *
 * The whole GUI is rendered into a 128x64 1-bit framebuffer (exactly like the
 * reference handheld this firmware emulates) and that framebuffer is then
 * integer-upscaled onto the T-Embed's 170x320 colour panel.  Everything below
 * therefore works in single-bit pixels.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Proportional 1-bit bitmap font. Glyphs are stored row-major, MSB first,
 * one byte per 8 horizontal pixels. */
typedef struct {
    uint8_t first_char;
    uint8_t last_char;
    uint8_t height;      /* total line height (baseline + descender room) */
    uint8_t cap;         /* capital height, i.e. baseline position        */
    const uint8_t *widths;
    const uint8_t *heights;
    const int8_t *dx;    /* ink x offset from pen                          */
    const int8_t *dy;    /* ink y offset from baseline row (may be <0)     */
    const uint8_t *adv;  /* advance width                                  */
    const uint16_t *offsets;
    const uint8_t *data;
} TfFont;

typedef struct {
    uint8_t w;
    uint8_t h;
    const uint8_t *data; /* row-major MSB first                            */
} TfIcon;

extern const TfFont tf_primary;
extern const TfFont tf_primary_bold;
extern const TfFont tf_secondary;
extern const TfFont tf_big;
extern const TfFont tf_digits;
extern const TfFont tf_keyboard;

#ifdef __cplusplus
}
#endif
