/*
 * door_interface.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "interface.h"

static void poll_i2c_channel_states(int8_t polled_master, int8_t polled_slave)
{
	char poll_buff[64] = {0};
	bool master_acked = polled_master < 0;
	bool slave_acked = polled_slave < 0;

	while (!master_acked || !slave_acked)
	{
		HAL_Delay(2);

		if (!master_acked && i2c_channel_master_states[polled_master] > 0)
		{
			sprintf(poll_buff, "I2C%d reports data %s.", polled_master+1, i2c_channel_master_states[polled_master] == SCSTATE_SENT ? "sent" : "received");
			i2c_channel_master_states[polled_master] = SCSTATE_NONE;
			master_acked = true;
			serial_print_line(poll_buff, 0);
		}

		if (!slave_acked && i2c_channel_slave_states[polled_slave] > 0)
		{
			sprintf(poll_buff, "I2C%d reports data %s.", polled_slave+1, i2c_channel_slave_states[polled_slave] == SCSTATE_SENT ? "sent" : "received");
			i2c_channel_slave_states[polled_slave] = SCSTATE_NONE;
			slave_acked = true;
			serial_print_line(poll_buff, 0);
		}
	}
}

static void rx_evaluate(const char *rx_msg, uint16_t rx_msg_len)
{
	// * send msg to both I2c interfaces in turn -
	// - awaiting loopback to happen -
	// - and reporting back to both UART interfaces. *

	bzero(uart_buff, sizeof(uart_buff));
	bzero(i2c_tx_buff, sizeof(i2c_tx_buff));
	bzero(i2c_rx_buff, sizeof(i2c_rx_buff));

	bzero(i2c_channel_master_states, 4);
	bzero(i2c_channel_slave_states, 4);

	sprintf(uart_buff, "User input: [%s]", rx_msg);
	serial_print_line(uart_buff, strlen(uart_buff));

	HAL_Delay(100);

 	serial_print_line("Testing loopback I2C4 -> I2C2.", 0);
	strcpy((char *)i2c_tx_buff, rx_msg);
	serial_i2c_send(&hi2c4, i2c_tx_buff, sizeof(i2c_tx_buff), hi2c2.Init.OwnAddress1);
	serial_i2c_receive(&hi2c2, i2c_rx_buff, sizeof(i2c_tx_buff));

	poll_i2c_channel_states(3, 1);

	sprintf(uart_buff, "I2C2 received: [%s]", (char *)i2c_rx_buff);
	serial_print_line(uart_buff, strlen(uart_buff));

	HAL_Delay(100);

	bzero(uart_buff, sizeof(uart_buff));
	bzero(i2c_tx_buff, sizeof(i2c_tx_buff));
	bzero(i2c_rx_buff, sizeof(i2c_rx_buff));

	bzero(i2c_channel_master_states, 4);
	bzero(i2c_channel_slave_states, 4);

	serial_print_line("Testing loopback I2C2 -> I2C4.", 0);
	strcpy((char *)i2c_tx_buff, rx_msg);
	serial_i2c_send(&hi2c2, i2c_tx_buff, sizeof(i2c_tx_buff), hi2c4.Init.OwnAddress1);
	serial_i2c_receive(&hi2c4, i2c_rx_buff, sizeof(i2c_tx_buff));

	poll_i2c_channel_states(1, 3);

	sprintf(uart_buff, "I2C4 received: [%s]", (char *)i2c_rx_buff);
	serial_print_line(uart_buff, strlen(uart_buff));

	bzero(i2c_channel_master_states, 4);
	bzero(i2c_channel_slave_states, 4);

	serial_print_line("Loopback test Concluded.", 0);
}

void interface_loop(void)
{
	HAL_Delay(1);
	char buff[256];

	/*
	serial_print_line("Ready for user input.", 0);
	serial_scan(buff, 256);
	rx_evaluate(buff, strlen(buff));
	*/

	serial_print_line("\r\nI2C External RX test!", 0);
	HAL_I2C_EnableListen_IT(&hi2c1);
	serial_print_line("Awaiting input on I2C1...", 0);
	poll_i2c_channel_states(-1, 0);
	sprintf(uart_buff, "I2C1 buffer: [%s]", (char *)i2c_slave_buff);
	serial_print_line(uart_buff, strlen(uart_buff));
	sprintf(uart_buff, "I2C1 registers: [%s]", (char *)i2c_slave_regs);
	serial_print_line(uart_buff, strlen(uart_buff));
	serial_print_line("External RX test concluded.", 0);
}
