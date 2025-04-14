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
	SCSTATE_ACKNOWLEDGED = 3,
} SerialChannelState_t;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c4;

extern char uart_buff[128];
extern uint8_t i2c_tx_buff[64];
extern uint8_t i2c_rx_buff[64];

extern SerialChannelState_t i2c4_master_state;
extern SerialChannelState_t i2c4_slave_state;
extern SerialChannelState_t i2c2_master_state;
extern SerialChannelState_t i2c2_slave_state;

void serial_print(const char *msg, uint16_t len);
void serial_print_line(const char *msg, uint16_t len);
void serial_print_char(const char c);
uint8_t serial_scan(char *buffer, const uint8_t max_len);

void serial_i2c_send(I2C_HandleTypeDef *device, const uint8_t *msg, uint16_t len, uint16_t address);
void serial_i2c_receive(I2C_HandleTypeDef *device, uint8_t *buffer, uint16_t max_len);

#endif /* SERIAL_IO_H_ */
