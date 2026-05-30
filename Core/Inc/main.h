/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LCD_CS_Pin GPIO_PIN_3
#define LCD_CS_GPIO_Port GPIOF
#define SD_CS_Pin GPIO_PIN_8
#define SD_CS_GPIO_Port GPIOF
#define LCD_LED_Pin GPIO_PIN_0
#define LCD_LED_GPIO_Port GPIOC
#define LCD_DC_Pin GPIO_PIN_1
#define LCD_DC_GPIO_Port GPIOC
#define LCD_RST_Pin GPIO_PIN_0
#define LCD_RST_GPIO_Port GPIOB
#define T_CS_Pin GPIO_PIN_12
#define T_CS_GPIO_Port GPIOB
#define T_IRQ_Pin GPIO_PIN_11
#define T_IRQ_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern int selected_voice;
#define SD_SPI_HANDLE hspi3


#define MAX_USERS 10 // may change

typedef struct {
    uint16_t fp_id;          // fingerprint sensor slot (1-based, 0 = unused)
    char     passcode[16];   // keypad PIN
    char     name[24];       // display name, typed via on-screen keypad or preset
    uint32_t access_count;   // how many times this user has authenticated
    uint32_t late_count;
    uint32_t total_late_minutes;
} UserProfile;

typedef enum {
	NONE = 0,
	ON_TIME,
	LATE,
	MISSED
} DoseStatus;

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	DoseStatus status;
} DoseHistoryEntry;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
