#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "stm32l4xx_hal.h"

// Packet Components
#define FINGERPRINT_STARTCODE       0xEF01
#define FINGERPRINT_COMMANDPACKET   0x01
#define FINGERPRINT_ACKPACKET       0x07

// Instructions (Subcategories)
#define FINGERPRINT_GETIMAGE        0x01
#define FINGERPRINT_IMAGE2TZ        0x02
#define FINGERPRINT_REGMODEL        0x05
#define FINGERPRINT_STORE           0x06
#define FINGERPRINT_FASTSEARCH      0x04
#define FINGERPRINT_EMPTY           0x0D

// Confirmation Codes
#define FINGERPRINT_OK              0x00
#define FINGERPRINT_PACKETRECIEVEERR 0x01
#define FINGERPRINT_NOFINGER        0x02
#define FINGERPRINT_NOTFOUND        0x09

typedef struct {
    UART_HandleTypeDef *huart;
    uint16_t fingerID;
    uint16_t confidence;
} Fingerprint_Handle;

// Function Prototypes
void Fingerprint_Init(Fingerprint_Handle *dev, UART_HandleTypeDef *huart);
uint8_t Fingerprint_GetImage(Fingerprint_Handle *dev);
uint8_t Fingerprint_Image2Tz(Fingerprint_Handle *dev, uint8_t slot);
uint8_t Fingerprint_RegModel(Fingerprint_Handle *dev);
uint8_t Fingerprint_Store(Fingerprint_Handle *dev, uint16_t id);
uint8_t Fingerprint_FastSearch(Fingerprint_Handle *dev);
uint8_t Fingerprint_EmptyDatabase(Fingerprint_Handle *dev);

#endif
