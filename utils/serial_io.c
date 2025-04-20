/*
 * serial_io.c
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#include "serial_io.h"

#define UART_TX_HANDLE (huart3)
#define UART_RX_HANDLE (huart3)

static const uint16_t i2c_slave_buff_size = 256;

char uart_buff[512] = {0};

uint8_t i2c_tx_buff[256] = {0};
uint8_t i2c_rx_buff[256] = {0};

uint8_t i2c_slave_buff[256] = {0};
uint8_t i2c_slave_regs[256] = {0};
volatile uint8_t i2c_slave_direction = 0;
uint16_t i2c_slave_rx_count = 0;
uint16_t i2c_slave_tx_count = 0;
uint16_t i2c_slave_tx_position = 0;

I2C_TypeDef *i2c_instances[4] = { I2C1, I2C2, 0, I2C4 };

SerialChannelState_t i2c_channel_master_states[4] = {0};
SerialChannelState_t i2c_channel_slave_states[4] = {0};

static void process_i2c_rx(I2C_HandleTypeDef *hi2c)
{
	int start_reg = i2c_slave_buff[0];
	int num_regs = i2c_slave_rx_count - 1;

	int end_reg = start_reg + num_regs - 1;

	if (end_reg > 255)
	{
		Error_Handler();
	}

	for (int idx = 0; idx < num_regs; idx++)
	{
		i2c_slave_regs[start_reg + idx] = i2c_slave_buff[idx+1];
	}

	bzero(i2c_slave_buff+i2c_slave_rx_count, sizeof(i2c_slave_buff)-i2c_slave_rx_count);
	i2c_slave_rx_count = 0;

	for (int i = 0; i < 4; i++)
	{
		if (hi2c->Instance == i2c_instances[i])
		{
			i2c_channel_slave_states[i] = SCSTATE_RECEIVED;
			break;
		}
	}
}

static void process_i2c_tx(I2C_HandleTypeDef *hi2c)
{
	i2c_slave_tx_count = 0;

	for (int i = 0; i < 4; i++)
	{
		if (hi2c->Instance == i2c_instances[i])
		{
			i2c_channel_slave_states[i] = SCSTATE_SENT;
			break;
		}
	}
}

void HAL_I2C_MasterTxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	for (int i = 0; i < 4; i++)
	{
		if (hi2c->Instance == i2c_instances[i])
		{
			i2c_channel_master_states[i] = SCSTATE_SENT;
			break;
		}
	}
}

void HAL_I2C_MasterRxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	for (int i = 0; i < 4; i++)
	{
		if (hi2c->Instance == i2c_instances[i])
		{
			i2c_channel_master_states[i] = SCSTATE_RECEIVED;
			break;
		}
	}
}

void HAL_I2C_SlaveTxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	i2c_slave_tx_count++;

	if (i2c_slave_tx_count < i2c_slave_buff_size)
	{
		if (i2c_slave_tx_count == i2c_slave_buff_size - 1)
		{
			HAL_I2C_Slave_Seq_Transmit_DMA(hi2c, i2c_slave_regs+i2c_slave_tx_position+i2c_slave_tx_count, 1, I2C_LAST_FRAME);
		}
		else
		{
			HAL_I2C_Slave_Seq_Transmit_DMA(hi2c, i2c_slave_regs+i2c_slave_tx_position+i2c_slave_tx_count, 1, I2C_NEXT_FRAME);
		}
	}
	else if (i2c_slave_tx_count == i2c_slave_buff_size)
	{
		process_i2c_tx(hi2c);
	}
}

void HAL_I2C_SlaveRxCpltCallback (I2C_HandleTypeDef * hi2c)
{
	i2c_slave_rx_count++;

	if (i2c_slave_rx_count < i2c_slave_buff_size)
	{
		if (i2c_slave_rx_count == i2c_slave_buff_size - 1)
		{
			HAL_I2C_Slave_Seq_Receive_DMA(hi2c, i2c_slave_buff+i2c_slave_rx_count, 1, I2C_LAST_FRAME);
		}
		else
		{
			HAL_I2C_Slave_Seq_Receive_DMA(hi2c, i2c_slave_buff+i2c_slave_rx_count, 1, I2C_NEXT_FRAME);
		}
	}
	else if (i2c_slave_rx_count == i2c_slave_buff_size)
	{
		process_i2c_rx(hi2c);
	}
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	for (int i = 0; i < 4; i++)
	{
		if (hi2c->Instance == i2c_instances[i])
		{
			i2c_channel_slave_states[i] = SCSTATE_SENT;
			break;
		}
	}
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	for (int i = 0; i < 4; i++)
	{
		if (hi2c->Instance == i2c_instances[i])
		{
			i2c_channel_slave_states[i] = SCSTATE_RECEIVED;
			break;
		}
	}
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef * hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}

extern void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	i2c_slave_direction = TransferDirection;

	if(i2c_slave_direction == I2C_DIRECTION_TRANSMIT)  // if the master wants to transmit the data
	{
		i2c_slave_buff[0] = 0;
		i2c_slave_rx_count = 0;
		HAL_I2C_Slave_Sequential_Receive_DMA(hi2c, i2c_slave_buff, 1, I2C_FIRST_FRAME);
	}
	else
	{
		i2c_slave_tx_position = i2c_slave_buff[0];
		i2c_slave_buff[0] = 0;
		i2c_slave_tx_count = 0;
		HAL_I2C_Slave_Sequential_Transmit_DMA(hi2c, i2c_slave_regs+i2c_slave_tx_position, 1, I2C_FIRST_FRAME);
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	uint32_t error_code = HAL_I2C_GetError(hi2c);

	if (error_code == 4)  // AF error
	{
		if (i2c_slave_direction == I2C_DIRECTION_TRANSMIT)
		{
			process_i2c_rx(hi2c);
		}
		else
		{
			process_i2c_tx(hi2c);
		}
	}
	/* BERR Error commonly occurs during the Direction switch
	* Here we the software reset bit is set by the HAL error handler
	* Before resetting this bit, we make sure the I2C lines are released and the bus is free
	* I am simply reinitializing the I2C to do so
	*/
	else if (error_code == 1)  // BERR Error
	{
		HAL_I2C_DeInit(hi2c);
		HAL_I2C_Init(hi2c);
		bzero(i2c_slave_buff, i2c_slave_buff_size);
		i2c_slave_rx_count = 0;
	}

	HAL_I2C_EnableListen_IT(hi2c);
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
	HAL_UART_Transmit(&UART_TX_HANDLE, (uint8_t *)"\r\n", 2, 0xFFFF);
}

static void serial_upline(uint16_t line_length)
{
	char buff[16] = {0};
	sprintf(buff, "\r\e[%uC%c \b", line_length-1, 0x8D);
	serial_print(buff, 0);
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
						serial_upline(echo_line_length);
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
						serial_newline();
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
	sprintf(buff, "I2C Master Transmit DMA result: %s", ret == 0 ? "good" : "bad");
	serial_print_line(buff, strlen(buff));
}

/*void serial_i2c_receive(I2C_HandleTypeDef *device, uint8_t *buffer, uint16_t max_len)
{
	char buff[64];
	HAL_I2C_StateTypeDef state;
	HAL_StatusTypeDef ret;
	while ((state = HAL_I2C_GetState(device)) != HAL_I2C_STATE_READY) HAL_Delay(1);
	while((ret = HAL_I2C_Slave_Receive_DMA(device, buffer, max_len)) != HAL_OK)
	{
		HAL_Delay(10);
	}
	sprintf(buff, "I2C Slave Receive DMA result: %s", ret == 0 ? "good" : "bad");
	serial_print_line(buff, strlen(buff));
}*/
