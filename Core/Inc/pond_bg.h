#ifndef POND_BG_H
#define POND_BG_H

#include "ili9488.h"

#define POND_DEEP       ILI9488_COLOR(30,  90,  140)
#define POND_MID        ILI9488_COLOR(50,  120,  170)
#define POND_LIGHT      ILI9488_COLOR(30,  80, 120)
#define POND_SURFACE    ILI9488_COLOR(50, 110, 150)
#define POND_LILYPAD    ILI9488_COLOR(40, 130,  60)
#define POND_LILYDK     ILI9488_COLOR(25,  95,  40)
#define POND_FLOWER     ILI9488_COLOR(240, 180, 200)
#define POND_FLOWCTR    ILI9488_COLOR(255, 220, 80)
#define POND_RIPPLE     ILI9488_COLOR(60, 130, 170)
#define POND_MENU_BG    ILI9488_COLOR(15,  45,  70)
#define POND_CATTAIL    ILI9488_COLOR(90,  70,  30)
#define POND_CATTOP     ILI9488_COLOR(110, 80,  35)
#define POND_BLUE		ILI9488_COLOR(200, 234,  239)

void ILI9488_FillCircle(uint16_t cx, uint16_t cy, uint16_t r, uint32_t color);
void ILI9488_FillEllipse(uint16_t cx, uint16_t cy, uint16_t rx, uint16_t ry, uint32_t color);

void Pond_DrawMainBG(void);
void Pond_DrawSettingsBG(void);
void Pond_DrawVerifyBG(void);
void Pond_DrawStatsBG(void);
void Pond_DrawSubScreenBG(void);

#endif
