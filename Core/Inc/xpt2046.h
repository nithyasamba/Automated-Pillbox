#ifndef XPT2046_H
#define XPT2046_H

#include "main.h"
//#include "spi.h"

/* ── Pin control macros ───────────────────────────────────────────── */
#define T_CS_LOW()    HAL_GPIO_WritePin(T_CS_GPIO_Port,  T_CS_Pin,  GPIO_PIN_RESET)
#define T_CS_HIGH()   HAL_GPIO_WritePin(T_CS_GPIO_Port,  T_CS_Pin,  GPIO_PIN_SET)
#define T_IRQ_READ()  HAL_GPIO_ReadPin(T_IRQ_GPIO_Port,  T_IRQ_Pin)

/* ── XPT2046 channel commands ─────────────────────────────────────── */
#define XPT2046_CMD_X   0xD0   /* Read X position */
#define XPT2046_CMD_Y   0x90   /* Read Y position */

/* ── Display dimensions for coordinate mapping ────────────────────── */
#define XPT2046_SCREEN_WIDTH    480
#define XPT2046_SCREEN_HEIGHT   320

/* ── Calibration values — adjust these after testing ─────────────── */
/* These are typical values — run calibration to get exact numbers    */
#define XPT2046_X_MIN    1200
#define XPT2046_X_MAX    2400
#define XPT2046_Y_MIN    1300
#define XPT2046_Y_MAX    3400

/* ── Touch data structure ─────────────────────────────────────────── */
typedef struct {
    uint16_t x;       /* Mapped screen X (0 to SCREEN_WIDTH-1)  */
    uint16_t y;       /* Mapped screen Y (0 to SCREEN_HEIGHT-1) */
    uint16_t raw_x;   /* Raw ADC value from XPT2046 */
    uint16_t raw_y;   /* Raw ADC value from XPT2046 */
    uint8_t  touched; /* 1 = screen is being touched, 0 = not */
} XPT2046_Touch;

/* ── Public API ───────────────────────────────────────────────────── */
void    XPT2046_Init(void);
uint8_t XPT2046_IsTouched(void);
void    XPT2046_GetTouch(XPT2046_Touch *touch);

#endif /* XPT2046_H */
