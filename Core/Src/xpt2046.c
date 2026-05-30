#include "xpt2046.h"

/* ── Internal helpers ─────────────────────────────────────────────── */

/* Send one byte over SPI2 and return the received byte */
static uint8_t spi2_transfer(uint8_t data)
{
    uint8_t rx = 0;
    HAL_SPI_TransmitReceive(&hspi2, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

/* Read a 12-bit ADC value from the XPT2046 for a given channel command */
static uint16_t read_channel(uint8_t cmd)
{
    uint8_t hi, lo;

    spi2_transfer(cmd);   /* Send channel select command */
    hi = spi2_transfer(0x00);  /* Read high byte */
    lo = spi2_transfer(0x00);  /* Read low byte  */

    /* Result is 12 bits: top 7 bits of hi + top 5 bits of lo */
    return ((hi << 8) | lo) >> 3;
}

///* Average multiple readings to reduce noise */
//static uint16_t read_channel_avg(uint8_t cmd, uint8_t samples)
//{
//    uint32_t sum = 0;
//    for (uint8_t i = 0; i < samples; i++) {
//        sum += read_channel(cmd);
//        HAL_Delay(1);
//    }
//    return (uint16_t)(sum / samples);
//}
//static int compare_u16(const void *a, const void *b) {
//    return (*(uint16_t*)a - *(uint16_t*)b);
//}

static uint16_t read_channel_median(uint8_t cmd, uint8_t samples)
{
    uint16_t buf[samples];
    for (uint8_t i = 0; i < samples; i++) {
        buf[i] = read_channel(cmd);
        HAL_Delay(1);
    }
    /* Simple bubble sort for small arrays */
    for (uint8_t i = 0; i < samples - 1; i++)
        for (uint8_t j = 0; j < samples - i - 1; j++)
            if (buf[j] > buf[j+1]) {
                uint16_t tmp = buf[j];
                buf[j] = buf[j+1];
                buf[j+1] = tmp;
            }
    return buf[samples / 2];  // middle value
}

/* Map raw ADC value to screen coordinates */
static uint16_t map_value(uint16_t val, uint16_t in_min, uint16_t in_max,
                           uint16_t out_min, uint16_t out_max)
{
	int32_t v    = (int32_t)val;
	    int32_t imin = (int32_t)in_min;
	    int32_t imax = (int32_t)in_max;
	    int32_t omin = (int32_t)out_min;
	    int32_t omax = (int32_t)out_max;

	    /* Clamp — works whether imin > imax or imin < imax */
	    if (imin < imax) {
	        if (v <= imin) return out_min;
	        if (v >= imax) return out_max;
	    } else {
	        if (v >= imin) return out_min;
	        if (v <= imax) return out_max;
	    }

	    int32_t result = (v - imin) * (omax - omin) / (imax - imin) + omin;
	    if (result < 0)      return out_min;
	    if (result > 65535)  return out_max;
	    return (uint16_t)result;
}

/* ── Public API ───────────────────────────────────────────────────── */

void XPT2046_Init(void)
{
    /* Make sure CS starts high (deselected) */
    T_CS_HIGH();
    HAL_Delay(10);
}

uint8_t XPT2046_IsTouched(void)
{
    /* T_IRQ goes LOW when the screen is touched */
    return (T_IRQ_READ() == GPIO_PIN_RESET) ? 1 : 0;
}

void XPT2046_GetTouch(XPT2046_Touch *touch)
{
    if (!XPT2046_IsTouched()) {
        touch->touched = 0;
        return;
    }

    touch->touched = 1;

    T_CS_LOW();

//    /* Read raw X and Y — average 8 samples for stability */
//    uint16_t raw_x = read_channel_avg(XPT2046_CMD_X, 16);
//    uint16_t raw_y = read_channel_avg(XPT2046_CMD_Y, 16);
    uint16_t raw_x = read_channel_median(XPT2046_CMD_X, 7);
    uint16_t raw_y = read_channel_median(XPT2046_CMD_Y, 7);

    T_CS_HIGH();


    touch->raw_x = raw_x;
    touch->raw_y = raw_y;

    /* Map raw ADC values to screen pixel coordinates */
    touch->x = map_value(raw_y, 3800, 340,
                             0, XPT2046_SCREEN_WIDTH  - 1);
    touch->y = map_value(raw_x, 3650, 430,
                             0, XPT2046_SCREEN_HEIGHT - 1);
}
