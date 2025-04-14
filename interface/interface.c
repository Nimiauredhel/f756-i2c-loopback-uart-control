/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "interface.h"

static void rx_evaluate(const char *rx_msg)
{
	// * send msg to both I2c interfaces in turn -
	// - awaiting loopback to happen -
	// - and reporting back to both UART interfaces. *

	i2c4_master_state = 0;
	i2c4_slave_state = 0;
	i2c2_master_state = 0;
	i2c2_slave_state = 0;

	sprintf(uart_buff, "User input: [%s]", rx_msg);
	serial_print_line(uart_buff, strlen(uart_buff));

	HAL_Delay(100);

	//bzero(uart_buff, 64);
	//bzero(i2c_buff, 32);

 	serial_print_line("Testing loopback I2C4 -> I2C2.", 0);
	strcpy((char *)i2c_tx_buff, rx_msg);
	serial_i2c_send(&hi2c4, i2c_tx_buff, sizeof(i2c_tx_buff), hi2c2.Init.OwnAddress1);
	serial_i2c_receive(&hi2c2, i2c_rx_buff, sizeof(i2c_tx_buff));

	while (i2c4_master_state != SCSTATE_ACKNOWLEDGED || i2c2_slave_state != SCSTATE_ACKNOWLEDGED)
	{
		HAL_Delay(1);
		if (i2c4_master_state == SCSTATE_SENT)
		{
			serial_print_line("I2C4 reports data sent.", 0);
			i2c4_master_state = SCSTATE_ACKNOWLEDGED;
		}

		if (i2c2_slave_state == SCSTATE_RECEIVED)
		{
			serial_print_line("I2C2 reports data received.", 0);
			i2c2_slave_state = SCSTATE_ACKNOWLEDGED;
		}
	}

	sprintf(uart_buff, "I2C2 received: [%s]", (char *)i2c_rx_buff);
	serial_print_line(uart_buff, strlen(uart_buff));

	HAL_Delay(100);

	//bzero(uart_buff, 64);
	//bzero(i2c_buff, 32);

	i2c4_master_state = 0;
	i2c4_slave_state = 0;
	i2c2_master_state = 0;
	i2c2_slave_state = 0;

	serial_print_line("Testing loopback I2C2 -> I2C4.", 0);
	strcpy((char *)i2c_tx_buff, rx_msg);
	serial_i2c_send(&hi2c2, i2c_tx_buff, sizeof(i2c_tx_buff), hi2c4.Init.OwnAddress1);
	serial_i2c_receive(&hi2c4, i2c_rx_buff, sizeof(i2c_tx_buff));


	while (i2c2_master_state != SCSTATE_ACKNOWLEDGED || i2c4_slave_state != SCSTATE_ACKNOWLEDGED)
	{
		HAL_Delay(1);
		if (i2c2_master_state == SCSTATE_SENT)
		{
			serial_print_line("I2C2 reports data sent.", 0);
			i2c2_master_state = SCSTATE_ACKNOWLEDGED;
		}

		if (i2c4_slave_state == SCSTATE_RECEIVED)
		{
			serial_print_line("I2C4 reports data received.", 0);
			i2c4_slave_state = SCSTATE_ACKNOWLEDGED;
		}
	}

	sprintf(uart_buff, "I2C4 received: [%s]", (char *)i2c_rx_buff);
	serial_print_line(uart_buff, strlen(uart_buff));

	serial_print_line("Test Concluded.", 0);
}

void interface_loop(void)
{
	HAL_Delay(1);
	char input[32];

	serial_print_line("Ready for user input.", 0);
	serial_scan(input, 32);
	rx_evaluate(input);
}
