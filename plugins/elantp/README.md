Elan TouchPad
=============

Introduction
------------

This plugin allows updating Touchpad devices from Elan. Devices are enumerated
from either raw i2c nodes or using HID, as the mode may depend on the OS default.

Firmware Format
---------------

The daemon will decompress the cabinet archive and extract a firmware blob in
an unspecified binary file format.

This plugin supports the following protocol ID:

 * tw.com.emc.elantp

GUID Generation
---------------

These device uses the standard DeviceInstanceId values, e.g.

 * `HIDRAW\VEN_17EF&DEV_6047`

Additionally another instance ID is added which corresponds to the module ID:

 * `HIDRAW\VEN_17EF&DEV_6047&MOD_1234`

These devices also use custom GUID values for the IC configuration, e.g.

 * `ELANTP\ICTYPE_09`

Vendor ID Security
------------------

The vendor ID is set from the HID vendor, for example set to `HIDRAW:0x17EF`

Quirk use
---------

This plugin uses the following plugin-specific quirks:

| Quirk                  | Description                               | Minimum fwupd version |
|------------------------|-------------------------------------------|-----------------------|
| `ElantpIcPageCount`    | The IC page count                         | 1.4.5                 |
