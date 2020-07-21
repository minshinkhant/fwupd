/*
 * Copyright (C) 2020 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include "fu-plugin.h"

#define FU_TYPE_ELANTP_I2C_DEVICE (fu_elantp_ic2_device_get_type ())
G_DECLARE_FINAL_TYPE (FuElantpI2cDevice, fu_elantp_ic2_device, FU, ELANTP_I2C_DEVICE, FuUdevDevice)
