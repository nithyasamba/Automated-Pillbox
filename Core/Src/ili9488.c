#include "ili9488.h"

static void spi_write_byte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

static void write_cmd(uint8_t cmd)
{
    LCD_DC_LOW();
    LCD_CS_LOW();
    spi_write_byte(cmd);
    LCD_CS_HIGH();
}

static void write_data(uint8_t data)
{
    LCD_DC_HIGH();
    LCD_CS_LOW();
    spi_write_byte(data);
    LCD_CS_HIGH();
}

void set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_cmd(0x2A);          
    write_data(x0 >> 8);
    write_data(x0 & 0xFF);
    write_data(x1 >> 8);
    write_data(x1 & 0xFF);

    write_cmd(0x2B);       
    write_data(y0 >> 8);
    write_data(y0 & 0xFF);
    write_data(y1 >> 8);
    write_data(y1 & 0xFF);

    write_cmd(0x2C);      
}

void ILI9488_Init(void)
{
    LCD_RST_HIGH();
    HAL_Delay(5);
    LCD_RST_LOW();
    HAL_Delay(20);
    LCD_RST_HIGH();
    HAL_Delay(150);

    LCD_LED_ON();

    write_cmd(0xE0);           
    write_data(0x00);
    write_data(0x03);
    write_data(0x09);
    write_data(0x08);
    write_data(0x16);
    write_data(0x0A);
    write_data(0x3F);
    write_data(0x78);
    write_data(0x4C);
    write_data(0x09);
    write_data(0x0A);
    write_data(0x08);
    write_data(0x16);
    write_data(0x1A);
    write_data(0x0F);

    write_cmd(0xE1);        
    write_data(0x00);
    write_data(0x16);
    write_data(0x19);
    write_data(0x03);
    write_data(0x0F);
    write_data(0x05);
    write_data(0x32);
    write_data(0x45);
    write_data(0x46);
    write_data(0x04);
    write_data(0x0E);
    write_data(0x0D);
    write_data(0x35);
    write_data(0x37);
    write_data(0x0F);

    write_cmd(0xC0);      
    write_data(0x17);
    write_data(0x15);

    write_cmd(0xC1);    
    write_data(0x41);

    write_cmd(0xC5);   
    write_data(0x00);
    write_data(0x12);
    write_data(0x80);

    write_cmd(0x36);          
    write_data(0x28);

    write_cmd(0x3A);        
    write_data(0x66);

    write_cmd(0xB0);           
    write_data(0x80);       

    write_cmd(0xB1);        
    write_data(0xA0);

    write_cmd(0xB4);         
    write_data(0x02);

    write_cmd(0xB6);        
    write_data(0x02);
    write_data(0x02);        

    write_cmd(0xE9);          
    write_data(0x00);    

    write_cmd(0xF7);        
    write_data(0xA9);
    write_data(0x51);
    write_data(0x2C);
    write_data(0x82);

    write_cmd(0x11);      
    HAL_Delay(120);

    write_cmd(0x29);         
    HAL_Delay(25);
}

void ILI9488_DrawPixel(uint16_t x, uint16_t y, uint32_t color)
{
    if (x >= ILI9488_WIDTH || y >= ILI9488_HEIGHT) return;

    set_address_window(x, y, x, y);

    LCD_DC_HIGH();
    LCD_CS_LOW();
    spi_write_byte((color >> 16) & 0xFF);
    spi_write_byte((color >> 8)  & 0xFF);
    spi_write_byte( color        & 0xFF);
    LCD_CS_HIGH();
}

void ILI9488_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)
{
    if (x >= ILI9488_WIDTH || y >= ILI9488_HEIGHT) return;
    if (x + w > ILI9488_WIDTH)  w = ILI9488_WIDTH  - x;
    if (y + h > ILI9488_HEIGHT) h = ILI9488_HEIGHT - y;

    set_address_window(x, y, x + w - 1, y + h - 1);

    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8)  & 0xFF;
    uint8_t b =  color        & 0xFF;

    uint8_t linebuf[480 * 3];
    for (uint16_t i = 0; i < w; i++) {
        linebuf[i * 3 + 0] = r;
        linebuf[i * 3 + 1] = g;
        linebuf[i * 3 + 2] = b;
    }

    LCD_DC_HIGH();
    LCD_CS_LOW();

    for (uint16_t row = 0; row < h; row++) {
        HAL_SPI_Transmit(&hspi1, linebuf, w * 3, HAL_MAX_DELAY);
    }
    LCD_CS_HIGH();
}

void ILI9488_FillScreen(uint32_t color)
{
    ILI9488_FillRect(0, 0, ILI9488_WIDTH, ILI9488_HEIGHT, color);
}


void ILI9488_SetRotation(uint8_t rotation)
{
    write_cmd(0x36);
    switch (rotation % 4) {
        case 0: write_data(0x48); break; 
        case 1: write_data(0x28); break;  
        case 2: write_data(0x88); break; 
        case 3: write_data(0xE8); break;  
    }
}
