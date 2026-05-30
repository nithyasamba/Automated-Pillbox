#include "fingerprint.h"

void Fingerprint_Init(Fingerprint_Handle *dev, UART_HandleTypeDef *huart) {
    dev->huart = huart;
}

static void sendPacket(Fingerprint_Handle *dev, uint8_t type, uint16_t len, uint8_t *data) {
    // The checksum is the sum of (Package Type + Package Length + Package Contents)
    uint16_t sum = type + (len >> 8) + (len & 0xFF);
    for (uint16_t i = 0; i < len - 2; i++) {
        sum += data[i];
    }

    uint8_t header[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, type, (len >> 8), (len & 0xFF)};
    HAL_UART_Transmit(dev->huart, header, 9, 100);
    HAL_UART_Transmit(dev->huart, data, len - 2, 100);

    uint8_t checkSum[] = {(uint8_t)(sum >> 8), (uint8_t)(sum & 0xFF)};
    HAL_UART_Transmit(dev->huart, checkSum, 2, 100);
}

uint8_t Fingerprint_GetImage(Fingerprint_Handle *dev) {
    uint8_t data[] = {FINGERPRINT_GETIMAGE};
    sendPacket(dev, FINGERPRINT_COMMANDPACKET, 0x0003, data);

    uint8_t resp[12] = {0};
    // 100ms is the "sweet spot" for most optical sensors
    if (HAL_UART_Receive(dev->huart, resp, 12, 100) != HAL_OK) return 0xFF;

    return resp[9];
}

uint8_t Fingerprint_Image2Tz(Fingerprint_Handle *dev, uint8_t slot) {
    uint8_t data[] = {FINGERPRINT_IMAGE2TZ, slot};
    sendPacket(dev, FINGERPRINT_COMMANDPACKET, 0x0004, data);
    uint8_t resp[12];
    if (HAL_UART_Receive(dev->huart, resp, 12, 1000) != HAL_OK) return 0xFF;
    return resp[9];
}

uint8_t Fingerprint_RegModel(Fingerprint_Handle *dev) {
    uint8_t data[] = {FINGERPRINT_REGMODEL};
    sendPacket(dev, FINGERPRINT_COMMANDPACKET, 0x0003, data);
    uint8_t resp[12];
    if (HAL_UART_Receive(dev->huart, resp, 12, 1000) != HAL_OK) return 0xFF;
    return resp[9];
}

uint8_t Fingerprint_Store(Fingerprint_Handle *dev, uint16_t id) {
    uint8_t data[] = {FINGERPRINT_STORE, 0x01, (id >> 8), (id & 0xFF)};
    sendPacket(dev, FINGERPRINT_COMMANDPACKET, 0x0006, data);
    uint8_t resp[12];
    if (HAL_UART_Receive(dev->huart, resp, 12, 1000) != HAL_OK) return 0xFF;
    return resp[9];
}

uint8_t Fingerprint_FastSearch(Fingerprint_Handle *dev) {
    // Parameters: Command, BufferID, StartPageH, StartPageL, CountH, CountL
    // We are searching only the first 64 slots (0x0040)
    uint8_t data[] = {FINGERPRINT_FASTSEARCH, 0x01, 0x00, 0x00, 0x00, 0x40};
    sendPacket(dev, FINGERPRINT_COMMANDPACKET, 0x0008, data);

    uint8_t resp[16] = {0};
    // Standard response for FastSearch is actually 16 bytes
    if (HAL_UART_Receive(dev->huart, resp, 16, 1000) != HAL_OK) return 0xFF;

    if (resp[9] == 0x00) {
        dev->fingerID = (resp[10] << 8) | resp[11];
        dev->confidence = (resp[12] << 8) | resp[13];
    }
    return resp[9];
}

uint8_t Fingerprint_EmptyDatabase(Fingerprint_Handle *dev) {
    uint8_t data[] = {FINGERPRINT_EMPTY};
    sendPacket(dev, FINGERPRINT_COMMANDPACKET, 0x0003, data);
    uint8_t resp[12];
    if (HAL_UART_Receive(dev->huart, resp, 12, 1000) != HAL_OK) return 0xFF;
    return resp[9];
}
