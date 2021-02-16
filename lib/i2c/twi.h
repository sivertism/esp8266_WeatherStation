/**************************************************************************************/
/*
   twi.h - Software/bit-bang master I²C library for ESP8266 Arduino

   Modified 2019 by enjoyneering79, source code: https://github.com/enjoyneering/

   Specials pins are required:
   Board:                                     SDA        SCL        Level
   ESP8266................................... GPIO4      GPIO5      3.3v/5v
   ESP8266 ESP-01............................ GPIO0/D5   GPIO2/D3   3.3v/5v
   NodeMCU 1.0, WeMos D1 Mini................ GPIO4/D2   GPIO5/D1   3.3v/5v

   Copyright (c) 2015 Hristo Gochkov. All rights reserved.
   This file is part of the esp8266 core for Arduino environment.
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**************************************************************************************/

#ifndef SI2C_h
#define SI2C_h

#include<stdint.h>
#include "esp8266_peri.h"
#include "esp8266_undocumented.h"

#define TWI_I2C_DEFAULT_CLOCK           100000 //default I2C speed, in Hz
#define TWI_I2C_SCL_STRCH_LIMIT         1250   //maximum SCL stretch time, in μsec
#define TWI_I2C_SDA_POLLING_LIMIT       20     //maximum number of SCL pulses to release I2C bus if slave blocked SDA low

#define TWI_I2C_NACK                    HIGH   //1
#define TWI_I2C_ACK                     LOW    //0

#define I2C_OK                          0      //bus OK
#define I2C_SDA_HELD_LOW                1      //SDA held low by another device, no procedure available to recover
#define I2C_SDA_HELD_LOW_AFTER_INIT     2      //SDA held low beyond slave clock stretch time
#define I2C_SCL_HELD_LOW                3      //SCL held low by another device, no procedure available to recover
#define I2C_SCL_HELD_LOW_AFTER_READ     4      //SCL held low beyond slave clock stretch time

#define I2C_SDA_OK                      5      //SDA free
#define I2C_SDA_RELEASED                6      //SDA released after use by another device


//void    twi_init(uint8_t sda, uint8_t scl);
void    twi_init();
void    twi_setClock(uint32_t freq);
void    twi_setClockStretchLimit(uint32_t limit); //  = TWI_I2C_SCL_STRCH_LIMIT
uint8_t twi_writeTo(uint8_t address, const uint8_t *buffer, uint8_t length, bool sendStop);
uint8_t twi_readFrom(uint8_t address, uint8_t *buffer, uint8_t length, bool sendStop);
uint8_t twi_status();

#endif
