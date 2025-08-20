/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    flash.c
  * @brief   This file provides code for the configuration
  *          of the flash instances.
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
#include "flash.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* FLASH init function */
void MX_FLASH_Init(void)
{

  /* USER CODE BEGIN FLASH_Init 0 */

  /* USER CODE END FLASH_Init 0 */

  FLASH_OBProgramInitTypeDef pOBInit = {0};
  FLASH_BBAttributesTypeDef FLASH_BBSecInitStruct = {0};

  /* USER CODE BEGIN FLASH_Init 1 */

  /* USER CODE END FLASH_Init 1 */
  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    Error_Handler();
  }

  /* Option Bytes settings */

  if (HAL_FLASH_OB_Unlock() != HAL_OK)
  {
    Error_Handler();
  }
  pOBInit.OptionType = OPTIONBYTE_WMSEC;
  pOBInit.Banks = FLASH_BANK_BOTH;
  pOBInit.WMSecStartSector = 0;
  pOBInit.WMSecEndSector = 13;
  if (HAL_FLASHEx_OBProgram(&pOBInit) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FLASH_OB_Lock() != HAL_OK)
  {
    Error_Handler();
  }

  /* Block-based sector protection */

  FLASH_BBSecInitStruct.Bank = FLASH_BANK_1;
  FLASH_BBSecInitStruct.BBAttributesType = FLASH_BB_PRIV|FLASH_BB_SEC;
  FLASH_BBSecInitStruct.BBAttributes_array[0] =   0x0000FFFF;
  FLASH_BBSecInitStruct.BBAttributes_array[1] =   0x00000000
                              ;
  FLASH_BBSecInitStruct.BBAttributes_array[2] =   0x00000000;
  FLASH_BBSecInitStruct.BBAttributes_array[3] =   0x00000000;
  if (HAL_FLASHEx_ConfigBBAttributes(&FLASH_BBSecInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  FLASH_BBSecInitStruct.BBAttributesType = FLASH_BB_PRIV;
  FLASH_BBSecInitStruct.BBAttributes_array[0] =   0xFFFF0000;
  FLASH_BBSecInitStruct.BBAttributes_array[1] =   0xFFFFFFFF
                              ;
  FLASH_BBSecInitStruct.BBAttributes_array[2] =   0xFFFFFFFF;
  FLASH_BBSecInitStruct.BBAttributes_array[3] =   0xFFFFFFFF;
  if (HAL_FLASHEx_ConfigBBAttributes(&FLASH_BBSecInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  FLASH_BBSecInitStruct.Bank = FLASH_BANK_2;
  FLASH_BBSecInitStruct.BBAttributesType = FLASH_BB_PRIV|FLASH_BB_SEC;
  FLASH_BBSecInitStruct.BBAttributes_array[0] =   0x0000FFFF;
  FLASH_BBSecInitStruct.BBAttributes_array[1] =   0x00000000
                              ;
  FLASH_BBSecInitStruct.BBAttributes_array[2] =   0x00000000;
  FLASH_BBSecInitStruct.BBAttributes_array[3] =   0x00000000;
  if (HAL_FLASHEx_ConfigBBAttributes(&FLASH_BBSecInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  FLASH_BBSecInitStruct.BBAttributesType = FLASH_BB_PRIV;
  FLASH_BBSecInitStruct.BBAttributes_array[0] =   0xFFFF0000;
  FLASH_BBSecInitStruct.BBAttributes_array[1] =   0xFFFFFFFF
                              ;
  FLASH_BBSecInitStruct.BBAttributes_array[2] =   0xFFFFFFFF;
  FLASH_BBSecInitStruct.BBAttributes_array[3] =   0xFFFFFFFF;
  if (HAL_FLASHEx_ConfigBBAttributes(&FLASH_BBSecInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FLASH_Lock() != HAL_OK)
  {
    Error_Handler();
  }

  /* Launch Option Bytes Loading */
  /*HAL_FLASH_OB_Launch(); */

  /* USER CODE BEGIN FLASH_Init 2 */

  /* USER CODE END FLASH_Init 2 */

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
