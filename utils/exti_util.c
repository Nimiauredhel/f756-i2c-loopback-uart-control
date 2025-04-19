/*
 * exti_util.c
 *
 *  Created on: Apr 19, 2025
 *      Author: mickey
 */

#include "exti_util.h"

volatile uint32_t user_btn_press_count = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == USER_Btn_Pin)
	{
		user_btn_press_count++;
	}
}
