/*
 * Copyright (C) 2020 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <glib.h>

#define FW_PAGE_SIZE			64
//#define MAX_FW_PAGE_COUNT		1024
//#define MAX_FW_SIZE			 (MAX_FW_PAGE_COUNT*FW_PAGE_SIZE)

#define MAX_REC_SIZE 950

/* Control Elan trackpad I2C over USB */
#define ETP_I2C_INF_LENGTH		2

/* Elan trackpad firmware information related */
#define ETP_I2C_NEW_IAP_VERSION_CMD	0x0110
#define ETP_I2C_IAP_VERSION_CMD		0x0111
#define ETP_I2C_FW_VERSION_CMD		0x0102
#define ETP_I2C_IAP_CHECKSUM_CMD	0x0315
#define ETP_I2C_FW_CHECKSUM_CMD		0x030F
#define ETP_I2C_OSM_VERSION_CMD		0x0103
#define ETP_I2C_IAP_ICBODY_CMD		0x0110
#define ETP_GET_MODULE_ID_CMD		0x0101
#define ETP_GET_HARDWARE_ID_CMD		0x0100

/* Update preparation */
#define ETP_I2C_IAP_RESET_CMD		0x0314
#define ETP_I2C_IAP_RESET		0xF0F0
#define ETP_I2C_IAP_CTRL_CMD		0x0310
#define ETP_I2C_MAIN_MODE_ON		(1 << 9)
#define ETP_I2C_IAP_CMD			0x0311
#define ETP_I2C_IAP_PASSWORD		0x1EA5

#define ETP_I2C_IAP_REG_L		0x01
#define ETP_I2C_IAP_REG_H		0x06

#define ETP_FW_IAP_INTF_ERR		(1 << 4)
#define ETP_FW_IAP_PAGE_ERR		(1 << 5)
#define ETP_FW_IAP_CHECK_PW		(1 << 7)
#define ETP_FW_IAP_LAST_FIT		(1 << 9)

/* Firmware block update */
#define ETP_IAP_START_ADDR		0x0083

#define ETP_I2C_ENABLE_REPORT		0x0800

guint16		 fu_elantp_calc_checksum	(const guint8	*data,
						 gsize		 length);
