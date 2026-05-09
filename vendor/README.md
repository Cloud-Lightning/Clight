# Clight Vendor Sources

This directory contains third-party or vendor-origin source that Clight keeps as a local maintained copy because the project has modified or frozen it.

Current maintained trees:

- `hpm/`: HPM-related Ethernet PHY, lwIP, EtherCAT, and EEPROM-emulation source copied from official examples or SDK components and adapted for Clight.
- `tinyusb/`: TinyUSB source used by the Clight USB modules.

Rules:

- Keep license headers and upstream license files.
- Commit source and text configuration needed by the framework.
- Do not commit build outputs, generated firmware images, local IDE files, credentials, private keys, or machine-local SDK caches.
- If a vendor tree becomes too large or unmodified, move it back to an external dependency instead of keeping it here.
