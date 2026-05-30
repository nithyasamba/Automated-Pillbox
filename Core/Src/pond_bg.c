#include "pond_bg.h"

// this is for the aesthetics of the display 

void ILI9488_FillCircle(uint16_t cx, uint16_t cy, uint16_t r, uint32_t color)
{
    int16_t f     = 1 - (int16_t)r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t px    = 0;
    int16_t py    = (int16_t)r;

    ILI9488_FillRect(cx - r, cy, 2 * r + 1, 1, color);

    while (px < py) {
        if (f >= 0) { py--; ddF_y += 2; f += ddF_y; }
        px++; ddF_x += 2; f += ddF_x;

        ILI9488_FillRect(cx - px, cy + py, 2 * px + 1, 1, color);
        ILI9488_FillRect(cx - px, cy - py, 2 * px + 1, 1, color);
        ILI9488_FillRect(cx - py, cy + px, 2 * py + 1, 1, color);
        ILI9488_FillRect(cx - py, cy - px, 2 * py + 1, 1, color);
    }
}

void ILI9488_FillEllipse(uint16_t cx, uint16_t cy,
                          uint16_t rx, uint16_t ry, uint32_t color)
{
    if (rx == 0 || ry == 0) return;

    int32_t rx2 = (int32_t)rx * rx;
    int32_t ry2 = (int32_t)ry * ry;

    for (int16_t y = -(int16_t)ry; y <= (int16_t)ry; y++) {
        int32_t y2 = (int32_t)y * y;
        int32_t hw = 0;
        {
            int32_t num = rx2 * (ry2 - y2);
            if (num <= 0) continue;
            int32_t val = num / ry2;
            int32_t g = 1;
            while (g * g < val) g++;
            if (g * g > val) g--;
            hw = g;
        }
        if (hw > 0)
            ILI9488_FillRect(cx - hw, cy + y, 2 * hw + 1, 1, color);
    }
}

// Lily pad drawing
static void draw_lilypad(uint16_t cx, uint16_t cy,
                          uint16_t rx, uint16_t ry)
{
    ILI9488_FillEllipse(cx, cy, rx + 1, ry + 1, POND_LILYDK);
    ILI9488_FillEllipse(cx, cy, rx, ry, POND_LILYPAD);
    ILI9488_FillRect(cx, cy - ry/2, 1, ry, POND_LILYDK);
}

// flower drawing
static void draw_lotus(uint16_t cx, uint16_t cy)
{
    ILI9488_FillEllipse(cx,     cy - 5, 4, 7, POND_FLOWER);
    ILI9488_FillEllipse(cx + 5, cy - 2, 4, 6, POND_FLOWER);
    ILI9488_FillEllipse(cx - 5, cy - 2, 4, 6, POND_FLOWER);
    ILI9488_FillEllipse(cx + 3, cy + 3, 4, 5, POND_FLOWER);
    ILI9488_FillEllipse(cx - 3, cy + 3, 4, 5, POND_FLOWER);
    ILI9488_FillCircle(cx, cy, 3, POND_FLOWCTR);
}

static void draw_ripple(uint16_t cx, uint16_t cy, uint16_t r)
{
    int16_t f = 1 - (int16_t)r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t px = 0;
    int16_t py = (int16_t)r;

    while (px < py) {
        if (f >= 0) { py--; ddF_y += 2; f += ddF_y; }
        px++; ddF_x += 2; f += ddF_x;
        if (px % 2 == 0) {
            ILI9488_FillRect(cx - px, cy - py, 2 * px + 1, 1, POND_RIPPLE);
        }
    }
}

static void draw_cattail(uint16_t x, uint16_t y_top, uint16_t height)
{
    ILI9488_FillRect(x, y_top, 2, height, POND_LILYDK);
    ILI9488_FillEllipse(x + 1, y_top + 4, 4, 10, POND_CATTAIL);
    ILI9488_FillEllipse(x + 1, y_top + 2, 3, 8,  POND_CATTOP);
}

void Pond_DrawMainBG(void)
{
    ILI9488_FillRect(140, 0,   340, 110, POND_DEEP);
    ILI9488_FillRect(140, 110, 340, 110, POND_DEEP);
    ILI9488_FillRect(140, 220, 340, 100, POND_DEEP);

    ILI9488_FillRect(0, 0, 140, 320, POND_MENU_BG);

    ILI9488_FillRect(200, 30,  60, 1, POND_SURFACE);
    ILI9488_FillRect(350, 55,  40, 1, POND_SURFACE);
    ILI9488_FillRect(250, 145, 80, 1, POND_SURFACE);
    ILI9488_FillRect(380, 180, 50, 1, POND_SURFACE);

    draw_lilypad(420, 80,  22, 14);
    draw_lilypad(310, 280, 18, 11);
    draw_lilypad(450, 260, 15, 10);

    draw_lotus(420, 72);

    draw_ripple(250, 200, 25);
    draw_ripple(350, 130, 15);

    ILI9488_DrawVLine(140, 0, 320, POND_SURFACE);
}

void Pond_DrawSettingsBG(void)
{
    ILI9488_FillRect(0, 0, 480, 90,  POND_MID);
    ILI9488_FillRect(0, 90, 480, 230, POND_DEEP);

    draw_cattail(20,  95,  220);
    draw_cattail(35,  110, 200);
    draw_cattail(450, 100, 215);
    draw_cattail(462, 120, 195);

    draw_lilypad(440, 290, 16, 10);
    draw_lilypad(60,  300, 12, 8);

    ILI9488_FillRect(100, 200, 50, 1, POND_SURFACE);
    ILI9488_FillRect(300, 250, 70, 1, POND_SURFACE);
}

void Pond_DrawVerifyBG(void)
{
    ILI9488_FillRect(0, 0,   480, 90,  POND_DEEP);
    ILI9488_FillRect(0, 90,  480, 230, POND_DEEP);

    draw_ripple(120, 270, 20);
    draw_ripple(400, 250, 18);

    ILI9488_FillRect(60,  280, 40, 1, POND_SURFACE);
    ILI9488_FillRect(350, 300, 55, 1, POND_SURFACE);
}

void Pond_DrawStatsBG(void)
{
    ILI9488_FillRect(0, 0,   480, 90,  POND_MID);
    ILI9488_FillRect(0, 90,  480, 230, POND_DEEP);

    draw_lilypad(60,  300, 14, 9);
    draw_lilypad(430, 295, 12, 8);

    draw_ripple(240, 300, 30);

    ILI9488_FillRect(150, 280, 60, 1, POND_SURFACE);
}

void Pond_DrawSubScreenBG(void)
{
    ILI9488_FillRect(0, 0,   480, 90,  POND_MID);
    ILI9488_FillRect(0, 90,  480, 230, POND_DEEP);

    ILI9488_FillRect(380, 300, 45, 1, POND_SURFACE);
    ILI9488_FillRect(50,  290, 35, 1, POND_SURFACE);

    draw_ripple(440, 280, 12);
}
