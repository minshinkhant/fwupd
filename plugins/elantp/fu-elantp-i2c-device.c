/*
 * Copyright (C) 2020 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <linux/i2c-dev.h>

#include "config.h"

#include "fu-elantp-common.h"
#include "fu-elantp-i2c-device.h"
#include "fu-chunk.h"

struct _FuElantpI2cDevice {
	FuUdevDevice		 parent_instance;
	guint16			 iap_ctrl;
	guint16			 ic_page_count;
};

G_DEFINE_TYPE (FuElantpI2cDevice, fu_elantp_ic2_device, FU_TYPE_UDEV_DEVICE)

static gboolean
fu_elantp_i2c_device_send_cmd (FuElantpI2cDevice *self,
			       guint8 *tx, gint txsz,
			       guint8 *rx, gint rxsz,
			       GError **error)
{
	if (!fu_udev_device_pwrite_full (FU_UDEV_DEVICE (self), 0, tx, txsz, error))
		return FALSE;
	if (rxsz <= 0)
		return TRUE;
	if (!fu_udev_device_pread_full (FU_UDEV_DEVICE (self), 0, rx, rxsz, error))
		return FALSE;
	return TRUE;
}

G_GNUC_UNUSED static gboolean
fu_elantp_i2c_device_write_cmd (FuElantpI2cDevice *self, guint16 reg, guint16 cmd, GError **error)
{
	guint8 buf[4];
	fu_common_write_uint16 (buf + 0x0, reg, G_LITTLE_ENDIAN);
	fu_common_write_uint16 (buf + 0x2, cmd, G_LITTLE_ENDIAN);
	if (g_getenv ("FWUPD_ELANTP_VERBOSE") != NULL)
		fu_common_dump_raw (G_LOG_DOMAIN, "WriteCmd", buf, sizeof(buf));
	return fu_elantp_i2c_device_send_cmd (self, buf, sizeof(buf), NULL, 0, error);
}

G_GNUC_UNUSED static gboolean
fu_elantp_i2c_device_read_cmd (FuElantpI2cDevice *self, guint16 reg,
			       guint8 *rx, gsize rxsz, GError **error)
{
	guint8 buf[2];
	guint8 buf_tmp[ETP_I2C_INF_LENGTH] = { 0x0 };
	fu_common_write_uint16 (buf + 0x0, reg, G_LITTLE_ENDIAN);
	if (g_getenv ("FWUPD_ELANTP_VERBOSE") != NULL)
		fu_common_dump_raw (G_LOG_DOMAIN, "ReadCmd", buf, sizeof(buf));
	if (!fu_elantp_i2c_device_send_cmd (self, buf, sizeof(buf),
					    buf_tmp, sizeof(buf_tmp), error))
		return FALSE;
	if (rxsz > 0)
		memcpy (rx, buf_tmp, rxsz);
	return TRUE;
}

static gboolean
fu_elantp_i2c_device_ensure_iap_ctrl (FuElantpI2cDevice *self, GError **error)
{
	return TRUE;
}

G_GNUC_UNUSED static gboolean
fu_elantp_i2c_device_write_fw_block (FuElantpI2cDevice *self,
				     const guint8 *raw_data,
				     guint16 checksum,
				     GError **error)
{
	guint8 buf[FW_PAGE_SIZE + 4];

	buf[0] = ETP_I2C_IAP_REG_L;
	buf[1] = ETP_I2C_IAP_REG_H;
	memcpy (buf + 2, raw_data, FW_PAGE_SIZE);
	fu_common_write_uint16 (buf + FW_PAGE_SIZE + 2, checksum, G_LITTLE_ENDIAN);

	if (!fu_elantp_i2c_device_send_cmd (self, buf, sizeof(buf), 0, 0, error))
		return FALSE;
	g_usleep (35 * 1000);
	if (!fu_elantp_i2c_device_ensure_iap_ctrl (self, error))
		return FALSE;
	if (self->iap_ctrl & (ETP_FW_IAP_PAGE_ERR | ETP_FW_IAP_INTF_ERR)) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_WRITE,
			     "IAP reports failed write: 0x%x",
			     self->iap_ctrl);
		return FALSE;
	}
	return TRUE;
}

static gboolean
fu_elantp_ic2_device_open (FuUdevDevice *device, GError **error)
{
//	FuElantpI2cDevice *self = FU_ELANTP_I2C_DEVICE (device);
	gint addr = 0x15;
	guint8 tx_buf[] = { 0x02, 0x01 };

	/* set slave address */
	if (!fu_udev_device_ioctl (device, I2C_SLAVE, (guint8 *) &addr, NULL, NULL)) {
		if (!fu_udev_device_ioctl (device, I2C_SLAVE_FORCE, (guint8 *) &addr, NULL, error))
			return FALSE;
	}

	/* read i2c device */
	return fu_udev_device_pwrite_full (device, 0x0, tx_buf, sizeof(tx_buf), error);
}

