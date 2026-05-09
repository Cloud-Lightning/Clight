# TinyUsbHost Module

Application-facing TinyUSB host wrapper.

This module exposes host lifecycle and callback registration for MSC and HID.
It returns `Status::Unsupported` unless the platform build enables TinyUSB host
support and the board provides the required OTG/VBUS hardware.
