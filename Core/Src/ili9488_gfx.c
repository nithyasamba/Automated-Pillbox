#include "ili9488.h"
#include "font.h"

// Draw a single character
void ILI9488_DrawChar(uint16_t x, uint16_t y, char c,
                      uint32_t fg, uint32_t bg, uint8_t scale)
{
    if (c < FONT_FIRST || c > FONT_LAST) c = '?';
    const uint8_t *glyph = font5x7[c - FONT_FIRST];

    for (uint8_t col = 0; col < FONT_WIDTH; col++) {
        for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
            uint32_t color = (glyph[col] & (1 << row)) ? fg : bg;
            ILI9488_FillRect(x + col * scale,
                             y + row * scale,
                             scale, scale, color);
        }
    }
   // this adds a gap column 
    ILI9488_FillRect(x + FONT_WIDTH * scale, y,
                     scale, FONT_HEIGHT * scale, bg);
}

// draw a string of characters
void ILI9488_DrawString(uint16_t x, uint16_t y, const char *str,
                        uint32_t fg, uint32_t bg, uint8_t scale)
{
    uint16_t cx = x;
    while (*str) {
        ILI9488_DrawChar(cx, y, *str, fg, bg, scale);
        cx += (FONT_WIDTH + 1) * scale;
        str++;
    }
}

// draw a string centered within a rectangle
void ILI9488_DrawStringCentered(uint16_t x, uint16_t y,
                                 uint16_t w, uint16_t h,
                                 const char *str,
                                 uint32_t fg, uint32_t bg, uint8_t scale)
{
    uint16_t len = 0;
    while (str[len]) len++;
    uint16_t str_w = len * (FONT_WIDTH + 1) * scale;
    uint16_t str_h = FONT_HEIGHT * scale;

    uint16_t tx = x + (w > str_w ? (w - str_w) / 2 : 0);
    uint16_t ty = y + (h > str_h ? (h - str_h) / 2 : 0);

    ILI9488_DrawString(tx, ty, str, fg, bg, scale);
}

// fill in the interior of a rounded rectangle
void ILI9488_FillRoundRect(uint16_t x, uint16_t y,
                            uint16_t w, uint16_t h,
                            uint16_t r, uint32_t color)
{
    ILI9488_FillRect(x + r, y, w - 2*r, h, color);
    ILI9488_FillRect(x, y + r, r, h - 2*r, color);
    ILI9488_FillRect(x + w - r, y + r, r, h - 2*r, color);

    int16_t f     =  1 - r;
    int16_t ddF_x =  1;
    int16_t ddF_y = -2 * r;
    int16_t px    =  0;
    int16_t py    =  r;

    while (px < py) {
        if (f >= 0) { py--; ddF_y += 2; f += ddF_y; }
        px++; ddF_x += 2; f += ddF_x;

        ILI9488_FillRect(x + r - px, y + r - py, 2*px, 1, color);
        ILI9488_FillRect(x + r - px, y + h - r + py - 1, 2*px, 1, color);
        ILI9488_FillRect(x + r - py, y + r - px, 2*py, 1, color);
        ILI9488_FillRect(x + r - py, y + h - r + px - 1, 2*py, 1, color);
    }
}

// draw a horizontal line
void ILI9488_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint32_t color)
{
    ILI9488_FillRect(x, y, w, 1, color);
}

// draw a vertial line
void ILI9488_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint32_t color)
{
    ILI9488_FillRect(x, y, 1, h, color);
}
