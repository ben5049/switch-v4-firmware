/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32h5xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void SystemClock_Config(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PHY2_INT_Pin GPIO_PIN_14
#define PHY2_INT_GPIO_Port GPIOG
#define PHY2_INT_EXTI_IRQn EXTI14_IRQn
#define PHY1_INT_Pin GPIO_PIN_10
#define PHY1_INT_GPIO_Port GPIOG
#define PHY1_INT_EXTI_IRQn EXTI10_IRQn
#define PHY0_INT_Pin GPIO_PIN_7
#define PHY0_INT_GPIO_Port GPIOD
#define PHY0_INT_EXTI_IRQn EXTI7_IRQn
#define PHY_CLK_EN_Pin GPIO_PIN_6
#define PHY_CLK_EN_GPIO_Port GPIOD
#define SWCH_CS_Pin GPIO_PIN_11
#define SWCH_CS_GPIO_Port GPIOA
#define PHY_WAKE_Pin GPIO_PIN_0
#define PHY_WAKE_GPIO_Port GPIOF
#define PHY3_INT_Pin GPIO_PIN_3
#define PHY3_INT_GPIO_Port GPIOF
#define PHY3_INT_EXTI_IRQn EXTI3_IRQn
#define PHY_RST_Pin GPIO_PIN_8
#define PHY_RST_GPIO_Port GPIOF
#define SWCH_RST_Pin GPIO_PIN_6
#define SWCH_RST_GPIO_Port GPIOH

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
