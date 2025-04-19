/*
 * serial_io.h
 *
 *  Created on: Apr 11, 2025
 *      Author: mickey
 */

#ifndef SERIAL_IO_H_
#define SERIAL_IO_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "main.h"

typedef enum SerialChannelState
{
	SCSTATE_NONE = 0,
	SCSTATE_SENT = 1,
	SCSTATE_RECEIVED = 2,
} SerialChannelState_t;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c4;

extern char uart_buff[512];
extern uint8_t i2c_tx_buff[256];
extern uint8_t i2c_rx_buff[256];
extern uint8_t i2c_slave_buff[256];
extern uint8_t i2c_slave_regs[256];

extern SerialChannelState_t i2c_channel_master_states[4];
extern SerialChannelState_t i2c_channel_slave_states[4];

void serial_print(const char *msg, uint16_t len);
void serial_print_line(const char *msg, uint16_t len);
void serial_print_char(const char c);
uint16_t serial_scan(char *buffer, const uint16_t max_len);

void serial_i2c_send(I2C_HandleTypeDef *device, uint8_t *msg, uint16_t len, uint16_t address);
//void serial_i2c_receive(I2C_HandleTypeDef *device, uint8_t *buffer, uint16_t max_len);

#endif /* SERIAL_IO_H_ */
