#ifndef ILI9488_H
#define ILI9488_H

#include "main.h"
//#include "spi.h"

/* ── Display dimensions ───────────────────────────────────────────── */
#define ILI9488_WIDTH   480
#define ILI9488_HEIGHT  320

/* ── Pin control macros ───────────────────────────────────────────── */
#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_SET)
#define LCD_DC_LOW()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_RESET)
#define LCD_DC_HIGH()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_SET)
#define LCD_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)
#define LCD_LED_ON()    HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_SET)

/* ── 18-bit color (R8G8B8 → 3 bytes, top 6 bits per channel) ─────── */
#define ILI9488_COLOR(r, g, b) ((uint32_t)(((r) & 0xFC) << 16 | ((g) & 0xFC) << 8 | ((b) & 0xFC)))

#define COLOR_BLACK     ILI9488_COLOR(0,   0,   0)
#define COLOR_WHITE     ILI9488_COLOR(255, 255, 255)
#define COLOR_RED       ILI9488_COLOR(255, 0,   0)
#define COLOR_GREEN     ILI9488_COLOR(0,  238, 144)
#define COLOR_BLUE      ILI9488_COLOR(100,   220,   140)
#define COLOR_YELLOW    ILI9488_COLOR(255, 255, 0)
#define COLOR_CYAN      ILI9488_COLOR(0,   255, 255)
#define COLOR_MAGENTA   ILI9488_COLOR(255, 0,   255)

/* ── Public API ───────────────────────────────────────────────────── */
void ILI9488_Init(void);
void ILI9488_FillScreen(uint32_t color);
void ILI9488_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void ILI9488_DrawPixel(uint16_t x, uint16_t y, uint32_t color);
void ILI9488_SetRotation(uint8_t rotation);

#endif /* ILI9488_H */

/* ── Text & shape extensions ──────────────────────────────────────── */
/* Text scale: 1=tiny(5x7) 2=medium(10x14) 3=large(15x21)            */
void ILI9488_DrawChar(uint16_t x, uint16_t y, char c, uint32_t fg, uint32_t bg, uint8_t scale);
void ILI9488_DrawString(uint16_t x, uint16_t y, const char *str, uint32_t fg, uint32_t bg, uint8_t scale);
void ILI9488_DrawStringCentered(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *str, uint32_t fg, uint32_t bg, uint8_t scale);
void ILI9488_FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint32_t color);
void ILI9488_DrawHLine(uint16_t x, uint16_t y, uint16_t w, uint32_t color);
void ILI9488_DrawVLine(uint16_t x, uint16_t y, uint16_t h, uint32_t color);
void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/* Extra colors */
#define COLOR_DARKGREY  ILI9488_COLOR(64,  64,  64)
#define COLOR_DARKBLUE  ILI9488_COLOR(0,   0,   139)
#define COLOR_ORANGE    ILI9488_COLOR(255, 165, 0)