static gboolean
fu_elantp_ic2_device_probe (FuUdevDevice *device, GError **error)
{
//	FuElantpI2cDevice *self = FU_ELANTP_I2C_DEVICE (device);
	g_warning ("%s", fu_device_to_string (FU_DEVICE (device)));

	/* check is valid */
	if (g_strcmp0 (fu_udev_device_get_subsystem (device), "i2c") != 0) {
		g_set_error (error,
			     FWUPD_ERROR,
			     FWUPD_ERROR_NOT_SUPPORTED,
			     "is not correct subsystem=%s, expected i2c",
			     fu_udev_device_get_subsystem (device));
		return FALSE;
	}
	if (fu_udev_device_get_device_file (device) == NULL) {
		g_set_error_literal (error,
				     FWUPD_ERROR,
				     FWUPD_ERROR_NOT_SUPPORTED,
				     "no device file");
		return FALSE;
	}

	/* set the physical ID */
	if (!fu_udev_device_set_physical_id (device, "i2c", error))
		return FALSE;

	return TRUE;
}

static gboolean
fu_elantp_ic2_device_setup (FuDevice *device, GError **error)
{
	FuElantpI2cDevice *self = FU_ELANTP_I2C_DEVICE (device);
	guint8 buf[30] = { 0x0 };
	guint32 vid;
	guint32 pid;

	/* read the HID descriptor */
	if (!fu_elantp_i2c_device_read_cmd (self, 0x0001, buf, sizeof(buf), error)) {
		g_prefix_error (error, "failed to get HID descriptor: ");
		return FALSE;
	}
	vid = fu_common_read_uint16 (buf + 20, G_LITTLE_ENDIAN);
	pid = fu_common_read_uint16 (buf + 22, G_LITTLE_ENDIAN);

	/* add GUIDs in order of priority */
	if (vid != 0x0 && pid != 0x0) {
		g_autofree gchar *devid = NULL;
		devid = g_strdup_printf ("I2C\\VID_%04X&PID_%04X",
					 vid, pid);
		fu_device_add_instance_id (device, devid);
	}

	/* success */
	return TRUE;
}

static gboolean
fu_elantp_ic2_device_write_firmware (FuDevice *device,
			      FuFirmware *firmware,
			      FwupdInstallFlags flags,
			      GError **error)
{
	fu_device_set_progress (device, 100);
	return TRUE;
}

static gboolean
fu_elantp_ic2_device_set_quirk_kv (FuDevice *device,
			    const gchar *key,
			    const gchar *value,
			    GError **error)
{
	FuElantpI2cDevice *self = FU_ELANTP_I2C_DEVICE (device);
	if (g_strcmp0 (key, "ElantpIcPageCount") == 0) {
		guint64 tmp = fu_common_strtoull (value);
		if (tmp > 0xffff) {
			g_set_error_literal (error,
					     G_IO_ERROR,
					     G_IO_ERROR_NOT_SUPPORTED,
					     "ElantpIcPageCount only supports "
					     "values <= 0xffff");
			return FALSE;
		}
		self->ic_page_count = (guint16) tmp;
	}
	g_set_error_literal (error,
			     G_IO_ERROR,
			     G_IO_ERROR_NOT_SUPPORTED,
			     "quirk key not supported");
	return FALSE;
}

static void
fu_elantp_ic2_device_init (FuElantpI2cDevice *self)
{
	fu_device_add_flag (FU_DEVICE (self), FWUPD_DEVICE_FLAG_INTERNAL);
	fu_device_add_flag (FU_DEVICE (self), FWUPD_DEVICE_FLAG_UPDATABLE);
	fu_device_set_summary (FU_DEVICE (self), "Elan Touchpad");
	fu_device_add_icon (FU_DEVICE (self), "input-touchpad");
	fu_device_set_protocol (FU_DEVICE (self), "tw.com.emc.elantp");
	fu_device_set_version_format (FU_DEVICE (self), FWUPD_VERSION_FORMAT_HEX);
	fu_udev_device_set_flags (FU_UDEV_DEVICE (self),
				  FU_UDEV_DEVICE_FLAG_OPEN_READ |
				  FU_UDEV_DEVICE_FLAG_OPEN_WRITE);
}

static void
fu_elantp_ic2_device_finalize (GObject *object)
{
	G_OBJECT_CLASS (fu_elantp_ic2_device_parent_class)->finalize (object);
}

static void
fu_elantp_ic2_device_class_init (FuElantpI2cDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	FuDeviceClass *klass_device = FU_DEVICE_CLASS (klass);
	FuUdevDeviceClass *klass_udev_device = FU_UDEV_DEVICE_CLASS (klass);
	object_class->finalize = fu_elantp_ic2_device_finalize;
	klass_device->set_quirk_kv = fu_elantp_ic2_device_set_quirk_kv;
	klass_device->setup = fu_elantp_ic2_device_setup;
	klass_device->write_firmware = fu_elantp_ic2_device_write_firmware;
	klass_udev_device->probe = fu_elantp_ic2_device_probe;
	klass_udev_device->open = fu_elantp_ic2_device_open;
}
