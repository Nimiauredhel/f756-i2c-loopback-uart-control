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

static void loopback_test_rx_evaluate(const char *rx_msg, uint16_t rx_msg_len)
{
	// * send msg to both I2c interfaces in turn -
	// - awaiting loopback to happen -
	// - and reporting back to both UART interfaces. *

	bzero(uart_buff, sizeof(uart_buff));
	bzero(i2c_tx_buff, sizeof(i2c_tx_buff));
	bzero(i2c_slave_regs, sizeof(i2c_slave_regs));
	bzero(i2c_slave_buff, sizeof(i2c_slave_buff));

	bzero(i2c_channel_master_states, 4);
	bzero(i2c_channel_slave_states, 4);

	sprintf(uart_buff, "User input: [%s]", rx_msg);
	serial_print_line(uart_buff, strlen(uart_buff));

	HAL_Delay(100);

 	serial_print_line("Testing loopback I2C4 -> I2C2.", 0);
 	i2c_tx_buff[0] = 0;
	strcpy((char *)i2c_tx_buff+1, rx_msg);
	HAL_I2C_EnableListen_IT(&hi2c2);
	serial_i2c_send(&hi2c4, i2c_tx_buff, sizeof(i2c_tx_buff), hi2c2.Init.OwnAddress1);

	poll_i2c_channel_states(3, 1);

	sprintf(uart_buff, "I2C2 received: [%s]", (char *)i2c_slave_regs);
	serial_print_line(uart_buff, strlen(uart_buff));
	HAL_I2C_DisableListen_IT(&hi2c2);

	HAL_Delay(100);

	bzero(uart_buff, sizeof(uart_buff));
	bzero(i2c_tx_buff, sizeof(i2c_tx_buff));
	bzero(i2c_slave_buff, sizeof(i2c_slave_buff));
	bzero(i2c_slave_regs, sizeof(i2c_slave_regs));

	bzero(i2c_channel_master_states, 4);
	bzero(i2c_channel_slave_states, 4);

	serial_print_line("Testing loopback I2C2 -> I2C4.", 0);
 	i2c_tx_buff[0] = 0;
	strcpy((char *)i2c_tx_buff+1, rx_msg);
	HAL_I2C_EnableListen_IT(&hi2c4);
	serial_i2c_send(&hi2c2, i2c_tx_buff, sizeof(i2c_tx_buff), hi2c4.Init.OwnAddress1);

	poll_i2c_channel_states(1, 3);

	sprintf(uart_buff, "I2C4 received: [%s]", (char *)i2c_slave_regs);
	serial_print_line(uart_buff, strlen(uart_buff));
	HAL_I2C_DisableListen_IT(&hi2c4);

	bzero(i2c_channel_master_states, 4);
	bzero(i2c_channel_slave_states, 4);

	serial_print_line("Loopback test Concluded.", 0);
}

static void loopback_test_routine(void)
{
	char test_input[128] = {0};
	HAL_Delay(1);

	serial_print_line("Ready for user input.", 0);

	serial_scan(test_input, sizeof(test_input));
	loopback_test_rx_evaluate(test_input, strlen(test_input));
}

static void slave_test_routine(void)
{
	HAL_Delay(1);

	serial_print_line("\r\nI2C External RX test!", 0);
	HAL_I2C_EnableListen_IT(&hi2c1);
	serial_print_line("Awaiting input on I2C1...", 0);
	poll_i2c_channel_states(-1, 0);
	sprintf(uart_buff, "I2C1 buffer: [%s]", (char *)i2c_slave_buff);
	serial_print_line(uart_buff, strlen(uart_buff));
	sprintf(uart_buff, "I2C1 registers: [%s]", (char *)i2c_slave_regs);
	serial_print_line(uart_buff, strlen(uart_buff));
	poll_i2c_channel_states(-1, 0);
	serial_print_line("External RX test concluded.", 0);
}

static void event_queue_test_routine(void)
{
}

void interface_loop(void)
{
	HAL_Delay(100);

	serial_print_line("Please select a test routine from the list:", 0);
	serial_print_line("1: I2C Loopback", 0);
	serial_print_line("2: I2C Generic Slave Device", 0);
	serial_print_line("3: I2C Slave with Event Queue", 0);

	bzero(uart_buff, sizeof(uart_buff));
	serial_scan(uart_buff, 1);

	switch(uart_buff[0])
	{
	case '1':
		loopback_test_routine();
		break;
	case '2':
		slave_test_routine();
		break;
	case '3':
		event_queue_test_routine();
		break;
	}
}
