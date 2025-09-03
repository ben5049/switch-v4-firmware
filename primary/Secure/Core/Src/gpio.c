/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
     PA14(JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
     PC15-OSC32_OUT(OSC32_OUT)   ------> RCC_OSC32_OUT
     PC14-OSC32_IN(OSC32_IN)   ------> RCC_OSC32_IN
     PA13(JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PH0-OSC_IN(PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT(PH1)   ------> RCC_OSC_OUT
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FRAM_HOLD_GPIO_Port, FRAM_HOLD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FRAM_WP_GPIO_Port, FRAM_WP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(STAT_GPIO_Port, STAT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FRAM_CS_GPIO_Port, FRAM_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : FRAM_HOLD_Pin */
  GPIO_InitStruct.Pin = FRAM_HOLD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FRAM_HOLD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : FRAM_WP_Pin */
  GPIO_InitStruct.Pin = FRAM_WP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FRAM_WP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : STAT_Pin */
  GPIO_InitStruct.Pin = STAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(STAT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : FRAM_CS_Pin */
  GPIO_InitStruct.Pin = FRAM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(FRAM_CS_GPIO_Port, &GPIO_InitStruct);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOG, PHY2_INT_Pin|PHY1_INT_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOD, PHY0_INT_Pin|PHY_CLK_EN_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOI, GPIO_PIN_3|GPIO_PIN_2, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOA, SWCH_CS_Pin|GPIO_PIN_9|GPIO_PIN_1|GPIO_PIN_2
                          |GPIO_PIN_7, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOF, PHY_WAKE_Pin|PHY3_INT_Pin|PHY_RST_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_12|GPIO_PIN_15|GPIO_PIN_11, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(SWCH_RST_GPIO_Port, SWCH_RST_Pin, GPIO_PIN_NSEC);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
