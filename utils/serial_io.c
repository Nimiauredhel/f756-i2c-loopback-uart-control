/*
 * serial_io.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "serial_io.h"

#define UART_TX_HANDLE (huart2)
#define UART_RX_HANDLE (huart2)

char uart_buff[512] = {0};
uint8_t i2c_tx_buff[256] = {0};
uint8_t i2c_rx_buff[256] = {0};

volatile SerialChannelState_t i2c4_master_state = 0;
volatile SerialChannelState_t i2c4_slave_state = 0;
volatile SerialChannelState_t i2c2_master_state = 0;
volatile SerialChannelState_t i2c2_slave_state = 0;

void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	if (hi2c->Instance == I2C4)
	{
		i2c4_master_state = SCSTATE_SENT;
	}
	else
	{
		i2c2_master_state = SCSTATE_SENT;
	}
}

void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	if (hi2c->Instance == I2C4)
	{
		i2c4_master_state = SCSTATE_RECEIVED;
	}
	else
	{
		i2c2_master_state = SCSTATE_RECEIVED;
	}
}

void HAL_I2C_SlaveTxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	if (hi2c->Instance == I2C4)
	{
		i2c4_slave_state = SCSTATE_SENT;
	}
	else
	{
		i2c2_slave_state = SCSTATE_SENT;
	}
}

void HAL_I2C_SlaveRxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	if (hi2c->Instance == I2C4)
	{
		i2c4_slave_state = SCSTATE_RECEIVED;
	}
	else
	{
		i2c2_slave_state = SCSTATE_RECEIVED;
	}

}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef * hi2c)
{
	HAL_I2C_Slave_Receive_IT(hi2c, i2c_rx_buff, sizeof(i2c_rx_buff));
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

static void serial_backspace_destructive(uint16_t count)
{

	for (uint16_t idx = 0; idx < count; idx++)
	{
		HAL_UART_Transmit(&UART_TX_HANDLE, (uint8_t *)"\b \b", 3, 0xFFFF);
	}
}

static void serial_newline(void)
{
	HAL_UART_Transmit(&UART_TX_HANDLE, (uint8_t *)"\n\r", 2, 0xFFFF);
}

void serial_print(const char *msg, uint16_t len)
{
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&UART_TX_HANDLE, (uint8_t *)msg, len, 0xFFFF);
}

void serial_print_line(const char *msg, uint16_t len)
{
	if (len == 0) len = strlen(msg);
	HAL_UART_Transmit(&UART_TX_HANDLE, (uint8_t *)msg, len, 0xFFFF);
	serial_newline();
}

void serial_print_char(const char c)
{
	HAL_UART_Transmit(&UART_TX_HANDLE, (uint8_t *)&c, 1, 0xFFFF);
}

uint16_t serial_scan(char *buffer, const uint16_t max_len)
{
	static const uint8_t echo_line_length = 128;
	char util_buff[16] = {0};
	uint8_t inchar = 0;
	uint16_t input_idx = 0;

	bzero(buffer, max_len);

	for(;;)
	{
		inchar = 0;
		HAL_Delay(1);

		if (HAL_OK == HAL_UART_Receive(&UART_RX_HANDLE, &inchar, 1, 0x10))
		{
			switch (inchar)
			{
			case 0:
				break;
			case '\b':
				if (input_idx > 0)
				{
					buffer[input_idx] = '\0';
					serial_backspace_destructive(1);
					input_idx--;
					if (input_idx == echo_line_length - 1
						|| input_idx % echo_line_length == echo_line_length - 1)
					{
						sprintf(util_buff, "\r\e[%uC%c \b", echo_line_length-1, 0x8D);
						serial_print(util_buff, 0);
						/*
						serial_backspace_destructive(1);
						serial_print("\e[8C", 0);
						serial_print_char(0x8D);
						*/
					}
				}
				break;
			case '\n':
			case '\r':
				buffer[input_idx] = '\0';
				serial_newline();
				return input_idx+1;
			default:
				if (input_idx < max_len)
				{
					buffer[input_idx] = inchar;
					serial_print_char(inchar);
					inchar = 0;
					input_idx++;
					if (input_idx >= echo_line_length
						&& input_idx % echo_line_length == 0)
					{
						serial_print_char('\r');
						serial_print_char('\n');
					}
				}
				HAL_Delay(1);
				break;
			}
		}
	}
}

void serial_i2c_send(I2C_HandleTypeDef *device, uint8_t *msg, uint16_t len, uint16_t address)
{
	char buff[64];
	HAL_I2C_StateTypeDef state;
	HAL_StatusTypeDef ret;
	while ((state = HAL_I2C_GetState(device)) != HAL_I2C_STATE_READY) HAL_Delay(1);
	while((ret = HAL_I2C_Master_Transmit_DMA(device, address, msg, len)) != HAL_OK)
	{
		HAL_Delay(10);
	}
	sprintf(buff, "I2C Master Transmit IT result: %s", ret == 0 ? "good" : "bad");
	serial_print_line(buff, strlen(buff));
}

void serial_i2c_receive(I2C_HandleTypeDef *device, uint8_t *buffer, uint16_t max_len)
{
	char buff[64];
	HAL_I2C_StateTypeDef state;
	HAL_StatusTypeDef ret;
	while ((state = HAL_I2C_GetState(device)) != HAL_I2C_STATE_READY) HAL_Delay(1);
	while((ret = HAL_I2C_Slave_Receive_DMA(device, buffer, max_len)) != HAL_OK)
	{
		HAL_Delay(10);
	}
	sprintf(buff, "I2C Slave Receive IT result: %s", ret == 0 ? "good" : "bad");
	serial_print_line(buff, strlen(buff));
}
